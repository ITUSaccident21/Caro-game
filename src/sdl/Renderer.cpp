#include "Renderer.h"
#include <cmath>
#include <cstdlib>
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

// Piece textures (DR-001)
static SDL_Texture* s_texX = nullptr;
static SDL_Texture* s_texO = nullptr;
static int          s_pieceSize = 0;

// Background textures: menu/ui = Yellow, game = Blue
static SDL_Texture* s_bgMenu = nullptr;
static SDL_Texture* s_bgGame = nullptr;

// Board skin textures (random per ván)
#define N_BOARDS 4
static SDL_Texture* s_boardTex[N_BOARDS] = {};
static int          s_boardCount   = 0;
static int          s_currentBoard = 0;

// Decoration sprites (Cozy Lands)
static SDL_Texture* s_decoTree    = nullptr;
static SDL_Texture* s_decoBush    = nullptr;
static SDL_Texture* s_decoFlower  = nullptr;

bool Renderer_Init(SDL_Renderer* renderer) {
    s_renderer = renderer;
    if (!renderer) return false;

    // Background textures (fallback = nullptr → procedural)
    // bg_menu.jpg: pixel art sky với mây — dùng cho menu/ui/load screens
    // bg_game.png: dùng cho game screen (hoặc fallback nếu không có)
    s_bgMenu = IMG_LoadTexture(renderer, "assets/sprites/bg_menu.jpg");
    s_bgGame = IMG_LoadTexture(renderer, "assets/sprites/bg_game.png");
    // Nếu bg_game chưa có, dùng chung bg_menu
    if (!s_bgGame) s_bgGame = s_bgMenu;

    // Board skins — load tuần tự, dừng khi không tìm thấy file
    const char* boardPaths[N_BOARDS] = {
        "assets/sprites/board_01.jpg",
        "assets/sprites/board_02.jpg",
        "assets/sprites/board_03.jpg",
        "assets/sprites/board_04.jpg",
    };
    s_boardCount = 0;
    for (int i = 0; i < N_BOARDS; i++) {
        s_boardTex[i] = IMG_LoadTexture(renderer, boardPaths[i]);
        if (s_boardTex[i]) s_boardCount++;
        else               break;
    }

    // Decoration sprites
    s_decoTree   = IMG_LoadTexture(renderer, "assets/sprites/deco_tree.png");
    s_decoBush   = IMG_LoadTexture(renderer, "assets/sprites/deco_bush.png");
    s_decoFlower = IMG_LoadTexture(renderer, "assets/sprites/deco_flower_y.png");

    return true;
}

void Renderer_Shutdown() {
    auto Destroy = [](SDL_Texture*& t){ if(t){ SDL_DestroyTexture(t); t=nullptr; } };
    Destroy(s_bgMenu); Destroy(s_bgGame);
    for (int i = 0; i < N_BOARDS; i++) Destroy(s_boardTex[i]);
    Destroy(s_decoTree); Destroy(s_decoBush); Destroy(s_decoFlower);
    s_renderer = nullptr;
}

void Renderer_RandomizeBoard() {
    if (s_boardCount > 1)
        s_currentBoard = rand() % s_boardCount;
    else
        s_currentBoard = 0;
}


// ── Background ───────────────────────────────────────────────────
// Painter's Algorithm Layer 0: phủ toàn màn hình trước mọi thứ khác
// isGameScreen=true → bg_game (blue), false → bg_menu (yellow)
// Nếu texture chưa load → fallback procedural màu đơn
void Renderer_DrawBackground(SDL_Renderer* r, bool isGameScreen) {
    SDL_Texture* bg = isGameScreen ? s_bgGame : s_bgMenu;

    if (bg) {
        // Scale texture PNG cho vừa cửa sổ
        SDL_Rect dst = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderCopy(r, bg, nullptr, &dst);

        // Scatter decoration sprites ở các góc
        if (s_decoTree) {
            SDL_Rect t1 = { -8,  WINDOW_HEIGHT-96, 88, 88 };
            SDL_Rect t2 = { WINDOW_WIDTH-72, WINDOW_HEIGHT-88, 80, 80 };
            SDL_RenderCopy(r, s_decoTree, nullptr, &t1);
            SDL_RenderCopy(r, s_decoTree, nullptr, &t2);
        }
        if (s_decoBush) {
            SDL_Rect b1 = { 12,  WINDOW_HEIGHT-58, 60, 58 };
            SDL_Rect b2 = { WINDOW_WIDTH-68, WINDOW_HEIGHT-56, 60, 56 };
            SDL_RenderCopy(r, s_decoBush, nullptr, &b1);
            SDL_RenderCopy(r, s_decoBush, nullptr, &b2);
        }
        if (s_decoFlower && !isGameScreen) {
            // Hoa chỉ hiển thị ở màn menu/ui, không hiển thị trong game
            SDL_Rect f1 = { 80, WINDOW_HEIGHT-42, 36, 36 };
            SDL_RenderCopy(r, s_decoFlower, nullptr, &f1);
        }
    } else {
        // Fallback: procedural gradient đơn giản
        SDL_SetRenderDrawColor(r, isGameScreen ? 42 : 212,
                                  isGameScreen ? 72 : 195,
                                  isGameScreen ? 42 : 158, 255);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(r, &full);
    }
}

// ── Board ────────────────────────────────────────────────────────
// Nếu có board texture → blit 740×740 tại offset (-40,-40) từ board area
//   → frame của ảnh hiện ra ngoài, panel HUD che phần nhô
// Fallback → checkerboard procedural bán trong suốt (cũ)
void Renderer_DrawBoard(SDL_Renderer* r, const _GAMESTATE& /*state*/) {
    if (s_boardCount > 0 && s_boardTex[s_currentBoard]) {
        // board.jpg là 1024×1024, frame ~113px mỗi cạnh (~11% ảnh gốc)
        // Điều kiện để grid nằm sau frame: offset > 113×(660+2×offset)/1024 → offset > 93.5
        // Dùng 100px làm margin an toàn: frame scaled = 113×(860/1024) ≈ 95px < 100
        static const int BOARD_IMG_OFFSET = 100;
        static const int BOARD_IMG_SIZE   = BOARD_PIXEL_SIZE + 2 * BOARD_IMG_OFFSET; // 860
        SDL_Rect dst = { BOARD_OFFSET_X - BOARD_IMG_OFFSET,
                         BOARD_OFFSET_Y - BOARD_IMG_OFFSET,
                         BOARD_IMG_SIZE, BOARD_IMG_SIZE };
        SDL_RenderCopy(r, s_boardTex[s_currentBoard], nullptr, &dst);
    } else {
        // Fallback: viền + checkerboard procedural
        SDL_SetRenderDrawColor(r, 90, 60, 25, 255);
        SDL_Rect border = { BOARD_OFFSET_X-6, BOARD_OFFSET_Y-6,
                            BOARD_PIXEL_SIZE+12, BOARD_PIXEL_SIZE+12 };
        SDL_RenderFillRect(r, &border);

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (int row = 0; row < BOARD_SIZE; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                bool light = (row + col) % 2 == 0;
                SDL_SetRenderDrawColor(r,
                    light ? 215 : 185, light ? 185 : 155, light ? 138 : 108,
                    light ? 160 : 140);
                SDL_Rect cell = { BOARD_OFFSET_X + col*CELL_SIZE,
                                  BOARD_OFFSET_Y + row*CELL_SIZE, CELL_SIZE, CELL_SIZE };
                SDL_RenderFillRect(r, &cell);
            }
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    // Grid lines + star points luôn vẽ lên trên (dù texture hay procedural)
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 55, 32, 10, 140);
    for (int i = 0; i <= BOARD_SIZE; i++) {
        int x = BOARD_OFFSET_X + i * CELL_SIZE;
        SDL_RenderDrawLine(r, x, BOARD_OFFSET_Y, x, BOARD_OFFSET_Y + BOARD_PIXEL_SIZE);
        int y = BOARD_OFFSET_Y + i * CELL_SIZE;
        SDL_RenderDrawLine(r, BOARD_OFFSET_X, y, BOARD_OFFSET_X + BOARD_PIXEL_SIZE, y);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    static const int STARS[][2] = { {3,3},{3,11},{11,3},{11,11},{7,7} };
    SDL_SetRenderDrawColor(r, 52, 28, 8, 255);
    for (auto& s : STARS) {
        DrawFilledCircle(r, BOARD_OFFSET_X + s[1]*CELL_SIZE, BOARD_OFFSET_Y + s[0]*CELL_SIZE, 4);
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

            SDL_Texture* tex = (c == -1) ? s_texX : s_texO;
            if (tex) {
                SDL_Rect dst = { cx - s_pieceSize/2, cy - s_pieceSize/2,
                                 s_pieceSize, s_pieceSize };
                SDL_RenderCopy(r, tex, NULL, &dst);
            }
        }
    }
}


// ── Hover ────────────────────────────────────────────────────────
// A: Ghost piece — dùng SDL_SetTextureAlphaMod để hiển thị quân mờ
// D: Corner brackets — 4 góc L-shape thay ô đặc, tinh tế hơn
void Renderer_DrawHover(SDL_Renderer* r, const _GAMESTATE& state) {
    int row = state.hoveredRow, col = state.hoveredCol;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    if (state._BOARD[row][col].c != 0) return;
    if (state.gameStatus != CHUA_KET_THUC) return;

    int cx = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
    int cy = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;
    SDL_Rect cell = {
        BOARD_OFFSET_X + col * CELL_SIZE + 1,
        BOARD_OFFSET_Y + row * CELL_SIZE + 1,
        CELL_SIZE - 2, CELL_SIZE - 2
    };

    // Ghost piece: chỉ hiện khi đến lượt người chơi thật
    bool humanTurn = (state.mode == MODE_PVP) || (state.mode == MODE_PVE && state.turn);
    if (humanTurn && s_texX && s_texO) {
        SDL_Texture* ghost = state.turn ? s_texX : s_texO;
        SDL_SetTextureAlphaMod(ghost, 75);
        SDL_Rect dst = { cx - s_pieceSize/2, cy - s_pieceSize/2, s_pieceSize, s_pieceSize };
        SDL_RenderCopy(r, ghost, nullptr, &dst);
        SDL_SetTextureAlphaMod(ghost, 255);
    }

    // Corner brackets: 4 góc L-shape thay vì tô đặc toàn ô
    const int CL = 8;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 238, 0, 210);
    // Góc trên-trái
    SDL_RenderDrawLine(r, cell.x,          cell.y,          cell.x+CL,        cell.y);
    SDL_RenderDrawLine(r, cell.x,          cell.y,          cell.x,            cell.y+CL);
    // Góc trên-phải
    SDL_RenderDrawLine(r, cell.x+cell.w,   cell.y,          cell.x+cell.w-CL, cell.y);
    SDL_RenderDrawLine(r, cell.x+cell.w,   cell.y,          cell.x+cell.w,    cell.y+CL);
    // Góc dưới-trái
    SDL_RenderDrawLine(r, cell.x,          cell.y+cell.h,   cell.x+CL,        cell.y+cell.h);
    SDL_RenderDrawLine(r, cell.x,          cell.y+cell.h,   cell.x,            cell.y+cell.h-CL);
    // Góc dưới-phải
    SDL_RenderDrawLine(r, cell.x+cell.w,   cell.y+cell.h,   cell.x+cell.w-CL, cell.y+cell.h);
    SDL_RenderDrawLine(r, cell.x+cell.w,   cell.y+cell.h,   cell.x+cell.w,    cell.y+cell.h-CL);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// ── Public symbol drawing ─────────────────────────────────────────
void Renderer_DrawSymbolX(SDL_Renderer* r, int cx, int cy, int half, int thickness) {
    DrawX(r, cx, cy, half, thickness);
}

void Renderer_DrawSymbolO(SDL_Renderer* r, int cx, int cy, int radius, int thickness) {
    DrawO(r, cx, cy, radius, thickness);
}

// ── Win cells: đường thắng pulsing (thay blink ô) ────────────────
// C: Dùng sin(t*2π) để tạo alpha dao động → gạch ngang 5 quân thắng
void Renderer_DrawWinCells(SDL_Renderer* r, const _GAMESTATE& state) {
    if (state.gameStatus == CHUA_KET_THUC || state.gameStatus == HOA) return;

    // Validate
    for (int k = 0; k < WIN_COUNT; k++) {
        int col = state._WIN_CELLS[k].x, row = state._WIN_CELLS[k].y;
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    }

    // Tâm pixel của ô đầu và ô cuối
    int x1 = BOARD_OFFSET_X + state._WIN_CELLS[0].x * CELL_SIZE + CELL_SIZE / 2;
    int y1 = BOARD_OFFSET_Y + state._WIN_CELLS[0].y * CELL_SIZE + CELL_SIZE / 2;
    int xN = BOARD_OFFSET_X + state._WIN_CELLS[WIN_COUNT-1].x * CELL_SIZE + CELL_SIZE / 2;
    int yN = BOARD_OFFSET_Y + state._WIN_CELLS[WIN_COUNT-1].y * CELL_SIZE + CELL_SIZE / 2;

    // Alpha pulsing: dao động từ 120 đến 255 theo sóng sin
    float t   = (SDL_GetTicks() % 1400) / 1400.0f;
    Uint8 alp = static_cast<Uint8>(120 + 135 * std::sin(t * 2.0f * 3.14159f));

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 228, 0, alp);
    DrawThickLine(r, x1, y1, xN, yN, 6);
    // Đường mỏng hơn màu trắng ở giữa để nổi bật
    SDL_SetRenderDrawColor(r, 255, 255, 200, static_cast<Uint8>(alp * 0.6f));
    DrawThickLine(r, x1, y1, xN, yN, 2);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}


// 
bool Renderer_CreatePieceTextures(SDL_Renderer* renderer)
{
    int size = CELL_SIZE;
    s_pieceSize = size;
    int half = size / 2 - 6;

    // Tạo texture cho X
    s_texX = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(s_texX, SDL_BLENDMODE_BLEND);

    SDL_SetRenderTarget(renderer, s_texX);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    // Stroke: vẽ lớp viền tối trước (lớn hơn 2px, dày hơn), rồi vẽ màu thật lên trên
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

void Renderer_DestroyPieceTextures()
{
    if(s_texX){SDL_DestroyTexture(s_texX); 
        s_texX = nullptr;}

    if(s_texO){
        SDL_DestroyTexture(s_texO);
        s_texO = nullptr;
    }
}

