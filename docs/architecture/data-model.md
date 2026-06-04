# Data Model — Structs, Enums, Constants

## Hằng Số (GameDef.h)

```cpp
// Bàn cờ
#define BOARD_SIZE      15      // 15×15 ô
#define WIN_COUNT        5      // 5 quân liên tiếp để thắng

// Cửa sổ (pixel)
#define WINDOW_WIDTH    1100
#define WINDOW_HEIGHT    780
#define CELL_SIZE         44    // px mỗi ô
#define BOARD_PIXEL_SIZE 660    // 15 × 44
#define BOARD_OFFSET_X   220    // lề trái bàn cờ
#define BOARD_OFFSET_Y    60    // lề trên bàn cờ

// Hiệu năng
#define FPS               60
```

---

## Struct Cốt Lõi

### `_CELL` — một ô trên bàn cờ

```cpp
struct _CELL {
    int x, y;   // tọa độ pixel (dùng cho WIN_CELLS)
    int c;      // 0 = trống, -1 = X (P1), +1 = O (P2/AI)
};
```

**Encoding quan trọng:** `c = -1` là X (người chơi 1), `c = +1` là O (AI hoặc P2).
AI luôn là màu `+1` — điểm dương có lợi cho AI, điểm âm có lợi cho người.

### `_PLAYER` — thông tin người chơi

```cpp
struct _PLAYER {
    char    name[30];
    int     moves, wins, losses, draws;
    wchar_t mark;      // L'X' hoặc L'O'
};
```

### `_GAMESTATE` — trạng thái toàn bộ game

```cpp
struct _GAMESTATE {
    // Bàn cờ
    _CELL   _BOARD[15][15];
    _CELL   _WIN_CELLS[WIN_COUNT];   // 5 ô thắng (để highlight)

    // Người chơi
    _PLAYER players[2];              // [0]=P1, [1]=P2/AI
    bool    turn;                    // true = lượt P1, false = lượt P2

    // Lịch sử
    int     _LastI, _LastJ;          // ô vừa đặt
    int     totalMoves;

    // Trạng thái game
    int     gameStatus;              // enum eGameStatus

    // SDL2 runtime
    GameMode     mode;               // PVP hoặc PVE
    AIDifficulty difficulty;         // độ khó AI
    bool         aiThinking;         // AI đang tính toán (background thread)
    int          hoveredRow, hoveredCol;    // ô con trỏ chuột
    int          selectedRow, selectedCol; // ô vừa chọn
};
```

---

## Enums

```cpp
enum eGameStatus {
    HOA          = 0,    // hòa
    P1_THANG     = -1,   // P1 thắng (X)
    P2_THANG     = 1,    // P2/AI thắng (O)
    CHUA_KET_THUC = 2    // đang chơi
};

enum AppState {
    STATE_MENU,
    STATE_NAME_INPUT,
    STATE_PLAYING,
    STATE_LOAD_GAME,
    STATE_EXIT
};

enum GameMode  { MODE_PVP, MODE_PVE };

enum AIDifficulty {
    AI_EASY   = 2,    // depth 2
    AI_MEDIUM = 4,    // depth 4
    AI_HARD   = 6     // depth 6
};

enum AnimState {
    ANIM_IDLE,
    ANIM_WALK_TO_CELL,
    ANIM_PLACING,
    ANIM_CELEBRATE
};

enum SFXEvent {
    SFX_MOVE, SFX_WIN, SFX_DRAW, SFX_MENU_HOVER, SFX_MENU_SELECT
};
```

---

## Struct AI

```cpp
struct Move {
    int row = -1;   // -1 = chưa có nước
    int col = -1;
};

struct AIBenchResult {
    float timeMs;
    int   nodesVisited;
    int   candidatesSkipped;    // thực sự bị cắt bởi alpha-beta
    float realPruneRatio;       // candidatesSkipped / (visited + skipped)
    int   sortCalls;
    int   depth;
    int   moveNumber;
    // ...
};
```

---

## Quy Ước Encoding

| Giá trị | Ý nghĩa |
|---|---|
| `c = 0` | Ô trống |
| `c = -1` | X — Player 1 (human trong PVE) |
| `c = +1` | O — Player 2 / AI |
| `turn = true` | Lượt Player 1 |
| `turn = false` | Lượt Player 2 / AI |

**Điểm AI:** dương = tốt cho AI (`c=+1`), âm = tốt cho human (`c=-1`). `EvaluateBoard` trả về từ góc nhìn AI.
