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

// ── Cozy Style Palette ────────────────────────────────────────────
// Tất cả màu UI tham chiếu qua đây — dễ thay đổi đồng loạt
static const SDL_Color COZY_PARCHMENT  = {212, 180, 134, 225};
static const SDL_Color COZY_PARCHMENT2 = {195, 162, 112, 240};  // darker panel
static const SDL_Color COZY_WOOD_DARK  = {107,  56,  24, 255};
static const SDL_Color COZY_WOOD_MID   = {160,  90,  35, 255};
static const SDL_Color COZY_TEXT_DARK  = { 72,  36,   8, 255};
static const SDL_Color COZY_TEXT_LIGHT = {240, 218, 175, 255};
static const SDL_Color COZY_GOLD       = {232, 198,  58, 255};
static const SDL_Color COZY_RED        = {178,  50,  24, 255};
static const SDL_Color COZY_GREEN      = { 68, 140,  52, 255};

// ── Icon textures (Garden Cozy Icons Pack) ────────────────────────
enum IconID {
    ICON_SETTINGS=0, ICON_X, ICON_ARROW_L, ICON_ARROW_R,
    ICON_PAUSE, ICON_BACK, ICON_TRASH, ICON_TROPHY,
    ICON_SOUND, ICON_MUTE, ICON_BOOK, ICON_COUNT
};
static SDL_Texture* s_icons[ICON_COUNT] = {};

// ── Character portrait layers (Free Base by Cozy Fae) ─────────────
// P1 (X/red): 5 layers; P2 (O/blue): 5 layers
static SDL_Texture* s_charP1[5] = {};
static SDL_Texture* s_charP2[5] = {};

// ── Title sign (menu screen) ──────────────────────────────────────
static SDL_Texture* s_titleSign = nullptr;

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

// Animation: hover scale (0→1) và bounce countdown mỗi nút
// s_menuHoverScale[i]: 0.0=bình thường, 1.0=đang hover → nút phóng to 8%
// s_menuBounce[i]: đếm ngược từ 0.20s → 0 khi click → nút "nhún" xuống rồi nảy lên
static float s_menuHoverScale[5] = {};
static float s_menuBounce[5]     = {};

// Deferred transition: sau khi bounce chạy xong mới chuyển màn hình
static AppState s_pendingMenuState = STATE_MENU;
static float    s_pendingDelay     = 0.0f;

static const char* MENU_ITEMS[] = {
    "Player vs Player",
    "Player vs AI",
    "Load Game",
    "Settings",
    "Quit"
};
static const int MENU_COUNT = 5;

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

    // Load icon textures (Garden Cozy Icons Pack)
    const char* iconPaths[ICON_COUNT] = {
        "assets/sprites/icon_settings.png",
        "assets/sprites/icon_x.png",
        "assets/sprites/icon_arrow_l.png",
        "assets/sprites/icon_arrow_r.png",
        "assets/sprites/icon_pause.png",
        "assets/sprites/icon_back.png",
        "assets/sprites/icon_trash.png",
        "assets/sprites/icon_trophy.png",
        "assets/sprites/icon_sound.png",
        "assets/sprites/icon_mute.png",
        "assets/sprites/icon_book.png",
    };
    for (int i = 0; i < ICON_COUNT; i++)
        s_icons[i] = IMG_LoadTexture(renderer, iconPaths[i]);

    // Title sign (fallback graceful nếu file chưa có)
    s_titleSign = IMG_LoadTexture(renderer, "assets/sprites/title_sign.jpg");

    // Load character portrait layers (Free Base by Cozy Fae)
    // P1 (X): skin03, red top, brown pants, short brown hair, blue eyes
    const char* p1Paths[5] = {
        "assets/sprites/char_p1_base.png",
        "assets/sprites/char_p1_top.png",
        "assets/sprites/char_p1_bottom.png",
        "assets/sprites/char_p1_hair.png",
        "assets/sprites/char_p1_eyes.png",
    };
    // P2 (O): skin07, blue top, blue pants, medium yellow hair, green eyes
    const char* p2Paths[5] = {
        "assets/sprites/char_p2_base.png",
        "assets/sprites/char_p2_top.png",
        "assets/sprites/char_p2_bottom.png",
        "assets/sprites/char_p2_hair.png",
        "assets/sprites/char_p2_eyes.png",
    };
    for (int i = 0; i < 5; i++) {
        s_charP1[i] = IMG_LoadTexture(renderer, p1Paths[i]);
        s_charP2[i] = IMG_LoadTexture(renderer, p2Paths[i]);
    }

    InitSaveFolder();
    return true;
}

void UIManager_Shutdown() {
    if (s_fontLg) { TTF_CloseFont(s_fontLg); s_fontLg = nullptr; }
    if (s_fontMd) { TTF_CloseFont(s_fontMd); s_fontMd = nullptr; }
    if (s_fontSm) { TTF_CloseFont(s_fontSm); s_fontSm = nullptr; }
    for (int i = 0; i < ICON_COUNT; i++)
        if (s_icons[i]) { SDL_DestroyTexture(s_icons[i]); s_icons[i] = nullptr; }
    if (s_titleSign) { SDL_DestroyTexture(s_titleSign); s_titleSign = nullptr; }
    for (int i = 0; i < 5; i++) {
        if (s_charP1[i]) { SDL_DestroyTexture(s_charP1[i]); s_charP1[i] = nullptr; }
        if (s_charP2[i]) { SDL_DestroyTexture(s_charP2[i]); s_charP2[i] = nullptr; }
    }
    TTF_Quit();
    s_renderer = nullptr;
}

// ── Cozy helpers ──────────────────────────────────────────────────

// Vẽ icon từ texture cache với alpha tùy chọn
static void DrawIcon(SDL_Renderer* r, IconID id, int cx, int cy, int size, Uint8 alpha = 255) {
    if (id < 0 || id >= ICON_COUNT || !s_icons[id]) return;
    SDL_SetTextureAlphaMod(s_icons[id], alpha);
    SDL_Rect dst = { cx - size/2, cy - size/2, size, size };
    SDL_RenderCopy(r, s_icons[id], nullptr, &dst);
    SDL_SetTextureAlphaMod(s_icons[id], 255);
}

// Vẽ character portrait bằng cách composite nhiều layer lên nhau
// Sprite sheet idle: 2 cột × 3 hàng — dùng frame 0 (góc trên-trái)
static void DrawCharPortrait(SDL_Renderer* r, SDL_Texture** layers, int x, int y, int size) {
    if (!layers[0]) return;
    int tw = 1, th = 1;
    SDL_QueryTexture(layers[0], nullptr, nullptr, &tw, &th);
    // Idle sheet: 2 cols × 3 rows
    SDL_Rect src = { 0, 0, tw / 2, th / 3 };
    SDL_Rect dst = { x, y, size, size };
    for (int i = 0; i < 5; i++)
        if (layers[i]) SDL_RenderCopy(r, layers[i], &src, &dst);
}

// Vẽ panel bo góc (rounded corners) với fill semi-transparent
// Kỹ thuật: 3 SDL_RenderFillRect tạo hình chữ thập + 4 góc tô thủ công bằng scanline
static void DrawCozyPanel(SDL_Renderer* r, int x, int y, int w, int h, bool highlighted = false) {
    const int R = 10;  // bán kính góc bo

    // Màu fill: semi-transparent để background (sky) thấu qua nhẹ
    SDL_Color fill   = highlighted
        ? SDL_Color{232, 198, 58, 235}    // gold khi selected
        : SDL_Color{255, 248, 230, 195};  // kem trắng semi-transparent
    SDL_Color border = highlighted ? COZY_GOLD : COZY_WOOD_DARK;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);

    // 3 rects: trên-dưới (bỏ góc) và giữa (full width)
    SDL_Rect rv = {x+R, y,   w-2*R, h  };  SDL_RenderFillRect(r, &rv); // thân đứng
    SDL_Rect rh = {x,   y+R, w,     h-2*R}; SDL_RenderFillRect(r, &rh); // thân ngang
    // Tô 4 góc bằng scanline hình tròn
    for (int dy = 0; dy < R; dy++) {
        int dx = R - static_cast<int>(std::sqrt(static_cast<float>(R*R - (R-dy)*(R-dy))));
        SDL_RenderDrawLine(r, x+dx,   y+dy,     x+R-1,   y+dy);     // góc trên-trái
        SDL_RenderDrawLine(r, x+w-R,  y+dy,     x+w-dx-1,y+dy);     // góc trên-phải
        SDL_RenderDrawLine(r, x+dx,   y+h-dy-1, x+R-1,   y+h-dy-1); // góc dưới-trái
        SDL_RenderDrawLine(r, x+w-R,  y+h-dy-1, x+w-dx-1,y+h-dy-1); // góc dưới-phải
    }

    // Border viền ngoài theo đường tròn góc
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderDrawLine(r, x+R,   y,     x+w-R, y);      // top
    SDL_RenderDrawLine(r, x+R,   y+h,   x+w-R, y+h);    // bottom
    SDL_RenderDrawLine(r, x,     y+R,   x,     y+h-R);  // left
    SDL_RenderDrawLine(r, x+w,   y+R,   x+w,   y+h-R);  // right
    // 4 góc border (arc thủ công)
    for (int dy = 0; dy < R; dy++) {
        int dx = R - static_cast<int>(std::sqrt(static_cast<float>(R*R - (R-dy)*(R-dy))));
        SDL_RenderDrawPoint(r, x+dx,   y+dy);       // top-left
        SDL_RenderDrawPoint(r, x+w-dx, y+dy);       // top-right
        SDL_RenderDrawPoint(r, x+dx,   y+h-dy);     // bot-left
        SDL_RenderDrawPoint(r, x+w-dx, y+h-dy);     // bot-right
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void UIManager_RenderText(SDL_Renderer* r, const char* text,
                          int x, int y, SDL_Color color,
                          bool center, int fontSize) {
    TTF_Font* f = (fontSize >= 2) ? s_fontLg
                : (fontSize == 1) ? s_fontMd
                :                   s_fontSm;
    RT(r, f, text, x, y, color, center);
}

// ── Menu ─────────────────────────────────────────────────────────
void UIManager_ShowMenu() {
    s_menuSel          = 0;
    s_menuHue          = 0.0f;
    s_showResult       = false;
    s_pendingMenuState = STATE_MENU;
    s_pendingDelay     = 0.0f;
    for (int i = 0; i < MENU_COUNT; i++) {
        s_menuHoverScale[i] = 0.0f;
        s_menuBounce[i]     = 0.0f;
    }
}

// Mỗi frame: cập nhật tất cả animation nút, xử lý deferred transition
// Trả về STATE_MENU bình thường, trả về state khác khi bounce xong → App chuyển màn
AppState UIManager_UpdateMenu(float dt) {
    s_menuHue = fmodf(s_menuHue + 80.0f * dt, 360.0f);

    for (int i = 0; i < MENU_COUNT; i++) {
        // Hover scale: lerp mượt về 1.0 khi hover, về 0.0 khi không
        float target = (i == s_menuSel) ? 1.0f : 0.0f;
        s_menuHoverScale[i] += (target - s_menuHoverScale[i]) * 12.0f * dt;

        // Bounce: đếm ngược, clamp về 0
        if (s_menuBounce[i] > 0.0f) {
            s_menuBounce[i] -= dt;
            if (s_menuBounce[i] < 0.0f) s_menuBounce[i] = 0.0f;
        }
    }

    // Khi deferred delay hết hạn → thực sự chuyển màn hình
    if (s_pendingDelay > 0.0f) {
        s_pendingDelay -= dt;
        if (s_pendingDelay <= 0.0f) {
            AppState result    = s_pendingMenuState;
            s_pendingMenuState = STATE_MENU;
            s_pendingDelay     = 0.0f;
            return result;
        }
    }

    return STATE_MENU;
}

void UIManager_RenderMenu(SDL_Renderer* r) {
    // Title: dùng title_sign.jpg nếu có, fallback sang text vàng
    if (s_titleSign) {
        int tw = 1, th = 1;
        SDL_QueryTexture(s_titleSign, nullptr, nullptr, &tw, &th);
        int signH = 130;
        int signW = tw * signH / th;
        SDL_Rect dst = { WINDOW_WIDTH/2 - signW/2, 20, signW, signH };
        SDL_RenderCopy(r, s_titleSign, nullptr, &dst);
    } else {
        // Vàng nhạt + text-shadow để nổi bật trên sky background
        RT(r, s_fontLg, "CO CARO  -  GOMOKU",
           WINDOW_WIDTH/2 + 2, 107, {80, 50, 10, 180}, true);  // shadow
        RT(r, s_fontLg, "CO CARO  -  GOMOKU",
           WINDOW_WIDTH/2, 105, {255, 242, 160, 255}, true);    // text vàng
    }

    const int CX  = WINDOW_WIDTH / 2;
    const int BW  = 310;   // chiều rộng nút cơ bản
    const int BH  = 46;    // chiều cao nút cơ bản

    // Giãn cách đều: chia đều MENU_COUNT nút vào khoảng y=[215, 620]
    // Mỗi nút được định vị bằng tọa độ TÂM (centerY) để scale không bị lệch
    const int Y_TOP = 215;
    const int Y_BOT = 620;
    const int STEP  = (MENU_COUNT > 1) ? (Y_BOT - Y_TOP) / (MENU_COUNT - 1) : 0;

    for (int i = 0; i < MENU_COUNT; i++) {
        int centerY = Y_TOP + i * STEP;

        // ── Tính scale tổng hợp ─────────────────────────────────
        // Hover: phóng to tối đa 9% khi con trỏ ở trên nút
        float hScale = 1.0f + 0.09f * s_menuHoverScale[i];

        // Bounce: khi click → nút "nhún" xuống (scale giảm) rồi nảy lên
        // Dùng sin(progress * PI): dips ở giữa rồi trở về 1.0 khi kết thúc
        float bScale = 1.0f;
        if (s_menuBounce[i] > 0.0f) {
            float progress = 1.0f - s_menuBounce[i] / 0.20f;  // 0→1 trong 0.20s
            bScale = 1.0f - 0.16f * sinf(progress * 3.14159f);
        }

        float scale = hScale * bScale;

        // ── Kích thước và vị trí sau scale ──────────────────────
        int w  = (int)(BW * scale);
        int h  = (int)(BH * scale);
        int bx = CX - w / 2;
        int by = centerY - h / 2;
        bool sel = (i == s_menuSel);
        DrawCozyPanel(r, bx, by, w, h, sel);
        SDL_Color tc = sel ? COZY_GOLD : COZY_TEXT_DARK;
        RT(r, s_fontMd, MENU_ITEMS[i], CX, by + h/2 - 11, tc, true);
    }

    RT(r, s_fontSm, "Arrow keys / mouse   Enter / click to select",
       CX, WINDOW_HEIGHT - 48, COZY_TEXT_DARK, true);
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
        const int CX=WINDOW_WIDTH/2, BW=310, BH=46;
        const int STEP = (MENU_COUNT > 1) ? (620-215)/(MENU_COUNT-1) : 0;
        for (int i = 0; i < MENU_COUNT; i++) {
            int centerY = 215 + i * STEP;
            SDL_Rect b = { CX-BW/2, centerY-BH/2, BW, BH };
            if (e.motion.x>=b.x && e.motion.x<=b.x+b.w &&
                e.motion.y>=b.y && e.motion.y<=b.y+b.h)
                s_menuSel = i;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        const int CX=WINDOW_WIDTH/2, BW=310, BH=46;
        const int STEP = (MENU_COUNT > 1) ? (620-215)/(MENU_COUNT-1) : 0;
        for (int i = 0; i < MENU_COUNT; i++) {
            int centerY = 215 + i * STEP;
            SDL_Rect b = { CX-BW/2, centerY-BH/2, BW, BH };
            if (e.button.x>=b.x && e.button.x<=b.x+b.w &&
                e.button.y>=b.y && e.button.y<=b.y+b.h)
                { s_menuSel = i; goto CONFIRM; }
        }
    }
    return STATE_MENU;

CONFIRM:
    // Kích hoạt bounce animation, defer transition 0.20s để bounce chạy xong
    {
        AppState target = STATE_MENU;
        switch (s_menuSel) {
        case 0: s_pendingMode = MODE_PVP; target = STATE_NAME_INPUT; break;
        case 1: s_pendingMode = MODE_PVE; target = STATE_NAME_INPUT; break;
        case 2: target = STATE_LOAD_GAME; break;
        case 3: target = STATE_SETTINGS;  break;
        case 4: target = STATE_EXIT;      break;
        }
        if (target != STATE_MENU) {
            s_menuBounce[s_menuSel] = 0.20f;   // bắt đầu bounce
            s_pendingMenuState      = target;
            s_pendingDelay          = 0.20f;   // chờ bounce xong rồi chuyển
        }
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
    RT(r, s_fontLg, "Player Setup", WINDOW_WIDTH/2, 75, COZY_WOOD_DARK, true);

    const int LX = WINDOW_WIDTH/2 - 190;
    const int FW = 380, FH = 38;
    int y = 155;

    // Vẽ field với label — cozy parchment style
    auto Field = [&](const char* label, char* buf, int idx) {
        RT(r, s_fontSm, label, LX, y, COZY_TEXT_DARK);
        y += 24;
        bool act = (s_ni.field == idx);
        DrawCozyPanel(r, LX, y, FW, FH, act);
        char disp[34]; snprintf(disp, sizeof(disp), "%s%s", buf, act ? "_" : "");
        RT(r, s_fontMd, disp, LX+8, y+6, COZY_TEXT_DARK);
        y += FH + 18;
    };

    Field("Player 1 Name:", s_ni.p1, 0);

    if (s_ni.mode == MODE_PVP) {
        Field("Player 2 Name:", s_ni.p2, 1);
    } else {
        RT(r, s_fontSm, "AI Difficulty:", LX, y, {170,170,170,255});
        y += 24;

        // Arrow selector: [<]  MEDIUM  [>]
        // Nhấn mũi tên trái/phải (hoặc click) để chuyển độ khó
        static const char* diffs[] = { "EASY", "MEDIUM", "HARD" };
        static const int   dvals[] = { AI_EASY, AI_MEDIUM, AI_HARD };
        int curIdx = 1;
        for (int i = 0; i < 3; i++) if (s_ni.diff == dvals[i]) { curIdx = i; break; }

        bool act = (s_ni.field == 1);
        SDL_Color arrowC = act ? SDL_Color{255,235,60,255} : SDL_Color{130,130,130,255};

        // Arrow selector dùng icon sprites
        DrawCozyPanel(r, LX,       y, 44,       FH, act);
        DrawIcon(r, ICON_ARROW_L, LX+22,     y+FH/2, 28);

        DrawCozyPanel(r, LX+52,    y, FW-104,  FH, act);
        SDL_Color diffC = curIdx==0 ? COZY_GREEN : curIdx==1 ? COZY_GOLD : COZY_RED;
        RT(r, s_fontMd, diffs[curIdx], LX+52+(FW-104)/2, y+7, diffC, true);

        DrawCozyPanel(r, LX+FW-44, y, 44,       FH, act);
        DrawIcon(r, ICON_ARROW_R, LX+FW-22,  y+FH/2, 28);

        y += FH + 18;
    }

    // Start button — cozy style
    {
        bool sel = (s_ni.field == 2);
        DrawCozyPanel(r, WINDOW_WIDTH/2-95, y, 190, 46, sel);
        RT(r, s_fontMd, "Start Game", WINDOW_WIDTH/2, y+12, sel ? COZY_GOLD : COZY_TEXT_DARK, true);
    }

    RT(r, s_fontSm, "Tab: next field   Left/Right: doi do kho   Enter: start   ESC: back",
       WINDOW_WIDTH/2, WINDOW_HEIGHT-55, COZY_TEXT_DARK, true);
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
        case SDLK_LEFT:
        case SDLK_RIGHT:
            // Khi đang ở field độ khó (PVE, field==1): mũi tên trái/phải đổi mức
            if (s_ni.mode == MODE_PVE && s_ni.field == 1) {
                static const int dvals[] = { AI_EASY, AI_MEDIUM, AI_HARD };
                int cur = 1;
                for (int i = 0; i < 3; i++) if (s_ni.diff == dvals[i]) { cur = i; break; }
                cur = (e.key.keysym.sym == SDLK_LEFT)
                      ? (cur + 2) % 3
                      : (cur + 1) % 3;
                s_ni.diff = dvals[cur];
            }
            break;
        default: break;
        }
    } else if (e.type == SDL_TEXTINPUT) {
        if (s_ni.field == 0 && strlen(s_ni.p1) < 28)
            strncat_s(s_ni.p1, sizeof(s_ni.p1), e.text.text, _TRUNCATE);
        else if (s_ni.field == 1 && s_ni.mode == MODE_PVP && strlen(s_ni.p2) < 28)
            strncat_s(s_ni.p2, sizeof(s_ni.p2), e.text.text, _TRUNCATE);
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        // Arrow selector cho độ khó (PVE)
        if (s_ni.mode == MODE_PVE) {
            static const int dvals[] = { AI_EASY, AI_MEDIUM, AI_HARD };
            const int LX2   = WINDOW_WIDTH/2 - 190;
            const int FW2   = 380, FH2 = 38;
            const int btnY  = 155 + 24 + 38 + 18 + 24;  // y sau Field P1Name + label diff
            int cur = 1;
            for (int i = 0; i < 3; i++) if (s_ni.diff == dvals[i]) { cur = i; break; }
            SDL_Rect leftBtn  = { LX2,          btnY, 44, FH2 };
            SDL_Rect rightBtn = { LX2+FW2-44,   btnY, 44, FH2 };
            if (e.button.x>=leftBtn.x  && e.button.x<=leftBtn.x+leftBtn.w   &&
                e.button.y>=leftBtn.y  && e.button.y<=leftBtn.y+leftBtn.h)
                s_ni.diff = dvals[(cur + 2) % 3];
            if (e.button.x>=rightBtn.x && e.button.x<=rightBtn.x+rightBtn.w &&
                e.button.y>=rightBtn.y && e.button.y<=rightBtn.y+rightBtn.h)
                s_ni.diff = dvals[(cur + 1) % 3];
            s_ni.field = 1;  // focus về difficulty khi click vào selector
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

    bool isTurn = (state.gameStatus == CHUA_KET_THUC) && (state.turn == (pidx == 0));

    // Cozy parchment panel (highlighted khi đến lượt)
    DrawCozyPanel(r, px, PY, pw, PH, isTurn);

    // Avatar area
    int avSide = pw - 24;
    if (avSide > 150) avSide = 150;
    int avX = px + (pw - avSide) / 2;
    int avY = PY + 14;

    // Vẽ character portrait nếu có sprite, fallback sang placeholder
    SDL_Texture** charLayers = (pidx == 0) ? s_charP1 : s_charP2;
    if (charLayers[0]) {
        // Khung avatar cozy
        SDL_SetRenderDrawColor(r, COZY_WOOD_DARK.r, COZY_WOOD_DARK.g, COZY_WOOD_DARK.b, 255);
        SDL_Rect av = {avX, avY, avSide, avSide};
        SDL_RenderDrawRect(r, &av);
        // Character portrait (composite layers)
        DrawCharPortrait(r, charLayers, avX+2, avY+2, avSide-4);
    } else {
        // Fallback placeholder
        SDL_Color avBdr = (pidx==0) ? SDL_Color{178,50,24,255} : SDL_Color{50,90,178,255};
        SDL_SetRenderDrawColor(r, COZY_PARCHMENT2.r, COZY_PARCHMENT2.g, COZY_PARCHMENT2.b, 255);
        SDL_Rect av = {avX, avY, avSide, avSide};
        SDL_RenderFillRect(r, &av);
        SDL_SetRenderDrawColor(r, avBdr.r, avBdr.g, avBdr.b, 255);
        SDL_RenderDrawRect(r, &av);
        RT(r, s_fontLg, pidx==0 ? "X" : "O", avX+avSide/2, avY+avSide/2-17, avBdr, true);
    }

    int y = avY + avSide + 12;

    // Tên người chơi
    SDL_Color nameC = (pidx==0) ? COZY_RED : SDL_Color{50,90,178,255};
    RT(r, s_fontMd, state.players[pidx].name, px + pw/2, y, nameC, true);
    y += 28;

    // Thống kê
    char buf[48];
    // Trophy icon + số thắng
    DrawIcon(r, ICON_TROPHY, px+18, y+10, 20);
    snprintf(buf, sizeof(buf), "%d W | %d L", state.players[pidx].wins, state.players[pidx].losses);
    RT(r, s_fontSm, buf, px+32, y, COZY_TEXT_DARK);
    y += 22;

    snprintf(buf, sizeof(buf), "Moves: %d", state.players[pidx].moves);
    RT(r, s_fontSm, buf, px + pw/2, y, COZY_TEXT_DARK, true);
    y += 28;

    // Trạng thái lượt
    if (isTurn) {
        RT(r, s_fontSm, "YOUR TURN", px + pw/2, y, COZY_GOLD, true);
    } else if (state.mode == MODE_PVE && pidx == 1 && state.aiThinking) {
        RT(r, s_fontSm, "Thinking...", px + pw/2, y, COZY_WOOD_MID, true);
    }

    // Kết quả
    if (s_showResult && s_resultMsg[0]) {
        int ry = PY + PH - 80;
        RT(r, s_fontSm, s_resultMsg, px + pw/2, ry, COZY_RED, true);
        RT(r, s_fontSm, "R:replay  ESC:menu", px + pw/2, ry + 22, COZY_GOLD, true);
    }

    // Gợi ý phím P1
    if (pidx == 0) {
        int hy = PY + PH - (s_showResult ? 140 : 100);
        const char* hints[] = { "WASD/Mouse:move", "Enter/Click:place", "L:save  T:load", "ESC:pause" };
        for (auto h : hints) { RT(r, s_fontSm, h, px+6, hy, COZY_TEXT_DARK); hy += 20; }
    }
}

void UIManager_RenderHUD(SDL_Renderer* r, const _GAMESTATE& state) {
    RenderPanel(r, state, 0, LEFT_PANEL_X,  LEFT_PANEL_WIDTH);
    RenderPanel(r, state, 1, HUD_PANEL_X, HUD_PANEL_WIDTH);
}

// ── Load screen ───────────────────────────────────────────────────
static int s_loadAction = 0;   // 0=none, 1=load, 2=delete (btn hover)

void UIManager_ShowLoadScreen() {
    s_saves   = GetSaveFiles();
    s_loadSel = 0;
    s_loadAction = 0;
}

// Lấy tên file ngắn (bỏ đường dẫn) để hiển thị
static std::string BaseName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

void UIManager_RenderLoadScreen(SDL_Renderer* r) {
    // Background vẽ bởi Renderer_DrawBackground

    // Tiêu đề
    RT(r, s_fontLg, "TAI GAME", WINDOW_WIDTH/2, 38, COZY_WOOD_DARK, true);

    // Panel danh sách save
    const int PX = 80, PY = 115, PW = 740, PH = 480;
    SDL_SetRenderDrawColor(r, 16, 16, 42, 255);
    SDL_Rect panel = {PX, PY, PW, PH};
    SDL_RenderFillRect(r, &panel);
    SDL_SetRenderDrawColor(r, 80, 62, 22, 255);
    SDL_RenderDrawRect(r, &panel);

    if (s_saves.empty()) {
        RT(r, s_fontMd, "Khong tim thay file luu nao.",
           WINDOW_WIDTH/2, PY + PH/2 - 12, {170,90,90,255}, true);
    } else {
        const int ROW_H = 52, ROW_PX = PX+14;
        for (int i = 0; i < static_cast<int>(s_saves.size()); i++) {
            bool sel = (i == s_loadSel);
            int ry = PY + 14 + i * ROW_H;

            if (sel) {
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, 62, 62, 148, 195);
                SDL_Rect bar = {ROW_PX, ry, PW-28, ROW_H-4};
                SDL_RenderFillRect(r, &bar);
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
                SDL_SetRenderDrawColor(r, 138, 105, 28, 255);
                SDL_RenderDrawRect(r, &bar);
            }

            // Icon ▶ cho file được chọn
            SDL_Color ic = sel ? SDL_Color{255,228,50,255} : SDL_Color{188,188,188,255};
            RT(r, s_fontSm, sel ? "> " : "  ", ROW_PX+6, ry+16, ic);
            RT(r, s_fontMd, BaseName(s_saves[i]).c_str(), ROW_PX+28, ry+14, ic);
        }
    }

    // Cột nút bên phải
    const int BX = PX + PW + 18, BY0 = PY, BW2 = 182, BH2 = 52;
    // Action buttons — cozy style với icon
    const char* btnLabels[] = { "TAI GAME", "XOA", "QUAY LAI" };
    IconID btnIcons[] = { ICON_BOOK, ICON_TRASH, ICON_BACK };
    for (int i = 0; i < 3; i++) {
        int by = BY0 + i * (BH2 + 12);
        bool hover = (s_loadAction == i+1);
        DrawCozyPanel(r, BX, by, BW2, BH2, hover);
        DrawIcon(r, btnIcons[i], BX+24, by+BH2/2, 26);
        RT(r, s_fontMd, btnLabels[i], BX+BW2/2+8, by+14, hover ? COZY_GOLD : COZY_TEXT_DARK, true);
    }

    RT(r, s_fontSm, "Up/Down: chon   Enter: tai   Del: xoa   ESC: quay lai",
       WINDOW_WIDTH/2, WINDOW_HEIGHT-32, COZY_TEXT_DARK, true);
}

AppState UIManager_HandleLoadEvent(const SDL_Event& e, _GAMESTATE& state) {
    int n = static_cast<int>(s_saves.size());

    // Hit-test nút bên phải
    const int BX = 80+740+18, BY0 = 115, BW2 = 182, BH2 = 52;
    auto HitBtn = [&](int i) {
        int by = BY0 + i*(BH2+12);
        return e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT
            && e.button.x>=BX && e.button.x<=BX+BW2
            && e.button.y>=by && e.button.y<=by+BH2;
    };
    auto HoverBtn = [&](int i) {
        int by = BY0 + i*(BH2+12);
        return e.type == SDL_MOUSEMOTION
            && e.motion.x>=BX && e.motion.x<=BX+BW2
            && e.motion.y>=by && e.motion.y<=by+BH2;
    };

    if (e.type == SDL_MOUSEMOTION) {
        s_loadAction = 0;
        for (int i=0;i<3;i++) if(HoverBtn(i)) s_loadAction=i+1;
    }

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: return STATE_MENU;
        case SDLK_UP:   if (n>0) s_loadSel=(s_loadSel+n-1)%n; break;
        case SDLK_DOWN: if (n>0) s_loadSel=(s_loadSel+1)%n;   break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            if (n>0 && s_loadSel<n && LoadGame(state, s_saves[s_loadSel])) {
                s_showResult = false; return STATE_PLAYING;
            }
            break;
        case SDLK_DELETE:
            if (n>0 && s_loadSel<n) {
                std::remove(s_saves[s_loadSel].c_str());
                s_saves = GetSaveFiles();
                if (s_loadSel >= static_cast<int>(s_saves.size()))
                    s_loadSel = std::max(0, (int)s_saves.size()-1);
            }
            break;
        default: break;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        // Click vào dòng save
        const int ROW_H=52, ROW_PX=80+14, PY=115;
        for (int i=0;i<n;i++) {
            SDL_Rect row={ROW_PX, PY+14+i*ROW_H, 740-28, ROW_H-4};
            if (e.button.x>=row.x && e.button.x<=row.x+row.w &&
                e.button.y>=row.y && e.button.y<=row.y+row.h)
                s_loadSel=i;
        }
        // Click nút TAI GAME
        if (HitBtn(0) && n>0 && LoadGame(state, s_saves[s_loadSel])) {
            s_showResult=false; return STATE_PLAYING;
        }
        // Click nút XÓA
        if (HitBtn(1) && n>0) {
            std::remove(s_saves[s_loadSel].c_str());
            s_saves=GetSaveFiles();
            if (s_loadSel>=(int)s_saves.size()) s_loadSel=std::max(0,(int)s_saves.size()-1);
        }
        // Click nút QUAY LẠI
        if (HitBtn(2)) return STATE_MENU;
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

        SDL_Rect box = { WINDOW_WIDTH/2-230, WINDOW_HEIGHT/2-105, 460, 210 };
        SDL_SetRenderDrawColor(s_renderer, 14, 14, 38, 255);
        SDL_RenderFillRect(s_renderer, &box);
        SDL_SetRenderDrawColor(s_renderer, 118, 92, 28, 255);
        SDL_RenderDrawRect(s_renderer, &box);
        // Viền trong mỏng hơn
        SDL_Rect inner = {box.x+3, box.y+3, box.w-6, box.h-6};
        SDL_SetRenderDrawColor(s_renderer, 58, 45, 14, 255);
        SDL_RenderDrawRect(s_renderer, &inner);

        RT(s_renderer, s_fontMd, "LUU GAME",
           WINDOW_WIDTH/2, box.y+18, {218,192,78,255}, true);

        // Đường kẻ dưới tiêu đề
        SDL_SetRenderDrawColor(s_renderer, 75, 58, 18, 255);
        SDL_RenderDrawLine(s_renderer, box.x+14, box.y+52, box.x+box.w-14, box.y+52);

        RT(s_renderer, s_fontSm, "Nhap ten file luu:",
           box.x+20, box.y+62, {148,148,148,255});

        SDL_Rect inp = {box.x+18, box.y+84, box.w-36, 40};
        SDL_SetRenderDrawColor(s_renderer, 38, 38, 88, 255);
        SDL_RenderFillRect(s_renderer, &inp);
        SDL_SetRenderDrawColor(s_renderer, 118, 118, 218, 255);
        SDL_RenderDrawRect(s_renderer, &inp);

        char disp[32]; snprintf(disp, sizeof(disp), "%s_", name);
        RT(s_renderer, s_fontMd, disp, inp.x+10, inp.y+7, {255,228,62,255});

        RT(s_renderer, s_fontSm, "Enter: luu   ESC: huy",
           WINDOW_WIDTH/2, box.y+148, {88,88,88,255}, true);

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

// ================================================================
//  SPLASH SCREEN
//  Màn hình chờ hiển thị khi game khởi động.
//  Kỹ thuật: alpha fade — nhân màu với hệ số alpha tăng/giảm theo thời gian.
//  Phase 1 (0–0.6s):  fade in  (alpha 0→255)
//  Phase 2 (0.6–2.0s): hold    (alpha 255)
//  Phase 3 (2.0–2.6s): fade out (alpha 255→0)
//  Sau 2.6s: tự động chuyển STATE_MENU
// ================================================================
static float s_splashTimer = 0.0f;
static const float SPLASH_FADEIN  = 0.6f;
static const float SPLASH_HOLD    = 2.0f;
static const float SPLASH_FADEOUT = 2.6f;

void UIManager_ShowSplash() {
    s_splashTimer = 0.0f;
}

AppState UIManager_UpdateSplash(float dt) {
    s_splashTimer += dt;
    if (s_splashTimer >= SPLASH_FADEOUT) return STATE_MENU;
    return STATE_SPLASH;
}

void UIManager_RenderSplash(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 5, 5, 18, 255);
    SDL_RenderClear(r);

    // Tính alpha theo phase
    Uint8 alpha;
    if (s_splashTimer < SPLASH_FADEIN) {
        alpha = static_cast<Uint8>(255.0f * s_splashTimer / SPLASH_FADEIN);
    } else if (s_splashTimer < SPLASH_HOLD) {
        alpha = 255;
    } else {
        float prog = (s_splashTimer - SPLASH_HOLD) / (SPLASH_FADEOUT - SPLASH_HOLD);
        alpha = static_cast<Uint8>(255.0f * (1.0f - prog));
    }

    // Tiêu đề chính
    SDL_Color titleC = { 218, 192, 78, alpha };
    RT(r, s_fontLg, "CO CARO", WINDOW_WIDTH/2, WINDOW_HEIGHT/2 - 55, titleC, true);

    // Subtitle
    SDL_Color subC = { 155, 155, 155, alpha };
    RT(r, s_fontMd, "GOMOKU", WINDOW_WIDTH/2, WINDOW_HEIGHT/2 + 5, subC, true);

    // Dòng gợi ý nhỏ
    if (s_splashTimer > SPLASH_FADEIN) {
        SDL_Color hintC = { 70, 70, 70, alpha };
        RT(r, s_fontSm, "Nhan phim bat ky de bo qua...", WINDOW_WIDTH/2, WINDOW_HEIGHT - 55, hintC, true);
    }
}

// ================================================================
//  HEADER BAR
//  Vẽ trong khoảng trống 60px phía trên bàn cờ (y=0 đến y=58).
//  Hiển thị: chế độ chơi + số ván | lượt hiện tại | hint ESC
// ================================================================
void UIManager_RenderGameHeader(SDL_Renderer* r, const _GAMESTATE& state) {
    // Header bar — cozy parchment bán trong suốt
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, COZY_PARCHMENT.r, COZY_PARCHMENT.g, COZY_PARCHMENT.b, 200);
    SDL_Rect bar = { 0, 0, WINDOW_WIDTH, BOARD_OFFSET_Y - 2 };
    SDL_RenderFillRect(r, &bar);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(r, COZY_WOOD_DARK.r, COZY_WOOD_DARK.g, COZY_WOOD_DARK.b, 255);
    SDL_RenderDrawLine(r, 0, BOARD_OFFSET_Y - 2, WINDOW_WIDTH, BOARD_OFFSET_Y - 2);

    // Tên chế độ + số ván (wins+losses+draws+1 = ván hiện tại)
    int session = state.players[0].wins + state.players[0].losses + state.players[0].draws + 1;
    char header[64];
    const char* modeName = (state.mode == MODE_PVE) ? "AI BATTLE" : "VS PLAYER";
    snprintf(header, sizeof(header), "%s  -  VAN %d", modeName, session);
    RT(r, s_fontMd, header, WINDOW_WIDTH / 2, 7, COZY_WOOD_DARK, true);

    char turnBuf[32];
    snprintf(turnBuf, sizeof(turnBuf), "Luot: %d", state.totalMoves + 1);
    RT(r, s_fontSm, turnBuf, WINDOW_WIDTH / 2, 34, COZY_WOOD_MID, true);

    DrawIcon(r, ICON_PAUSE, WINDOW_WIDTH - 30, BOARD_OFFSET_Y/2, 26);
    RT(r, s_fontSm, "[ESC]", WINDOW_WIDTH - 65, 22, COZY_WOOD_MID);
}

// ================================================================
//  PAUSE OVERLAY
//  Khi người chơi nhấn ESC trong lúc chơi, một lớp overlay tối
//  phủ lên toàn màn hình, và một panel dọc nhỏ xuất hiện ở giữa.
//  Painter's Algorithm: overlay là layer trên cùng — vẽ sau cùng
//  trong App_Render để che tất cả các layer phía dưới.
// ================================================================
static bool s_showPause = false;
static int  s_pauseSel  = 0;

static const char* PAUSE_ITEMS[] = { "Tiep tuc", "Choi lai", "Luu game", "Thoat" };
static const int   PAUSE_COUNT   = 4;

void UIManager_ShowPause() { s_showPause = true;  s_pauseSel = 0; }
void UIManager_HidePause() { s_showPause = false; }
bool UIManager_IsPauseShown() { return s_showPause; }

PauseAction UIManager_HandlePauseEvent(const SDL_Event& e, _GAMESTATE& state) {
    if (!s_showPause) return PAUSE_NONE;

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_UP:
            s_pauseSel = (s_pauseSel + PAUSE_COUNT - 1) % PAUSE_COUNT;
            return PAUSE_NONE;
        case SDLK_DOWN:
            s_pauseSel = (s_pauseSel + 1) % PAUSE_COUNT;
            return PAUSE_NONE;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            goto CONFIRM_PAUSE;
        default: break;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        const int PW = 240;
        const int PH = PAUSE_COUNT * 58 + 58;
        const int PX = WINDOW_WIDTH / 2 - PW / 2;
        const int PY = WINDOW_HEIGHT / 2 - PH / 2;
        for (int i = 0; i < PAUSE_COUNT; i++) {
            SDL_Rect btn = { PX + 14, PY + 50 + i * 56, PW - 28, 46 };
            if (e.button.x >= btn.x && e.button.x <= btn.x + btn.w &&
                e.button.y >= btn.y && e.button.y <= btn.y + btn.h) {
                s_pauseSel = i;
                goto CONFIRM_PAUSE;
            }
        }
    }
    return PAUSE_NONE;

CONFIRM_PAUSE:
    switch (s_pauseSel) {
    case 0: return PAUSE_RESUME;
    case 1: return PAUSE_RESTART;
    case 2: UIManager_ShowSaveDialog(state); return PAUSE_NONE;
    case 3: return PAUSE_QUIT;
    }
    return PAUSE_NONE;
}

void UIManager_RenderPauseOverlay(SDL_Renderer* r) {
    if (!s_showPause) return;

    // Lớp tối phủ toàn màn hình — Alpha Blending tạo hiệu ứng "đóng băng" game
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 168);
    SDL_Rect ov = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(r, &ov);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // Panel trung tâm
    const int PW = 240;
    const int PH = PAUSE_COUNT * 58 + 58;
    const int PX = WINDOW_WIDTH / 2 - PW / 2;
    const int PY = WINDOW_HEIGHT / 2 - PH / 2;

    DrawCozyPanel(r, PX, PY, PW, PH, false);
    RT(r, s_fontMd, "DUNG GAME", WINDOW_WIDTH / 2, PY + 11, COZY_WOOD_DARK, true);
    SDL_SetRenderDrawColor(r, COZY_WOOD_MID.r, COZY_WOOD_MID.g, COZY_WOOD_MID.b, 255);
    SDL_RenderDrawLine(r, PX + 12, PY + 42, PX + PW - 12, PY + 42);

    for (int i = 0; i < PAUSE_COUNT; i++) {
        bool sel = (i == s_pauseSel);
        int BY = PY + 50 + i * 56;
        DrawCozyPanel(r, PX + 14, BY, PW - 28, 46, sel);
        SDL_Color tc = sel ? COZY_GOLD : COZY_TEXT_DARK;
        RT(r, s_fontMd, PAUSE_ITEMS[i], WINDOW_WIDTH / 2, BY + 11, tc, true);
    }
}

// ================================================================
//  SETTINGS SCREEN
//  Màn hình cài đặt với 4 tab: Âm thanh | Phím chơi | Luật chơi | Thông tin
//  Dùng kỹ thuật "tab state": biến s_settingsTab quyết định nội dung
//  nào được vẽ trong panel — giống như switch/case trong render loop.
// ================================================================
static int s_settingsTab = 0;
static const char* SETTINGS_TABS[] = { "AM THANH", "PHIM CHOI", "LUAT CHOI", "THONG TIN" };
static const int   SETTINGS_TAB_COUNT = 4;

void UIManager_ShowSettings() {
    s_settingsTab = 0;
}

AppState UIManager_HandleSettingsEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE)  return STATE_MENU;
        if (e.key.keysym.sym == SDLK_LEFT)
            s_settingsTab = (s_settingsTab + SETTINGS_TAB_COUNT - 1) % SETTINGS_TAB_COUNT;
        if (e.key.keysym.sym == SDLK_RIGHT)
            s_settingsTab = (s_settingsTab + 1) % SETTINGS_TAB_COUNT;
    } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        // Click tab
        const int TW = 170;
        const int TX0 = WINDOW_WIDTH / 2 - (SETTINGS_TAB_COUNT * TW) / 2;
        for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
            SDL_Rect tab = { TX0 + i * TW, 60, TW - 4, 40 };
            if (e.button.x >= tab.x && e.button.x <= tab.x + tab.w &&
                e.button.y >= tab.y && e.button.y <= tab.y + tab.h)
                s_settingsTab = i;
        }
        // Click nút X (đóng)
        SDL_Rect xBtn = { WINDOW_WIDTH - 52, 14, 34, 34 };
        if (e.button.x >= xBtn.x && e.button.x <= xBtn.x + xBtn.w &&
            e.button.y >= xBtn.y && e.button.y <= xBtn.y + xBtn.h)
            return STATE_MENU;
    }
    return STATE_SETTINGS;
}

void UIManager_RenderSettings(SDL_Renderer* r) {
    RT(r, s_fontLg, "CAI DAT", WINDOW_WIDTH / 2, 12, COZY_WOOD_DARK, true);

    // Nút X dùng icon sprite
    {
        SDL_Rect xBtn = { WINDOW_WIDTH - 52, 14, 34, 34 };
        DrawCozyPanel(r, xBtn.x, xBtn.y, xBtn.w, xBtn.h, false);
        DrawIcon(r, ICON_X, xBtn.x + 17, xBtn.y + 17, 28);
    }

    // Tab bar
    const int TW = 170;
    const int TX0 = WINDOW_WIDTH / 2 - (SETTINGS_TAB_COUNT * TW) / 2;
    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
        bool sel = (i == s_settingsTab);
        DrawCozyPanel(r, TX0 + i * TW, 60, TW - 4, 40, sel);
        SDL_Color tc = sel ? COZY_GOLD : COZY_TEXT_DARK;
        RT(r, s_fontSm, SETTINGS_TABS[i], TX0 + i * TW + TW / 2 - 2, 72, tc, true);
    }

    // Panel nội dung
    const int CX = WINDOW_WIDTH / 2;
    const int CW = 720, CH = 545, CY = 112;
    DrawCozyPanel(r, CX - CW/2, CY, CW, CH, false);

    const int IX = CX - CW / 2 + 32;
    int iy = CY + 28;

    switch (s_settingsTab) {

    // ── Âm thanh ────────────────────────────────────────────────
    case 0: {
        const int SW = CW - 64;
        auto Slider = [&](const char* label, float pct) {
            RT(r, s_fontMd, label, IX, iy, { 195, 195, 195, 255 });
            iy += 32;
            // Track
            SDL_SetRenderDrawColor(r, 48, 48, 88, 255);
            SDL_Rect track = { IX, iy, SW, 14 };
            SDL_RenderFillRect(r, &track);
            // Fill
            SDL_SetRenderDrawColor(r, 88, 158, 88, 255);
            SDL_Rect fill = { IX, iy, (int)(SW * pct), 14 };
            SDL_RenderFillRect(r, &fill);
            // Thumb
            SDL_SetRenderDrawColor(r, 148, 218, 148, 255);
            SDL_Rect thumb = { IX + (int)(SW * pct) - 7, iy - 5, 14, 24 };
            SDL_RenderFillRect(r, &thumb);
            iy += 52;
        };
        Slider("Nhac nen (BGM)", 0.8f);
        Slider("Hieu ung am thanh (SFX)", 0.65f);
        iy += 8;
        RT(r, s_fontSm, "(Tinh nang dieu chinh am luong se cap nhat sau)", IX, iy, { 75, 75, 75, 255 });
        break;
    }

    // ── Phím chơi ───────────────────────────────────────────────
    case 1: {
        struct KM { const char* key; const char* desc; };
        KM maps[] = {
            { "W / A / S / D",     "Di chuyen con tro tren ban co" },
            { "Mouse",             "Di chuyen, hover o co" },
            { "Enter / Click",     "Dat quan co" },
            { "ESC",               "Mo/tat menu dung game" },
            { "L",                 "Luu game hien tai" },
            { "T",                 "Vao man hinh tai game" },
            { "R  (sau ket thuc)", "Bat dau van moi (giu diem)" },
        };
        for (auto& km : maps) {
            SDL_SetRenderDrawColor(r, 38, 38, 72, 255);
            SDL_Rect row = { IX - 10, iy - 5, CW - 44, 36 };
            SDL_RenderFillRect(r, &row);
            RT(r, s_fontSm, km.key,  IX + 5,   iy + 4, { 252, 218, 78, 255 });
            RT(r, s_fontSm, km.desc, IX + 245,  iy + 4, { 185, 185, 185, 255 });
            iy += 44;
        }
        break;
    }

    // ── Luật chơi ───────────────────────────────────────────────
    case 2: {
        const char* rules[] = {
            "LUAT CO CARO TIEU CHUAN (GOMOKU):",
            "",
            "1. Hai nguoi choi lan luot dat quan X va O.",
            "   Nguoi choi 1 (X) di truoc, nguoi choi 2 (O) di sau.",
            "",
            "2. Nguoi dau tien co 5 quan lien tiep thang hang",
            "   tren mot hang, cot, hoac duong cheo la THANG.",
            "",
            "3. Neu ban co day ma khong ai co 5 lien tiep -> HOA.",
            "",
            "4. Che do AI (PvE):",
            "   EASY   = AI nhin truoc 2 nuoc (< 10ms)",
            "   MEDIUM  = AI nhin truoc 4 nuoc (< 200ms)",
            "   HARD    = AI nhin truoc 6 nuoc (< 1500ms)",
        };
        for (auto& rule : rules) {
            if (rule[0] == '\0') { iy += 8; continue; }
            bool isTitle = (rule[0] == 'L' && rule[1] == 'U');
            SDL_Color rc = isTitle ? SDL_Color{ 252, 208, 58, 255 } : SDL_Color{ 182, 182, 182, 255 };
            RT(r, s_fontSm, rule, IX, iy, rc);
            iy += 26;
        }
        break;
    }

    // ── Thông tin ────────────────────────────────────────────────
    case 3: {
        RT(r, s_fontMd, "CO CARO  -  GOMOKU SDL2", CX, iy, { 218, 192, 78, 255 }, true);
        iy += 42;
        const char* lines[] = {
            "Ngon ngu:   C++ (C++17)",
            "Thu vien:   SDL2, SDL2_ttf, SDL2_mixer, SDL2_image",
            "",
            "He thong AI:",
            "  Thuat toan:  Minimax + Alpha-Beta Pruning",
            "  Toi uu hoa:  Move Ordering, Selective Sort, Time Limit",
            "  Do sau:      EASY=2 | MEDIUM=4 | HARD=6",
            "",
            "He thong do hoa:",
            "  Renderer:    SDL2 procedural (primitives + texture cache)",
            "  Painter Alg: 5 layers (BG > Board > Pieces > Hover > UI)",
            "  Alpha Blend: SDL_BLENDMODE_BLEND cho transparent board",
            "",
            "Phim tat:   ESC=Dung | L=Luu | T=Tai | R=Van moi",
        };
        for (auto& line : lines) {
            if (line[0] == '\0') { iy += 10; continue; }
            bool isSection = (line[0] == 'H' || line[0] == 'N' || line[0] == 'P');
            SDL_Color lc = isSection ? SDL_Color{ 162, 208, 162, 255 } : SDL_Color{ 168, 168, 168, 255 };
            RT(r, s_fontSm, line, IX, iy, lc);
            iy += 26;
        }
        break;
    }
    }

    RT(r, s_fontSm, "ESC hoac nut X de quay lai menu",
       CX, WINDOW_HEIGHT - 30, COZY_TEXT_DARK, true);
}
