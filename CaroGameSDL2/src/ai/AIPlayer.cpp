#include "AIPlayer.h"
#include <algorithm>
#include <climits>

// ================================================================
//  AIPlayer.cpp — Minimax with alpha-beta pruning
//
//  Encoding: AI = O (c=1), human = X (c=-1)
//  state.turn: true = Player1's turn (human in PVE), false = AI's turn
//  depth is set by state.difficulty (AI_EASY=2, AI_MEDIUM=4, AI_HARD=6)
// ================================================================

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
static int EvaluateBoard(const _GAMESTATE& state) {
    static const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };
    int score = 0;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
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
    if (depth == 0)    return EvaluateBoard(state);

    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int nCands = 0;
    GetCandidates(state, cands, nCands);
    if (nCands == 0) return 0;

    int color = isMaximising ? 1 : -1;
    int best  = isMaximising ? INT_MIN : INT_MAX;

    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        state._BOARD[r][c].c = color;

        int val = Minimax(state, depth - 1, alpha, beta, !isMaximising, r, c);

        state._BOARD[r][c].c = 0;  // undo

        if (isMaximising) {
            best  = std::max(best, val);
            alpha = std::max(alpha, best);
        } else {
            best  = std::min(best, val);
            beta  = std::min(beta, best);
        }
        if (beta <= alpha) break;
    }
    return best;
}

Move AI_FindBestMove(const _GAMESTATE& state) {
    // Copy state so we can mutate during search without touching original
    _GAMESTATE working = state;
    int depth = static_cast<int>(state.difficulty);

    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int nCands = 0;
    GetCandidates(working, cands, nCands);

    Move best;
    int bestScore = INT_MIN;

    for (int i = 0; i < nCands; i++) {
        int r = cands[i][0], c = cands[i][1];
        working._BOARD[r][c].c = 1;  // AI plays O

        int score = Minimax(working, depth - 1, INT_MIN, INT_MAX, false, r, c);
        working._BOARD[r][c].c = 0;

        if (score > bestScore) {
            bestScore = score;
            best.row  = r;
            best.col  = c;
        }
    }
    return best;
}
