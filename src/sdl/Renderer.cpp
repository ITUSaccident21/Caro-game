#include "sdl/Renderer.h"
#include "sdl/Easing.h"
#include "sdl/HarvestResult.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>

static void DrawFilledCircle(SDL_Renderer* r, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius*radius - dy*dy)));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

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

static float MaxFloat(float a, float b) { return (a > b) ? a : b; }

static void DrawX(SDL_Renderer* r, int cx, int cy, int half, int thickness = 4) {
    DrawThickLine(r, cx - half, cy - half, cx + half, cy + half, thickness);
    DrawThickLine(r, cx + half, cy - half, cx - half, cy + half, thickness);
}

static void DrawO(SDL_Renderer* r, int cx, int cy, int radius, int thickness = 4) {
    int inner = radius - thickness;
    for (int dy = -radius; dy <= radius; dy++) {
        float dxo = std::sqrt(MaxFloat(0.0f, static_cast<float>(radius*radius - dy*dy)));
        float dxi = (inner > 0)
                    ? std::sqrt(MaxFloat(0.0f, static_cast<float>(inner*inner - dy*dy)))
                    : 0.0f;
        int lo = static_cast<int>(dxi) + 1;
        int hi = static_cast<int>(dxo);
        if (hi >= lo) {
            SDL_RenderDrawLine(r, cx - hi, cy + dy, cx - lo, cy + dy);
            SDL_RenderDrawLine(r, cx + lo, cy + dy, cx + hi, cy + dy);
        }
    }
}

static SDL_Renderer* s_renderer = nullptr;

static SDL_Texture* s_sceneBg          = nullptr;
static SDL_Texture* s_sceneBoardShadow = nullptr;
static SDL_Texture* s_sceneBoardFrame  = nullptr;
static SDL_Texture* s_sceneGrid        = nullptr;
static SDL_Texture* s_scenePieceShadow = nullptr;
static SDL_Texture* s_sceneHover       = nullptr;
static SDL_Texture* s_sceneLastMove    = nullptr;
static SDL_Texture* s_sceneWinGlow     = nullptr;
static SDL_Texture* s_sceneForeground  = nullptr;
static SDL_Texture* s_pieceContactPatch = nullptr;

static SDL_Texture* s_boardSoil  = nullptr;
static SDL_Texture* s_boardGrid  = nullptr;
static SDL_Texture* s_boardFrame = nullptr;
static bool         s_boardReady = false;

static SDL_Texture* s_terrainSoil     = nullptr;
static SDL_Texture* s_terrainFurrows  = nullptr;
static SDL_Texture* s_terrainFrame    = nullptr;
static SDL_Texture* s_terrainSoilSoft = nullptr;

static SDL_Texture* s_growthTex[2][3] = { { nullptr } };
static bool         s_growthReady = false;

static SDL_Texture* s_texX = nullptr;
static SDL_Texture* s_texO = nullptr;
static int          s_pieceSize = 0;

struct CellAnimation {
    bool  active;
    float elapsed;
};
static CellAnimation s_cellAnims[BOARD_SIZE][BOARD_SIZE] = {};
static const float WIGGLE_DUR    = 0.44f;
static const float SQUASH_DUR    = 0.18f;
static const float ERUPT_DUR     = 0.82f;
static const float POP_DUR       = WIGGLE_DUR + SQUASH_DUR + ERUPT_DUR;
static const float PI_F          = 3.14159265f;
static const float PLACE_RISE_PX = 8.0f;

void Renderer_TriggerPiecePop(int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    s_cellAnims[row][col].active = true;
    s_cellAnims[row][col].elapsed = 0.0f;
}

void Renderer_ResetCellAnims() {
    for (int row = 0; row < BOARD_SIZE; row++)
        for (int col = 0; col < BOARD_SIZE; col++)
            s_cellAnims[row][col] = {};
}

void Renderer_UpdatePop(float dt) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (!s_cellAnims[row][col].active) continue;
            s_cellAnims[row][col].elapsed += dt;
            if (s_cellAnims[row][col].elapsed >= POP_DUR)
                s_cellAnims[row][col].active = false;
        }
    }
}

struct PlacementPose {
    int   stage;
    float scaleX;
    float scaleY;
    int   riseY;
    bool  active;
};

static PlacementPose PlacementPoseAt(float elapsed, int row, int col) {
    PlacementPose p = { 3, 1.0f, 1.0f, 0, elapsed < POP_DUR };
    if (elapsed < 0.0f) elapsed = 0.0f;

    if (elapsed < WIGGLE_DUR) {
        float wave = (float)std::sin(elapsed / WIGGLE_DUR * 3.5f * PI_F);
        p.stage = 0;
        p.scaleY = 1.0f + 0.10f * wave;
        p.scaleX = 1.0f - 0.045f * wave;
        p.riseY = 0;
        return p;
    }

    if (elapsed < WIGGLE_DUR + SQUASH_DUR) {
        float u = EaseInCubic((elapsed - WIGGLE_DUR) / SQUASH_DUR);
        p.stage = 0;
        p.scaleY = LerpFloat(0.90f, 0.30f, u);
        p.scaleX = LerpFloat(1.045f, 1.40f, u);
        p.riseY = 0;
        return p;
    }

    float u = Clamp01((elapsed - WIGGLE_DUR - SQUASH_DUR) / ERUPT_DUR);
    p.stage = 3;
    if (u < 0.18f) {
        float q = EaseOutCubic(u / 0.18f);
        p.scaleY = LerpFloat(0.30f, 1.20f, q);
        p.scaleX = LerpFloat(1.40f, 0.92f, q);
        p.riseY = (int)(PLACE_RISE_PX * q);
    } else {
        float q = (u - 0.18f) / 0.82f;
        float spring = (float)(std::exp(-5.0f * q) * std::cos(22.0f * q));
        p.scaleY = 1.0f + 0.16f * spring;
        p.scaleX = 1.0f - 0.08f * spring;
        p.riseY = (int)(PLACE_RISE_PX * (1.0f - EaseOutCubic(q)));
    }
    return p;
}

bool Renderer_Init(SDL_Renderer* renderer) {
    s_renderer = renderer;
    if (!renderer) return false;

    const char* scenePath = "assets/gameplay/match_scene/";
    auto LoadSceneTexture = [&](const char* file) -> SDL_Texture* {
        std::string path = std::string(scenePath) + file;
        SDL_Texture* t = IMG_LoadTexture(renderer, path.c_str());
        if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        return t;
    };
    s_sceneBg          = LoadSceneTexture("background_grove_day.png");
    s_sceneBoardShadow = LoadSceneTexture("board_shadow.png");
    s_sceneBoardFrame  = LoadSceneTexture("board_frame.png");
    s_sceneGrid        = LoadSceneTexture("board_grid.png");
    s_scenePieceShadow = LoadSceneTexture("piece_shadow.png");
    s_sceneHover       = LoadSceneTexture("cell_hover.png");
    s_sceneLastMove    = LoadSceneTexture("cell_last_move.png");
    s_sceneWinGlow     = LoadSceneTexture("cell_win_glow.png");
    s_sceneForeground  = LoadSceneTexture("foreground_foliage.png");

#ifdef BERRY_BOARD_12
    if (s_sceneGrid) { SDL_DestroyTexture(s_sceneGrid); s_sceneGrid = nullptr; }
    s_sceneGrid = IMG_LoadTexture(renderer, "assets/gameplay/board_detail/grid_overlay_12x12.png");
    if (s_sceneGrid) SDL_SetTextureBlendMode(s_sceneGrid, SDL_BLENDMODE_BLEND);
#endif
    s_pieceContactPatch = IMG_LoadTexture(renderer, "assets/gameplay/board_detail/piece_contact_patch.png");
    if (s_pieceContactPatch) SDL_SetTextureBlendMode(s_pieceContactPatch, SDL_BLENDMODE_BLEND);

    const char* boardPath = "assets/gameplay/garden_board/";
    s_boardSoil  = IMG_LoadTexture(renderer, (std::string(boardPath) + "soil_base_12x12.png").c_str());
    s_boardGrid  = IMG_LoadTexture(renderer, (std::string(boardPath) + "grid_overlay_12x12.png").c_str());
    s_boardFrame = IMG_LoadTexture(renderer, (std::string(boardPath) + "edge_frame.png").c_str());
    for (SDL_Texture* t : { s_boardSoil, s_boardGrid, s_boardFrame })
        if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    s_boardReady = (s_boardSoil && s_boardGrid && s_boardFrame);

    const char* terrainPath = "assets/gameplay/terrain/";
    s_terrainSoil    = IMG_LoadTexture(renderer, (std::string(terrainPath) + "soil_base.png").c_str());
    s_terrainFurrows = IMG_LoadTexture(renderer, (std::string(terrainPath) + "furrows.png").c_str());
    s_terrainFrame   = IMG_LoadTexture(renderer, (std::string(terrainPath) + "frame_ring.png").c_str());
    for (SDL_Texture* t : { s_terrainSoil, s_terrainFurrows, s_terrainFrame })
        if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

    if (s_boardSoil) {
        SDL_SetTextureScaleMode(s_boardSoil, SDL_ScaleModeLinear);
        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        auto downStep = [&](SDL_Texture* src, int sz) -> SDL_Texture* {
            SDL_Texture* dst = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_TARGET, sz, sz);
            if (!dst) return nullptr;
            SDL_SetTextureScaleMode(dst, SDL_ScaleModeLinear);
            SDL_SetTextureBlendMode(dst, SDL_BLENDMODE_BLEND);
            SDL_SetRenderTarget(renderer, dst);
            SDL_RenderClear(renderer);
            SDL_Rect d = { 0, 0, sz, sz };
            SDL_RenderCopy(renderer, src, nullptr, &d);
            return dst;
        };
        SDL_Texture* step1 = downStep(s_boardSoil, 256);
        SDL_Texture* step2 = step1 ? downStep(step1, 64)
                                   : downStep(s_boardSoil, 64);
        SDL_SetRenderTarget(renderer, prev);
        if (step1 && step1 != step2) SDL_DestroyTexture(step1);
        s_terrainSoilSoft = step2;
    }

    const char* kBerry[2]       = { "strawberry", "blueberry" };
    const int   kGrowthFiles[3] = { 1, 2, 4 };
    s_growthReady = true;
    for (int f = 0; f < 2; f++) {
        for (int s = 0; s < 3; s++) {
            char path[96];
            std::snprintf(path, sizeof(path), "assets/sprites/growth/%s_growth_0%d.png", kBerry[f], kGrowthFiles[s]);
            s_growthTex[f][s] = IMG_LoadTexture(renderer, path);
            if (s_growthTex[f][s]) SDL_SetTextureBlendMode(s_growthTex[f][s], SDL_BLENDMODE_BLEND);
            else                   s_growthReady = false;
        }
    }

    return true;
}

void Renderer_Shutdown() {
    auto Destroy = [](SDL_Texture*& t){ if(t){ SDL_DestroyTexture(t); t=nullptr; } };
    Destroy(s_sceneBg); Destroy(s_sceneBoardShadow); Destroy(s_sceneBoardFrame);
    Destroy(s_sceneGrid); Destroy(s_scenePieceShadow); Destroy(s_sceneHover);
    Destroy(s_sceneLastMove); Destroy(s_sceneWinGlow); Destroy(s_sceneForeground);
    Destroy(s_pieceContactPatch);
    Destroy(s_boardSoil); Destroy(s_boardGrid); Destroy(s_boardFrame);
    Destroy(s_terrainSoil); Destroy(s_terrainFurrows); Destroy(s_terrainFrame); Destroy(s_terrainSoilSoft);
    for (int f = 0; f < 2; f++) for (int s = 0; s < 3; s++) Destroy(s_growthTex[f][s]);
    s_renderer = nullptr;
}

static void BlitTexture(SDL_Renderer* r, SDL_Texture* t, int x, int y, int w, int h) {
    if (!t) return;
    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(r, t, nullptr, &dst);
}

static const float     PIECE_SHADOW_ALPHA_MULT    = 0.85f;
static const float     PIECE_SHADOW_SOFTNESS_SCALE = 1.0f;
static const int       BOARD_READABILITY_DIM_ALPHA = 18;
static const SDL_Color BOARD_READABILITY_DIM_COLOR = { 104, 70, 44, 255 };
static const float     WIN_GLOW_ALPHA_MULT        = 0.82f;
static const float     HOVER_ALPHA_MULT           = 1.0f;

static inline Uint8 AlphaMul(float m) {
    int a = (int)(255.0f * m + 0.5f);
    return (Uint8)(a < 0 ? 0 : (a > 255 ? 255 : a));
}

static const int   BOARD_FRAME_X       = 402;
static const int   BOARD_FRAME_Y       = 7;
static const int   BOARD_FRAME_SIZE    = 1114;
static const Uint8 BOARD_FRAME_ALPHA   = 232;
static const Uint8 BOARD_FRAME_COLOR_R = 238;
static const Uint8 BOARD_FRAME_COLOR_G = 224;
static const Uint8 BOARD_FRAME_COLOR_B = 198;

static int CellHash(int row, int col) {
    int h = row * 928371 + col * 689287 + row * col * 37;
    h ^= (h >> 13);
    return h & 0x7fffffff;
}

static void DrawCellSoilVariation(SDL_Renderer* r) {
#ifdef BERRY_BOARD_12
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            int h = CellHash(row, col);
            int x = BOARD_OFFSET_X + col * CELL_SIZE;
            int y = BOARD_OFFSET_Y + row * CELL_SIZE;
            SDL_Rect cell = { x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4 };

            switch (h % 6) {
            case 0: SDL_SetRenderDrawColor(r, 105, 64, 36, 9); break;
            case 1: SDL_SetRenderDrawColor(r, 197, 125, 66, 7); break;
            case 2: SDL_SetRenderDrawColor(r, 80, 52, 32, 6); break;
            case 3: SDL_SetRenderDrawColor(r, 218, 154, 88, 5); break;
            default: SDL_SetRenderDrawColor(r, 0, 0, 0, 0); break;
            }
            if ((h % 6) < 4) SDL_RenderFillRect(r, &cell);

            if ((h % 11) == 3) {
                SDL_SetRenderDrawColor(r, 78, 48, 28, 24);
                DrawFilledCircle(r, x + 18 + (h % 19), y + 22 + ((h / 7) % 23), 2);
            } else if ((h % 13) == 5) {
                SDL_SetRenderDrawColor(r, 190, 126, 68, 18);
                DrawFilledCircle(r, x + 26 + (h % 17), y + 30 + ((h / 9) % 19), 2);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
#endif
}

void Renderer_DrawMatchBackground(SDL_Renderer* r) {
    BlitTexture(r, s_sceneBg, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

static void DrawBoardReadabilityDim(SDL_Renderer* r) {
    if (BOARD_READABILITY_DIM_ALPHA > 0) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, BOARD_READABILITY_DIM_COLOR.r, BOARD_READABILITY_DIM_COLOR.g,
                               BOARD_READABILITY_DIM_COLOR.b, (Uint8)BOARD_READABILITY_DIM_ALPHA);
        SDL_Rect soil = { BOARD_OFFSET_X, BOARD_OFFSET_Y, BOARD_PIXEL_SIZE, BOARD_PIXEL_SIZE };
        SDL_RenderFillRect(r, &soil);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}

void Renderer_DrawBoard(SDL_Renderer* r) {
#ifdef BERRY_BOARD_12
    if (s_boardReady) {
        const int PX = BOARD_OFFSET_X, PY = BOARD_OFFSET_Y, PS = BOARD_PIXEL_SIZE;

        BlitTexture(r, s_sceneBoardShadow, 410, 31, 1100, 1100);

        const bool ringOnTop = (s_terrainFrame != nullptr);
        if (!ringOnTop) {
            SDL_SetTextureAlphaMod(s_boardFrame, BOARD_FRAME_ALPHA);
            SDL_SetTextureColorMod(s_boardFrame, BOARD_FRAME_COLOR_R, BOARD_FRAME_COLOR_G, BOARD_FRAME_COLOR_B);
            BlitTexture(r, s_boardFrame, BOARD_FRAME_X, BOARD_FRAME_Y, BOARD_FRAME_SIZE, BOARD_FRAME_SIZE);
            SDL_SetTextureAlphaMod(s_boardFrame, 255);
            SDL_SetTextureColorMod(s_boardFrame, 255, 255, 255);
        }

        const int OVER = ringOnTop ? 0 : 8;
        SDL_Texture* soil = s_terrainSoil ? s_terrainSoil
                          : (s_terrainSoilSoft ? s_terrainSoilSoft : s_boardSoil);
        BlitTexture(r, soil, PX - OVER, PY - OVER, PS + 2 * OVER, PS + 2 * OVER);

        if (!s_terrainSoil) DrawCellSoilVariation(r);

        SDL_Texture* furrows = s_terrainFurrows ? s_terrainFurrows
                             : (s_terrainSoil   ? nullptr : s_boardGrid);
        if (furrows) BlitTexture(r, furrows, PX, PY, PS, PS);

        DrawBoardReadabilityDim(r);

        if (ringOnTop)
            BlitTexture(r, s_terrainFrame, BOARD_FRAME_X, BOARD_FRAME_Y, BOARD_FRAME_SIZE, BOARD_FRAME_SIZE);

        return;
    }
#endif
    BlitTexture(r, s_sceneBoardShadow, 410, 31, 1100, 1100);
    BlitTexture(r, s_sceneBoardFrame,  466, 71,  988,  988);
    BlitTexture(r, s_sceneGrid, BOARD_OFFSET_X, BOARD_OFFSET_Y, BOARD_PIXEL_SIZE, BOARD_PIXEL_SIZE);
    DrawBoardReadabilityDim(r);
}

struct PiecePresetSpec {
    int  pieceSize;
    int  shadowW, shadowH;
    int  shadowYOff;
    int  pieceYOff;
    const char* name;
};
static const PiecePresetSpec kPiecePresets[] = {
    { 62, 88, 46,  9, -7, "BASELINE" },
    { 66, 94, 50, 10,  0, "MEDIUM"   },
    { 72,102, 54, 11,  0, "LARGE"    },
    { 76,108, 56, 12,  0, "XL"       },
    { 78,104, 52, 13,  0, "12-78"    },
    { 84,110, 54, 14,  0, "12-84"    },
    { 88,116, 56, 14,  0, "12-88"    },
};
static const int  kPiecePresetCount = (int)(sizeof(kPiecePresets) / sizeof(kPiecePresets[0]));
#ifdef BERRY_BOARD_12
static int        s_pieceReadability = 5;
#else
static int        s_pieceReadability = 2;
#endif
static bool       s_pieceDebugShown  = false;

static const int  PIECE_BLUEBERRY_BONUS = 4;

int Renderer_GetFactionPieceSize(int factionIdx) {
    return kPiecePresets[s_pieceReadability].pieceSize
         + (factionIdx == 1 ? PIECE_BLUEBERRY_BONUS : 0);
}

#ifdef BERRY_BOARD_12
static const bool  PIECE_USE_CONTACT_PATCH        = true;
static const int   PIECE_CONTACT_W                = 86;
static const int   PIECE_CONTACT_H                = 50;
static const int   PIECE_CONTACT_YOFF             = 11;
static const float PIECE_SHADOW_ALPHA_MULT_ROOTED = 0.42f;
#endif

void Renderer_SetPieceReadability(int preset) {
    if (preset < 0) preset = 0;
    if (preset >= kPiecePresetCount) preset = kPiecePresetCount - 1;
    s_pieceReadability = preset;
}
void Renderer_CyclePieceReadability() {
    s_pieceReadability = (s_pieceReadability + 1) % kPiecePresetCount;
    s_pieceDebugShown = true;
}
const char* Renderer_GetPieceReadabilityName() { return kPiecePresets[s_pieceReadability].name; }
bool Renderer_IsPieceDebugShown() { return s_pieceDebugShown; }

static const float REST_FOLIAGE_SCALE = 1.25f;

static void DrawRestingBerry(SDL_Renderer* r, int cx, int groundY, int faction,
                             SDL_Texture* heroTex, int baseSize,
                             float scaleX, float scaleY, int riseY) {
    int gY = groundY - riseY;
    static const float REST_FULL_PIECE_SCALE = 0.90f;
    if (s_growthReady && s_growthTex[faction][2]) {
        int fw = (int)(baseSize * REST_FULL_PIECE_SCALE * scaleX);
        int fh = (int)(baseSize * REST_FULL_PIECE_SCALE * scaleY);
        SDL_Rect fd = { cx - fw / 2, gY - fh, fw, fh };
        SDL_RenderCopy(r, s_growthTex[faction][2], nullptr, &fd);
        return;
    }
    if (s_growthReady) {
        SDL_Texture* base = s_growthTex[faction][1];
        if (base) {
            int bw = (int)(baseSize * REST_FOLIAGE_SCALE * scaleX);
            int bh = (int)(baseSize * REST_FOLIAGE_SCALE * scaleY);
            SDL_Rect bd = { cx - bw / 2, gY - bh, bw, bh };
            SDL_RenderCopy(r, base, nullptr, &bd);
        }
    }
    if (heroTex) {
        int hw = (int)(baseSize * scaleX);
        int hh = (int)(baseSize * scaleY);
        int heroBottom = gY - (int)(baseSize * 0.16f * scaleY);
        SDL_Rect hd = { cx - hw / 2, heroBottom - hh, hw, hh };
        SDL_RenderCopy(r, heroTex, nullptr, &hd);
    }
}

static bool DrawGrowingPiece(SDL_Renderer* r, int cx, int groundY, int faction,
                             SDL_Texture* finalTex, int baseSize, float p) {
    if (!s_growthReady) return false;
    if (p < 0.0f) p = 0.0f; else if (p > 1.0f) p = 1.0f;

    const int   NB   = 3;
    float beat = p * NB;
    int   bi   = (int)beat; if (bi > NB - 1) bi = NB - 1;
    float lt   = beat - bi;

    float bounce = EaseOutBack(lt < 0.7f ? lt / 0.7f : 1.0f);
    float beatGrow = (bi + (lt < 0.7f ? bounce : 1.0f)) / NB;
    float grow     = 0.60f + 0.40f * beatGrow;
    float riseT = (lt < 0.6f) ? lt / 0.6f : 1.0f;
    int   riseY = (int)((1.0f - riseT) * 9.0f * (1.0f - 0.2f * bi));

    if (bi < 2) {
        int stageTex = bi;
        SDL_Texture* gtex = s_growthTex[faction][stageTex];
        if (gtex) {
            int G = (int)(baseSize * 1.52f * grow);
            int gy = (groundY - riseY) - G;
            SDL_Rect gd = { cx - G / 2, gy, G, G };
            SDL_RenderCopy(r, gtex, nullptr, &gd);
        }
    } else {
        DrawRestingBerry(r, cx, groundY, faction, finalTex, baseSize, grow, grow, riseY);
    }
    return true;
}

static void DrawGrowthStage(SDL_Renderer* r, int cx, int groundY, SDL_Texture* tex,
                            int baseSize, float scaleX, float scaleY, int riseY) {
    if (!tex) return;
    int w  = (int)(baseSize * 1.52f * scaleX);
    int h  = (int)(baseSize * 1.52f * scaleY);
    int gY = groundY - riseY;
    SDL_Rect d = { cx - w / 2, gY - h, w, h };
    SDL_RenderCopy(r, tex, nullptr, &d);
}

static void DrawCellBerry(SDL_Renderer* r, int row, int col, int cx, int groundY,
                          int faction, SDL_Texture* heroTex, int baseSize, bool isLast) {
    const CellAnimation& ca = s_cellAnims[row][col];
    if (ca.active) {
        PlacementPose p = PlacementPoseAt(ca.elapsed, row, col);
        if (p.stage < 3) {

            DrawGrowthStage(r, cx, groundY, s_growthTex[faction][0],
                            baseSize, p.scaleX, p.scaleY, p.riseY);
        } else {
            DrawRestingBerry(r, cx, groundY, faction, heroTex, baseSize,
                             p.scaleX, p.scaleY, p.riseY);
        }
        return;
    }
    if (!isLast) {
        DrawRestingBerry(r, cx, groundY, faction, heroTex, baseSize, 1.0f, 1.0f, 0);
        return;
    }
    float pulse = 0.5f + 0.5f * (float)std::sin(SDL_GetTicks() * 0.004f);
    int   lift  = (int)(5.0f + 3.0f * pulse);
    if (s_sceneWinGlow) {
        int   g  = (int)(baseSize * 1.7f);
        int   gcx = cx, gcy = (int)(groundY - baseSize * 0.45f - lift);
        Uint8 gr = (faction == 1) ? 150 : 255;
        Uint8 gg = (faction == 1) ? 190 : 170;
        Uint8 gb = (faction == 1) ? 255 : 150;
        SDL_SetTextureBlendMode(s_sceneWinGlow, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(s_sceneWinGlow, gr, gg, gb);
        SDL_SetTextureAlphaMod(s_sceneWinGlow, (Uint8)(38 + 46 * pulse));
        SDL_Rect gd = { gcx - g / 2, gcy - g / 2, g, g };
        SDL_RenderCopy(r, s_sceneWinGlow, nullptr, &gd);
        SDL_SetTextureAlphaMod(s_sceneWinGlow, 255);
        SDL_SetTextureColorMod(s_sceneWinGlow, 255, 255, 255);
        SDL_SetTextureBlendMode(s_sceneWinGlow, SDL_BLENDMODE_BLEND);
    }
    DrawRestingBerry(r, cx, groundY, faction, heroTex, baseSize, 1.0f, 1.0f, lift);
}

void Renderer_DrawPieces(SDL_Renderer* r, const _GAMESTATE& state) {
    const PiecePresetSpec& ps = kPiecePresets[s_pieceReadability];

#ifdef BERRY_BOARD_12
    const bool  usesPatch   = PIECE_USE_CONTACT_PATCH && s_pieceContactPatch;
    const float shadowMult  = usesPatch ? PIECE_SHADOW_ALPHA_MULT_ROOTED : PIECE_SHADOW_ALPHA_MULT;
#else
    const float shadowMult  = PIECE_SHADOW_ALPHA_MULT;
#endif

    if (s_scenePieceShadow)
        SDL_SetTextureAlphaMod(s_scenePieceShadow, AlphaMul(shadowMult));
    const int shW = (int)(ps.shadowW * PIECE_SHADOW_SOFTNESS_SCALE);
    const int shH = (int)(ps.shadowH * PIECE_SHADOW_SOFTNESS_SCALE);

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            int c = state._BOARD[row][col].c;
            if (c == 0) continue;

            if (HarvestResult_ShouldHideCell(row, col)) continue;

            int cx = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
            int cy = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;

            int factionIdx = (c == -1) ? 0 : 1;
            SDL_Texture* tex = (c == -1) ? s_texX : s_texO;
            if (!tex) continue;

            int baseSize = Renderer_GetFactionPieceSize(factionIdx);
            int groundY  = cy + ps.pieceYOff + baseSize / 2;

            const CellAnimation& ca = s_cellAnims[row][col];

            if (s_growthReady) {
                bool isLast = (row == state._LastI && col == state._LastJ &&
                               state.gameStatus == CHUA_KET_THUC);
                DrawCellBerry(r, row, col, cx, groundY, factionIdx, tex, baseSize, isLast);
                continue;
            }

#ifdef BERRY_BOARD_12
            if (usesPatch)
                BlitTexture(r, s_pieceContactPatch, cx - PIECE_CONTACT_W / 2,
                        cy + PIECE_CONTACT_YOFF, PIECE_CONTACT_W, PIECE_CONTACT_H);
#endif
            BlitTexture(r, s_scenePieceShadow, cx - shW / 2, cy + ps.shadowYOff, shW, shH);

            PlacementPose pose = ca.active
                ? PlacementPoseAt(ca.elapsed, row, col)
                : PlacementPose{ 3, 1.0f, 1.0f, 0, false };
            int w = static_cast<int>(baseSize * pose.scaleX);
            int h = static_cast<int>(baseSize * pose.scaleY);
            int groundLine = cy + ps.pieceYOff + baseSize / 2 - pose.riseY;
            SDL_Rect dst = { cx - w / 2, groundLine - h, w, h };
            bool fadingIn = ca.active && ca.elapsed < 0.12f;
            if (fadingIn) {
                Uint8 pa = (Uint8)(255.0f * (0.35f + 0.65f * (ca.elapsed / 0.12f)));
                SDL_SetTextureAlphaMod(tex, pa);
            }
            SDL_RenderCopy(r, tex, nullptr, &dst);
            if (fadingIn) SDL_SetTextureAlphaMod(tex, 255);
        }
    }

    if (s_scenePieceShadow) SDL_SetTextureAlphaMod(s_scenePieceShadow, 255);
}

static void DrawSoilHover(SDL_Renderer* r, int cx, int cy, float breathe) {
    const int H = CELL_SIZE / 2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int ring = 4; ring >= 0; ring--) {
        int inset = ring * (H / 5);
        Uint8 a = (Uint8)((9 + (4 - ring) * 8) * breathe);
        SDL_SetRenderDrawColor(r, 224, 182, 108, a);
        SDL_Rect g = { cx - H + inset, cy - H + inset,
                       CELL_SIZE - inset * 2, CELL_SIZE - inset * 2 };
        SDL_RenderFillRect(r, &g);
    }
    Uint8 ra = (Uint8)(80 * breathe);
    SDL_SetRenderDrawColor(r, 58, 36, 16, ra);
    SDL_Rect o1 = { cx - H + 3, cy - H + 3, CELL_SIZE - 6, CELL_SIZE - 6 };
    SDL_RenderDrawRect(r, &o1);
    SDL_SetRenderDrawColor(r, 58, 36, 16, (Uint8)(ra * 0.45f));
    SDL_Rect o2 = { cx - H + 5, cy - H + 5, CELL_SIZE - 10, CELL_SIZE - 10 };
    SDL_RenderDrawRect(r, &o2);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void Renderer_DrawMarkers(SDL_Renderer* r, const _GAMESTATE& state) {
    const int M = CELL_SIZE, H = CELL_SIZE / 2;
    const bool playing = (state.gameStatus == CHUA_KET_THUC);

    if (playing && s_sceneLastMove && state.totalMoves > 0 &&
        state._LastI >= 0 && state._LastI < BOARD_SIZE &&
        state._LastJ >= 0 && state._LastJ < BOARD_SIZE) {
        int cx = BOARD_OFFSET_X + state._LastJ * CELL_SIZE + CELL_SIZE / 2;
        int cy = BOARD_OFFSET_Y + state._LastI * CELL_SIZE + CELL_SIZE / 2;
        BlitTexture(r, s_sceneLastMove, cx - H, cy - H, M, M);
    }

    int hr = state.hoveredRow, hc = state.hoveredCol;
    bool humanTurn = (state.mode == MODE_PVP) || (state.mode == MODE_PVE && state.turn);
    if (humanTurn && playing &&
        hr >= 0 && hr < BOARD_SIZE && hc >= 0 && hc < BOARD_SIZE &&
        state._BOARD[hr][hc].c == 0) {
        int cx = BOARD_OFFSET_X + hc * CELL_SIZE + CELL_SIZE / 2;
        int cy = BOARD_OFFSET_Y + hr * CELL_SIZE + CELL_SIZE / 2;
        float breathe = 0.72f + 0.28f * (0.5f + 0.5f * std::sin(SDL_GetTicks() * 0.005f));
        DrawSoilHover(r, cx, cy, breathe);

        int factionIdx = state.turn ? 0 : 1;
        SDL_Texture* ghost = Renderer_GetPieceTexture(factionIdx);
        if (ghost) {
            const PiecePresetSpec& ps = kPiecePresets[s_pieceReadability];
            int size = Renderer_GetFactionPieceSize(factionIdx);
            SDL_Rect dst = { cx - size / 2, cy + ps.pieceYOff - size / 2, size, size };
            SDL_SetTextureAlphaMod(ghost, (Uint8)(70 * breathe));
            SDL_RenderCopy(r, ghost, nullptr, &dst);
            SDL_SetTextureAlphaMod(ghost, 255);
        }
    }

    if (playing && s_sceneWinGlow && (state.gameStatus == P1_THANG || state.gameStatus == P2_THANG)) {
        SDL_SetTextureAlphaMod(s_sceneWinGlow, AlphaMul(WIN_GLOW_ALPHA_MULT));
        for (int k = 0; k < WIN_COUNT; k++) {
            int col = state._WIN_CELLS[k].x, row = state._WIN_CELLS[k].y;
            if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) continue;
            int cx = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
            int cy = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;
            BlitTexture(r, s_sceneWinGlow, cx - H, cy - H, M, M);
        }
        SDL_SetTextureAlphaMod(s_sceneWinGlow, 255);
    }
}

void Renderer_DrawForeground(SDL_Renderer* r) {
    BlitTexture(r, s_sceneForeground, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

bool Renderer_CreatePieceTextures(SDL_Renderer* renderer)
{
    s_texX = IMG_LoadTexture(renderer, "assets/sprites/piece_p1_gameplay_crop.png");
    if (!s_texX) s_texX = IMG_LoadTexture(renderer, "assets/sprites/piece_p1.png");

    s_texO = IMG_LoadTexture(renderer, "assets/sprites/piece_p2_single_blueberry_gameplay.png");
    if (!s_texO) s_texO = IMG_LoadTexture(renderer, "assets/sprites/piece_p2_gameplay_crop.png");
    if (!s_texO) s_texO = IMG_LoadTexture(renderer, "assets/sprites/piece_p2.png");
    if (s_texX && s_texO) {
        SDL_SetTextureBlendMode(s_texX, SDL_BLENDMODE_BLEND);
        SDL_SetTextureBlendMode(s_texO, SDL_BLENDMODE_BLEND);
        s_pieceSize = 56;
        return true;
    }
    if (s_texX) { SDL_DestroyTexture(s_texX); s_texX = nullptr; }
    if (s_texO) { SDL_DestroyTexture(s_texO); s_texO = nullptr; }

    int size = CELL_SIZE;
    s_pieceSize = size;
    int half = size / 2 - 6;

    s_texX = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(s_texX, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, s_texX);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 65, 15, 15, 255);
    DrawX(renderer, size/2, size/2, half+2, 6);
    SDL_SetRenderDrawColor(renderer, 215, 58, 58, 255);
    DrawX(renderer, size/2, size/2, half, 4);
    SDL_SetRenderTarget(renderer, NULL);

    s_texO = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(s_texO, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, s_texO);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 15, 32, 78, 255);
    DrawO(renderer, size/2, size/2, half+2, 6);
    SDL_SetRenderDrawColor(renderer, 58, 105, 215, 255);
    DrawO(renderer, size/2, size/2, half, 4);
    SDL_SetRenderTarget(renderer, NULL);

    return (s_texX && s_texO);
}

SDL_Texture* Renderer_GetPieceTexture(int playerIdx) {
    return playerIdx == 0 ? s_texX : s_texO;
}

SDL_Texture* Renderer_GetGrowthFinalTexture(int factionIdx) {
    if (!s_growthReady) return nullptr;
    int f = (factionIdx == 0) ? 0 : 1;
    return s_growthTex[f][2];
}

void Renderer_DestroyPieceTextures()
{
    if(s_texX){SDL_DestroyTexture(s_texX); 
        s_texX = nullptr;}

    if(s_texO){
        SDL_DestroyTexture(s_texO);
        s_texO = nullptr;
    }
}
