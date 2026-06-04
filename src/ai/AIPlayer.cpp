#include "AIPlayer.h"
#include <algorithm>
#include <climits>
#include <chrono>
#include <cstdio>
#include <ctime>

// ================================================================
//  AIPlayer.cpp — Minimax with alpha-beta pruning
//
//  Encoding: AI = O (c=1), human = X (c=-1)
//  state.turn: true = Player1's turn (human in PVE), false = AI's turn
//  depth is set by state.difficulty (AI_EASY=2, AI_MEDIUM=4, AI_HARD=6)
// ================================================================

static const int MAX_CANDS = 20;
// ── Benchmark state ──────────────────────────────────────────────
static AIBenchResult s_bench;
static int           s_moveNumber = 0;

static int s_nodesVisited      = 0;
static int s_nodesPruned       = 0;
static int s_sortCalls         = 0;
static int s_candidatesSkipped = 0;

AIBenchResult AI_GetLastBenchmark() { return s_bench; }
void          AI_SetMoveNumber(int n) { s_moveNumber = n; }

static const char* DifficultyName(AIDifficulty d) {
    switch (d) {
        case AI_EASY:   return "EASY  ";
        case AI_MEDIUM: return "MEDIUM";
        case AI_HARD:   return "HARD  ";
        default:        return "?     ";
    }
}

static void LogBenchmark(const AIBenchResult& b, AIDifficulty diff) {
    // Timestamp
    time_t now = time(nullptr);
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char* rating;
    if (diff == AI_HARD) {
        rating = b.timeMs < 80.0f ? "EXCELLENT" : b.timeMs < 200.0f ? "GOOD" : b.timeMs < 500.0f ? "OK" : "SLOW";
    } else if (diff == AI_MEDIUM) {
        rating = b.timeMs < 15.0f ? "EXCELLENT" : b.timeMs < 50.0f ? "GOOD" : b.timeMs < 150.0f ? "OK" : "SLOW";
    } else {
        rating = b.timeMs < 1.0f  ? "EXCELLENT" : b.timeMs < 5.0f  ? "GOOD" : b.timeMs < 20.0f  ? "OK" : "SLOW";
    }

    char line[320];
    snprintf(line, sizeof(line),
        "[%s] %s d=%d move#%-3d | %7.1fms | %8d nodes | sort=%-5d skip=%-6d | real=%.1f%% | %s\n",
        ts, DifficultyName(diff), b.depth, b.moveNumber,
        b.timeMs, b.nodesVisited,
        b.sortCalls, b.candidatesSkipped, b.realPruneRatio * 100.0f,
        rating);

    // Console
    fputs(line, stdout);

    // Append to log file
    FILE* f = fopen("Log/ai_benchmark.log", "a");
    if (f) { fputs(line, f); fclose(f); }
}

// Pattern score table — heuristic evaluation of N-in-a-row threats
static const int SCORES[] = { 0, 10, 100, 1000, 100000, 10000000 };

// Count consecutive pieces + open ends in one direction from (r,c)
static int CountLine(const _GAMESTATE& state, int r, int c, int dr, int dc, int color) {
    int count = 1;
    int open  = 0;

    // forward
    int nr = r + dr, nc = c + dc;
    while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
           && state._BOARD[nr][nc].c == color) {
        count++; nr += dr; nc += dc;
    }
    if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && state._BOARD[nr][nc].c == 0) open++;

    // backward
    nr = r - dr; nc = c - dc;
    while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
           && state._BOARD[nr][nc].c == color) {
        count++; nr -= dr; nc -= dc;
    }
    if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && state._BOARD[nr][nc].c == 0) open++;

    if (count >= WIN_COUNT) return SCORES[WIN_COUNT];
    return (open > 0) ? SCORES[count] : SCORES[count] / 2;
}

// Evaluate the full board from AI's perspective (positive = good for AI)
static int EvaluateBoard(const _GAMESTATE& state, int lastR, int lastC) {
    static const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    int score = 0;

    int rMin = std::max(0, lastR - (WIN_COUNT - 1));
    int rMax = std::min(BOARD_SIZE - 1, lastR + (WIN_COUNT - 1));
    int cMin = std::max(0, lastC - (WIN_COUNT - 1));
    int cMax = std::min(BOARD_SIZE - 1, lastC + (WIN_COUNT -1));

    for (int r = rMin; r <= rMax; r++) {
        for (int c = cMin; c <= cMax; c++) {
            int col = state._BOARD[r][c].c;
            if (col == 0) continue;
            int sign = (col == 1) ? 1 : -1;   // AI=1=positive
            for (auto& d : dirs) {
                score += sign * CountLine(state, r, c, d[0], d[1], col);
            }
        }
    }
    return score;
}

// Check terminal: returns INT_MAX (AI wins), INT_MIN (human wins), 0 (draw/ongoing)
static int CheckTerminal(const _GAMESTATE& state, int lastR, int lastC) {
    if (lastR < 0) return 0;
    int col = state._BOARD[lastR][lastC].c;
    if (col == 0) return 0;
    static const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    for (auto& d : dirs) {
        int cnt = 1;
        for (int s = 1; s < WIN_COUNT; s++) {
            int nr = lastR + d[0]*s, nc = lastC + d[1]*s;
            if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE
                || state._BOARD[nr][nc].c != col) break;
            cnt++;
        }
        for (int s = 1; s < WIN_COUNT; s++) {
            int nr = lastR - d[0]*s, nc = lastC - d[1]*s;
            if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE
                || state._BOARD[nr][nc].c != col) break;
            cnt++;
        }
        if (cnt >= WIN_COUNT)
            return (col == 1) ? INT_MAX / 2 : INT_MIN / 2;
    }
    return 0;
}

// QuickScore: chấm điểm nhanh cho ô (r,c) nếu color đặt vào đó
// Xét 4 hướng, cộng cả điểm tấn công lẫn điểm phòng thủ
static int QuickScore(const _GAMESTATE& state, int r, int c, int color) {
    static const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    int score = 0;
    for (auto& d : dirs) {
        score += CountLine(state, r, c, d[0], d[1],  color);   // tấn công
        score += CountLine(state, r, c, d[0], d[1], -color);   // phòng thủ
    }
    return score;
}

// SortCandidates: sort giảm dần theo QuickScore → nước tốt được xét trước → Alpha-Beta pruning sớm hơn
static void SortCandidates(const _GAMESTATE& state, int cands[][2], int nCands, int color) {
    s_sortCalls++;
    struct ScoredMove { int r, c, score; };
    ScoredMove tmp[BOARD_SIZE * BOARD_SIZE];

    for (int i = 0; i < nCands; i++)
        tmp[i] = { cands[i][0], cands[i][1],
                   QuickScore(state, cands[i][0], cands[i][1], color) };

    std::sort(tmp, tmp + nCands,
        [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    for (int i = 0; i < nCands; i++) {
        cands[i][0] = tmp[i].r;
        cands[i][1] = tmp[i].c;
    }
}

// Collect candidate moves: only cells within 2 of any placed piece
static void GetCandidates(const _GAMESTATE& state, int out[][2], int& count) {
    count = 0;
    bool visited[BOARD_SIZE][BOARD_SIZE] = {};
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (state._BOARD[r][c].c == 0) continue;
            for (int dr = -2; dr <= 2; dr++) {
                for (int dc = -2; dc <= 2; dc++) {
                    int nr = r+dr, nc = c+dc;
                    if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE) continue;
                    if (state._BOARD[nr][nc].c != 0) continue;
                    if (visited[nr][nc]) continue;
                    visited[nr][nc] = true;
                    out[count][0] = nr;
                    out[count][1] = nc;
                    count++;
                }
            }
        }
    }
    // Fallback: empty board → play center
    if (count == 0) {
        out[0][0] = BOARD_SIZE / 2;
        out[0][1] = BOARD_SIZE / 2;
        count = 1;
    }
}

// Minimax with alpha-beta.
// isMaximising: true = AI's turn (maximise), false = human's turn (minimise)
static int Minimax(_GAMESTATE& state, int depth, int alpha, int beta,
                   bool isMaximising, int lastR, int lastC) {
    int terminal = CheckTerminal(state, lastR, lastC);
    if (terminal != 0) return terminal;
    if (depth == 0)    return EvaluateBoard(state, lastR, lastC);

    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int nCands = 0;
    GetCandidates(state, cands, nCands);
    if (nCands == 0) return 0;

    int color = isMaximising ? 1 : -1;

    SortCandidates(state, cands, nCands, color);
    if(nCands > MAX_CANDS) nCands = MAX_CANDS;

    int best  = isMaximising ? INT_MIN : INT_MAX;

    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        state._BOARD[r][c].c = color;

        s_nodesVisited++;
        int val = Minimax(state, depth - 1, alpha, beta, !isMaximising, r, c);

        state._BOARD[r][c].c = 0;  // undo

        if (isMaximising) {
            best  = std::max(best, val);
            alpha = std::max(alpha, best);
        } else {
            best  = std::min(best, val);
            beta  = std::min(beta, best);
        }
        if (beta <= alpha) { s_nodesPruned++; s_candidatesSkipped += (nCands - i - 1); break; }
    }
    return best;
}

Move AI_FindBestMove(const _GAMESTATE& state) {
    // Reset counters
    s_nodesVisited      = 0;
    s_nodesPruned       = 0;
    s_sortCalls         = 0;
    s_candidatesSkipped = 0;
    auto t0 = std::chrono::high_resolution_clock::now();

    // Copy state so we can mutate during search without touching original
    _GAMESTATE working = state;
    int depth = static_cast<int>(state.difficulty);
    float timeLimitMs;
    if(state.difficulty == AI_HARD) timeLimitMs = 1500.0f;
    else if(state.difficulty == AI_MEDIUM) timeLimitMs = 500.0f;
    else timeLimitMs = 100.0f;
    
    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int nCands = 0;
    GetCandidates(working, cands, nCands);

    // Priority 1: Take immediate win — always play it regardless of minimax
    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        working._BOARD[r][c].c = 1;
        bool aiWins = (CheckTerminal(working, r, c) != 0);
        working._BOARD[r][c].c = 0;
        if (aiWins) return Move{r, c};
    }

    // Priority 2: Block immediate human win
    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        working._BOARD[r][c].c = -1;
        bool humanWins = (CheckTerminal(working, r, c) != 0);
        working._BOARD[r][c].c = 0;
        if (humanWins) return Move{r, c};
    }

    // Priority 3: Full minimax for deeper strategy
    SortCandidates(working, cands, nCands, 1);  // AI = color 1, sort trước khi minimax
    if(nCands > MAX_CANDS) nCands = MAX_CANDS;
    Move best;
    int bestScore = INT_MIN;

    for (int i = 0; i < nCands; i++) {

        auto elapsed = std::chrono::duration<float, std::milli>(
            std::chrono::high_resolution_clock::now() - t0).count();
        if(elapsed > timeLimitMs){break;}

        int r = cands[i][0], c = cands[i][1];
        working._BOARD[r][c].c = 1;  // AI plays O

        s_nodesVisited++;
        int score = Minimax(working, depth - 1, INT_MIN, INT_MAX, false, r, c);
        working._BOARD[r][c].c = 0;

        if (score > bestScore) {
            bestScore = score;
            best.row  = r;
            best.col  = c;
        }
    }

    if (best.row == -1) best = Move{cands[0][0], cands[0][1]};  // fallback: nước sort tốt nhất
    // Record and log benchmark
    auto t1 = std::chrono::high_resolution_clock::now();
    int total     = s_nodesVisited + s_nodesPruned;
    int realTotal = s_nodesVisited + s_candidatesSkipped;
    s_bench.timeMs            = std::chrono::duration<float, std::milli>(t1 - t0).count();
    s_bench.nodesVisited      = s_nodesVisited;
    s_bench.nodesPruned       = s_nodesPruned;
    s_bench.pruneRatio        = (total > 0)     ? (float)s_nodesPruned       / total     : 0.0f;
    s_bench.sortCalls         = s_sortCalls;
    s_bench.candidatesSkipped = s_candidatesSkipped;
    s_bench.realPruneRatio    = (realTotal > 0) ? (float)s_candidatesSkipped / realTotal : 0.0f;
    s_bench.depth             = depth;
    s_bench.rootCandidates    = nCands;
    s_bench.moveNumber        = s_moveNumber;
    LogBenchmark(s_bench, state.difficulty);

    return best;
}
