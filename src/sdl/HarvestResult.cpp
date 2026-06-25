#include "sdl/HarvestResult.h"
#include "sdl/Easing.h"
#include "sdl/UIManager.h"
#include "sdl/Renderer.h"
#include <cmath>
#include <algorithm>

static const int CX = WINDOW_WIDTH  / 2;
static const int CY = WINDOW_HEIGHT / 2;

static const int RESULT_BASKET_CX    = WINDOW_WIDTH / 2;
static const int RESULT_BASKET_CY    = 560;
static const int RESULT_BASKET_W     = 470;
static const int RESULT_BASKET_H     = 300;
static const int RESULT_RIBBON_W     = 580;
static const int RESULT_RIBBON_H     = 104;
static const int RESULT_RIBBON_DY    = 92;
static const int RESULT_MSG_FONT     = 2;
static const int RESULT_SUBTITLE_FONT= 1;
static const int RESULT_HINT_FONT    = 1;
static const int RESULT_MSG_DY       = -6;

static const int   RESULT_SHADOW_W     = 470;
static const int   RESULT_SHADOW_H     = 74;
static const Uint8 RESULT_SHADOW_ALPHA = 70;
static const SDL_Color RESULT_SHADOW_COLOR = { 30, 18, 10, 255 };

static const float WIN_GLOW_END     = 0.45f;
static const float WIN_LIFT_END     = 1.30f;
static const float WIN_BASKET_START = 1.00f;
static const float WIN_BASKET_REST  = 1.85f;
static const float WIN_IDLE_START   = 1.95f;
static const float LIFT_STAGGER     = 0.08f;
static const float LIFT_CLONE_DUR   = 0.55f;
static const float DRAW_DROP_END    = 0.60f;

struct ResultSpec {
    const char* message;
    const char* subtitle;
    const char* basketPath;
    const char* ribbonPath;
    SDL_Color   accent;
    SDL_Color   dimColor;
    Uint8       dimAlpha;
};
static const ResultSpec kSpecs[3] = {

    { "It's Strawberry Festival!", "A sweet row came into bloom.",
      "assets/ui/result_basket_strawberry.png", "assets/ui/result_ribbon_red.png",
      {206, 58, 50, 255}, {16, 6, 4, 255}, 140 },

    { "It's Blueberry Season!", "A quiet line took root.",
      "assets/ui/result_basket_blueberry.png", "assets/ui/result_ribbon_blue.png",
      {78, 110, 192, 255}, {6, 8, 20, 255}, 140 },
    { "The Grove Is Full", "Every patch has been planted.",
      "assets/ui/result_basket_mixed.png", "assets/ui/result_ribbon_neutral.png",
      {216, 182, 120, 255}, {18, 16, 14, 255}, 120 },
};

static SDL_Renderer* s_r = nullptr;
static SDL_Texture*  s_basketTex[3] = {};
static SDL_Texture*  s_ribbonTex[3] = {};
static SDL_Texture*  s_ribbonBakedTex[3] = {};
static SDL_Rect      s_basketSrc[3] = {};
static SDL_Rect      s_ribbonSrc[3] = {};
static SDL_Rect      s_ribbonBakedSrc[3] = {};

static const char* kRibbonBakedPaths[3] = {
    "assets/ui/result_ribbon_red_text.png",
    "assets/ui/result_ribbon_blue_text.png",
    "assets/ui/result_ribbon_neutral_text.png",
};

static HarvestPhase      s_phase = RESULT_NONE;
static HarvestResultType s_type  = DRAW_RESULT;
static bool              s_isWin = false;
static float             s_timer = 0.0f;
static int               s_winCellX[WIN_COUNT];
static int               s_winCellY[WIN_COUNT];
static int               s_winRow[WIN_COUNT];
static int               s_winCol[WIN_COUNT];
static int               s_winCount = 0;

static SDL_Texture* LoadCropped(SDL_Renderer* r, const char* path, SDL_Rect* outSrc) {
    *outSrc = { 0, 0, 0, 0 };
    SDL_Surface* s = IMG_Load(path);
    if (!s) return nullptr;
    SDL_Surface* c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(s);
    if (!c) return nullptr;

    int W = c->w, H = c->h, minX = W, minY = H, maxX = -1, maxY = -1;
    SDL_LockSurface(c);
    const Uint32* px = static_cast<const Uint32*>(c->pixels);
    const int pitch = c->pitch / 4;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Uint8 rr, gg, bb, aa;
            SDL_GetRGBA(px[y * pitch + x], c->format, &rr, &gg, &bb, &aa);
            if (aa > 16) {
                if (x < minX) minX = x; if (x > maxX) maxX = x;
                if (y < minY) minY = y; if (y > maxY) maxY = y;
            }
        }
    }
    SDL_UnlockSurface(c);

    SDL_Texture* t = SDL_CreateTextureFromSurface(r, c);
    SDL_FreeSurface(c);
    if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    if (maxX >= minX && maxY >= minY)
        *outSrc = { minX, minY, maxX - minX + 1, maxY - minY + 1 };
    return t;
}

void HarvestResult_Init(SDL_Renderer* r) {
    s_r = r;
    for (int i = 0; i < 3; i++) {
        s_basketTex[i]      = LoadCropped(r, kSpecs[i].basketPath,   &s_basketSrc[i]);
        s_ribbonTex[i]      = LoadCropped(r, kSpecs[i].ribbonPath,   &s_ribbonSrc[i]);
        s_ribbonBakedTex[i] = LoadCropped(r, kRibbonBakedPaths[i],  &s_ribbonBakedSrc[i]);
    }
}

void HarvestResult_Shutdown() {
    auto D = [](SDL_Texture*& t){ if (t) { SDL_DestroyTexture(t); t = nullptr; } };
    for (int i = 0; i < 3; i++) { D(s_basketTex[i]); D(s_ribbonTex[i]); D(s_ribbonBakedTex[i]); }
    s_r = nullptr;
}

void HarvestResult_Start(const _GAMESTATE& state, int gameResult) {
    if      (gameResult == P1_THANG) s_type = STRAWBERRY_RESULT;
    else if (gameResult == P2_THANG) s_type = BLUEBERRY_RESULT;
    else                             s_type = DRAW_RESULT;
    s_isWin = (s_type != DRAW_RESULT);

    s_winCount = 0;
    if (s_isWin) {
        for (int k = 0; k < WIN_COUNT; k++) {
            int col = state._WIN_CELLS[k].x;
            int row = state._WIN_CELLS[k].y;
            if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) continue;
            s_winRow[s_winCount]  = row;
            s_winCol[s_winCount]  = col;
            s_winCellX[s_winCount] = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
            s_winCellY[s_winCount] = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;
            s_winCount++;
        }
    }
    s_phase = s_isWin ? RESULT_HARVEST_WIN_LINE : RESULT_BASKET_DROP;
    s_timer = 0.0f;
}

void HarvestResult_Reset() {
    s_phase = RESULT_NONE;
    s_timer = 0.0f;
    s_winCount = 0;
}

void HarvestResult_Update(float dt) {
    if (s_phase == RESULT_NONE) return;
    s_timer += dt;
    if (s_isWin) {
        if      (s_timer < WIN_GLOW_END)   s_phase = RESULT_HARVEST_WIN_LINE;
        else if (s_timer < WIN_LIFT_END)   s_phase = RESULT_FRUIT_LIFT;
        else if (s_timer < WIN_IDLE_START) s_phase = RESULT_BASKET_DROP;
        else                               s_phase = RESULT_BASKET_IDLE;
    } else {
        s_phase = (s_timer < DRAW_DROP_END) ? RESULT_BASKET_DROP : RESULT_BASKET_IDLE;
    }
}

bool HarvestResult_IsActive()    { return s_phase != RESULT_NONE; }
bool HarvestResult_IsAnimating() {
    return s_phase != RESULT_NONE && s_phase != RESULT_BASKET_IDLE;
}
HarvestPhase      HarvestResult_Phase() { return s_phase; }
HarvestResultType HarvestResult_Type()  { return s_type; }

bool HarvestResult_ShouldHideCell(int row, int col) {
    if (!s_isWin || s_winCount == 0) return false;
    if (s_phase != RESULT_HARVEST_WIN_LINE && s_phase != RESULT_FRUIT_LIFT) return false;
    for (int k = 0; k < s_winCount; k++)
        if (s_winRow[k] == row && s_winCol[k] == col) return true;
    return false;
}

static void FillCircle(SDL_Renderer* r, int cx, int cy, int rad) {
    for (int dy = -rad; dy <= rad; dy++) {
        int dx = (int)std::sqrt((float)(rad*rad - dy*dy));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void DrawSoftEllipse(SDL_Renderer* r, int cx, int cy, int w, int h,
                            SDL_Color c, Uint8 maxAlpha) {
    if (w <= 0 || h <= 0) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    const int L = 6;
    Uint8 layerA = (Uint8)std::max(1, maxAlpha / L);
    for (int i = L; i >= 1; i--) {
        float f = 0.35f + 0.65f * (float)i / L;
        int ew = (int)(w * f), eh = (int)(h * f);
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, layerA);
        for (int dy = -eh / 2; dy <= eh / 2; dy++) {
            float t = (eh > 0) ? (float)dy / (eh / 2.0f) : 0.0f;
            int dx = (int)((ew / 2.0f) * std::sqrt(std::max(0.0f, 1.0f - t * t)));
            SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void DrawHarvestClones(SDL_Renderer* r, float mouthY) {
    if (s_winCount == 0) return;
    int factionIdx = (s_type == BLUEBERRY_RESULT) ? 1 : 0;
    SDL_Texture* tex = Renderer_GetGrowthFinalTexture(factionIdx);
    if (!tex) tex = Renderer_GetPieceTexture(factionIdx);
    if (!tex) return;
    int base = (int)(Renderer_GetFactionPieceSize(factionIdx) * 1.6f);
    const SDL_Color a = kSpecs[s_type].accent;

    for (int k = 0; k < s_winCount; k++) {
        float sx = (float)s_winCellX[k], sy = (float)s_winCellY[k];
        float px = sx, py = sy, scale = 1.0f; Uint8 alpha = 255;

        if (s_phase == RESULT_HARVEST_WIN_LINE) {
            float g = EaseOutCubic(Clamp01(s_timer / WIN_GLOW_END));
            scale = 1.0f + 0.12f * g;
        } else {
            float tLift = s_timer - WIN_GLOW_END;
            float tk = (tLift - k * LIFT_STAGGER) / LIFT_CLONE_DUR;
            if (tk <= 0.0f) {
                scale = 1.12f;
            } else if (tk >= 1.0f) {
                continue;
            } else {
                float e = EaseOutCubic(tk);
                float endX = (float)CX;
                float endY = mouthY;
                float ctrlX = (sx + endX) * 0.5f;
                float ctrlY = sy - 190.0f;
                float u = 1.0f - e;
                px = u*u*sx + 2*u*e*ctrlX + e*e*endX;
                py = u*u*sy + 2*u*e*ctrlY + e*e*endY;
                scale = 1.12f - 0.54f * e;
                alpha = (tk < 0.85f) ? 255 : (Uint8)(255.0f * (1.0f - (tk - 0.85f) / 0.15f));
            }
        }

        int sz = (int)(base * scale);

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (int ring = 2; ring >= 1; ring--) {
            int e2 = ring * 10;
            SDL_SetRenderDrawColor(r, a.r, a.g, a.b, (Uint8)((alpha / 255.0f) * (18 - ring * 5)));
            SDL_Rect hr = { (int)px - sz/2 - e2, (int)py - sz/2 - e2, sz + e2*2, sz + e2*2 };
            SDL_RenderFillRect(r, &hr);
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

        SDL_SetTextureAlphaMod(tex, alpha);
        SDL_Rect d = { (int)px - sz/2, (int)py - sz/2, sz, sz };
        SDL_RenderCopy(r, tex, nullptr, &d);
        SDL_SetTextureAlphaMod(tex, 255);
    }
}

static void DrawWinLineGlow(SDL_Renderer* r) {
    if (s_winCount == 0) return;
    float pulse = (std::sin(SDL_GetTicks() * 0.006f) + 1.0f) * 0.5f;
    const SDL_Color a = kSpecs[s_type].accent;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int k = 0; k < s_winCount; k++) {
        int half = CELL_SIZE / 2;
        for (int ring = 3; ring >= 1; ring--) {
            int expand = ring * (7 + (int)(pulse * 5));
            Uint8 al = (Uint8)((28 - ring * 6) + pulse * 22);
            SDL_SetRenderDrawColor(r, a.r, a.g, a.b, al);
            SDL_Rect g = { s_winCellX[k]-half-expand, s_winCellY[k]-half-expand,
                           CELL_SIZE + expand*2, CELL_SIZE + expand*2 };
            SDL_RenderFillRect(r, &g);
        }

        SDL_SetRenderDrawColor(r, 255, 240, 190, (Uint8)(40 + pulse * 60));
        SDL_Rect inner = { s_winCellX[k]-half+4, s_winCellY[k]-half+4, CELL_SIZE-8, CELL_SIZE-8 };
        SDL_RenderFillRect(r, &inner);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void DrawFallbackBasket(SDL_Renderer* r, int cx, int topY) {
    const int topW = 360, botW = 250, bodyH = 220;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int yy = 0; yy <= bodyH; yy++) {
        float f = (float)yy / bodyH;
        int w = (int)(topW + (botW - topW) * f);
        bool weave = ((yy / 8) % 2) == 0;
        SDL_SetRenderDrawColor(r, weave ? 150 : 128, weave ? 100 : 84, weave ? 58 : 46, 255);
        SDL_RenderDrawLine(r, cx - w/2, topY + yy, cx + w/2, topY + yy);
    }

    SDL_SetRenderDrawColor(r, 120, 78, 42, 255);
    SDL_Rect rim = { cx - topW/2 - 6, topY - 16, topW + 12, 26 };
    SDL_RenderFillRect(r, &rim);
    SDL_SetRenderDrawColor(r, 168, 120, 70, 255);
    SDL_Rect rimTop = { cx - topW/2 - 6, topY - 16, topW + 12, 6 };
    SDL_RenderFillRect(r, &rimTop);

    auto berry = [&](int bx, int by, SDL_Color c){
        SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255); FillCircle(r, bx, by, 16);
        SDL_SetRenderDrawColor(r, 90, 150, 70, 255);   FillCircle(r, bx+10, by-14, 4);
    };
    SDL_Color RED = {200, 56, 50, 255}, BLUE = {72, 104, 188, 255};
    if (s_type == STRAWBERRY_RESULT) {
        berry(cx-90, topY-6, RED); berry(cx-30, topY-12, RED); berry(cx+34, topY-8, RED); berry(cx+96, topY-4, RED);
    } else if (s_type == BLUEBERRY_RESULT) {
        berry(cx-90, topY-6, BLUE); berry(cx-30, topY-12, BLUE); berry(cx+34, topY-8, BLUE); berry(cx+96, topY-4, BLUE);
    } else {
        berry(cx-90, topY-6, RED); berry(cx-30, topY-12, BLUE); berry(cx+34, topY-8, RED); berry(cx+96, topY-4, BLUE);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void DrawFallbackRibbon(SDL_Renderer* r, int cx, int cy, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect body = { cx - w/2, cy - h/2, w, h };
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 235);
    SDL_RenderFillRect(r, &body);

    SDL_SetRenderDrawColor(r, (Uint8)std::min(255, c.r+40), (Uint8)std::min(255, c.g+40), (Uint8)std::min(255, c.b+40), 210);
    SDL_Rect hi = { body.x, body.y, w, 6 };  SDL_RenderFillRect(r, &hi);
    SDL_SetRenderDrawColor(r, (Uint8)(c.r*0.6f), (Uint8)(c.g*0.6f), (Uint8)(c.b*0.6f), 220);
    SDL_Rect lo = { body.x, body.y + h - 6, w, 6 };  SDL_RenderFillRect(r, &lo);

    SDL_SetRenderDrawColor(r, (Uint8)(c.r*0.55f), (Uint8)(c.g*0.55f), (Uint8)(c.b*0.55f), 235);
    SDL_Rect tl = { body.x - 22, cy - h/2 + 10, 22, h - 20 }; SDL_RenderFillRect(r, &tl);
    SDL_Rect tr = { body.x + w, cy - h/2 + 10, 22, h - 20 };  SDL_RenderFillRect(r, &tr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void HarvestResult_Render(SDL_Renderer* r, const _GAMESTATE& /*state*/) {
    if (s_phase == RESULT_NONE) return;
    const ResultSpec& spec = kSpecs[s_type];

    float dimProg = s_isWin ? Clamp01((s_timer - 0.70f) / 1.00f)
                            : Clamp01(s_timer / 0.50f);
    Uint8 dimA = (Uint8)(spec.dimAlpha * dimProg);
    if (dimA > 0) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, spec.dimColor.r, spec.dimColor.g, spec.dimColor.b, dimA);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(r, &full);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    if (s_phase == RESULT_HARVEST_WIN_LINE) DrawWinLineGlow(r);

    bool showBasket = s_isWin ? (s_timer >= WIN_BASKET_START)
                              : (s_phase == RESULT_BASKET_DROP || s_phase == RESULT_BASKET_IDLE);
    float dropP = s_isWin ? Clamp01((s_timer - WIN_BASKET_START) / (WIN_BASKET_REST - WIN_BASKET_START))
                          : Clamp01(s_timer / DRAW_DROP_END);
    bool haveBasket = (s_basketTex[s_type] && s_basketSrc[s_type].w > 0);
    int basketH = haveBasket ? RESULT_BASKET_W * s_basketSrc[s_type].h / s_basketSrc[s_type].w
                             : RESULT_BASKET_H;
    int basketCy = (int)((CY - 360) + (RESULT_BASKET_CY - (CY - 360)) * EaseOutBack(dropP));
    int basketBottom = basketCy + basketH / 2;

    if (showBasket) {
        DrawSoftEllipse(r, RESULT_BASKET_CX, basketBottom - 10,
                        RESULT_SHADOW_W, RESULT_SHADOW_H, RESULT_SHADOW_COLOR,
                        (Uint8)(RESULT_SHADOW_ALPHA * dropP));
        if (haveBasket) {
            SDL_Rect d = { RESULT_BASKET_CX - RESULT_BASKET_W/2, basketCy - basketH/2,
                           RESULT_BASKET_W, basketH };
            SDL_RenderCopy(r, s_basketTex[s_type], &s_basketSrc[s_type], &d);
        } else {
            DrawFallbackBasket(r, RESULT_BASKET_CX, basketCy - basketH/2 + 20);
        }
    }

    if (s_phase == RESULT_HARVEST_WIN_LINE || s_phase == RESULT_FRUIT_LIFT)
        DrawHarvestClones(r, (float)(RESULT_BASKET_CY - 90));

    if (showBasket) {
        int ribbonCy = basketCy + RESULT_RIBBON_DY;
        bool useBaked = s_ribbonBakedTex[s_type] && s_ribbonBakedSrc[s_type].w > 0;
        if (useBaked) {
            int rw = RESULT_RIBBON_W;
            int rh = rw * s_ribbonBakedSrc[s_type].h / s_ribbonBakedSrc[s_type].w;
            SDL_Rect d = { RESULT_BASKET_CX - rw/2, ribbonCy - rh/2, rw, rh };
            SDL_RenderCopy(r, s_ribbonBakedTex[s_type], &s_ribbonBakedSrc[s_type], &d);
        } else {

            if (s_ribbonTex[s_type] && s_ribbonSrc[s_type].w > 0) {
                int rw = RESULT_RIBBON_W;
                int rh = rw * s_ribbonSrc[s_type].h / s_ribbonSrc[s_type].w;
                SDL_Rect d = { RESULT_BASKET_CX - rw/2, ribbonCy - rh/2, rw, rh };
                SDL_RenderCopy(r, s_ribbonTex[s_type], &s_ribbonSrc[s_type], &d);
            } else {
                DrawFallbackRibbon(r, RESULT_BASKET_CX, ribbonCy, RESULT_RIBBON_W, RESULT_RIBBON_H, spec.accent);
            }
            int msgY = ribbonCy + RESULT_MSG_DY;
            UIManager_RenderText(r, spec.message, RESULT_BASKET_CX + 2, msgY + 2, {28, 16, 8, 200}, true, RESULT_MSG_FONT);
            UIManager_RenderText(r, spec.message, RESULT_BASKET_CX,     msgY,     {255, 248, 232, 255}, true, RESULT_MSG_FONT);
        }

        if (spec.subtitle && spec.subtitle[0]) {
            int subY = basketBottom + 26;
            UIManager_RenderText(r, spec.subtitle, RESULT_BASKET_CX + 1, subY + 1, {24, 14, 6, 190}, true, RESULT_SUBTITLE_FONT);
            UIManager_RenderText(r, spec.subtitle, RESULT_BASKET_CX,     subY,     {244, 232, 210, 250}, true, RESULT_SUBTITLE_FONT);
        }

        if (s_phase == RESULT_BASKET_IDLE) {
            const char* hint = "Plant Again (R)    Leave the Grove (ESC)";
            int hy = basketBottom + 66;
            UIManager_RenderText(r, hint, RESULT_BASKET_CX + 2, hy + 2, {20, 12, 4, 220}, true, RESULT_HINT_FONT);
            UIManager_RenderText(r, hint, RESULT_BASKET_CX,     hy,     {255, 244, 224, 255}, true, RESULT_HINT_FONT);
        }
    }
}
