#include "WinEffect.h"
#include "Renderer.h"
#include "UIManager.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>

// ================================================================
//  WinEffect.cpp — Hiệu ứng chiến thắng
//
//  GLOW:     5 quân thắng phát sáng, nhấp nháy vàng (1.2s)
//  CONVERGE: 5 quân bay về tâm màn hình, thu nhỏ dần (0.9s)
//  ZOOM:     Ký hiệu X/O lớn phóng to từ tâm với easeOutBack (0.7s)
//  ANNOUNCE: Overlay tối, ký hiệu cố định, tên người thắng, R/ESC hint
// ================================================================

static const float GLOW_DUR     = 1.2f;
static const float CONVERGE_DUR = 0.9f;
static const float ZOOM_DUR     = 0.7f;

static const int SCR_CX = WINDOW_WIDTH  / 2;   // 550
static const int SCR_CY = WINDOW_HEIGHT / 2;   // 390

// Tâm ký hiệu lớn — dịch lên để nhường chỗ cho text
static const int SYM_CX = SCR_CX;
static const int SYM_CY = SCR_CY - 50;

enum WinPhase { WIN_IDLE, WIN_GLOW, WIN_CONVERGE, WIN_ZOOM, WIN_ANNOUNCE };

struct WinParticle { float startX, startY; };

static WinPhase    s_phase = WIN_IDLE;
static float       s_timer = 0.0f;
static bool        s_isX   = false;
static char        s_name[30] = {};
static WinParticle s_particles[WIN_COUNT];

// ── Easing ───────────────────────────────────────────────────────

static float EaseInOutQuad(float t) {
    return t < 0.5f
        ? 2.0f * t * t
        : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * 0.5f;
}

static float EaseOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    float tm = t - 1.0f;
    return 1.0f + c3 * tm * tm * tm + c1 * tm * tm;
}

// ── Public API ───────────────────────────────────────────────────

void WinEffect_Start(const _GAMESTATE& state) {
    // ProcessFinish không lật turn khi thắng → state.turn = lượt người vừa thắng
    s_isX = state.turn;  // true = P1 (X), false = P2 (O)
    int pidx = state.turn ? 0 : 1;
    strncpy_s(s_name, sizeof(s_name), state.players[pidx].name, _TRUNCATE);

    for (int k = 0; k < WIN_COUNT; k++) {
        int col = state._WIN_CELLS[k].x;
        int row = state._WIN_CELLS[k].y;
        s_particles[k].startX = static_cast<float>(
            BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2);
        s_particles[k].startY = static_cast<float>(
            BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2);
    }

    s_phase = WIN_GLOW;
    s_timer = 0.0f;
}

void WinEffect_Update(float dt) {
    switch (s_phase) {
    case WIN_GLOW:
        s_timer += dt;
        if (s_timer >= GLOW_DUR) { s_timer = 0.0f; s_phase = WIN_CONVERGE; }
        break;
    case WIN_CONVERGE:
        s_timer += dt;
        if (s_timer >= CONVERGE_DUR) { s_timer = 0.0f; s_phase = WIN_ZOOM; }
        break;
    case WIN_ZOOM:
        s_timer += dt;
        if (s_timer >= ZOOM_DUR) { s_timer = ZOOM_DUR; s_phase = WIN_ANNOUNCE; }
        break;
    default: break;
    }
}

void WinEffect_Reset() {
    s_phase = WIN_IDLE;
    s_timer = 0.0f;
}

bool WinEffect_IsPlaying()   { return s_phase != WIN_IDLE; }
bool WinEffect_IsAnimating() {
    return s_phase == WIN_GLOW || s_phase == WIN_CONVERGE || s_phase == WIN_ZOOM;
}

// ── Render internals ─────────────────────────────────────────────

static void DrawOverlay(SDL_Renderer* r, Uint8 alpha) {
    if (alpha == 0) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, alpha);
    SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void DrawBigSymbol(SDL_Renderer* r, int cx, int cy, int half) {
    if (half <= 0) return;
    int thickness = std::max(4, half / 10);
    if (s_isX) {
        SDL_SetRenderDrawColor(r, 230, 80, 80, 255);
        Renderer_DrawSymbolX(r, cx, cy, half, thickness);
    } else {
        SDL_SetRenderDrawColor(r, 80, 130, 230, 255);
        Renderer_DrawSymbolO(r, cx, cy, half, thickness);
    }
}

static void RenderGlow(SDL_Renderer* r) {
    // Pulse 3Hz: 0..1
    float pulse = (sinf(s_timer * 2.0f * 3.14159f * 3.0f) + 1.0f) * 0.5f;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int k = 0; k < WIN_COUNT; k++) {
        int pcx  = static_cast<int>(s_particles[k].startX);
        int pcy  = static_cast<int>(s_particles[k].startY);
        int half = CELL_SIZE / 2;

        // Vầng sáng ngoài (halo), kích thước nhấp nháy
        int expand = 8 + static_cast<int>(pulse * 7.0f);
        SDL_SetRenderDrawColor(r, 255, 215, 0,
            static_cast<Uint8>(45.0f + pulse * 55.0f));
        SDL_Rect halo = { pcx - half - expand, pcy - half - expand,
                          CELL_SIZE + expand * 2, CELL_SIZE + expand * 2 };
        SDL_RenderFillRect(r, &halo);

        // Highlight trong ô
        SDL_SetRenderDrawColor(r, 255, 255, 180,
            static_cast<Uint8>(55.0f + pulse * 90.0f));
        SDL_Rect inner = { pcx - half + 2, pcy - half + 2,
                           CELL_SIZE - 4, CELL_SIZE - 4 };
        SDL_RenderFillRect(r, &inner);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void RenderConverge(SDL_Renderer* r) {
    float t  = s_timer / CONVERGE_DUR;
    float et = EaseInOutQuad(t);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int k = 0; k < WIN_COUNT; k++) {
        // Nội suy từ vị trí ô → tâm màn hình
        float px = s_particles[k].startX + (SYM_CX - s_particles[k].startX) * et;
        float py = s_particles[k].startY + (SYM_CY - s_particles[k].startY) * et;

        // Thu nhỏ và mờ dần khi về trung tâm
        int  symHalf  = std::max(3, static_cast<int>(16.0f * (1.0f - et * 0.75f)));
        Uint8 alpha   = static_cast<Uint8>(255.0f * (1.0f - et * 0.5f));

        if (s_isX) {
            SDL_SetRenderDrawColor(r, 230, 80, 80, alpha);
            Renderer_DrawSymbolX(r,
                static_cast<int>(px), static_cast<int>(py), symHalf, 3);
        } else {
            SDL_SetRenderDrawColor(r, 80, 130, 230, alpha);
            Renderer_DrawSymbolO(r,
                static_cast<int>(px), static_cast<int>(py), symHalf, 3);
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void RenderZoom(SDL_Renderer* r) {
    float t  = std::min(s_timer / ZOOM_DUR, 1.0f);
    float et = EaseOutBack(t);

    // Overlay mờ dần vào
    DrawOverlay(r, static_cast<Uint8>(t * 185.0f));

    // Ký hiệu phóng to với easeOutBack (có thể vượt rồi bật lại)
    int symHalf = static_cast<int>(120.0f * std::max(0.0f, et));
    DrawBigSymbol(r, SYM_CX, SYM_CY, symHalf);
}

static void RenderAnnounce(SDL_Renderer* r) {
    // Overlay tối cố định
    DrawOverlay(r, 185);

    // Ký hiệu lớn cố định
    DrawBigSymbol(r, SYM_CX, SYM_CY, 120);

    // "NAME WINS!"
    char msg[64];
    snprintf(msg, sizeof(msg), "%s WINS!", s_name);
    UIManager_RenderText(r, msg,
        SCR_CX, SCR_CY + 90, { 255, 220, 50, 255 }, true, 2);

    // R / ESC hint
    UIManager_RenderText(r, "R: Choi lai       ESC: Menu",
        SCR_CX, SCR_CY + 148, { 200, 200, 200, 210 }, true, 0);
}

void WinEffect_Render(SDL_Renderer* r) {
    switch (s_phase) {
    case WIN_GLOW:     RenderGlow(r);     break;
    case WIN_CONVERGE: RenderConverge(r); break;
    case WIN_ZOOM:     RenderZoom(r);     break;
    case WIN_ANNOUNCE: RenderAnnounce(r); break;
    default: break;
    }
}
