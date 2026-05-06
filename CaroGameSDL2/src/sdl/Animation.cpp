#include "Animation.h"
#include <cmath>

// ================================================================
//  Animation.cpp — Một nhân vật duy nhất đặt cả X lẫn O
//
//  playerIdx (0=P1/X, 1=P2/O) được lưu để đổi màu nhân vật
//  theo lượt, nhưng vẫn là một thực thể duy nhất.
//
//  App_PlacePiece được forward-declared để tránh circular include.
// ================================================================

void App_PlacePiece(_GAMESTATE& state, int row, int col);
void App_CellToPixel(int row, int col, int& outX, int& outY);

static const float WALK_SPEED  = 420.0f;  // px/s
static const float PLACE_DELAY = 0.22f;   // s

struct CharState {
    AnimState anim       = ANIM_IDLE;
    Direction dir        = DIR_DOWN;
    float     x          = 0.0f;
    float     y          = 0.0f;
    float     targetX    = 0.0f;
    float     targetY    = 0.0f;
    int       targetRow  = -1;
    int       targetCol  = -1;
    int       playerIdx  = 0;    // 0=X(đỏ), 1=O(xanh) — đổi màu theo lượt
    float     placeTimer = 0.0f;
    float     celebTimer = 0.0f;
};

static CharState s_char;

static void ResetChar() {
    // Vị trí ban đầu: giữa-dưới bàn cờ
    s_char      = {};
    s_char.x    = static_cast<float>(BOARD_OFFSET_X + BOARD_PIXEL_SIZE / 2);
    s_char.y    = static_cast<float>(BOARD_OFFSET_Y + BOARD_PIXEL_SIZE / 2);
    s_char.anim = ANIM_IDLE;
    s_char.dir  = DIR_DOWN;
}

void Animation_Init()     { ResetChar(); }
void Animation_Shutdown() {}
void Animation_Reset()    { ResetChar(); }

void Animation_StartMove(_GAMESTATE& state, int targetRow, int targetCol, int charIdx) {
    int px, py;
    App_CellToPixel(targetRow, targetCol, px, py);

    s_char.targetX    = static_cast<float>(px);
    s_char.targetY    = static_cast<float>(py);
    s_char.targetRow  = targetRow;
    s_char.targetCol  = targetCol;
    s_char.playerIdx  = charIdx;
    s_char.anim       = ANIM_WALK_TO_CELL;
    s_char.placeTimer = 0.0f;

    float dx = s_char.targetX - s_char.x;
    float dy = s_char.targetY - s_char.y;
    if (std::abs(dx) >= std::abs(dy))
        s_char.dir = (dx >= 0) ? DIR_RIGHT : DIR_LEFT;
    else
        s_char.dir = (dy >= 0) ? DIR_DOWN : DIR_UP;

    (void)state;
}

void Animation_StartCelebrate(_GAMESTATE& state, int charIdx) {
    s_char.playerIdx  = charIdx;
    s_char.anim       = ANIM_CELEBRATE;
    s_char.celebTimer = 2.0f;
    (void)state;
}

void Animation_Update(float dt, _GAMESTATE& state) {
    switch (s_char.anim) {
    case ANIM_WALK_TO_CELL: {
        float dx   = s_char.targetX - s_char.x;
        float dy   = s_char.targetY - s_char.y;
        float dist = std::sqrt(dx*dx + dy*dy);
        float step = WALK_SPEED * dt;

        if (dist <= step) {
            s_char.x          = s_char.targetX;
            s_char.y          = s_char.targetY;
            s_char.anim       = ANIM_PLACING;
            s_char.placeTimer = PLACE_DELAY;
        } else {
            s_char.x += dx / dist * step;
            s_char.y += dy / dist * step;
            if (std::abs(dx) >= std::abs(dy))
                s_char.dir = (dx >= 0) ? DIR_RIGHT : DIR_LEFT;
            else
                s_char.dir = (dy >= 0) ? DIR_DOWN : DIR_UP;
        }
        break;
    }
    case ANIM_PLACING:
        s_char.placeTimer -= dt;
        if (s_char.placeTimer <= 0.0f) {
            s_char.anim = ANIM_IDLE;
            App_PlacePiece(state, s_char.targetRow, s_char.targetCol);
        }
        break;

    case ANIM_CELEBRATE:
        s_char.celebTimer -= dt;
        if (s_char.celebTimer <= 0.0f)
            s_char.anim = ANIM_IDLE;
        break;

    default: break;
    }
}

void Animation_Render(SDL_Renderer* r, const _GAMESTATE& /*state*/) {
    // Màu nhân vật theo playerIdx: đỏ=P1(X), xanh=P2(O)
    Uint8 br = (s_char.playerIdx == 0) ? 210 :  60;
    Uint8 bg = (s_char.playerIdx == 0) ?  60 : 100;
    Uint8 bb = (s_char.playerIdx == 0) ?  60 : 210;

    // Celebrate: vàng rực
    if (s_char.anim == ANIM_CELEBRATE) { br = 240; bg = 200; bb = 40; }

    int bw = 20, bh = 28;
    int bx = static_cast<int>(s_char.x) - bw / 2;
    int by = static_cast<int>(s_char.y) - bh;

    // Thân
    SDL_SetRenderDrawColor(r, br, bg, bb, 255);
    SDL_Rect body = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &body);

    // Đầu
    SDL_SetRenderDrawColor(r, 235, 195, 155, 255);
    SDL_Rect head = { bx + 3, by - 14, 14, 14 };
    SDL_RenderFillRect(r, &head);

    // Đặt quân: hiệu ứng "thả" quân xuống
    if (s_char.anim == ANIM_PLACING) {
        float prog = 1.0f - (s_char.placeTimer / PLACE_DELAY);
        int   dropY = static_cast<int>(s_char.y - 12 + prog * 16);
        SDL_SetRenderDrawColor(r, 255, 230, 60, 255);
        SDL_Rect drop = { static_cast<int>(s_char.x) - 6, dropY, 12, 12 };
        SDL_RenderFillRect(r, &drop);
    }

    // Celebrate: nhảy lên
    if (s_char.anim == ANIM_CELEBRATE) {
        float bounce = std::abs(std::sin(s_char.celebTimer * 9.0f)) * 10.0f;
        SDL_SetRenderDrawColor(r, 255, 255, 60, 255);
        SDL_Rect star = { bx + 3, by - static_cast<int>(bounce) - 18, 14, 14 };
        SDL_RenderFillRect(r, &star);
    }
}

bool Animation_IsPlaying() {
    return s_char.anim == ANIM_WALK_TO_CELL || s_char.anim == ANIM_PLACING;
}
