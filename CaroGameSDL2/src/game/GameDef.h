#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>

// ================================================================
//  GameDef.h
//  Cấu trúc dữ liệu trung tâm cho toàn bộ game.
//  - Giữ nguyên 100% các field cũ (không phá vỡ Model.cpp)
//  - Bổ sung enum và field phục vụ SDL2 + AI
// ================================================================

// ─── Game constants ─────────────────────────────────────────────
#define BOARD_SIZE   15
#define WIN_COUNT    5

// Legacy console layout offsets — kept so _CELL.x/_CELL.y stays
// binary-compatible with old save files. Not used by SDL2 rendering.
#define LEFT 0
#define TOP  0

// ─── SDL2 Layout (pixel) ────────────────────────────────────────
#define WINDOW_WIDTH        1100
#define WINDOW_HEIGHT       780
#define CELL_SIZE           44          // px mỗi ô
#define BOARD_PIXEL_SIZE    (BOARD_SIZE * CELL_SIZE)  // 660px
#define BOARD_OFFSET_X      220         // bàn cờ căn giữa giữa 2 panel
#define BOARD_OFFSET_Y      60          // lề trên bàn cờ

// Panel trái (Player 1) và panel phải (Player 2) — đối xứng
// Mỗi panel rộng 200px, cách mép cửa sổ 10px, cách bàn cờ 10px
// 10 + 200 + 10 + 660 + 10 + 200 + 10 = 1100 ✓
#define LEFT_PANEL_X        10
#define LEFT_PANEL_WIDTH    200
#define HUD_PANEL_X         (BOARD_OFFSET_X + BOARD_PIXEL_SIZE + 10)  // 890
#define HUD_PANEL_WIDTH     200
#define FPS                 60
#define FIXED_STEP_MS       (1000 / FPS)

// ─── Enums ──────────────────────────────────────────────────────
enum eGameStatus { CHUA_KET_THUC = 2, HOA = 0, P1_THANG = -1, P2_THANG = 1 };
enum AppState { STATE_MENU, STATE_NAME_INPUT, STATE_PLAYING, STATE_LOAD_GAME, STATE_EXIT };
enum GameMode { MODE_PVP, MODE_PVE };
enum AIDifficulty { AI_EASY = 2, AI_MEDIUM = 4, AI_HARD = 6 };

// Trạng thái animation nhân vật
enum AnimState { ANIM_IDLE, ANIM_WALK_TO_CELL, ANIM_PLACING, ANIM_CELEBRATE };
// Hướng nhân vật đang nhìn (khớp với row trong LPC sprite sheet)
enum Direction { DIR_DOWN = 0, DIR_LEFT = 1, DIR_RIGHT = 2, DIR_UP = 3 };

// ─── Core data structures (KHÔNG thay đổi so với phiên bản cũ) ──
struct _CELL {
    int x, y;   // [CŨ] tọa độ console — giữ để FileHandling binary save tương thích
    int c;      // 0: trống | -1: X (Player 1) | 1: O (Player 2)
};

struct _PLAYER {
    char    name[30];
    int     moves;
    int     wins, losses, draws;
    wchar_t mark;           // L'X' hoặc L'O'
};

// ─── _GAMESTATE ─────────────────────────────────────────────────
struct _GAMESTATE {
    // ── Dữ liệu bàn cờ (giữ nguyên) ────────────────────────────
    _CELL   _BOARD[BOARD_SIZE][BOARD_SIZE];
    _PLAYER players[2];

    // ── Cursor console (giữ để Model.cpp/FileHandling tương thích)
    int     _X, _Y;
    bool    turn;               // true = lượt Player 1
    int     _LastI, _LastJ;     // chỉ số ô vừa đánh
    int     command;
    int     gameStatus;
    int     totalMoves;
    _CELL   _WIN_CELLS[WIN_COUNT];  // 5 ô thắng cho hiệu ứng

    // ── Thêm mới: chế độ chơi & AI ──────────────────────────────
    GameMode     mode;          // PVP hoặc PVE
    AIDifficulty difficulty;    // độ sâu minimax
    bool         aiThinking;   // đang tính toán → khóa input

    // ── Thêm mới: SDL2 cursor (pixel-based, độc lập với _X _Y) ──
    int     hoveredRow;         // ô đang trỏ (-1 nếu ngoài bàn)
    int     hoveredCol;
    int     selectedRow;        // ô người chơi vừa chọn
    int     selectedCol;
};