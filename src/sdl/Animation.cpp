#include "Animation.h"
#include <cmath>

// ================================================================
//  Animation.cpp — Hai nhân vật độc lập, thay phiên đặt quân
//
//  s_chars[0] = P1 (đỏ), đứng panel trái
//  s_chars[1] = P2/AI (xanh), đứng panel phải
//
//  Mỗi lượt: nhân vật đang đến lượt đi đến ô → placing → idle
//  Nhân vật kia đứng yên tại vị trí cuối cùng của họ.
// ================================================================

void App_PlacePiece(_GAMESTATE& state, int row, int col);
void App_CellToPixel(int row, int col, int& outX, int& outY);

static const float WALK_SPEED  = 420.0f;
static const float PLACE_DELAY = 0.22f;

// Vị trí home mỗi nhân vật (giữa panel trái / phải)
static const float HOME_X[2] = {
    static_cast<float>(LEFT_PANEL_X  + LEFT_PANEL_WIDTH  / 2),  // P1: 110px
    static_cast<float>(HUD_PANEL_X   + HUD_PANEL_WIDTH   / 2),  // P2: 990px
};
static const float HOME_Y = static_cast<float>(BOARD_OFFSET_Y + BOARD_PIXEL_SIZE / 2);

struct CharState {
    AnimState anim       = ANIM_IDLE;
    Direction dir        = DIR_DOWN;
    float     x          = 0.0f;
    float     y          = 0.0f;
    float     targetX    = 0.0f;
    float     targetY    = 0.0f;
    int       targetRow  = -1;
    int       targetCol  = -1;
    int       playerIdx  = 0;
    float     placeTimer = 0.0f;
    float     celebTimer = 0.0f;
};

static CharState s_chars[2];

static void ResetChar(int idx) {
    s_chars[idx]           = {};
    s_chars[idx].playerIdx = idx;
    s_chars[idx].x         = HOME_X[idx];
    s_chars[idx].y         = HOME_Y;
    s_chars[idx].anim      = ANIM_IDLE;
    s_chars[idx].dir       = DIR_DOWN;
}

void Animation_Init()     { ResetChar(0); ResetChar(1); }
void Animation_Shutdown() {}
void Animation_Reset()    { ResetChar(0); ResetChar(1); }

void Animation_StartMove(_GAMESTATE& state, int targetRow, int targetCol, int charIdx) {
    int px, py;
    App_CellToPixel(targetRow, targetCol, px, py);

    CharState& ch = s_chars[charIdx];
    ch.targetX    = static_cast<float>(px);
    ch.targetY    = static_cast<float>(py);
    ch.targetRow  = targetRow;
    ch.targetCol  = targetCol;
    ch.playerIdx  = charIdx;
    ch.anim       = ANIM_WALK_TO_CELL;
    ch.placeTimer = 0.0f;

    float dx = ch.targetX - ch.x;
    float dy = ch.targetY - ch.y;
    if (std::abs(dx) >= std::abs(dy))
        ch.dir = (dx >= 0) ? DIR_RIGHT : DIR_LEFT;
    else
        ch.dir = (dy >= 0) ? DIR_DOWN : DIR_UP;

    (void)state;
}

void Animation_StartCelebrate(_GAMESTATE& state, int charIdx) {
    s_chars[charIdx].anim       = ANIM_CELEBRATE;
    s_chars[charIdx].celebTimer = 2.0f;
    (void)state;
}

void Animation_Update(float dt, _GAMESTATE& state) {
    for (int i = 0; i < 2; i++) {
        CharState& ch = s_chars[i];
        switch (ch.anim) {
        case ANIM_WALK_TO_CELL: {
            float dx   = ch.targetX - ch.x;
            float dy   = ch.targetY - ch.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            float step = WALK_SPEED * dt;

            if (dist <= step) {
                ch.x          = ch.targetX;
                ch.y          = ch.targetY;
                ch.anim       = ANIM_PLACING;
                ch.placeTimer = PLACE_DELAY;
            } else {
                ch.x += dx / dist * step;
                ch.y += dy / dist * step;
                if (std::abs(dx) >= std::abs(dy))
                    ch.dir = (dx >= 0) ? DIR_RIGHT : DIR_LEFT;
                else
                    ch.dir = (dy >= 0) ? DIR_DOWN : DIR_UP;
            }
            break;
        }
        case ANIM_PLACING:
            ch.placeTimer -= dt;
            if (ch.placeTimer <= 0.0f) {
                ch.anim = ANIM_IDLE;
                App_PlacePiece(state, ch.targetRow, ch.targetCol);
            }
            break;

        case ANIM_CELEBRATE:
            ch.celebTimer -= dt;
            if (ch.celebTimer <= 0.0f)
                ch.anim = ANIM_IDLE;
            break;

        default: break;
        }
    }
}

static void RenderChar(SDL_Renderer* r, const CharState& ch) {
    Uint8 br = (ch.playerIdx == 0) ? 210 :  60;
    Uint8 bg = (ch.playerIdx == 0) ?  60 : 100;
    Uint8 bb = (ch.playerIdx == 0) ?  60 : 210;

    if (ch.anim == ANIM_CELEBRATE) { br = 240; bg = 200; bb = 40; }

    int bw = 20, bh = 28;
    int bx = static_cast<int>(ch.x) - bw / 2;
    int by = static_cast<int>(ch.y) - bh;

    // Thân
    SDL_SetRenderDrawColor(r, br, bg, bb, 255);
    SDL_Rect body = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &body);

    // Đầu
    SDL_SetRenderDrawColor(r, 235, 195, 155, 255);
    SDL_Rect head = { bx + 3, by - 14, 14, 14 };
    SDL_RenderFillRect(r, &head);

    // Đặt quân: hiệu ứng "thả" quân xuống
    if (ch.anim == ANIM_PLACING) {
        float prog = 1.0f - (ch.placeTimer / PLACE_DELAY);
        int   dropY = static_cast<int>(ch.y - 12 + prog * 16);
        SDL_SetRenderDrawColor(r, 255, 230, 60, 255);
        SDL_Rect drop = { static_cast<int>(ch.x) - 6, dropY, 12, 12 };
        SDL_RenderFillRect(r, &drop);
    }

    // Celebrate: nhảy lên
    if (ch.anim == ANIM_CELEBRATE) {
        float bounce = std::abs(std::sin(ch.celebTimer * 9.0f)) * 10.0f;
        SDL_SetRenderDrawColor(r, 255, 255, 60, 255);
        SDL_Rect star = { bx + 3, by - static_cast<int>(bounce) - 18, 14, 14 };
        SDL_RenderFillRect(r, &star);
    }
}

void Animation_Render(SDL_Renderer* r, const _GAMESTATE& /*state*/) {
    // Render cả 2 nhân vật — nhân vật idle đứng yên ở vị trí cuối cùng
    for (int i = 0; i < 2; i++) {
        RenderChar(r, s_chars[i]);
    }
}

bool Animation_IsPlaying() {
    for (int i = 0; i < 2; i++) {
        if (s_chars[i].anim == ANIM_WALK_TO_CELL || s_chars[i].anim == ANIM_PLACING)
            return true;
    }
    return false;
}
