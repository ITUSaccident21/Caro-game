#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>

//  Cấu trúc dữ liệu trung tâm cho toàn bộ game.

#define BERRY_BOARD_12
#ifdef BERRY_BOARD_12
  #define BOARD_SIZE 12
#else
  #define BOARD_SIZE 15
#endif
#define WIN_COUNT    5

#define LEFT 0
#define TOP  0

// SDL_RenderSetLogicalSize → tọa độ logic luôn là 1920×1080

#define WINDOW_WIDTH        1920
#define WINDOW_HEIGHT       1080

#ifdef BERRY_BOARD_12
  #define CELL_SIZE         75
#else
  #define CELL_SIZE         60
#endif
#define BOARD_PIXEL_SIZE    (BOARD_SIZE * CELL_SIZE)
#define BOARD_OFFSET_X      510
#define BOARD_OFFSET_Y      115

#define BASKET_ZONE_W       BOARD_OFFSET_X
#define BASKET_L_X          0
#define BASKET_R_X          (BOARD_OFFSET_X + BOARD_PIXEL_SIZE)

#define FPS                 60
#define FIXED_STEP_MS       (1000 / FPS)

enum eGameStatus { CHUA_KET_THUC = 2, HOA = 0, P1_THANG = -1, P2_THANG = 1 };
enum AppState { STATE_MENU, STATE_NAME_INPUT, STATE_PLAYING, STATE_LOAD_GAME, STATE_SETTINGS, STATE_SPLASH, STATE_EXIT };
enum GameMode { MODE_PVP, MODE_PVE };
enum AIDifficulty { AI_EASY = 2, AI_MEDIUM = 4, AI_HARD = 6 };

struct _CELL {
    int x, y;
    int c;
};

struct _PLAYER {
    char    name[30];
    int     moves;
    int     wins, losses, draws;
    wchar_t mark;
};

struct _GAMESTATE {
    _CELL   _BOARD[BOARD_SIZE][BOARD_SIZE];
    _PLAYER players[2];

    int     _X, _Y;
    bool    turn;
    int     _LastI, _LastJ;
    int     command;
    int     gameStatus;
    int     totalMoves;
    _CELL   _WIN_CELLS[WIN_COUNT];

    GameMode     mode;
    AIDifficulty difficulty;
    bool         aiThinking;

    int     hoveredRow;
    int     hoveredCol;
    int     selectedRow;
    int     selectedCol;
};