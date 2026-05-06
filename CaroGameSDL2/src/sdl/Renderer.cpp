#include "Renderer.h"
#include <cmath>
#include <algorithm>

// ================================================================
//  Renderer.cpp — Pure SDL2 primitives
//
//  Quân cờ: X (Player 1, đỏ) và O (Player 2, xanh dương)
//  Board: gỗ sáng với lưới nâu, 5 star points chuẩn Gomoku
// ================================================================

// ── Primitive helpers ────────────────────────────────────────────

static void DrawFilledCircle(SDL_Renderer* r, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius*radius - dy*dy)));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

// Đường thẳng dày bằng cách offset theo vector pháp tuyến
static void DrawThickLine(SDL_Renderer* r, int x1, int y1, int x2, int y2, int thickness) {
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 1.0f) return;
    float nx = -dy / len, ny = dx / len;
    int half = thickness / 2;
    for (int t = -half; t <= half; t++) {
        SDL_RenderDrawLine(r,
            x1 + static_cast<int>(nx * t + 0.5f),
            y1 + static_cast<int>(ny * t + 0.5f),
            x2 + static_cast<int>(nx * t + 0.5f),
            y2 + static_cast<int>(ny * t + 0.5f));
    }
}

// Ký hiệu X: hai đường chéo dày
static void DrawX(SDL_Renderer* r, int cx, int cy, int half, int thickness = 4) {
    DrawThickLine(r, cx - half, cy - half, cx + half, cy + half, thickness);
    DrawThickLine(r, cx + half, cy - half, cx - half, cy + half, thickness);
}

// Ký hiệu O: vòng tròn rỗng (scanline ring)
static void DrawO(SDL_Renderer* r, int cx, int cy, int radius, int thickness = 4) {
    int inner = radius - thickness;
    for (int dy = -radius; dy <= radius; dy++) {
        float dxo = std::sqrt(std::max(0.0f, static_cast<float>(radius*radius - dy*dy)));
        float dxi = (inner > 0)
                    ? std::sqrt(std::max(0.0f, static_cast<float>(inner*inner - dy*dy)))
                    : 0.0f;
        int lo = static_cast<int>(dxi) + 1;
        int hi = static_cast<int>(dxo);
        if (hi >= lo) {
            SDL_RenderDrawLine(r, cx - hi, cy + dy, cx - lo, cy + dy);
            SDL_RenderDrawLine(r, cx + lo, cy + dy, cx + hi, cy + dy);
        }
    }
}

// ── Module state ─────────────────────────────────────────────────
static SDL_Renderer* s_renderer = nullptr;

bool Renderer_Init(SDL_Renderer* renderer) {
    s_renderer = renderer;
    return renderer != nullptr;
}

void Renderer_Shutdown() {
    s_renderer = nullptr;
}

// ── Background + panel areas ─────────────────────────────────────
void Renderer_DrawBackground(SDL_Renderer* r) {
    // Toàn màn hình: xanh đậm
    SDL_SetRenderDrawColor(r, 28, 48, 28, 255);
    SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(r, &full);

    // Viền bàn cờ
    SDL_SetRenderDrawColor(r, 90, 60, 25, 255);
    SDL_Rect border = {
        BOARD_OFFSET_X - 6, BOARD_OFFSET_Y - 6,
        BOARD_PIXEL_SIZE + 12, BOARD_PIXEL_SIZE + 12
    };
    SDL_RenderFillRect(r, &border);
}

// ── Board ────────────────────────────────────────────────────────
void Renderer_DrawBoard(SDL_Renderer* r, const _GAMESTATE& /*state*/) {
    // Nền gỗ
    SDL_SetRenderDrawColor(r, 212, 172, 105, 255);
    SDL_Rect board = { BOARD_OFFSET_X, BOARD_OFFSET_Y, BOARD_PIXEL_SIZE, BOARD_PIXEL_SIZE };
    SDL_RenderFillRect(r, &board);

    // Lưới
    SDL_SetRenderDrawColor(r, 95, 65, 28, 255);
    for (int i = 0; i <= BOARD_SIZE; i++) {
        int x = BOARD_OFFSET_X + i * CELL_SIZE;
        SDL_RenderDrawLine(r, x, BOARD_OFFSET_Y, x, BOARD_OFFSET_Y + BOARD_PIXEL_SIZE);
        int y = BOARD_OFFSET_Y + i * CELL_SIZE;
        SDL_RenderDrawLine(r, BOARD_OFFSET_X, y, BOARD_OFFSET_X + BOARD_PIXEL_SIZE, y);
    }

    // Star points chuẩn Gomoku (hàng/cột 3, 7, 11 — 0-indexed)
    static const int STARS[][2] = { {3,3},{3,11},{11,3},{11,11},{7,7} };
    SDL_SetRenderDrawColor(r, 60, 40, 18, 255);
    for (auto& s : STARS) {
        int px = BOARD_OFFSET_X + s[1] * CELL_SIZE;
        int py = BOARD_OFFSET_Y + s[0] * CELL_SIZE;
        DrawFilledCircle(r, px, py, 4);
    }
}

// ── Pieces (X và O) ──────────────────────────────────────────────
void Renderer_DrawPieces(SDL_Renderer* r, const _GAMESTATE& state) {
    int half = CELL_SIZE / 2 - 6;   // kích thước ký hiệu trong ô
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            int c = state._BOARD[row][col].c;
            if (c == 0) continue;

            int cx = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
            int cy = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;

            if (c == -1) {  // Player 1 — X đỏ
                SDL_SetRenderDrawColor(r, 210, 55, 55, 255);
                DrawX(r, cx, cy, half, 4);
            } else {         // Player 2 — O xanh
                SDL_SetRenderDrawColor(r, 55, 100, 210, 255);
                DrawO(r, cx, cy, half, 4);
            }
        }
    }
}

// ── Hover ────────────────────────────────────────────────────────
void Renderer_DrawHover(SDL_Renderer* r, const _GAMESTATE& state) {
    int row = state.hoveredRow, col = state.hoveredCol;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    if (state._BOARD[row][col].c != 0) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 240, 0, 75);
    SDL_Rect cell = {
        BOARD_OFFSET_X + col * CELL_SIZE + 1,
        BOARD_OFFSET_Y + row * CELL_SIZE + 1,
        CELL_SIZE - 2, CELL_SIZE - 2
    };
    SDL_RenderFillRect(r, &cell);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// ── Win cells blink (4Hz) ─────────────────────────────────────────
void Renderer_DrawWinCells(SDL_Renderer* r, const _GAMESTATE& state) {
    if (state.gameStatus == CHUA_KET_THUC || state.gameStatus == HOA) return;

    bool visible = (SDL_GetTicks() / 125) % 2 == 0;
    if (!visible) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 220, 0, 170);

    int half = CELL_SIZE / 2 - 3;
    for (int k = 0; k < WIN_COUNT; k++) {
        int col = state._WIN_CELLS[k].x;
        int row = state._WIN_CELLS[k].y;
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) continue;
        SDL_Rect cell = {
            BOARD_OFFSET_X + col * CELL_SIZE + 1,
            BOARD_OFFSET_Y + row * CELL_SIZE + 1,
            CELL_SIZE - 2, CELL_SIZE - 2
        };
        SDL_RenderFillRect(r, &cell);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
