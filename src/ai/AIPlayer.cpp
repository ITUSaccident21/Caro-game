#include "ai/AIPlayer.h"
#include <algorithm>
#include <climits>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <random>

//  AIPlayer.cpp — Negamax + Alpha-Beta + Iterative Deepening

//  Negamax convention: Negamax(state, depth, alpha, beta, color, ...)

//  (color=+1 → tới phiên AI, color=-1 → tới phiên người). EvaluateBoard

//  Iterative Deepening (ID): tìm kiếm depth 1..maxDepth, mỗi vòng dùng
//  khi hết giờ, mọi lệnh gọi Negamax đang mở trên stack trả về ngay

//  Toàn bộ state phụ trợ MỚI (Zobrist table, TT, killer/history, timing)
//  là `thread_local`: AIWorker (DR-017) detach thread cũ khi có request
//  khác nhau; thread_local tránh data race mà không cần mutex.

static const int MAX_CANDS = 20;
static const int MAX_PLY   = 8;
static const int TT_BITS   = 14;
static const int TT_SIZE   = 1 << TT_BITS;

// Đủ lớn để bound mọi giá trị Negamax trả về (EvaluateBoard tối đa ~32.4M,
static const int INF_SCORE = 2000000000;

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

    time_t now = time(nullptr);
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Iterative Deepening = anytime algorithm: HARD dùng gần hết 1500ms
    int maxDepth = static_cast<int>(diff);
    const char* rating = (b.depth >= maxDepth) ? "FULL" : "PARTIAL";

    char line[320];
    snprintf(line, sizeof(line),
        "[%s] %s d=%d/%-2d move#%-3d | %7.1fms | %8d nodes | sort=%-5d skip=%-6d | real=%.1f%% | %s\n",
        ts, DifficultyName(diff), b.depth, maxDepth, b.moveNumber,
        b.timeMs, b.nodesVisited,
        b.sortCalls, b.candidatesSkipped, b.realPruneRatio * 100.0f,
        rating);

    fputs(line, stdout);

    FILE* f = fopen("Log/ai_benchmark.log", "a");
    if (f) { fputs(line, f); fclose(f); }
}

// Pattern score table — heuristic evaluation of N-in-a-row threats
static const int SCORES[] = { 0, 10, 100, 1000, 100000, 10000000 };

static int CountLine(const _GAMESTATE& state, int r, int c, int dr, int dc, int color) {
    int count = 1;
    int open  = 0;

    int nr = r + dr, nc = c + dc;
    while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
           && state._BOARD[nr][nc].c == color) {
        count++; nr += dr; nc += dc;
    }
    if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && state._BOARD[nr][nc].c == 0) open++;

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
            int sign = (col == 1) ? 1 : -1;
            for (auto& d : dirs) {
                score += sign * CountLine(state, r, c, d[0], d[1], col);
            }
        }
    }
    return score;
}

// Check terminal: returns INT_MAX/2 (AI wins), INT_MIN/2 (human wins), 0 (draw/ongoing)
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

static int QuickScore(const _GAMESTATE& state, int r, int c, int color) {
    static const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    int score = 0;
    for (auto& d : dirs) {
        score += CountLine(state, r, c, d[0], d[1],  color);
        score += CountLine(state, r, c, d[0], d[1], -color);
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

    if (count == 0) {
        out[0][0] = BOARD_SIZE / 2;
        out[0][1] = BOARD_SIZE / 2;
        count = 1;
    }
}

//  Zobrist hashing — incremental position hash cho Transposition Table

static thread_local uint64_t s_zobrist[BOARD_SIZE][BOARD_SIZE][2];
static thread_local uint64_t s_zobristSide;
static thread_local bool     s_zobristInit = false;

static void InitZobrist() {
    if (s_zobristInit) return;
    std::mt19937_64 rng(0x5EED1234ULL);
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            for (int k = 0; k < 2; k++)
                s_zobrist[r][c][k] = rng();
    s_zobristSide = rng();
    s_zobristInit = true;
}

static inline int ZIdx(int color) { return (color == 1) ? 1 : 0; }

static uint64_t ComputeHash(const _GAMESTATE& state, int colorToMove) {
    uint64_t h = 0;
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++) {
            int col = state._BOARD[r][c].c;
            if (col != 0) h ^= s_zobrist[r][c][ZIdx(col)];
        }
    if (colorToMove == 1) h ^= s_zobristSide;
    return h;
}

enum TTFlag : uint8_t { TT_EXACT, TT_LOWER, TT_UPPER };

struct TTEntry {
    uint64_t key   = 0;
    int      depth = -1;
    int      score = 0;
    TTFlag   flag  = TT_EXACT;
    int      moveR = -1;
    int      moveC = -1;
    bool     used  = false;
};

static thread_local TTEntry s_tt[TT_SIZE];

static inline TTEntry* TTProbe(uint64_t key) {
    TTEntry& e = s_tt[key & (TT_SIZE - 1)];
    return (e.used && e.key == key) ? &e : nullptr;
}

static inline void TTStore(uint64_t key, int depth, int score, TTFlag flag, int mr, int mc) {
    TTEntry& e = s_tt[key & (TT_SIZE - 1)];
    if (!e.used || e.key != key || depth >= e.depth) {
        e.key = key; e.depth = depth; e.score = score; e.flag = flag;
        e.moveR = mr; e.moveC = mc; e.used = true;
    }
}

//  Killer moves + history heuristic — move ordering cho non-root nodes
static thread_local int s_killer[MAX_PLY][2][2];
static thread_local int s_history[BOARD_SIZE][BOARD_SIZE];

// ── Timing state cho Iterative Deepening ─────────────────────────
static thread_local std::chrono::high_resolution_clock::time_point s_t0;
static thread_local float s_timeLimitMs = 0.0f;
static thread_local bool  s_timedOut    = false;

static void ResetSearchTables() {
    InitZobrist();
    for (int i = 0; i < TT_SIZE; i++) s_tt[i].used = false;
    for (int p = 0; p < MAX_PLY; p++) {
        s_killer[p][0][0] = -1; s_killer[p][0][1] = -1;
        s_killer[p][1][0] = -1; s_killer[p][1][1] = -1;
    }
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++) s_history[r][c] = 0;
}

static void StoreKiller(int ply, int r, int c) {
    if (ply < 0 || ply >= MAX_PLY) return;
    if (s_killer[ply][0][0] == r && s_killer[ply][0][1] == c) return;
    s_killer[ply][1][0] = s_killer[ply][0][0];
    s_killer[ply][1][1] = s_killer[ply][0][1];
    s_killer[ply][0][0] = r;
    s_killer[ply][0][1] = c;
}

static void OrderMoves(const _GAMESTATE& state, int cands[][2], int nCands, int color,
                        int ply, int ttMoveR, int ttMoveC) {
    s_sortCalls++;
    struct ScoredMove { int r, c; long long score; };
    ScoredMove tmp[BOARD_SIZE * BOARD_SIZE];

    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        long long score = QuickScore(state, r, c, color) + s_history[r][c];
        if (r == ttMoveR && c == ttMoveC) {
            score += 1000000000LL;
        } else if (r == s_killer[ply][0][0] && c == s_killer[ply][0][1]) {
            score += 500000;
        } else if (r == s_killer[ply][1][0] && c == s_killer[ply][1][1]) {
            score += 250000;
        }
        tmp[i] = { r, c, score };
    }

    std::sort(tmp, tmp + nCands,
        [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    for (int i = 0; i < nCands; i++) {
        cands[i][0] = tmp[i].r;
        cands[i][1] = tmp[i].c;
    }
}

//  Negamax + Alpha-Beta + PVS + TT + Killer/History
//  `hash` = Zobrist hash của `state` TRƯỚC khi đi (đã bao gồm side-to-move).
static int Negamax(_GAMESTATE& state, int depth, int alpha, int beta, int color,
                    int lastR, int lastC, int ply, uint64_t hash) {
    s_nodesVisited++;
    if ((s_nodesVisited & 255) == 0) {
        float elapsed = std::chrono::duration<float, std::milli>(
            std::chrono::high_resolution_clock::now() - s_t0).count();
        if (elapsed > s_timeLimitMs) s_timedOut = true;
    }
    if (s_timedOut) return color * EvaluateBoard(state, lastR, lastC);

    int terminal = CheckTerminal(state, lastR, lastC);
    if (terminal != 0) return color * terminal;
    if (depth == 0)    return color * EvaluateBoard(state, lastR, lastC);

    int ttMoveR = -1, ttMoveC = -1;
    TTEntry* tte = TTProbe(hash);
    if (tte) {
        if (tte->depth >= depth) {
            if (tte->flag == TT_EXACT) return tte->score;
            if (tte->flag == TT_LOWER) alpha = std::max(alpha, tte->score);
            else if (tte->flag == TT_UPPER) beta = std::min(beta, tte->score);
            if (alpha >= beta) return tte->score;
        }
        ttMoveR = tte->moveR;
        ttMoveC = tte->moveC;
    }

    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int nCands = 0;
    GetCandidates(state, cands, nCands);
    if (nCands == 0) return 0;

    OrderMoves(state, cands, nCands, color, ply, ttMoveR, ttMoveC);
    if (nCands > MAX_CANDS) nCands = MAX_CANDS;

    int alphaOrig = alpha;
    int best  = -INF_SCORE;
    int bestR = -1, bestC = -1;

    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        state._BOARD[r][c].c = color;
        uint64_t childHash = hash ^ s_zobrist[r][c][ZIdx(color)] ^ s_zobristSide;

        int val;
        if (i == 0) {
            val = -Negamax(state, depth - 1, -beta, -alpha, -color, r, c, ply + 1, childHash);
        } else {
            val = -Negamax(state, depth - 1, -alpha - 1, -alpha, -color, r, c, ply + 1, childHash);
            if (val > alpha && val < beta) {
                val = -Negamax(state, depth - 1, -beta, -alpha, -color, r, c, ply + 1, childHash);
            }
        }

        state._BOARD[r][c].c = 0;

        if (val > best) { best = val; bestR = r; bestC = c; }
        if (best > alpha) alpha = best;
        if (alpha >= beta) {
            s_nodesPruned++;
            s_candidatesSkipped += (nCands - i - 1);
            StoreKiller(ply, r, c);
            s_history[r][c] += depth * depth;
            break;
        }
    }

    TTFlag flag = (best <= alphaOrig) ? TT_UPPER : (best >= beta) ? TT_LOWER : TT_EXACT;
    TTStore(hash, depth, best, flag, bestR, bestC);

    return best;
}

Move AI_FindBestMove(const _GAMESTATE& state) {

    s_nodesVisited      = 0;
    s_nodesPruned       = 0;
    s_sortCalls         = 0;
    s_candidatesSkipped = 0;
    auto t0 = std::chrono::high_resolution_clock::now();

    _GAMESTATE working = state;
    int maxDepth = static_cast<int>(state.difficulty);
    float timeLimitMs;
    if(state.difficulty == AI_HARD) timeLimitMs = 1500.0f;
    else if(state.difficulty == AI_MEDIUM) timeLimitMs = 500.0f;
    else timeLimitMs = 100.0f;

    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int nCands = 0;
    GetCandidates(working, cands, nCands);

    // Priority 1: Take immediate win — always play it regardless of search
    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        working._BOARD[r][c].c = 1;
        bool aiWins = (CheckTerminal(working, r, c) != 0);
        working._BOARD[r][c].c = 0;
        if (aiWins) return Move{r, c};
    }

    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        working._BOARD[r][c].c = -1;
        bool humanWins = (CheckTerminal(working, r, c) != 0);
        working._BOARD[r][c].c = 0;
        if (humanWins) return Move{r, c};
    }

    // Priority 3: Iterative Deepening negamax (PVS + TT + killer/history)
    SortCandidates(working, cands, nCands, 1);
    if (nCands > MAX_CANDS) nCands = MAX_CANDS;

    ResetSearchTables();
    s_t0          = t0;
    s_timeLimitMs = timeLimitMs;
    s_timedOut    = false;

    int order[MAX_CANDS][2];
    int scored[MAX_CANDS];
    for (int i = 0; i < nCands; i++) {
        order[i][0] = cands[i][0];
        order[i][1] = cands[i][1];
        scored[i]   = 0;
    }

    Move best = Move{ order[0][0], order[0][1] };
    int  completedDepth = 0;

    for (int d = 1; d <= maxDepth && !s_timedOut; d++) {
        uint64_t rootHash  = ComputeHash(working, 1);
        int  alpha         = -INF_SCORE;
        int  beta          =  INF_SCORE;
        int  iterBestScore = -INF_SCORE;
        int  iterBestIdx   = 0;
        bool iterComplete  = true;

        for (int i = 0; i < nCands; i++) {
            int r = order[i][0], c = order[i][1];
            working._BOARD[r][c].c = 1;
            uint64_t childHash = rootHash ^ s_zobrist[r][c][ZIdx(1)] ^ s_zobristSide;

            s_nodesVisited++;
            int val;
            if (i == 0) {
                val = -Negamax(working, d - 1, -beta, -alpha, -1, r, c, 1, childHash);
            } else {
                val = -Negamax(working, d - 1, -alpha - 1, -alpha, -1, r, c, 1, childHash);
                if (!s_timedOut && val > alpha && val < beta) {
                    val = -Negamax(working, d - 1, -beta, -alpha, -1, r, c, 1, childHash);
                }
            }

            working._BOARD[r][c].c = 0;
            scored[i] = val;

            if (s_timedOut) { iterComplete = false; break; }

            if (val > iterBestScore) { iterBestScore = val; iterBestIdx = i; }
            if (val > alpha) alpha = val;
        }

        if (iterComplete) {
            best           = Move{ order[iterBestIdx][0], order[iterBestIdx][1] };
            completedDepth = d;

            int idx[MAX_CANDS];
            for (int i = 0; i < nCands; i++) idx[i] = i;
            std::sort(idx, idx + nCands,
                [&](int a, int b) { return scored[a] > scored[b]; });

            int newOrder[MAX_CANDS][2];
            for (int i = 0; i < nCands; i++) {
                newOrder[i][0] = order[idx[i]][0];
                newOrder[i][1] = order[idx[i]][1];
            }
            for (int i = 0; i < nCands; i++) {
                order[i][0] = newOrder[i][0];
                order[i][1] = newOrder[i][1];
            }
        }
    }

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
    s_bench.depth             = completedDepth;
    s_bench.rootCandidates    = nCands;
    s_bench.moveNumber        = s_moveNumber;
    LogBenchmark(s_bench, state.difficulty);

    return best;
}
