#include "UIManager.h"
#include "../game/FileHandling.h"
#include <cstring>
#include <vector>
#include <string>
#include <cmath>

// ================================================================
//  UIManager.cpp
//
//  Layout HUD: Panel P1 (trái) | Board | Panel P2 (phải)
//  Mỗi panel rộng LEFT_PANEL_WIDTH / HUD_PANEL_WIDTH = 200px
// ================================================================

// ── Module statics ────────────────────────────────────────────────
static SDL_Renderer* s_renderer = nullptr;
static TTF_Font*     s_fontLg   = nullptr;   // 34pt — tiêu đề
static TTF_Font*     s_fontMd   = nullptr;   // 22pt — nội dung
static TTF_Font*     s_fontSm   = nullptr;   // 16pt — gợi ý

// ── Result dialog ─────────────────────────────────────────────────
static bool s_showResult = false;
static char s_resultMsg[64] = {};

// ── Menu ─────────────────────────────────────────────────────────
static int   s_menuSel   = 0;
static float s_menuHue   = 0.0f;
static GameMode s_pendingMode = MODE_PVP;

static const char* MENU_ITEMS[] = {
    "Player vs Player",
    "Player vs AI",
    "Load Game",
    "Quit"
};
static const int MENU_COUNT = 4;

// ── Name Input ────────────────────────────────────────────────────
struct NameInputUI {
    GameMode mode      = MODE_PVP;
    int      diff      = AI_MEDIUM;
    int      field     = 0;     // 0=P1name, 1=P2name/diff, 2=start
    char     p1[30]    = "Player 1";
    char     p2[30]    = "Player 2";
};
static NameInputUI s_ni;

// ── Load screen ───────────────────────────────────────────────────
static std::vector<std::string> s_saves;
static int s_loadSel = 0;

// ── Helpers ───────────────────────────────────────────────────────
static void RT(SDL_Renderer* r, TTF_Font* f, const char* t,
               int x, int y, SDL_Color c, bool center = false) {
    if (!f || !t || !t[0]) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, t, c);
    if (!s) return;
    SDL_Texture* tx = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    if (!tx) return;
    int w, h;
    SDL_QueryTexture(tx, nullptr, nullptr, &w, &h);
    SDL_Rect dst = { center ? x - w/2 : x, y, w, h };
    SDL_RenderCopy(r, tx, nullptr, &dst);
    SDL_DestroyTexture(tx);
}

static SDL_Color HueRGB(float hue) {
    float h = fmodf(hue, 360.f) / 60.f;
    int   i = static_cast<int>(h);
    float f = h - i;
    float rv, gv, bv;
    switch (i) {
    case 0:  rv=1;   gv=f;   bv=0;   break;
    case 1:  rv=1-f; gv=1;   bv=0;   break;
    case 2:  rv=0;   gv=1;   bv=f;   break;
    case 3:  rv=0;   gv=1-f; bv=1;   break;
    case 4:  rv=f;   gv=0;   bv=1;   break;
    default: rv=1;   gv=0;   bv=1-f; break;
    }
    return { static_cast<Uint8>(rv*255), static_cast<Uint8>(gv*255),
             static_cast<Uint8>(bv*255), 255 };
}

static TTF_Font* TryFont(const char* path, int pt) {
    TTF_Font* f = TTF_OpenFont(path, pt);
    if (f) return f;
    return TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", pt);
}

// ── Init / Shutdown ───────────────────────────────────────────────
bool UIManager_Init(SDL_Renderer* renderer) {
    s_renderer = renderer;

    if (TTF_Init() != 0)
        SDL_Log("TTF_Init failed: %s — text may be blank", TTF_GetError());

    const char* FONT = "assets/fonts/font.ttf";
    s_fontLg = TryFont(FONT, 34);
    s_fontMd = TryFont(FONT, 22);
    s_fontSm = TryFont(FONT, 16);

    if (!s_fontMd)
        SDL_Log("UIManager: no font found — text will be blank");

    InitSaveFolder();
    return true;
}

void UIManager_Shutdown() {
    if (s_fontLg) { TTF_CloseFont(s_fontLg); s_fontLg = nullptr; }
    if (s_fontMd) { TTF_CloseFont(s_fontMd); s_fontMd = nullptr; }
    if (s_fontSm) { TTF_CloseFont(s_fontSm); s_fontSm = nullptr; }
    TTF_Quit();
    s_renderer = nullptr;
}

// ── Menu ─────────────────────────────────────────────────────────
void UIManager_ShowMenu() {
    s_menuSel    = 0;
    s_menuHue    = 0.0f;
    s_showResult = false;
}

void UIManager_UpdateMenu(float dt) {
    s_menuHue = fmodf(s_menuHue + 80.0f * dt, 360.0f);
}

void UIManager_RenderMenu(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 8, 8, 25, 255);
    SDL_RenderClear(r);

    RT(r, s_fontLg, "CO CARO — GOMOKU",
       WINDOW_WIDTH/2, 110, HueRGB(s_menuHue), true);

    const int CX = WINDOW_WIDTH / 2;
    const int Y0 = 230, STEP = 62, BW = 320, BH = 46;

    for (int i = 0; i < MENU_COUNT; i++) {
        bool sel = (i == s_menuSel);
        SDL_Rect bar = { CX - BW/2, Y0 + i*STEP - 4, BW, BH };
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, sel ? 70 : 25, sel ? 70 : 25, sel ? 130 : 60,
                               sel ? 200 : 100);
        SDL_RenderFillRect(r, &bar);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, sel ? 180 : 80, sel ? 180 : 80, sel ? 255 : 120, 255);
        SDL_RenderDrawRect(r, &bar);

        SDL_Color tc = sel ? SDL_Color{255,235,60,255} : SDL_Color{200,200,200,255};
        RT(r, s_fontMd, MENU_ITEMS[i], CX, Y0 + i*STEP + 8, tc, true);
    }

    RT(r, s_fontSm, "Arrow keys / mouse   Enter / click to select",
       CX, WINDOW_HEIGHT - 55, {100,100,100,255}, true);
}

AppState UIManager_HandleMenuEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_UP:   s_menuSel = (s_menuSel + MENU_COUNT - 1) % MENU_COUNT; return STATE_MENU;
        case SDLK_DOWN: s_menuSel = (s_menuSel + 1) % MENU_COUNT;              return STATE_MENU;
        case SDLK_RETURN: case SDLK_KP_ENTER: goto CONFIRM;
        default: break;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        const int Y0=230, STEP=62, BW=320, BH=46, CX=WINDOW_WIDTH/2;
        for (int i = 0; i < MENU_COUNT; i++) {
            SDL_Rect b = { CX-BW/2, Y0+i*STEP-4, BW, BH };
            if (e.motion.x>=b.x && e.motion.x<=b.x+b.w &&
                e.motion.y>=b.y && e.motion.y<=b.y+b.h)
                s_menuSel = i;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        const int Y0=230, STEP=62, BW=320, BH=46, CX=WINDOW_WIDTH/2;
        for (int i = 0; i < MENU_COUNT; i++) {
            SDL_Rect b = { CX-BW/2, Y0+i*STEP-4, BW, BH };
            if (e.button.x>=b.x && e.button.x<=b.x+b.w &&
                e.button.y>=b.y && e.button.y<=b.y+b.h)
                { s_menuSel = i; goto CONFIRM; }
        }
    }
    return STATE_MENU;

CONFIRM:
    switch (s_menuSel) {
    case 0: s_pendingMode = MODE_PVP; return STATE_NAME_INPUT;
    case 1: s_pendingMode = MODE_PVE; return STATE_NAME_INPUT;
    case 2: return STATE_LOAD_GAME;
    case 3: return STATE_EXIT;
    }
    return STATE_MENU;
}

// ── Name Input ────────────────────────────────────────────────────
void UIManager_ShowNameInput(_GAMESTATE& state) {
    s_ni.mode  = s_pendingMode;
    s_ni.diff  = AI_MEDIUM;
    s_ni.field = 0;
    strncpy_s(s_ni.p1, sizeof(s_ni.p1), state.players[0].name, _TRUNCATE);
    strncpy_s(s_ni.p2, sizeof(s_ni.p2), state.players[1].name, _TRUNCATE);
    SDL_StartTextInput();
}

void UIManager_RenderNameInput(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 12, 12, 35, 255);
    SDL_RenderClear(r);

    RT(r, s_fontLg, "Player Setup", WINDOW_WIDTH/2, 75, {255,220,50,255}, true);

    const int LX = WINDOW_WIDTH/2 - 190;
    const int FW = 380, FH = 38;
    int y = 155;

    // Vẽ field với label
    auto Field = [&](const char* label, char* buf, int idx) {
        RT(r, s_fontSm, label, LX, y, {170,170,170,255});
        y += 24;
        bool act = (s_ni.field == idx);
        SDL_SetRenderDrawColor(r, act?70:35, act?70:35, act?150:80, 255);
        SDL_Rect box = {LX, y, FW, FH};
        SDL_RenderFillRect(r, &box);
        SDL_SetRenderDrawColor(r, act?180:90, act?180:90, 255, 255);
        SDL_RenderDrawRect(r, &box);
        char disp[34]; snprintf(disp, sizeof(disp), "%s%s", buf, act ? "_" : "");
        RT(r, s_fontMd, disp, LX+8, y+6, {230,230,230,255});
        y += FH + 18;
    };

    Field("Player 1 Name:", s_ni.p1, 0);

    if (s_ni.mode == MODE_PVP) {
        Field("Player 2 Name:", s_ni.p2, 1);
    } else {
        RT(r, s_fontSm, "AI Difficulty:", LX, y, {170,170,170,255});
        y += 24;
        const char* diffs[] = { "Easy", "Medium", "Hard" };
        int          dvals[] = { AI_EASY, AI_MEDIUM, AI_HARD };
        int dx = LX;
        for (int i = 0; i < 3; i++) {
            bool sel = (s_ni.diff == dvals[i]);
            SDL_SetRenderDrawColor(r, sel?55:28, sel?120:60, sel?55:28, 255);
            SDL_Rect btn = {dx, y, 115, 38};
            SDL_RenderFillRect(r, &btn);
            SDL_SetRenderDrawColor(r, sel?100:55, 200, sel?100:55, 255);
            SDL_RenderDrawRect(r, &btn);
            SDL_Color tc = sel ? SDL_Color{255,255,50,255} : SDL_Color{200,200,200,255};
            RT(r, s_fontMd, diffs[i], dx+57, y+8, tc, true);
            dx += 130;
        }
        y += 56;
    }

    // Start button
    {
        bool sel = (s_ni.field == 2);
        SDL_SetRenderDrawColor(r, sel?55:28, sel?140:80, sel?55:28, 255);
        SDL_Rect btn = {WINDOW_WIDTH/2-95, y, 190, 46};
        SDL_RenderFillRect(r, &btn);
        SDL_SetRenderDrawColor(r, 80, 200, 80, 255);
        SDL_RenderDrawRect(r, &btn);
        RT(r, s_fontMd, "Start Game", WINDOW_WIDTH/2, y+12, {235,235,235,255}, true);
    }

    RT(r, s_fontSm, "Tab: next field   Enter: start   ESC: back",
       WINDOW_WIDTH/2, WINDOW_HEIGHT-55, {90,90,90,255}, true);
}

AppState UIManager_HandleNameInputEvent(const SDL_Event& e, _GAMESTATE& state) {
    int nFields = (s_ni.mode == MODE_PVP) ? 3 : 2;   // P1, P2 or diff, start

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
            SDL_StopTextInput();
            return STATE_MENU;
        case SDLK_TAB:
            s_ni.field = (s_ni.field + 1) % nFields;
            return STATE_NAME_INPUT;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            goto DO_START;
        case SDLK_BACKSPACE:
            if (s_ni.field == 0 && s_ni.p1[0]) s_ni.p1[strlen(s_ni.p1)-1] = '\0';
            if (s_ni.field == 1 && s_ni.mode == MODE_PVP && s_ni.p2[0])
                s_ni.p2[strlen(s_ni.p2)-1] = '\0';
            break;
        default: break;
        }
    } else if (e.type == SDL_TEXTINPUT) {
        if (s_ni.field == 0 && strlen(s_ni.p1) < 28)
            strncat_s(s_ni.p1, sizeof(s_ni.p1), e.text.text, _TRUNCATE);
        else if (s_ni.field == 1 && s_ni.mode == MODE_PVP && strlen(s_ni.p2) < 28)
            strncat_s(s_ni.p2, sizeof(s_ni.p2), e.text.text, _TRUNCATE);
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        // Difficulty buttons (PVE)
        if (s_ni.mode == MODE_PVE) {
            int dvals[] = { AI_EASY, AI_MEDIUM, AI_HARD };
            int LX = WINDOW_WIDTH/2-190;
            int btnY = 155 + 24 + 38 + 18 + 24;  // approx
            for (int i = 0; i < 3; i++) {
                SDL_Rect btn = { LX + i*130, btnY, 115, 38 };
                if (e.button.x>=btn.x && e.button.x<=btn.x+btn.w &&
                    e.button.y>=btn.y && e.button.y<=btn.y+btn.h)
                    s_ni.diff = dvals[i];
            }
        }
        // Start button (approximate position)
        SDL_Rect startBtn = { WINDOW_WIDTH/2-95, WINDOW_HEIGHT-200, 190, 46 };
        if (e.button.x>=startBtn.x && e.button.x<=startBtn.x+startBtn.w &&
            e.button.y>=startBtn.y && e.button.y<=startBtn.y+startBtn.h)
            goto DO_START;
    }
    return STATE_NAME_INPUT;

DO_START:
    SDL_StopTextInput();
    if (s_ni.p1[0]) strncpy_s(state.players[0].name, sizeof(state.players[0].name), s_ni.p1, _TRUNCATE);
    if (s_ni.mode == MODE_PVP) {
        if (s_ni.p2[0]) strncpy_s(state.players[1].name, sizeof(state.players[1].name), s_ni.p2, _TRUNCATE);
    } else {
        strncpy_s(state.players[1].name, sizeof(state.players[1].name), "AI", _TRUNCATE);
        state.difficulty = static_cast<AIDifficulty>(s_ni.diff);
    }
    state.mode = s_ni.mode;
    return STATE_PLAYING;
}

// ── HUD: panel trái (P1) + panel phải (P2) ───────────────────────
static void RenderPanel(SDL_Renderer* r, const _GAMESTATE& state, int pidx, int px, int pw) {
    const int PY = BOARD_OFFSET_Y;
    const int PH = BOARD_PIXEL_SIZE;

    // Nền panel
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 12, 12, 30, 210);
    SDL_Rect bg = {px, PY, pw, PH};
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    bool isTurn = (state.gameStatus == CHUA_KET_THUC) && (state.turn == (pidx == 0));

    // Viền sáng khi đến lượt
    if (isTurn) {
        SDL_SetRenderDrawColor(r, 210, 200, 40, 255);
        SDL_Rect brd = {px+2, PY+2, pw-4, PH-4};
        SDL_RenderDrawRect(r, &brd);
    } else {
        SDL_SetRenderDrawColor(r, 55, 55, 80, 255);
        SDL_Rect brd = {px+2, PY+2, pw-4, PH-4};
        SDL_RenderDrawRect(r, &brd);
    }

    // Avatar placeholder (hình vuông căn giữa panel)
    int avSide = pw - 24;
    if (avSide > 150) avSide = 150;
    int avX = px + (pw - avSide) / 2;
    int avY = PY + 14;

    SDL_Color avBg  = (pidx==0) ? SDL_Color{70,22,22,255} : SDL_Color{22,35,75,255};
    SDL_Color avBdr = (pidx==0) ? SDL_Color{200,70,70,255} : SDL_Color{70,110,210,255};
    SDL_SetRenderDrawColor(r, avBg.r, avBg.g, avBg.b, 255);
    SDL_Rect av = {avX, avY, avSide, avSide};
    SDL_RenderFillRect(r, &av);
    SDL_SetRenderDrawColor(r, avBdr.r, avBdr.g, avBdr.b, 255);
    SDL_RenderDrawRect(r, &av);

    // Ký hiệu X hoặc O lớn bên trong avatar (text font to)
    RT(r, s_fontLg, pidx==0 ? "X" : "O",
       avX + avSide/2, avY + avSide/2 - 17, avBdr, true);

    int y = avY + avSide + 12;

    // Tên người chơi
    RT(r, s_fontMd, state.players[pidx].name,
       px + pw/2, y, avBdr, true);
    y += 28;

    // Thống kê
    char buf[48];
    snprintf(buf, sizeof(buf), "W:%d  L:%d  D:%d",
             state.players[pidx].wins,
             state.players[pidx].losses,
             state.players[pidx].draws);
    RT(r, s_fontSm, buf, px + pw/2, y, {170,170,170,255}, true);
    y += 21;

    snprintf(buf, sizeof(buf), "Moves: %d", state.players[pidx].moves);
    RT(r, s_fontSm, buf, px + pw/2, y, {150,150,150,255}, true);
    y += 28;

    // Trạng thái lượt / AI đang nghĩ
    if (isTurn) {
        RT(r, s_fontSm, "YOUR TURN", px + pw/2, y, {255,225,45,255}, true);
    } else if (state.mode == MODE_PVE && pidx == 1 && state.aiThinking) {
        RT(r, s_fontSm, "Thinking...", px + pw/2, y, {140,140,200,255}, true);
    }

    // Kết quả (hiển thị trong panel tương ứng khi thắng/hòa)
    if (s_showResult && s_resultMsg[0]) {
        int ry = PY + PH - 80;
        RT(r, s_fontSm, s_resultMsg, px + pw/2, ry, {255,90,90,255}, true);
        RT(r, s_fontSm, "R:replay  ESC:menu", px + pw/2, ry + 22, {200,200,50,255}, true);
    }

    // Gợi ý phím (chỉ hiện ở panel P1, phần dưới)
    if (pidx == 0) {
        int hy = PY + PH - (s_showResult ? 140 : 115);
        SDL_Color hint = {75,75,75,255};
        const char* hints[] = {
            "WASD/Mouse:move",
            "Enter/Click:place",
            "L:save  T:load",
            "ESC:menu"
        };
        for (auto h : hints) { RT(r, s_fontSm, h, px+6, hy, hint); hy += 20; }
    }
}

void UIManager_RenderHUD(SDL_Renderer* r, const _GAMESTATE& state) {
    RenderPanel(r, state, 0, LEFT_PANEL_X,  LEFT_PANEL_WIDTH);
    RenderPanel(r, state, 1, HUD_PANEL_X, HUD_PANEL_WIDTH);
}

// ── Load screen ───────────────────────────────────────────────────
void UIManager_ShowLoadScreen() {
    s_saves  = GetSaveFiles();
    s_loadSel = 0;
}

void UIManager_RenderLoadScreen(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 8, 8, 25, 255);
    SDL_RenderClear(r);

    RT(r, s_fontLg, "Load Game", WINDOW_WIDTH/2, 80, {255,200,50,255}, true);

    if (s_saves.empty()) {
        RT(r, s_fontMd, "No save files found.", WINDOW_WIDTH/2, 260, {180,100,100,255}, true);
    } else {
        const int Y0 = 180, STEP = 48;
        for (int i = 0; i < static_cast<int>(s_saves.size()); i++) {
            bool sel = (i == s_loadSel);
            if (sel) {
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, 55, 55, 135, 180);
                SDL_Rect bar = { WINDOW_WIDTH/2-215, Y0+i*STEP-4, 430, 42 };
                SDL_RenderFillRect(r, &bar);
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            }
            SDL_Color fc = sel ? SDL_Color{255,228,50,255} : SDL_Color{200,200,200,255};
            RT(r, s_fontMd, s_saves[i].c_str(), WINDOW_WIDTH/2, Y0+i*STEP, fc, true);
        }
    }

    RT(r, s_fontSm, "Up/Down: select   Enter: load   ESC: back",
       WINDOW_WIDTH/2, WINDOW_HEIGHT-55, {90,90,90,255}, true);
}

AppState UIManager_HandleLoadEvent(const SDL_Event& e, _GAMESTATE& state) {
    if (e.type == SDL_KEYDOWN) {
        int n = static_cast<int>(s_saves.size());
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: return STATE_MENU;
        case SDLK_UP:
            if (n > 0) s_loadSel = (s_loadSel + n - 1) % n;
            break;
        case SDLK_DOWN:
            if (n > 0) s_loadSel = (s_loadSel + 1) % n;
            break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            if (n > 0 && s_loadSel < n && LoadGame(state, s_saves[s_loadSel])) {
                s_showResult = false;
                return STATE_PLAYING;
            }
            break;
        default: break;
        }
    }
    return STATE_LOAD_GAME;
}

// ── Save dialog (inner event loop) ───────────────────────────────
void UIManager_ShowSaveDialog(_GAMESTATE& state) {
    char name[30] = {};
    bool done = false;
    SDL_StartTextInput();

    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { done = true; break; }
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: done = true; break;
                case SDLK_RETURN: case SDLK_KP_ENTER:
                    if (name[0]) SaveGame(state, name);
                    done = true;
                    break;
                case SDLK_BACKSPACE:
                    if (name[0]) name[strlen(name)-1] = '\0';
                    break;
                default: break;
                }
            } else if (e.type == SDL_TEXTINPUT && strlen(name) < 28) {
                strncat_s(name, sizeof(name), e.text.text, _TRUNCATE);
            }
        }

        // Overlay
        SDL_SetRenderDrawBlendMode(s_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 175);
        SDL_Rect ov = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderFillRect(s_renderer, &ov);
        SDL_SetRenderDrawBlendMode(s_renderer, SDL_BLENDMODE_NONE);

        SDL_Rect box = { WINDOW_WIDTH/2-210, WINDOW_HEIGHT/2-95, 420, 190 };
        SDL_SetRenderDrawColor(s_renderer, 28, 28, 65, 255);
        SDL_RenderFillRect(s_renderer, &box);
        SDL_SetRenderDrawColor(s_renderer, 90, 90, 200, 255);
        SDL_RenderDrawRect(s_renderer, &box);

        RT(s_renderer, s_fontMd, "Save Game",
           WINDOW_WIDTH/2, box.y+16, {230,230,230,255}, true);

        SDL_Rect inp = {box.x+20, box.y+72, box.w-40, 36};
        SDL_SetRenderDrawColor(s_renderer, 45, 45, 95, 255);
        SDL_RenderFillRect(s_renderer, &inp);
        SDL_SetRenderDrawColor(s_renderer, 130, 130, 230, 255);
        SDL_RenderDrawRect(s_renderer, &inp);

        char disp[32]; snprintf(disp, sizeof(disp), "%s_", name);
        RT(s_renderer, s_fontMd, disp, inp.x+8, inp.y+5, {255,225,60,255});

        RT(s_renderer, s_fontSm, "Enter: save   ESC: cancel",
           WINDOW_WIDTH/2, box.y+130, {100,100,100,255}, true);

        SDL_RenderPresent(s_renderer);
        SDL_Delay(16);
    }
    SDL_StopTextInput();
}

// ── Result ───────────────────────────────────────────────────────
void UIManager_ShowResult(const _GAMESTATE& state, int result) {
    s_showResult = true;
    if (result == P1_THANG)
        snprintf(s_resultMsg, sizeof(s_resultMsg), "%s wins!", state.players[0].name);
    else if (result == P2_THANG)
        snprintf(s_resultMsg, sizeof(s_resultMsg), "%s wins!", state.players[1].name);
    else
        snprintf(s_resultMsg, sizeof(s_resultMsg), "Draw!");
}

void UIManager_HideResult() {
    s_showResult = false;
    s_resultMsg[0] = '\0';
}

void UIManager_RenderResult(SDL_Renderer* /*r*/) {
    // Kết quả được render inline trong RenderPanel (bên trong UIManager_RenderHUD)
}
