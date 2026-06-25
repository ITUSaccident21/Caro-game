#define _CRT_SECURE_NO_WARNINGS
#include "sdl/UIManager.h"
#include "sdl/DesignTokens.h"
#include "sdl/Renderer.h"
#include "sdl/AudioManager.h"
#include "sdl/Particle.h"
#include "game/FileHandling.h"
#include <cstring>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_map>
#include <cstdio>

enum IconID {
    ICON_SOUND=0, ICON_MUTE, ICON_COUNT
};

static SDL_Renderer* s_renderer  = nullptr;
static TTF_Font*     s_fontXL    = nullptr;
static TTF_Font*     s_fontLg    = nullptr;
static TTF_Font*     s_fontMd    = nullptr;
static TTF_Font*     s_fontSm    = nullptr;

static SDL_Texture*  s_icons[ICON_COUNT]  = {};

static SDL_Texture*  s_hudPlaqueP1 = nullptr;
static SDL_Texture*  s_hudPlaqueP2 = nullptr;
static SDL_Texture*  s_turnBanner  = nullptr;
static SDL_Texture*  s_panelGlow    = nullptr;
static SDL_Texture*  s_panelStakeP1 = nullptr;
static SDL_Texture*  s_panelStakeP2 = nullptr;

static SDL_Texture*  s_splashBg    = nullptr;
static SDL_Texture*  s_menuBg      = nullptr;
static SDL_Texture*  s_settingsBg  = nullptr;
static SDL_Texture*  s_loadBg      = nullptr;

enum UiTexID {
    UI_MAIN_440x90 = 0,
    UI_MAIN_PLAIN_440x90,
    UI_SECONDARY_440x58,
    UI_MODE_308x56,
    UI_START_240x56,
    UI_MEDIUM_224x56,
    UI_PAUSE_360x60,
    UI_SMALL_118x44,
    UI_OK_200x58,
    UI_TAB_210x52,
    UI_TOGGLE_194x44,
    UI_SQUARE_56x56,
    UI_ICON_44x44,
    UI_VOLUME_52x44,
    UI_KEY_SINGLE_44x44,
    UI_KEY_SQUARE_56x56,
    UI_KEY_SHORT_72x44,
    UI_KEY_MEDIUM_96x44,
    UI_KEY_WIDE_128x44,
    UI_KEY_EXTRA_WIDE_160x44,
    UI_TEX_COUNT
};
static SDL_Texture* s_uiTex[UI_TEX_COUNT][4] = {};
static SDL_Texture* s_titleSign400 = nullptr;
static SDL_Texture* s_titleSign320 = nullptr;
static SDL_Texture* s_creditsSign240 = nullptr;
static SDL_Texture* s_soundToggleTex[2][4] = {};

static const char* MENU_ITEMS[] = { "New Game", "Continue", "Settings", "Leave the Grove" };
static const int   MENU_COUNT   = 4;

static int   s_menuSel            = 0;
static int   s_menuBtnState[4]    = {};
static float s_menuBounce[4]      = {};
static bool  s_showCredits        = false;
static bool  s_showHowTo          = false;
static int   s_htpBtnState        = 0;
static int   s_soundBtnState      = 0;
static AppState s_pendingState    = STATE_MENU;
static float    s_pendingDelay    = 0.0f;

struct NameInputUI {
    GameMode mode = MODE_PVP;
    int      diff = AI_MEDIUM;
    int      field= 0;
    char     p1[30] = "Player 1";
    char     p2[30] = "Player 2";
};
static NameInputUI s_ni;

static std::vector<std::string> s_saves;
static std::vector<std::string> s_saveSlots;
static int s_loadSel = 0;
static Uint32 s_saveMsgUntil = 0;

static bool s_showResult = false;
static char s_resultMsg[64] = {};

static float s_splashTimer = 0.0f;

static bool s_showPause = false;

static int s_settingsTab = 0;

static void FillRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

static void OutlineRect(SDL_Renderer* r, int x, int y, int w, int h, SDL_Color c, int t = 1) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    for (int i = 0; i < t; i++) {
        SDL_Rect rect = {x+i, y+i, w-2*i, h-2*i};
        SDL_RenderDrawRect(r, &rect);
    }
}

static void DrawCircleFill(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)std::sqrt((float)(radius*radius - dy*dy));
        SDL_RenderDrawLine(r, cx-dx, cy+dy, cx+dx, cy+dy);
    }
}

struct CachedText { SDL_Texture* tex; int w, h; Uint32 lastUse; };
static std::unordered_map<std::string, CachedText> s_textCache;
static Uint32 s_textFrame = 0;

static void TextCache_Tick() {
    s_textFrame++;
    for (auto it = s_textCache.begin(); it != s_textCache.end(); ) {
        if (s_textFrame - it->second.lastUse > 150) {
            if (it->second.tex) SDL_DestroyTexture(it->second.tex);
            it = s_textCache.erase(it);
        } else ++it;
    }
}
static void TextCache_Clear() {
    for (auto& kv : s_textCache) if (kv.second.tex) SDL_DestroyTexture(kv.second.tex);
    s_textCache.clear();
}

static void RT(SDL_Renderer* r, TTF_Font* f, const char* t,
               int x, int y, SDL_Color c, bool center = false) {
    if (!f || !t || !t[0]) return;

    char head[40];
    std::snprintf(head, sizeof(head), "%p|%02x%02x%02x%02x|", (void*)f, c.r, c.g, c.b, c.a);
    std::string key = head; key += t;

    SDL_Texture* tx; int w, h;
    auto it = s_textCache.find(key);
    if (it != s_textCache.end()) {
        tx = it->second.tex; w = it->second.w; h = it->second.h;
        it->second.lastUse = s_textFrame;
    } else {
        SDL_Surface* s = TTF_RenderUTF8_Blended(f, t, c);
        if (!s) return;
        tx = SDL_CreateTextureFromSurface(r, s);
        w = s->w; h = s->h;
        SDL_FreeSurface(s);
        if (!tx) return;
        SDL_SetTextureBlendMode(tx, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tx, SDL_ScaleModeLinear);
        s_textCache[key] = { tx, w, h, s_textFrame };
    }
    SDL_Rect dst = { center ? x - w / 2 : x, y - h / 2, w, h };
    SDL_RenderCopy(r, tx, nullptr, &dst);
}

static void RTO(SDL_Renderer* r, TTF_Font* f, const char* t,
                int x, int y, SDL_Color c, bool center = false,
                SDL_Color outline = {26, 16, 8, 220}) {
    if (!f || !t || !t[0]) return;
    for (int oy = -2; oy <= 2; oy += 2)
        for (int ox = -2; ox <= 2; ox += 2)
            if (ox || oy) RT(r, f, t, x + ox, y + oy, outline, center);
    RT(r, f, t, x, y, c, center);
}

static void DrawCover(SDL_Renderer* r, SDL_Texture* tex) {
    if (!tex) return;
    int tw, th; SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
    if (tw <= 0 || th <= 0) return;
    float targetAR = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
    float texAR    = (float)tw / th;
    SDL_Rect src;
    if (texAR > targetAR) {
        src.h = th; src.y = 0;
        src.w = (int)(th * targetAR); src.x = (tw - src.w) / 2;
    } else {
        src.w = tw; src.x = 0;
        src.h = (int)(tw / targetAR); src.y = (th - src.h) / 2;
    }
    SDL_Rect dst = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderCopy(r, tex, &src, &dst);
}

static SDL_Texture* LoadBlendTexture(SDL_Renderer* r, const char* path) {
    SDL_Texture* tex = IMG_LoadTexture(r, path);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
    }
    return tex;
}

static void DrawTextureRect(SDL_Renderer* r, SDL_Texture* tex, int x, int y, int w, int h) {
    if (!tex) return;
    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(r, tex, nullptr, &dst);
}

static void DrawButton(SDL_Renderer* r, int x, int y, int w, int h,
                       int state, const char* label);
static void DrawIcon(SDL_Renderer* r, IconID id, int cx, int cy, int size);

static int UiStateIndex(int state) {
    if (state == 1) return 1;
    if (state == 2) return 2;
    if (state < 0)  return 3;
    return 0;
}

static UiTexID PickButtonTexture(int w, int h, const char* label) {
    if (w == 440 && h == 90) return UI_MAIN_440x90;
    if (w == 440 && h == 58) return UI_SECONDARY_440x58;
    if (w == 308 && h == 56) return UI_MODE_308x56;
    if (w == 240 && h == 56) return UI_START_240x56;
    if (w == 224 && h == 56) return UI_MEDIUM_224x56;
    if (w == 360 && h == 60) return UI_PAUSE_360x60;
    if (w == 118 && h == 44) return UI_SMALL_118x44;
    if (w == 200 && h == 58) return UI_OK_200x58;
    if (w == 210 && h == 52) return UI_TAB_210x52;
    if (w == 194 && h == 44) return UI_TOGGLE_194x44;
    if (w == 56  && h == 56) return UI_SQUARE_56x56;
    if (w == 52  && h == 44) return UI_VOLUME_52x44;
    if (w == 44  && h == 44) return UI_ICON_44x44;
    if (label && label[0] == 'X' && w <= 60 && h <= 60) return UI_ICON_44x44;
    return UI_TEX_COUNT;
}

static SDL_Texture* GetButtonTexture(int w, int h, int state, const char* label) {
    UiTexID id = PickButtonTexture(w, h, label);
    if (id == UI_TEX_COUNT) return nullptr;

    if (label && label[0] && w >= 180 && h >= 52) {
        unsigned hash = 2166136261u;
        for (const char* p = label; *p; ++p) {
            hash ^= (unsigned char)(*p);
            hash *= 16777619u;
        }
        hash ^= (unsigned)w * 31u + (unsigned)h;
        if ((hash % 3u) == 0u && s_uiTex[UI_MAIN_PLAIN_440x90][UiStateIndex(state)]) {
            return s_uiTex[UI_MAIN_PLAIN_440x90][UiStateIndex(state)];
        }
    }
    return s_uiTex[id][UiStateIndex(state)];
}

static void DrawSoundToggle(SDL_Renderer* r, int cx, int cy, int size, bool muted, int state) {
    SDL_Texture* tex = s_soundToggleTex[muted ? 1 : 0][UiStateIndex(state)];
    if (tex) {
        DrawTextureRect(r, tex, cx - size / 2, cy - size / 2, size, size);
        return;
    }
    DrawIcon(r, muted ? ICON_MUTE : ICON_SOUND, cx, cy, size);
}

static void DrawKeycap(SDL_Renderer* r, int x, int y, int w, int h,
                       const char* label, const char* detail = nullptr) {
    UiTexID id = UI_KEY_MEDIUM_96x44;
    if (w <= 48) id = UI_KEY_SINGLE_44x44;
    else if (w <= 62 && h >= 50) id = UI_KEY_SQUARE_56x56;
    else if (w <= 80) id = UI_KEY_SHORT_72x44;
    else if (w <= 104) id = UI_KEY_MEDIUM_96x44;
    else if (w <= 136) id = UI_KEY_WIDE_128x44;
    else id = UI_KEY_EXTRA_WIDE_160x44;

    DrawTextureRect(r, s_uiTex[id][0], x, y, w, h);
    if (!s_uiTex[id][0]) DrawButton(r, x, y, w, h, 0, nullptr);
    if (label && label[0]) {
        RTO(r, s_fontSm, label, x + w / 2, y + h / 2,
            {58, 32, 14, 255}, true, {255, 248, 232, 225});
    }
    if (detail && detail[0]) {
        RT(r, s_fontSm, detail, x + w + 16, y + h / 2, DT_LIGHT, false);
    }
}

static void DrawPanel(SDL_Renderer* r, int x, int y, int w, int h) {
    FillRect(r, x, y, w, h, {DT_GROUND.r, DT_GROUND.g, DT_GROUND.b, 222});

    FillRect(r, x+3, y+2, w-6, 1, {DT_WARM.r, DT_WARM.g, DT_WARM.b, 40});
    OutlineRect(r, x, y, w, h, DT_LINE, 2);
}

static void DrawIcon(SDL_Renderer* r, IconID id, int cx, int cy, int size) {
    if (s_uiTex[UI_ICON_44x44][0]) {
        DrawTextureRect(r, s_uiTex[UI_ICON_44x44][0],
                        cx - size / 2, cy - size / 2, size, size);
    }
    if (id >= 0 && id < ICON_COUNT && s_icons[id]) {
        int iconSize = s_uiTex[UI_ICON_44x44][0] ? (int)(size * 0.58f) : size;
        SDL_Rect dst = {cx-iconSize/2, cy-iconSize/2, iconSize, iconSize};
        SDL_RenderCopy(r, s_icons[id], nullptr, &dst);
        return;
    }
    DrawCircleFill(r, cx, cy, size/3, {DT_WARM.r, DT_WARM.g, DT_WARM.b, 180});
}

static void DrawButton(SDL_Renderer* r, int x, int y, int w, int h,
                       int state, const char* label) {
    SDL_Texture* skin = GetButtonTexture(w, h, state, label);
    if (skin) {
        DrawTextureRect(r, skin, x, y, w, h);
        if (label && label[0]) {
            int ly = y + h / 2 + (state == 2 ? 1 : 0);
            RTO(r, s_fontLg, label, x + w / 2, ly,
                {58, 32, 14, 255}, true, {255, 248, 232, 235});
        }
        return;
    }

    SDL_Color top, bot, border, hiEdge;
    if      (state == 1) { top={152,110,62,250}; bot={114,78,40,250}; border={228,186,112,255}; hiEdge={240,206,138,150}; }
    else if (state == 2) { top={94,62,30,250};   bot={70,44,20,250};  border={150,104,56,255};  hiEdge={150,108,60,70};  }
    else                 { top={134,94,52,244};  bot={98,66,34,244};  border={176,126,68,235};  hiEdge={226,192,128,120}; }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < h; i++) {
        float t = (h > 1) ? (float)i / (h - 1) : 0.0f;
        Uint8 rr = (Uint8)(top.r + (bot.r - top.r) * t);
        Uint8 gg = (Uint8)(top.g + (bot.g - top.g) * t);
        Uint8 bb = (Uint8)(top.b + (bot.b - top.b) * t);
        SDL_SetRenderDrawColor(r, rr, gg, bb, top.a);
        SDL_RenderDrawLine(r, x, y + i, x + w - 1, y + i);
    }

    FillRect(r, x + 3, y + 2, w - 6, 2, hiEdge);
    FillRect(r, x + 3, y + h - 4, w - 6, 2, {40, 24, 10, 150});
    OutlineRect(r, x, y, w, h, border, 2);

    if (label && label[0]) {
        int ly = y + h / 2 + (state == 2 ? 1 : 0);
        RTO(r, s_fontLg, label, x + w / 2, ly,
            {58, 32, 14, 255}, true, {255, 248, 232, 235});
    }
}

static void DrawHangingSign(SDL_Renderer* r, int cx, int signTopY,
                             int signW, int signH, const char* title) {
    SDL_Texture* signTex = nullptr;
    if (signW == 400 && signH == 90) signTex = s_titleSign400;
    else if (signW == 320 && signH == 90) signTex = s_titleSign320;
    if (signTex) {
        int chainL = cx - signW/4;
        int chainR = cx + signW/4;
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, DT_LINE.r, DT_LINE.g, DT_LINE.b, 190);
        SDL_RenderDrawLine(r, chainL, 0, chainL, signTopY + 12);
        SDL_RenderDrawLine(r, chainR, 0, chainR, signTopY + 12);
        DrawTextureRect(r, signTex, cx - signW / 2, signTopY, signW, signH);
        if (title)
            RTO(r, s_fontXL, title, cx, signTopY + signH/2,
                {58, 32, 14, 255}, true, {255, 248, 232, 230});
        return;
    }

    int chainL = cx - signW/4;
    int chainR = cx + signW/4;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, DT_LINE.r, DT_LINE.g, DT_LINE.b, 190);
    SDL_RenderDrawLine(r, chainL, 0, chainL, signTopY);
    SDL_RenderDrawLine(r, chainR, 0, chainR, signTopY);

    SDL_SetRenderDrawColor(r, DT_WOOD.r, DT_WOOD.g, DT_WOOD.b, 230);
    for (int ry = 10; ry < signTopY; ry += 14) {
        SDL_Rect ring = {chainL-3, ry, 6, 6};
        SDL_RenderFillRect(r, &ring);
        ring.x = chainR-3;
        SDL_RenderFillRect(r, &ring);
    }

    FillRect(r, cx-signW/2, signTopY, signW, signH, DT_WOOD);
    FillRect(r, cx-signW/2+2, signTopY+1, signW-4, 2, {DT_WARM.r, DT_WARM.g, DT_WARM.b, 160});
    FillRect(r, cx-signW/2+2, signTopY+signH-3, signW-4, 2, {DT_DEEP.r, DT_DEEP.g, DT_DEEP.b, 200});
    OutlineRect(r, cx-signW/2, signTopY, signW, signH, DT_LINE, 2);

    if (title)
        RTO(r, s_fontXL, title, cx, signTopY + signH/2,
            {58, 32, 14, 255}, true, {255, 248, 232, 230});
}

bool UIManager_Init(SDL_Renderer* renderer) {
    s_renderer = renderer;
    InitSaveFolder();
    if (TTF_Init() != 0) {
        SDL_Log("UIManager_Init: TTF_Init failed: %s", TTF_GetError());
        return false;
    }
    const char* fontPath  = "assets/fonts/Baloo2.ttf";
    bool        isPixelFont = false;
    s_fontXL = TTF_OpenFont(fontPath, DT_T1);
    s_fontLg = TTF_OpenFont(fontPath, DT_T2);
    s_fontMd = TTF_OpenFont(fontPath, DT_T3);
    s_fontSm = TTF_OpenFont(fontPath, DT_T4);
    if (!s_fontMd) {
        SDL_Log("UIManager_Init: %s failed (%s) — falling back to m5x7",
                fontPath, TTF_GetError());
        fontPath    = "assets/fonts/m5x7.ttf";
        isPixelFont = true;
        s_fontXL = TTF_OpenFont(fontPath, DT_T1);
        s_fontLg = TTF_OpenFont(fontPath, DT_T2);
        s_fontMd = TTF_OpenFont(fontPath, DT_T3);
        s_fontSm = TTF_OpenFont(fontPath, DT_T4);
    }
    if (!s_fontMd) {
        SDL_Log("UIManager_Init: font load failed: %s", TTF_GetError());
        return false;
    }
    int hint = isPixelFont ? TTF_HINTING_MONO : TTF_HINTING_LIGHT;
    TTF_Font* faces[4] = { s_fontXL, s_fontLg, s_fontMd, s_fontSm };
    for (TTF_Font* f : faces)
        if (f) TTF_SetFontHinting(f, hint);

    if (!isPixelFont) {
        if (s_fontXL) TTF_SetFontStyle(s_fontXL, TTF_STYLE_BOLD);
        if (s_fontLg) TTF_SetFontStyle(s_fontLg, TTF_STYLE_BOLD);
    }

    const char* iconPaths[ICON_COUNT] = {
        "assets/ui/icon_sound.png", "assets/ui/icon_mute.png"
    };
    for (int i = 0; i < ICON_COUNT; i++) {
        s_icons[i] = IMG_LoadTexture(renderer, iconPaths[i]);
        if (s_icons[i]) SDL_SetTextureBlendMode(s_icons[i], SDL_BLENDMODE_BLEND);
    }

    const char* uiBase = "assets/ui/controls/";
    const char* stateNames[4] = { "normal", "hover", "pressed", "disabled" };
    const char* uiPrefixes[UI_TEX_COUNT] = {
        "button_main_440x90",
        "button_main_plain_440x90",
        "button_secondary_440x58",
        "button_mode_308x56",
        "button_start_240x56",
        "button_medium_224x56",
        "button_pause_360x60",
        "button_small_118x44",
        "button_confirm_200x58",
        "button_tab_210x52",
        "button_toggle_194x44",
        "button_square_56x56",
        "button_icon_44x44",
        "button_volume_52x44",
        "keycap_single_44x44",
        "keycap_square_56x56",
        "keycap_short_72x44",
        "keycap_medium_96x44",
        "keycap_wide_128x44",
        "keycap_extra_wide_160x44",
    };
    for (int i = 0; i < UI_TEX_COUNT; i++) {
        for (int st = 0; st < 4; st++) {
            char path[256];
            std::snprintf(path, sizeof(path), "%s%s_%s.png", uiBase, uiPrefixes[i], stateNames[st]);
            s_uiTex[i][st] = LoadBlendTexture(renderer, path);
        }
    }
    s_titleSign400  = LoadBlendTexture(renderer, "assets/ui/controls/title_sign_400x90.png");
    s_titleSign320  = LoadBlendTexture(renderer, "assets/ui/controls/title_sign_compact_320x90.png");
    s_creditsSign240 = LoadBlendTexture(renderer, "assets/ui/controls/credits_sign_240x80.png");
    const char* soundKinds[2] = { "sound", "mute" };
    for (int k = 0; k < 2; k++) {
        for (int st = 0; st < 4; st++) {
            char path[256];
            std::snprintf(path, sizeof(path),
                          "assets/ui/controls/audio_toggle/audio_toggle_%s_%s.png",
                          soundKinds[k], stateNames[st]);
            s_soundToggleTex[k][st] = LoadBlendTexture(renderer, path);
        }
    }

    s_hudPlaqueP1 = IMG_LoadTexture(renderer, "assets/gameplay/match_scene/hud_strawberry_panel.png");
    s_hudPlaqueP2 = IMG_LoadTexture(renderer, "assets/gameplay/match_scene/hud_blueberry_panel.png");
    s_turnBanner  = IMG_LoadTexture(renderer, "assets/gameplay/match_scene/turn_banner.png");
    if (s_hudPlaqueP1) SDL_SetTextureBlendMode(s_hudPlaqueP1, SDL_BLENDMODE_BLEND);
    if (s_hudPlaqueP2) SDL_SetTextureBlendMode(s_hudPlaqueP2, SDL_BLENDMODE_BLEND);
    if (s_turnBanner)  SDL_SetTextureBlendMode(s_turnBanner,  SDL_BLENDMODE_BLEND);

    s_panelGlow    = IMG_LoadTexture(renderer, "assets/ui/panel_active_glow.png");
    s_panelStakeP1 = IMG_LoadTexture(renderer, "assets/ui/player_panel_strawberry_name_stake_440x380.png");
    s_panelStakeP2 = IMG_LoadTexture(renderer, "assets/ui/player_panel_blueberry_name_stake_440x380.png");
    if (s_panelGlow)    SDL_SetTextureBlendMode(s_panelGlow,    SDL_BLENDMODE_BLEND);
    if (s_panelStakeP1) SDL_SetTextureBlendMode(s_panelStakeP1, SDL_BLENDMODE_BLEND);
    if (s_panelStakeP2) SDL_SetTextureBlendMode(s_panelStakeP2, SDL_BLENDMODE_BLEND);

    s_splashBg   = IMG_LoadTexture(renderer, "assets/ui/splash_bg.png");
    s_menuBg     = IMG_LoadTexture(renderer, "assets/ui/menu_bg.png");
    s_settingsBg = IMG_LoadTexture(renderer, "assets/ui/settings_bg.png");
    s_loadBg     = IMG_LoadTexture(renderer, "assets/ui/load_bg.png");
    if (s_splashBg)   SDL_SetTextureBlendMode(s_splashBg,   SDL_BLENDMODE_BLEND);
    if (s_menuBg)     SDL_SetTextureBlendMode(s_menuBg,     SDL_BLENDMODE_BLEND);
    if (s_settingsBg) SDL_SetTextureBlendMode(s_settingsBg, SDL_BLENDMODE_BLEND);
    if (s_loadBg)     SDL_SetTextureBlendMode(s_loadBg,     SDL_BLENDMODE_BLEND);

    return true;
}

void UIManager_BeginFrame() { TextCache_Tick(); }

void UIManager_Shutdown() {
    for (int i = 0; i < UI_TEX_COUNT; i++) {
        for (int st = 0; st < 4; st++) {
            if (s_uiTex[i][st]) { SDL_DestroyTexture(s_uiTex[i][st]); s_uiTex[i][st] = nullptr; }
        }
    }
    if (s_titleSign400)  { SDL_DestroyTexture(s_titleSign400);  s_titleSign400  = nullptr; }
    if (s_titleSign320)  { SDL_DestroyTexture(s_titleSign320);  s_titleSign320  = nullptr; }
    if (s_creditsSign240){ SDL_DestroyTexture(s_creditsSign240);s_creditsSign240= nullptr; }
    for (int k = 0; k < 2; k++) {
        for (int st = 0; st < 4; st++) {
            if (s_soundToggleTex[k][st]) {
                SDL_DestroyTexture(s_soundToggleTex[k][st]);
                s_soundToggleTex[k][st] = nullptr;
            }
        }
    }
    for (int i = 0; i < ICON_COUNT; i++) {
        if (s_icons[i]) { SDL_DestroyTexture(s_icons[i]); s_icons[i] = nullptr; }
    }
    if (s_hudPlaqueP1){ SDL_DestroyTexture(s_hudPlaqueP1);s_hudPlaqueP1= nullptr; }
    if (s_hudPlaqueP2){ SDL_DestroyTexture(s_hudPlaqueP2);s_hudPlaqueP2= nullptr; }
    if (s_turnBanner) { SDL_DestroyTexture(s_turnBanner); s_turnBanner = nullptr; }
    if (s_panelGlow)    { SDL_DestroyTexture(s_panelGlow);    s_panelGlow    = nullptr; }
    if (s_panelStakeP1) { SDL_DestroyTexture(s_panelStakeP1); s_panelStakeP1 = nullptr; }
    if (s_panelStakeP2) { SDL_DestroyTexture(s_panelStakeP2); s_panelStakeP2 = nullptr; }
    if (s_splashBg)   { SDL_DestroyTexture(s_splashBg);   s_splashBg   = nullptr; }
    if (s_menuBg)     { SDL_DestroyTexture(s_menuBg);     s_menuBg     = nullptr; }
    if (s_settingsBg) { SDL_DestroyTexture(s_settingsBg); s_settingsBg = nullptr; }
    if (s_loadBg)     { SDL_DestroyTexture(s_loadBg);     s_loadBg     = nullptr; }
    TextCache_Clear();
    TTF_CloseFont(s_fontXL); s_fontXL = nullptr;
    TTF_CloseFont(s_fontLg); s_fontLg = nullptr;
    TTF_CloseFont(s_fontMd); s_fontMd = nullptr;
    TTF_CloseFont(s_fontSm); s_fontSm = nullptr;
    TTF_Quit();
}

static const int MN_PANEL_X  = 40;
static const int MN_PANEL_W  = 680;
static const int MN_SIGN_CX  = 360;
static const int MN_SIGN_W   = 400;
static const int MN_SIGN_H   = 90;
static const int MN_SIGN_TOP = 210;
static const int MN_BTN_X    = 140;
static const int MN_BTN_W    = 440;
static const int MN_BTN_H    = 90;
static const int MN_BTN_YS[4] = {420, 540, 660, 780};
static const int MN_HTP_X = 140, MN_HTP_Y = 884, MN_HTP_W = 440, MN_HTP_H = 58;

static const int MN_CR_X = 1620, MN_CR_Y = 100, MN_CR_W = 240, MN_CR_H = 80;

static const int MN_SND_Y = 990;

void UIManager_ShowMenu() {
    s_menuSel = 0;
    memset(s_menuBtnState, 0, sizeof(s_menuBtnState));
    memset(s_menuBounce,   0, sizeof(s_menuBounce));
    s_pendingState = STATE_MENU;
    s_pendingDelay = 0.0f;
    s_showCredits  = false;
    s_showHowTo    = false;
    s_htpBtnState  = 0;
    s_soundBtnState = 0;
}

AppState UIManager_UpdateMenu(float dt) {
    for (int i = 0; i < MENU_COUNT; i++) {
        if (s_menuBounce[i] > 0.0f) s_menuBounce[i] -= dt;
    }
    if (s_pendingDelay > 0.0f) {
        s_pendingDelay -= dt;
        if (s_pendingDelay <= 0.0f) return s_pendingState;
    }
    return STATE_MENU;
}

void UIManager_RenderMenu(SDL_Renderer* r) {
    if (s_menuBg) DrawCover(r, s_menuBg);
    else { Renderer_DrawMatchBackground(r); Renderer_DrawForeground(r); }

    int panelRight = MN_PANEL_X + MN_PANEL_W;
    FillRect(r, 0, 0, panelRight, WINDOW_HEIGHT,
             {DT_DEEP.r, DT_DEEP.g, DT_DEEP.b, 150});

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 40; i++) {
        Uint8 a = (Uint8)(210 - i * 5);
        SDL_SetRenderDrawColor(r, DT_DEEP.r, DT_DEEP.g, DT_DEEP.b, a);
        SDL_RenderDrawLine(r, panelRight + i, 0, panelRight + i, WINDOW_HEIGHT);
    }

    FillRect(r, 0, 0, MN_PANEL_X, WINDOW_HEIGHT,
             {DT_WOOD.r, DT_WOOD.g, DT_WOOD.b, 220});
    OutlineRect(r, MN_PANEL_X, 0, 2, WINDOW_HEIGHT, DT_LINE, 1);

    DrawHangingSign(r, MN_SIGN_CX, MN_SIGN_TOP, MN_SIGN_W, MN_SIGN_H, "Berry Grove");

    for (int i = 0; i < MENU_COUNT; i++) {
        int by = MN_BTN_YS[i];
        if (s_menuBounce[i] > 0.0f) {
            float t = 1.0f - s_menuBounce[i] / 0.18f;
            by += (int)(std::sin(t * 3.14159f) * 5.0f);
        }
        DrawButton(r, MN_BTN_X, by, MN_BTN_W, MN_BTN_H, s_menuBtnState[i], MENU_ITEMS[i]);
    }

    DrawButton(r, MN_HTP_X, MN_HTP_Y, MN_HTP_W, MN_HTP_H, s_htpBtnState, "How To Play");

    DrawSoundToggle(r, 190, MN_SND_Y, 44, AudioManager_IsMuted(), s_soundBtnState);

    int ccx = MN_CR_X + MN_CR_W / 2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, DT_LINE.r, DT_LINE.g, DT_LINE.b, 180);
    SDL_RenderDrawLine(r, ccx, 0, ccx, MN_CR_Y);
    for (int ry = 8; ry < MN_CR_Y; ry += 12) {
        SDL_Rect ring = {ccx - 3, ry, 6, 5};
        SDL_RenderFillRect(r, &ring);
    }
    if (s_creditsSign240) {
        DrawTextureRect(r, s_creditsSign240, MN_CR_X, MN_CR_Y, MN_CR_W, MN_CR_H);
    } else {
        FillRect(r, MN_CR_X, MN_CR_Y, MN_CR_W, MN_CR_H, DT_WOOD);
        FillRect(r, MN_CR_X+2, MN_CR_Y+1, MN_CR_W-4, 2,
                 {DT_WARM.r, DT_WARM.g, DT_WARM.b, 140});
        OutlineRect(r, MN_CR_X, MN_CR_Y, MN_CR_W, MN_CR_H, DT_LINE, 2);
    }
    RTO(r, s_fontSm, "Credits", ccx, MN_CR_Y + MN_CR_H/2,
        {58, 32, 14, 255}, true, {255, 248, 232, 225});

    if (s_showCredits) {
        FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0, 0, 0, 180});
        const int cw = 760, ch = 460;
        const int cxp = WINDOW_WIDTH/2 - cw/2, cyp = WINDOW_HEIGHT/2 - ch/2;
        DrawPanel(r, cxp, cyp, cw, ch);
        int ty = cyp + 60;
        RT(r, s_fontLg, "Credits",                              WINDOW_WIDTH/2, ty, DT_WARM,  true); ty += 78;
        RT(r, s_fontLg, "Berry Grove",                          WINDOW_WIDTH/2, ty, DT_LIGHT, true); ty += 56;
        RT(r, s_fontMd, "A cozy five-in-a-row strategy game",   WINDOW_WIDTH/2, ty, DT_OFF,   true); ty += 64;
        RT(r, s_fontMd, "Built with SDL2 and C++",              WINDOW_WIDTH/2, ty, DT_OFF,   true); ty += 44;
        RT(r, s_fontMd, "24120472 - Truong Hue Tri",            WINDOW_WIDTH/2, ty, DT_OFF,   true);
        RT(r, s_fontSm, "Click anywhere to close",              WINDOW_WIDTH/2, cyp + ch - 44, DT_WARM, true);
    }

    if (s_showHowTo) {
        FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0, 0, 0, 185});
        const int cw = 900, ch = 660;
        const int cxp = WINDOW_WIDTH/2 - cw/2, cyp = WINDOW_HEIGHT/2 - ch/2;
        DrawPanel(r, cxp, cyp, cw, ch);
        RT(r, s_fontLg, "How To Play", WINDOW_WIDTH/2, cyp + 52, DT_WARM, true);
        RT(r, s_fontSm, "Plant berries. Grow five in a row. Harvest the grove.",
           WINDOW_WIDTH/2, cyp + 100, DT_OFF, true);

        const int lx = cxp + 70; int ly = cyp + 158;
        const char* rules[5] = {
            "1.  Plant a berry on an empty soil patch.",
            "2.  Take turns as Strawberry and Blueberry.",
            "3.  Grow five berries in a row to harvest the grove.",
            "4.  Rows can be horizontal, vertical, or diagonal.",
            "5.  If every patch is filled, the grove is full.",
        };
        for (int i = 0; i < 5; i++) { RT(r, s_fontMd, rules[i], lx, ly, DT_LIGHT, false); ly += 48; }

        ly += 18;
        RT(r, s_fontMd, "Controls", lx, ly, DT_WARM, false); ly += 44;
        DrawKeycap(r, lx, ly - 22, 96, 44, "Click", "plant a berry"); ly += 48;
        DrawKeycap(r, lx, ly - 22, 96, 44, "ESC", "pause / back"); ly += 48;
        DrawKeycap(r, lx, ly - 22, 44, 44, "R", "play again after the grove is harvested");

        RT(r, s_fontSm, "Click anywhere to close", WINDOW_WIDTH/2, cyp + ch - 40, DT_WARM, true);
    }
}

AppState UIManager_HandleMenuEvent(const SDL_Event& e) {
    static const AppState TARGETS[4] = {
        STATE_NAME_INPUT, STATE_LOAD_GAME, STATE_SETTINGS, STATE_EXIT
    };

    if (s_showCredits) {
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_KEYDOWN) {
            s_showCredits = false;
            AudioManager_PlaySFX(SFX_MENU_SELECT);
        }
        return STATE_MENU;
    }
    if (s_showHowTo) {
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_KEYDOWN) {
            s_showHowTo = false;
            AudioManager_PlaySFX(SFX_MENU_SELECT);
        }
        return STATE_MENU;
    }

    auto fireItem = [&](int i) {
        s_menuBtnState[i] = 2;
        s_menuBounce[i]   = 0.18f;
        s_menuSel         = i;
        s_pendingState    = TARGETS[i];
        s_pendingDelay    = 0.20f;
        AudioManager_PlaySFX(SFX_MENU_SELECT);
    };

    if (e.type == SDL_MOUSEMOTION) {
        for (int i = 0; i < MENU_COUNT; i++) {
            SDL_Rect b = {MN_BTN_X, MN_BTN_YS[i], MN_BTN_W, MN_BTN_H};
            bool hit = (e.motion.x>=b.x && e.motion.x<b.x+b.w &&
                        e.motion.y>=b.y && e.motion.y<b.y+b.h);
            if (hit) {
                if (s_menuBtnState[i] == 0) AudioManager_PlaySFX(SFX_MENU_HOVER);
                s_menuBtnState[i] = 1; s_menuSel = i;
            }
            else if (s_menuBtnState[i] == 1) s_menuBtnState[i] = 0;
        }

        int oldHtp = s_htpBtnState;
        s_htpBtnState = (e.motion.x>=MN_HTP_X && e.motion.x<MN_HTP_X+MN_HTP_W &&
                         e.motion.y>=MN_HTP_Y && e.motion.y<MN_HTP_Y+MN_HTP_H) ? 1 : 0;
        if (oldHtp == 0 && s_htpBtnState == 1) AudioManager_PlaySFX(SFX_MENU_HOVER);

        const int SND_X = 190, SND_S = 44;
        int oldSound = s_soundBtnState;
        s_soundBtnState = (e.motion.x>=SND_X-SND_S/2 && e.motion.x<SND_X+SND_S/2 &&
                           e.motion.y>=MN_SND_Y-SND_S/2 && e.motion.y<MN_SND_Y+SND_S/2) ? 1 : 0;
        if (oldSound == 0 && s_soundBtnState == 1) AudioManager_PlaySFX(SFX_MENU_HOVER);
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        for (int i = 0; i < MENU_COUNT; i++) {
            SDL_Rect b = {MN_BTN_X, MN_BTN_YS[i], MN_BTN_W, MN_BTN_H};
            if (e.button.x>=b.x && e.button.x<b.x+b.w &&
                e.button.y>=b.y && e.button.y<b.y+b.h)
                fireItem(i);
        }

        if (e.button.x>=MN_HTP_X && e.button.x<MN_HTP_X+MN_HTP_W &&
            e.button.y>=MN_HTP_Y && e.button.y<MN_HTP_Y+MN_HTP_H) {
            s_showHowTo = true;
            AudioManager_PlaySFX(SFX_MENU_SELECT);
        }

        if (e.button.x>=MN_CR_X && e.button.x<MN_CR_X+MN_CR_W &&
            e.button.y>=MN_CR_Y && e.button.y<MN_CR_Y+MN_CR_H) {
            s_showCredits = true;
            AudioManager_PlaySFX(SFX_MENU_SELECT);
        }

        {
            const int SND_X = 190, SND_S = 44;
            if (e.button.x>=SND_X-SND_S/2 && e.button.x<SND_X+SND_S/2 &&
                e.button.y>=MN_SND_Y-SND_S/2 && e.button.y<MN_SND_Y+SND_S/2) {
                s_soundBtnState = 2;
                bool nowMuted = !AudioManager_IsMuted();
                if (!nowMuted) AudioManager_SetMuted(false);
                AudioManager_PlaySFX(SFX_MENU_SELECT);
                if (nowMuted) AudioManager_SetMuted(true);
            }
        }
    }
    if (e.type == SDL_MOUSEBUTTONUP) {
        for (int i = 0; i < MENU_COUNT; i++)
            if (s_menuBtnState[i] == 2) s_menuBtnState[i] = 1;
        if (s_soundBtnState == 2) s_soundBtnState = 1;
    }
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_UP:   s_menuSel = (s_menuSel-1+MENU_COUNT)%MENU_COUNT; AudioManager_PlaySFX(SFX_MENU_HOVER); break;
        case SDLK_DOWN: s_menuSel = (s_menuSel+1)%MENU_COUNT; AudioManager_PlaySFX(SFX_MENU_HOVER); break;
        case SDLK_RETURN: case SDLK_SPACE: fireItem(s_menuSel); break;
        case SDLK_h: s_showHowTo = true; AudioManager_PlaySFX(SFX_MENU_SELECT); break;
        case SDLK_ESCAPE: s_pendingState = STATE_EXIT; s_pendingDelay = 0.10f; AudioManager_PlaySFX(SFX_MENU_SELECT); break;
        }
    }
    return STATE_MENU;
}

static const int NI_PNL_X = WINDOW_WIDTH/2 - 380;
static const int NI_PNL_W = 760;
static const int NI_PNL_H = 620;
static const int NI_PNL_Y = (WINDOW_HEIGHT - NI_PNL_H)/2;
static const int NI_FX    = NI_PNL_X + DT_XXL;
static const int NI_FW    = NI_PNL_W - DT_XXL*2;
static const int NI_FH    = 56;

void UIManager_ShowNameInput(_GAMESTATE& state) {
    s_ni.mode  = state.mode;
    s_ni.diff  = state.difficulty;
    s_ni.field = 0;
    memcpy(s_ni.p1, state.players[0].name, 29); s_ni.p1[29] = '\0';
    memcpy(s_ni.p2, state.players[1].name, 29); s_ni.p2[29] = '\0';
}

void UIManager_RenderNameInput(SDL_Renderer* r) {
    Renderer_DrawMatchBackground(r);
    FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0,0,0,150});

    DrawPanel(r, NI_PNL_X, NI_PNL_Y, NI_PNL_W, NI_PNL_H);

    DrawHangingSign(r, WINDOW_WIDTH/2, NI_PNL_Y - 8, 320, 90, "New Game");

    int fy = NI_PNL_Y + 130;

    RT(r, s_fontSm, "Mode:", NI_FX, fy, DT_LIGHT, false);
    fy += DT_L;
    DrawButton(r, NI_FX,           fy, NI_FW/2 - DT_S, NI_FH,
               s_ni.mode == MODE_PVP ? 1 : 0, "PvP");
    DrawButton(r, NI_FX + NI_FW/2, fy, NI_FW/2 - DT_S, NI_FH,
               s_ni.mode == MODE_PVE ? 1 : 0, "PvE");
    fy += NI_FH + DT_L;

    RT(r, s_fontSm, "Player 1 name:", NI_FX, fy, DT_LIGHT, false);
    fy += DT_L * 2;
    FillRect(r, NI_FX, fy - NI_FH/2, NI_FW, NI_FH,
             s_ni.field == 0 ? SDL_Color{DT_WOOD.r, DT_WOOD.g, DT_WOOD.b, 220}
                             : SDL_Color{DT_DEEP.r, DT_DEEP.g, DT_DEEP.b, 180});
    OutlineRect(r, NI_FX, fy - NI_FH/2, NI_FW, NI_FH,
                s_ni.field == 0 ? DT_WARM : DT_LINE, 2);
    RT(r, s_fontMd, s_ni.p1, NI_FX + DT_M, fy, DT_LIGHT, false);
    fy += NI_FH + DT_L;

    if (s_ni.mode == MODE_PVP) {
        RT(r, s_fontSm, "Player 2 name:", NI_FX, fy, DT_LIGHT, false);
        fy += DT_L * 2;
        FillRect(r, NI_FX, fy - NI_FH/2, NI_FW, NI_FH,
                 s_ni.field == 1 ? SDL_Color{DT_WOOD.r, DT_WOOD.g, DT_WOOD.b, 220}
                                 : SDL_Color{DT_DEEP.r, DT_DEEP.g, DT_DEEP.b, 180});
        OutlineRect(r, NI_FX, fy - NI_FH/2, NI_FW, NI_FH,
                    s_ni.field == 1 ? DT_WARM : DT_LINE, 2);
        RT(r, s_fontMd, s_ni.p2, NI_FX + DT_M, fy, DT_LIGHT, false);
    } else {
        RT(r, s_fontSm, "AI Difficulty:", NI_FX, fy, DT_LIGHT, false);
        fy += DT_L * 2;

        const char* diffLabels[3] = { "Easy", "Medium", "Hard" };
        int diffIdx = (s_ni.diff == AI_EASY) ? 0 : (s_ni.diff == AI_MEDIUM) ? 1 : 2;
        int arrW = NI_FH, arrH = NI_FH;
        DrawButton(r, NI_FX,                    fy - arrH/2, arrW, arrH, 0, "<");
        DrawButton(r, NI_FX + NI_FW - arrW,     fy - arrH/2, arrW, arrH, 0, ">");
        RT(r, s_fontLg, diffLabels[diffIdx], NI_FX + NI_FW/2, fy, DT_WARM, true);
    }
    fy += NI_FH + DT_XXL;

    RT(r, s_fontSm, "Tab: switch field    Enter: start", NI_FX, fy,
       {DT_OFF.r, DT_OFF.g, DT_OFF.b, 180}, false);
    fy += DT_XL;

    DrawButton(r, NI_FX + NI_FW/2 - 120, fy - NI_FH/2, 240, NI_FH, 0, "Start");
}

AppState UIManager_HandleNameInputEvent(const SDL_Event& e, _GAMESTATE& state) {

    int fy = NI_PNL_Y + 130;
    fy += DT_L + NI_FH + DT_L + DT_L*2 + NI_FH + DT_L + DT_L*2 + NI_FH + DT_XXL + DT_XL;
    SDL_Rect startBtn = {NI_FX + NI_FW/2 - 120, fy - NI_FH/2, 240, NI_FH};

    auto commitStart = [&]() -> AppState {
        memcpy(state.players[0].name, s_ni.p1, 29); state.players[0].name[29] = '\0';
        memcpy(state.players[1].name, s_ni.p2, 29); state.players[1].name[29] = '\0';
        state.mode       = s_ni.mode;
        state.difficulty = (AIDifficulty)s_ni.diff;
        AudioManager_PlaySFX(SFX_MENU_SELECT);
        return STATE_PLAYING;
    };

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: AudioManager_PlaySFX(SFX_MENU_SELECT); return STATE_MENU;
        case SDLK_RETURN: return commitStart();
        case SDLK_TAB:
            s_ni.field = (s_ni.field + 1) % 2;
            AudioManager_PlaySFX(SFX_MENU_HOVER);
            break;
        case SDLK_LEFT:
            if (s_ni.mode == MODE_PVE) {
                int diffs[3] = {AI_EASY, AI_MEDIUM, AI_HARD};
                int idx = (s_ni.diff == AI_EASY) ? 0 : (s_ni.diff == AI_MEDIUM) ? 1 : 2;
                s_ni.diff = diffs[(idx + 2) % 3];
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
            break;
        case SDLK_RIGHT:
            if (s_ni.mode == MODE_PVE) {
                int diffs[3] = {AI_EASY, AI_MEDIUM, AI_HARD};
                int idx = (s_ni.diff == AI_EASY) ? 0 : (s_ni.diff == AI_MEDIUM) ? 1 : 2;
                s_ni.diff = diffs[(idx + 1) % 3];
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
            break;
        case SDLK_BACKSPACE:
            if (s_ni.field == 0 && strlen(s_ni.p1) > 0)
                s_ni.p1[strlen(s_ni.p1)-1] = '\0';
            else if (s_ni.field == 1 && s_ni.mode == MODE_PVP && strlen(s_ni.p2) > 0)
                s_ni.p2[strlen(s_ni.p2)-1] = '\0';
            break;
        default:
            if (e.key.keysym.sym >= 32 && e.key.keysym.sym < 127) {
                char ch = (char)e.key.keysym.sym;
                if (e.key.keysym.mod & KMOD_SHIFT) ch = (char)toupper(ch);
                if (s_ni.field == 0 && (int)strlen(s_ni.p1) < 29) {
                    int n = (int)strlen(s_ni.p1);
                    s_ni.p1[n] = ch; s_ni.p1[n+1] = '\0';
                } else if (s_ni.field == 1 && s_ni.mode == MODE_PVP && (int)strlen(s_ni.p2) < 29) {
                    int n = (int)strlen(s_ni.p2);
                    s_ni.p2[n] = ch; s_ni.p2[n+1] = '\0';
                }
            }
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (mx>=startBtn.x && mx<startBtn.x+startBtn.w &&
            my>=startBtn.y && my<startBtn.y+startBtn.h)
            return commitStart();

        int modeY = NI_PNL_Y + 130 + DT_L;
        int modeH = NI_FH;
        SDL_Rect pvpR = {NI_FX, modeY, NI_FW/2 - DT_S, modeH};
        SDL_Rect pveR = {NI_FX + NI_FW/2, modeY, NI_FW/2 - DT_S, modeH};
        if (mx>=pvpR.x && mx<pvpR.x+pvpR.w && my>=pvpR.y && my<pvpR.y+pvpR.h) {
            if (s_ni.mode != MODE_PVP) AudioManager_PlaySFX(SFX_MENU_SELECT);
            s_ni.mode = MODE_PVP;
        }
        if (mx>=pveR.x && mx<pveR.x+pveR.w && my>=pveR.y && my<pveR.y+pveR.h) {
            if (s_ni.mode != MODE_PVE) AudioManager_PlaySFX(SFX_MENU_SELECT);
            s_ni.mode = MODE_PVE;
        }

        if (s_ni.mode == MODE_PVE) {
            int diffY = NI_PNL_Y + 130 + DT_L + NI_FH + DT_L + DT_L*2 + NI_FH + DT_L + DT_L*2;
            SDL_Rect leftArr  = {NI_FX,                 diffY - NI_FH/2, NI_FH, NI_FH};
            SDL_Rect rightArr = {NI_FX + NI_FW - NI_FH, diffY - NI_FH/2, NI_FH, NI_FH};
            int diffs[3] = {AI_EASY, AI_MEDIUM, AI_HARD};
            int idx = (s_ni.diff == AI_EASY) ? 0 : (s_ni.diff == AI_MEDIUM) ? 1 : 2;
            if (mx>=leftArr.x  && mx<leftArr.x +leftArr.w  && my>=leftArr.y  && my<leftArr.y +leftArr.h) {
                s_ni.diff = diffs[(idx + 2) % 3];
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
            if (mx>=rightArr.x && mx<rightArr.x+rightArr.w && my>=rightArr.y && my<rightArr.y+rightArr.h) {
                s_ni.diff = diffs[(idx + 1) % 3];
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
        }
    }
    return STATE_NAME_INPUT;
}

void UIManager_RenderHUD(SDL_Renderer* r, const _GAMESTATE& state) {

    const bool p1Turn = state.turn;

    static bool   s_huInit = false, s_huLastTurn = true;
    static Uint32 s_huTurnTick = 0;
    if (!s_huInit) { s_huLastTurn = state.turn; s_huInit = true; }
    if (state.turn != s_huLastTurn) { s_huLastTurn = state.turn; s_huTurnTick = SDL_GetTicks(); }
    Uint32 huSince = SDL_GetTicks() - s_huTurnTick;
    float turnPulse = (s_huTurnTick != 0 && huSince < 450) ? (1.0f - huSince / 450.0f) : 0.0f;

    const int PW = 440, PH = 380, PY = 375;
    const int LX = 20,  RX = 1460;
    const SDL_Color STRAW_COL = { 206,  80,  50, 255 };
    const SDL_Color BLUE_COL  = {  78, 110, 192, 255 };
    const SDL_Color SUB_COL   = { (Uint8)(DT_LINE.r+30),
                                   (Uint8)(DT_LINE.g+20),
                                   (Uint8)(DT_LINE.b+10), 190 };

    const int ICON_SIZE = 72;
    const int ICON_PAD  = 16;
    const int ICON_Y    = PY + 44;

    const int TX_OFF    = ICON_PAD + ICON_SIZE + 14;
    const int TX_W      = PW - TX_OFF - 12;
    char buf[64];

    for (int side = 0; side < 2; side++) {
        const int    px     = (side == 0) ? LX : RX;
        const int    cx     = px + PW / 2;
        const int    tcx    = px + TX_OFF + TX_W / 2;
        const bool   isP1   = (side == 0);
        const bool   active = isP1 ? p1Turn : !p1Turn;
        const SDL_Color& fc = isP1 ? STRAW_COL : BLUE_COL;
        const _PLAYER&   pl = state.players[side];

        SDL_Rect panelDst = { px, PY, PW, PH };
        SDL_Texture* boardTex = isP1 ? s_panelStakeP1 : s_panelStakeP2;
        if (boardTex) {
            SDL_RenderCopy(r, boardTex, nullptr, &panelDst);
        } else {
            DrawPanel(r, px, PY, PW, PH);
            OutlineRect(r, px, PY, PW, PH, DT_LINE, 2);
        }

        if (active) {
            Uint8 ba = turnPulse > 0 ? (Uint8)(80 + 140 * turnPulse) : 120;
            if (s_panelGlow) {
                SDL_SetTextureColorMod(s_panelGlow, fc.r, fc.g, fc.b);
                SDL_SetTextureAlphaMod(s_panelGlow, ba);
                SDL_RenderCopy(r, s_panelGlow, nullptr, &panelDst);
                SDL_SetTextureColorMod(s_panelGlow, 255, 255, 255);
                SDL_SetTextureAlphaMod(s_panelGlow, 255);
            } else {
                OutlineRect(r, px, PY, PW, PH, {fc.r, fc.g, fc.b, ba}, 2);
            }
        }

        if (!boardTex) {
            FillRect(r, px + 2, PY + 2, PW - 4, 8, {fc.r, fc.g, fc.b, 220});
            SDL_Texture* icon = Renderer_GetPieceTexture(isP1 ? 0 : 1);
            if (icon) {
                SDL_Rect dst = { px + ICON_PAD, ICON_Y, ICON_SIZE, ICON_SIZE };
                SDL_SetTextureAlphaMod(icon, active ? 255 : 160);
                SDL_RenderCopy(r, icon, nullptr, &dst);
                SDL_SetTextureAlphaMod(icon, 255);
            }
        }

        const char* factionLabel = isP1 ? "STRAWBERRY" : "BLUEBERRY";
        RTO(r, s_fontSm, factionLabel, cx, PY + 105, DT_WARM, true);

        const char* pname = (!isP1 && state.mode == MODE_PVE) ? "CPU" : pl.name;
        RTO(r, s_fontLg, pname, cx, PY + 170, active ? DT_WARM : DT_LIGHT, true);

        snprintf(buf, sizeof(buf), "%d planted", pl.moves);
        RTO(r, s_fontSm, buf, cx, PY + 237, DT_LIGHT, true);

        const bool aiThink = (!isP1 && state.mode == MODE_PVE && state.aiThinking);
        const char* statusStr;
        SDL_Color   statusCol;
        if (active) {
            if (aiThink) { statusStr = "Thinking..."; statusCol = { 210, 210, 90, 255 }; }
            else          { statusStr = "Growing...";  statusCol = DT_WARM; }
        } else {
            statusStr = "Resting";
            statusCol = { DT_WARM.r, DT_WARM.g, DT_WARM.b, 190 };
        }
        RTO(r, s_fontSm, statusStr, cx, PY + 300, statusCol, true);

    }

    const int BW = 400, BH = 100, BY = 10;
    int bw = (int)(BW * (1.0f + 0.05f * turnPulse));
    int bh = (int)(BH * (1.0f + 0.05f * turnPulse));
    int bx = (WINDOW_WIDTH - bw) / 2, by = BY - (bh - BH) / 2;
    if (s_turnBanner) { SDL_Rect d = { bx, by, bw, bh }; SDL_RenderCopy(r, s_turnBanner, nullptr, &d); }
    {
        const char* pName = p1Turn ? state.players[0].name : state.players[1].name;
        if (!pName || !pName[0]) pName = p1Turn ? "Grower 1" : "Grower 2";
        snprintf(buf, sizeof(buf), "%s's turn", pName);
    }
    RTO(r, s_fontLg, buf, WINDOW_WIDTH / 2, BY + BH / 2 + 2, DT_WARM, true);

    static Uint32 s_hintStart = 0;
    static int    s_hintPrevMoves = -1;
    if (state.totalMoves == 0 && s_hintPrevMoves != 0) s_hintStart = SDL_GetTicks();
    s_hintPrevMoves = state.totalMoves;

    bool hintOK = (state.totalMoves < 2) && (state.gameStatus == CHUA_KET_THUC) && !s_showPause;
    if (hintOK && s_hintStart != 0) {
        Uint32 since = SDL_GetTicks() - s_hintStart;
        float a = (since < 4000) ? 1.0f : (since < 6000) ? (1.0f - (since - 4000) / 2000.0f) : 0.0f;
        if (a > 0.02f) {
            int cx = WINDOW_WIDTH / 2;
            int cy = BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 30;
            FillRect(r, cx - 330, cy - 27, 660, 54, {26, 16, 8, (Uint8)(150 * a)});
            RTO(r, s_fontMd, "Plant five berries in a row to harvest.",
                cx, cy - 6, {255, 248, 232, (Uint8)(238 * a)}, true);
            RT (r, s_fontSm, "Click an empty soil patch.",
                cx, cy + 18, {235, 224, 204, (Uint8)(205 * a)}, true);
        }
    }
}

static const int LD_BD_X = 484, LD_BD_Y = 232;
static const int LD_BD_W = 932, LD_BD_H = 556;
static const int LD_ROW_H  = 60;
static const int LD_BTN_W = 224, LD_BTN_H = 56, LD_BTN_G = 26;
static const SDL_Color LD_INK     = {70, 48, 28, 255};
static const SDL_Color LD_INK_SEL = {120, 56, 16, 255};

static int  LD_RowsY()  { return LD_BD_Y + 126; }
static int  LD_BtnsY()  { return LD_BD_Y + LD_BD_H - LD_BTN_H - 56; }
static int  LD_BtnsX0() { return LD_BD_X + LD_BD_W/2 - (LD_BTN_W*3 + LD_BTN_G*2)/2; }

void UIManager_ShowLoadScreen() {
    s_loadSel = 0;
    s_saveSlots = GetSaveFiles();
    s_saves     = s_saveSlots;
}

void UIManager_RenderLoadScreen(SDL_Renderer* r) {
    if (s_loadBg) DrawCover(r, s_loadBg);
    else { Renderer_DrawMatchBackground(r); FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0,0,0,150}); }

    const int cx = LD_BD_X + LD_BD_W/2;

    RT(r, s_fontXL, "Continue", cx, LD_BD_Y + 46, LD_INK, true);

    if (s_saves.empty()) {
        RT(r, s_fontMd, "No saved games yet.", cx, LD_BD_Y + LD_BD_H/2 - 10, LD_INK, true);
        RT(r, s_fontSm, "Start a New Game to create one.", cx, LD_BD_Y + LD_BD_H/2 + 30, LD_INK, true);
    } else {
        const int LX = LD_BD_X + 70, LW = LD_BD_W - 140, ROW_Y0 = LD_RowsY();
        for (int i = 0; i < (int)s_saves.size(); i++) {
            int ry = ROW_Y0 + i * LD_ROW_H;
            bool sel = (i == s_loadSel);
            if (sel) {
                FillRect(r, LX, ry, LW, LD_ROW_H-6, {150, 110, 60, 70});
                OutlineRect(r, LX, ry, LW, LD_ROW_H-6, {120, 80, 40, 220}, 2);
            }
            RT(r, s_fontMd, s_saves[i].c_str(), LX+24, ry+(LD_ROW_H-6)/2, sel ? LD_INK_SEL : LD_INK, false);
        }
    }

    int bbX = LD_BtnsX0(), bbY = LD_BtnsY();
    DrawButton(r, bbX,                            bbY, LD_BTN_W, LD_BTN_H, 0, "Load");
    DrawButton(r, bbX + (LD_BTN_W+LD_BTN_G),      bbY, LD_BTN_W, LD_BTN_H, 0, "Delete");
    DrawButton(r, bbX + 2*(LD_BTN_W+LD_BTN_G),    bbY, LD_BTN_W, LD_BTN_H, 0, "Back");
}

AppState UIManager_HandleLoadEvent(const SDL_Event& e, _GAMESTATE& state) {
    const int BBW = LD_BTN_W, BBH = LD_BTN_H, BBG = LD_BTN_G;
    int bbY = LD_BtnsY();
    int bbX = LD_BtnsX0();

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) { AudioManager_PlaySFX(SFX_MENU_SELECT); return STATE_MENU; }
        if (e.key.keysym.sym == SDLK_UP && s_loadSel > 0) { s_loadSel--; AudioManager_PlaySFX(SFX_MENU_HOVER); }
        if (e.key.keysym.sym == SDLK_DOWN && s_loadSel < (int)s_saves.size()-1) { s_loadSel++; AudioManager_PlaySFX(SFX_MENU_HOVER); }
        if (e.key.keysym.sym == SDLK_RETURN && !s_saves.empty()) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            if (LoadGame(state, s_saveSlots[s_loadSel])) return STATE_PLAYING;
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;

        const int LX = LD_BD_X + 70, LW = LD_BD_W - 140, ROW_Y0 = LD_RowsY();
        for (int i = 0; i < (int)s_saves.size(); i++) {
            SDL_Rect row = {LX, ROW_Y0 + i*LD_ROW_H, LW, LD_ROW_H-6};
            if (mx>=row.x && mx<row.x+row.w && my>=row.y && my<row.y+row.h) {
                if (s_loadSel != i) AudioManager_PlaySFX(SFX_MENU_HOVER);
                s_loadSel = i;
            }
        }
        SDL_Rect loadR   = {bbX,               bbY, BBW, BBH};
        SDL_Rect delR    = {bbX+(BBW+BBG),     bbY, BBW, BBH};
        SDL_Rect backR   = {bbX+2*(BBW+BBG),   bbY, BBW, BBH};
        if (mx>=loadR.x && mx<loadR.x+loadR.w && my>=loadR.y && my<loadR.y+loadR.h && !s_saves.empty()) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            if (LoadGame(state, s_saveSlots[s_loadSel])) return STATE_PLAYING;
        }
        if (mx>=delR.x && mx<delR.x+delR.w && my>=delR.y && my<delR.y+delR.h && !s_saves.empty()) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            DeleteSave(s_saveSlots[s_loadSel]);
            UIManager_ShowLoadScreen();
        }
        if (mx>=backR.x && mx<backR.x+backR.w && my>=backR.y && my<backR.y+backR.h) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            return STATE_MENU;
        }
    }
    return STATE_LOAD_GAME;
}

static bool s_showSaveNameDlg = false;
static char s_saveNameBuf[21] = "";

static bool SaveNameIsValid(const char* s) {
    int n = (int)strlen(s);
    if (n == 0) return false;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 126) return false;
        if (c=='/' || c=='\\' || c==':' || c=='*' ||
            c=='?' || c=='"'  || c=='<' || c=='>'  || c=='|') return false;
    }
    return true;
}

void UIManager_ShowSaveNameDialog() {
    s_showSaveNameDlg = true;
    int n = (int)GetSaveFiles().size() + 1;
    snprintf(s_saveNameBuf, sizeof(s_saveNameBuf), "save_%d", n);
}

void UIManager_HideSaveNameDialog() { s_showSaveNameDlg = false; }
bool UIManager_IsSaveNameDialogOpen() { return s_showSaveNameDlg; }

void UIManager_RenderSaveNameDialog(SDL_Renderer* r) {
    if (!s_showSaveNameDlg) return;

    FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0, 0, 0, 160});

    const int PW = 360, PH = 300;
    const int PX = WINDOW_WIDTH / 2 - PW / 2;
    const int PY = WINDOW_HEIGHT / 2 - PH / 2;

    DrawPanel(r, PX, PY, PW, PH);
    OutlineRect(r, PX, PY, PW, PH, DT_LINE, 2);

    RT(r, s_fontMd, "Save Game", WINDOW_WIDTH / 2, PY + 42, DT_WARM, true);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, DT_LINE.r, DT_LINE.g, DT_LINE.b, 80);
    SDL_RenderDrawLine(r, PX + 20, PY + 66, PX + PW - 20, PY + 66);

    RT(r, s_fontSm, "Enter save name:", PX + 24, PY + 86, DT_LIGHT, false);

    const int IX = PX + 20, IY = PY + 106, IW = PW - 40, IH = 42;
    FillRect(r, IX, IY, IW, IH, {50, 30, 12, 220});
    OutlineRect(r, IX, IY, IW, IH, DT_WARM, 1);

    char dispBuf[26];
    bool cursorOn = (SDL_GetTicks() % 800) < 400;
    snprintf(dispBuf, sizeof(dispBuf), cursorOn ? "%s|" : "%s", s_saveNameBuf);
    RT(r, s_fontSm, dispBuf, IX + 10, IY + IH / 2, DT_WARM, false);

    char cntBuf[10];
    snprintf(cntBuf, sizeof(cntBuf), "%d/20", (int)strlen(s_saveNameBuf));
    RT(r, s_fontSm, cntBuf, PX + PW - 50, PY + 155, DT_LINE, true);

    const int BTW = 118, BTH = 44, BTY = PY + 216;
    DrawButton(r, PX + 50,  BTY, BTW, BTH, 0, "Save");
    DrawButton(r, PX + 192, BTY, BTW, BTH, 0, "Cancel");
}

int UIManager_HandleSaveNameDialogEvent(const SDL_Event& e, _GAMESTATE& state) {
    if (!s_showSaveNameDlg) return 0;

    const int PW = 360, PH = 300;
    const int PX = WINDOW_WIDTH / 2 - PW / 2;
    const int PY = WINDOW_HEIGHT / 2 - PH / 2;
    const int BTW = 118, BTH = 44, BTY = PY + 216;
    SDL_Rect saveR   = { PX + 50,  BTY, BTW, BTH };
    SDL_Rect cancelR = { PX + 192, BTY, BTW, BTH };

    auto doSave = [&]() -> int {
        if (!SaveNameIsValid(s_saveNameBuf)) return 0;
        SaveGame(state, s_saveNameBuf);
        UIManager_HideSaveNameDialog();
        s_saveMsgUntil = SDL_GetTicks() + 2200;
        AudioManager_PlaySFX(SFX_MENU_SELECT);
        return 1;
    };

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
            UIManager_HideSaveNameDialog();
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            return -1;
        case SDLK_RETURN: return doSave();
        case SDLK_BACKSPACE: {
            int n = (int)strlen(s_saveNameBuf);
            if (n > 0) s_saveNameBuf[n - 1] = '\0';
            break;
        }
        default: {
            int sym = e.key.keysym.sym;
            if (sym >= 32 && sym < 127) {
                char c = (char)sym;
                if (e.key.keysym.mod & KMOD_SHIFT) c = (char)toupper(c);
                if (c!='/' && c!='\\' && c!=':' && c!='*' &&
                    c!='?' && c!='"'  && c!='<' && c!='>'  && c!='|') {
                    int n = (int)strlen(s_saveNameBuf);
                    if (n < 20) { s_saveNameBuf[n] = c; s_saveNameBuf[n+1] = '\0'; }
                }
            }
            break;
        }
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (mx>=saveR.x   && mx<saveR.x+saveR.w   && my>=saveR.y   && my<saveR.y+saveR.h)
            return doSave();
        if (mx>=cancelR.x && mx<cancelR.x+cancelR.w && my>=cancelR.y && my<cancelR.y+cancelR.h) {
            UIManager_HideSaveNameDialog();
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            return -1;
        }
    }
    return 0;
}

void UIManager_ShowResult(const _GAMESTATE& state, int result) {
    s_showResult = true;
    if      (result == P1_THANG) snprintf(s_resultMsg, sizeof(s_resultMsg), "%s wins!", state.players[0].name);
    else if (result == P2_THANG) snprintf(s_resultMsg, sizeof(s_resultMsg), "%s wins!", state.players[1].name);
    else                         snprintf(s_resultMsg, sizeof(s_resultMsg), "Draw!");
}

void UIManager_RenderResult(SDL_Renderer* r) {
    if (!s_showResult) return;
    FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0,0,0,160});

    const int MW = 560, MH = 280, MX = WINDOW_WIDTH/2-MW/2, MY = WINDOW_HEIGHT/2-MH/2;
    DrawPanel(r, MX, MY, MW, MH);
    RT(r, s_fontLg, s_resultMsg, WINDOW_WIDTH/2, MY + 80, DT_WARM, true);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, DT_LINE.r, DT_LINE.g, DT_LINE.b, 100);
    SDL_RenderDrawLine(r, MX+DT_XXL, MY+120, MX+MW-DT_XXL, MY+120);

    DrawButton(r, MX+MW/2-100, MY+MH-90, 200, 58, 0, "OK");
}

void UIManager_HideResult() {
    s_showResult = false;
}

void UIManager_ShowSplash() {
    s_splashTimer = 2.5f;
}

static bool s_qaSplashFreeze = false;
void UIManager_QA_FreezeSplash() { s_splashTimer = 1.2f; s_qaSplashFreeze = true; }
void UIManager_QA_OpenCredits() { s_showCredits = true; }
void UIManager_QA_OpenHowTo()   { s_showHowTo = true; }
void UIManager_QA_FakeSaves(int n) {
    s_saves.clear(); s_saveSlots.clear();
    for (int i = 0; i < n; i++) { char b[32]; snprintf(b, sizeof(b), "qa_save_%d", i+1); s_saves.push_back(b); s_saveSlots.push_back(b); }
}

AppState UIManager_UpdateSplash(float dt) {
    if (s_qaSplashFreeze) return STATE_SPLASH;
    s_splashTimer -= dt;
    if (s_splashTimer <= 0.0f) return STATE_MENU;
    return STATE_SPLASH;
}

void UIManager_RenderSplash(SDL_Renderer* r) {
    float alpha = 1.0f;
    if (s_splashTimer > 2.0f)      alpha = 1.0f - (s_splashTimer - 2.0f) / 0.5f;
    else if (s_splashTimer < 0.5f) alpha = s_splashTimer / 0.5f;

    if (s_splashBg) {
        FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0, 0, 0, 255});
        SDL_SetTextureAlphaMod(s_splashBg, (Uint8)(alpha * 255));
        DrawCover(r, s_splashBg);
        SDL_SetTextureAlphaMod(s_splashBg, 255);
        SDL_Color hint = {255, 250, 235, (Uint8)(alpha * 200)};
        RTO(r, s_fontSm, "Press any key", WINDOW_WIDTH/2, WINDOW_HEIGHT - 60, hint, true);
        return;
    }

    FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {DT_DEEP.r, DT_DEEP.g, DT_DEEP.b, 255});
    SDL_Color c = {DT_WARM.r, DT_WARM.g, DT_WARM.b, (Uint8)(alpha * 255)};
    RT(r, s_fontXL, "Berry Grove", WINDOW_WIDTH/2, WINDOW_HEIGHT/2 - 30, c, true);
    SDL_Color c2 = {DT_LIGHT.r, DT_LIGHT.g, DT_LIGHT.b, (Uint8)(alpha * 180)};
    RT(r, s_fontSm, "A Strawtegy Game", WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 30, c2, true);
}

static const char* PAUSE_ITEMS[] = { "Resume", "Restart", "Save Game", "Quit to Menu" };
static const int   PAUSE_COUNT   = 4;
static int         s_pauseBtnState[4] = {};

void UIManager_ShowPause() {
    s_showPause = true;
    memset(s_pauseBtnState, 0, sizeof(s_pauseBtnState));
}
void UIManager_HidePause()    { s_showPause = false; }
bool UIManager_IsPauseShown() { return s_showPause; }

PauseAction UIManager_HandlePauseEvent(const SDL_Event& e, _GAMESTATE& state) {
    if (!s_showPause) return PAUSE_NONE;
    const int MW = 440, BTW = 360, BTH = 60, BTG = 10;
    const int MH = 80 + PAUSE_COUNT*(BTH+BTG) + DT_L;
    const int MX = WINDOW_WIDTH/2 - MW/2;
    const int MY = WINDOW_HEIGHT/2 - MH/2;
    const int bx = WINDOW_WIDTH/2 - BTW/2;

    if (e.type == SDL_MOUSEMOTION) {
        for (int i = 0; i < PAUSE_COUNT; i++) {
            int by = MY + 76 + i*(BTH+BTG);
            SDL_Rect b = {bx, by, BTW, BTH};
            bool hit = (e.motion.x>=b.x && e.motion.x<b.x+b.w &&
                        e.motion.y>=b.y && e.motion.y<b.y+b.h);
            if (hit && s_pauseBtnState[i] == 0) AudioManager_PlaySFX(SFX_MENU_HOVER);
            s_pauseBtnState[i] = hit ? 1 : 0;
        }
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        for (int i = 0; i < PAUSE_COUNT; i++) {
            int by = MY + 76 + i*(BTH+BTG);
            SDL_Rect b = {bx, by, BTW, BTH};
            if (e.button.x>=b.x && e.button.x<b.x+b.w &&
                e.button.y>=b.y && e.button.y<b.y+b.h) {
                static const PauseAction ACTS[4] = {PAUSE_RESUME, PAUSE_RESTART, PAUSE_SAVE, PAUSE_QUIT};
                AudioManager_PlaySFX(SFX_MENU_SELECT);
                return ACTS[i];
            }
        }
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        AudioManager_PlaySFX(SFX_MENU_SELECT);
        return PAUSE_RESUME;
    }
    return PAUSE_NONE;
}

void UIManager_RenderPauseOverlay(SDL_Renderer* r) {
    if (!s_showPause) return;
    const int MW = 440, BTW = 360, BTH = 60, BTG = 10;
    const int MH = 80 + PAUSE_COUNT*(BTH+BTG) + DT_L;
    const int MX = WINDOW_WIDTH/2 - MW/2;
    const int MY = WINDOW_HEIGHT/2 - MH/2;

    FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0,0,0,140});
    DrawPanel(r, MX, MY, MW, MH);
    RT(r, s_fontLg, "Paused", WINDOW_WIDTH/2, MY + 44, DT_LIGHT, true);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, DT_LINE.r, DT_LINE.g, DT_LINE.b, 80);
    SDL_RenderDrawLine(r, MX+DT_XXL, MY+68, MX+MW-DT_XXL, MY+68);

    int bx = WINDOW_WIDTH/2 - BTW/2;
    for (int i = 0; i < PAUSE_COUNT; i++) {
        DrawButton(r, bx, MY + 76 + i*(BTH+BTG), BTW, BTH,
                   s_pauseBtnState[i], PAUSE_ITEMS[i]);
    }

    if (SDL_GetTicks() < s_saveMsgUntil)
        RT(r, s_fontSm, "Game saved!", WINDOW_WIDTH/2, MY + MH - 22, DT_WARM, true);
}

static const char* SETTINGS_TABS[] = { "Sound", "Display" };
static const int   SETTINGS_TAB_COUNT = 2;
static int s_settingsBtnState[2] = {};

static const int ST_PW = 760, ST_PH = 470;
static const int ST_PX = WINDOW_WIDTH/2 - ST_PW/2;
static const int ST_PY = (WINDOW_HEIGHT - ST_PH)/2;
static const int ST_TW = 210, ST_TH = 52, ST_TG = 16;
static int ST_TabsX() { return ST_PX + ST_PW/2 - (ST_TW*SETTINGS_TAB_COUNT + ST_TG*(SETTINGS_TAB_COUNT-1))/2; }
static int ST_TabY()  { return ST_PY + 104; }
static SDL_Rect ST_CloseR() { SDL_Rect c = { ST_PX + ST_PW - 60, ST_PY + 16, 44, 44 }; return c; }
static int s_musicVol = 100, s_sfxVol = 100;
static const int ST_VBW = 52, ST_VBH = 44;
static int ST_VolRowY(int k) { return ST_TabY() + ST_TH + 64 + k*92; }
static int ST_VolBlockX()    { return ST_PX + ST_PW - 80 - (ST_VBW + 90 + ST_VBW); }
static SDL_Rect ST_VolMinus(int k){ SDL_Rect q={ST_VolBlockX(),              ST_VolRowY(k)-ST_VBH/2, ST_VBW, ST_VBH}; return q; }
static SDL_Rect ST_VolPlus(int k) { SDL_Rect q={ST_VolBlockX()+ST_VBW+90,    ST_VolRowY(k)-ST_VBH/2, ST_VBW, ST_VBH}; return q; }
static SDL_Rect ST_ToggleR() { SDL_Rect q={ST_VolBlockX(), ST_VolRowY(0)-ST_VBH/2, ST_VBW*2+90, ST_VBH}; return q; }

void UIManager_ShowSettings() {
    s_settingsTab = 0;
    memset(s_settingsBtnState, 0, sizeof(s_settingsBtnState));
}

AppState UIManager_HandleSettingsEvent(const SDL_Event& e) {
    SDL_Rect closeR = ST_CloseR();
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (mx>=closeR.x && mx<closeR.x+closeR.w && my>=closeR.y && my<closeR.y+closeR.h) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            return STATE_MENU;
        }
        for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
            SDL_Rect tabR = {ST_TabsX() + i*(ST_TW+ST_TG), ST_TabY(), ST_TW, ST_TH};
            if (mx>=tabR.x && mx<tabR.x+tabR.w && my>=tabR.y && my<tabR.y+tabR.h) {
                if (s_settingsTab != i) AudioManager_PlaySFX(SFX_MENU_SELECT);
                s_settingsTab = i;
            }
        }
        auto inRect = [&](SDL_Rect q){ return mx>=q.x && mx<q.x+q.w && my>=q.y && my<q.y+q.h; };
        if (s_settingsTab == 0) {
            int* vols[2] = { &s_musicVol, &s_sfxVol };
            for (int k = 0; k < 2; k++) {
                bool changed = false;
                if (inRect(ST_VolMinus(k))) { *vols[k] = (*vols[k] > 0)   ? *vols[k]-10 : 0;   changed = true; }
                if (inRect(ST_VolPlus(k)))  { *vols[k] = (*vols[k] < 100) ? *vols[k]+10 : 100; changed = true; }
                if (changed) {
                    int mix = (*vols[k]) * 128 / 100;
                    if (k == 0) AudioManager_SetMusicVolume(mix);
                    else        AudioManager_SetSFXVolume(mix);
                    AudioManager_PlaySFX(SFX_MENU_HOVER);
                }
            }
        }
        else if (s_settingsTab == 1) {
            if (inRect(ST_ToggleR())) {
                Particle_SetEnabled(!Particle_IsEnabled());
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
        }
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        AudioManager_PlaySFX(SFX_MENU_SELECT);
        return STATE_MENU;
    }
    return STATE_SETTINGS;
}

void UIManager_RenderSettings(SDL_Renderer* r) {
    if (s_settingsBg) DrawCover(r, s_settingsBg);
    else Renderer_DrawMatchBackground(r);
    FillRect(r, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, {0,0,0,140});

    DrawPanel(r, ST_PX, ST_PY, ST_PW, ST_PH);
    RT(r, s_fontLg, "Settings", ST_PX + ST_PW/2, ST_PY + 42, DT_LIGHT, true);

    SDL_Rect cr = ST_CloseR();
    DrawButton(r, cr.x, cr.y, cr.w, cr.h, 0, "X");

    for (int i = 0; i < SETTINGS_TAB_COUNT; i++)
        DrawButton(r, ST_TabsX() + i*(ST_TW+ST_TG), ST_TabY(), ST_TW, ST_TH,
                   s_settingsTab == i ? 1 : 0, SETTINGS_TABS[i]);

    int rowX = ST_PX + 80;
    if (s_settingsTab == 0) {
        const char* labels[2] = { "Music volume", "SFX volume" };
        int vols[2] = { s_musicVol, s_sfxVol };
        for (int k = 0; k < 2; k++) {
            int rowY = ST_VolRowY(k);
            RT(r, s_fontMd, labels[k], rowX, rowY, DT_LIGHT, false);
            SDL_Rect mR = ST_VolMinus(k), pR = ST_VolPlus(k);
            DrawButton(r, mR.x, mR.y, mR.w, mR.h, 0, "-");
            char vb[8]; snprintf(vb, sizeof(vb), "%d%%", vols[k]);
            RT(r, s_fontMd, vb, (mR.x + pR.x + pR.w)/2, rowY, DT_WARM, true);
            DrawButton(r, pR.x, pR.y, pR.w, pR.h, 0, "+");
        }
    } else {
        int rowY = ST_VolRowY(0);
        RT(r, s_fontMd, "Particle effects", rowX, rowY, DT_LIGHT, false);
        SDL_Rect tR = ST_ToggleR();
        bool on = Particle_IsEnabled();
        DrawButton(r, tR.x, tR.y, tR.w, tR.h, on ? 1 : 0, on ? "ON" : "OFF");
        RT(r, s_fontSm, "Falling-leaf and sparkle effects when planting or harvesting.",
           rowX, ST_VolRowY(1) - ST_VBH/2, DT_OFF, false);
    }
}

void UIManager_RenderText(SDL_Renderer* r, const char* text,
                          int x, int y, SDL_Color color,
                          bool center, int fontSize) {
    TTF_Font* f = (fontSize == 2) ? s_fontLg
                : (fontSize == 1) ? s_fontMd
                                  : s_fontSm;
    RT(r, f, text, x, y + 8, color, center);
}
