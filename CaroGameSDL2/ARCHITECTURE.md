# 🎮 Caro Game SDL2 - Kiến Trúc Dự Án Chi Tiết

> **Tài liệu tổng hợp** - Mô tả toàn bộ cấu trúc, từng file, từng hàm và chức năng

---

## 📑 Mục Lục

1. [📁 Cấu Trúc Thư Mục](#-cấu-trúc-thư-mục)
2. [🏗️ Phần Hệ Thống](#️-phần-hệ-thống)
3. [📊 Bảng Tóm Tắt Tất Cả File](#-bảng-tóm-tắt-tất-cả-file)
4. [🔄 Flow Chính](#-flow-chính)
5. [🎯 Phần Cốt Lõi Game Logic](#-phần-cốt-lõi-game-logic)
6. [🎨 Phần Đồ Họa & UI](#-phần-đồ-họa--ui)
7. [🤖 Phần AI](#-phần-ai)
8. [💾 Phần Save/Load](#-phần-saveload)
9. [🔀 State Diagram](#-state-diagram)
10. [📈 Bảng Hàm Có Thể Viết](#-bảng-hàm-có-thể-viết)

---

## 📁 Cấu Trúc Thư Mục

```
CaroGameSDL2/
│
├── main.cpp                          ← 🔴 Điểm vào chương trình
│
└── CaroGameSDL2/
    │
    ├── src/
    │   │
    │   ├── game/                     ← 🎮 Game Logic (Cốt lõi)
    │   │   ├── GameDef.h             │ Định nghĩa constants, enums, structs
    │   │   ├── Model.h               │ Khai báo logic game
    │   │   ├── Model.cpp             │ Thực thi logic game
    │   │   ├── FileHandling.h        │ Khai báo save/load
    │   │   └── FileHandling.cpp      │ Thực thi save/load
    │   │
    │   ├── sdl/                      ← 🎨 SDL2 Graphics & UI
    │   │   ├── APP.h                 │ Vòng lặp chính SDL2
    │   │   ├── App.cpp               │ Thực thi vòng lặp
    │   │   ├── Animation.h           │ Nhân vật hoạt ảnh
    │   │   ├── Animation.cpp         │ Thực thi animation
    │   │   ├── Renderer.h            │ Vẽ bàn cờ, quân cờ
    │   │   ├── Renderer.cpp          │ Thực thi vẽ
    │   │   ├── UIManager.h           │ Menu, HUD, dialog
    │   │   ├── UIManager.cpp         │ Thực thi UI
    │   │   ├── AudioManager.h        │ Khai báo âm thanh
    │   │   └── AudioManager.cpp      │ Thực thi âm thanh
    │   │
    │   └── ai/                       ← 🤖 Trí Tuệ Nhân Tạo
    │       ├── AIPlayer.h            │ Khai báo Minimax
    │       └── AIPlayer.cpp          │ Thực thi Minimax AI
    │
    └── README.md                     ← 📖 Bạn đang đọc đây!
```

---

## 🏗️ Phần Hệ Thống

### 🔴 Điểm Vào: main.cpp

```cpp
// main.cpp
int main() {
    AppContext ctx = {};
    _GAMESTATE state = {};
    AppState appState = STATE_MENU;

    if (!App_Init(ctx, state, appState)) {
        std::cerr << "Failed to init app\n";
        return 1;
    }

    App_Run(ctx, state, appState);
    App_Shutdown(ctx);
    return 0;
}
```

**Lưu lưu:**
1. Khởi tạo AppContext (SDL2 window/renderer)
2. Khởi tạo _GAMESTATE (dữ liệu game)
3. Đặt appState = STATE_MENU
4. Chạy vòng lặp chính App_Run()
5. Dọn dẹp resources

---

## 📊 Bảng Tóm Tắt Tất Cả File

### **PHẦN 1: CORE GAME LOGIC**

| # | File | Loại | Chức Năng | Hàm Chính | Dòng Code |
|---|------|------|----------|-----------|-----------|
| 1 | `GameDef.h` | Header | Định nghĩa constants, enums, structs | Constants, Enums, Structs | 100+ |
| 2 | `Model.h` | Header | Khai báo API logic game | 6 functions | 50 |
| 3 | `Model.cpp` | Source | Thực thi logic game (check board, win condition) | `CheckBoard()`, `winCheck()`, `TestBoard()` | 200+ |
| 4 | `FileHandling.h` | Header | Khai báo API save/load game | 5 functions | 30 |
| 5 | `FileHandling.cpp` | Source | Lưu/tải game (binary format) | `SaveGame()`, `LoadGame()` | 150+ |

#### 📋 Enum & Constants trong GameDef.h

```cpp
// 🎮 Game Constants
#define BOARD_SIZE   15           // Bàn cờ 15x15
#define WIN_COUNT    5            // Thắng 5 quân liên tiếp

// 🖥️ SDL2 Layout (pixel)
#define WINDOW_WIDTH        1100
#define WINDOW_HEIGHT       780
#define CELL_SIZE           44    // px mỗi ô
#define BOARD_PIXEL_SIZE    660   // 15*44
#define BOARD_OFFSET_X      220   // Căn giữa bàn cờ
#define BOARD_OFFSET_Y      60    // Lề trên

// 📊 Enums
enum eGameStatus       { CHUA_KET_THUC=2, HOA=0, P1_THANG=-1, P2_THANG=1 }
enum AppState          { STATE_MENU, STATE_NAME_INPUT, STATE_PLAYING, STATE_LOAD_GAME, STATE_EXIT }
enum GameMode          { MODE_PVP, MODE_PVE }
enum AIDifficulty      { AI_EASY=2, AI_MEDIUM=4, AI_HARD=6 }
enum AnimState         { ANIM_IDLE, ANIM_WALK_TO_CELL, ANIM_PLACING, ANIM_CELEBRATE }
enum Direction         { DIR_DOWN=0, DIR_LEFT=1, DIR_RIGHT=2, DIR_UP=3 }
```

#### 🔐 Structs Chính

```cpp
struct _CELL {
    int x, y;    // Tọa độ console (giữ tương thích)
    int c;       // 0=trống, -1=X(P1), 1=O(P2)
};

struct _PLAYER {
    char    name[30];
    int     moves, wins, losses, draws;
    wchar_t mark;  // 'X' hoặc 'O'
};

struct _GAMESTATE {
    _CELL   _BOARD[15][15];         // Bàn cờ
    _PLAYER players[2];              // 2 người chơi

    // Legacy console mode
    int     _X, _Y;                  // Cursor
    bool    turn;                    // true=P1, false=P2
    int     _LastI, _LastJ;          // Ô vừa đánh
    int     gameStatus;              // Status game
    int     totalMoves;              // Tổng nước đã chơi
    _CELL   _WIN_CELLS[5];           // 5 ô thắng

    // SDL2 mode
    GameMode     mode;               // PVP hoặc PVE
    AIDifficulty difficulty;         // Độ khó AI
    bool         aiThinking;         // AI đang tính toán

    int     hoveredRow, hoveredCol;  // Ô con trỏ đang trỏ tới
    int     selectedRow, selectedCol;// Ô vừa chọn
};
```

---

### **PHẦN 2: SDL2 APP & RENDERING**

| # | File | Loại | Chức Năng | Hàm Chính | Dòng Code |
|---|------|------|----------|-----------|-----------|
| 6 | `APP.h` | Header | Vòng lặp chính SDL2 | 15 functions | 80 |
| 7 | `App.cpp` | Source | Thực thi vòng lặp (event, update, render) | `App_Run()`, `App_HandleEvents()`, `App_Update()`, `App_Render()` | 400+ |
| 8 | `Renderer.h` | Header | Khai báo vẽ board, pieces, effects | 6 functions | 40 |
| 9 | `Renderer.cpp` | Source | Vẽ bàn cờ, quân cờ, hiệu ứng | `Renderer_DrawBoard()`, `Renderer_DrawPieces()` | 250+ |

#### 🎮 Vòng Lặp Chính (App_Run)

```cpp
// Fixed timestep game loop
while (appState != STATE_EXIT) {
    Uint64 ticks = SDL_GetTicks64();
    float  dt = (ticks - prevTicks) / 1000.0f;
    prevTicks = ticks;
    accumulator += dt;

    // Event handling
    App_HandleEvents(ctx, state, appState);

    // Fixed update (~16.67ms = 60 FPS)
    while (accumulator >= FIXED_STEP_MS / 1000.0f) {
        App_Update(FIXED_STEP_MS / 1000.0f, state, appState);
        accumulator -= FIXED_STEP_MS / 1000.0f;
    }

    // Render
    App_Render(ctx, state, appState);
}
```

---

### **PHẦN 3: ANIMATION & CHARACTER**

| # | File | Loại | Chức Năng | Hàm Chính | Dòng Code |
|---|------|------|----------|-----------|-----------|
| 10 | `Animation.h` | Header | Khai báo nhân vật hoạt ảnh | 7 functions | 60 |
| 11 | `Animation.cpp` | Source | State machine (IDLE→WALK→PLACING→IDLE) | `Animation_Update()`, `Animation_Render()` | 300+ |

#### 🎬 State Machine Animation

```
START
  ↓
IDLE (chờ input)
  ↓ (khi click ô)
WALK_TO_CELL (di chuyển)
  ├─ Tính toán hướng
  ├─ Di chuyển smooth (420px/s)
  └─ Khi gần đích → PLACING
  ↓
PLACING (đặt quân)
  ├─ Vẽ hiệu ứng "thả quân"
  ├─ Đếm ngược 0.22s
  └─ Gọi App_PlacePiece() khi xong
  ↓
IDLE (lại chờ)

*** Hoặc: CELEBRATE (khi thắng)
  ├─ Nhảy lên (sin animation)
  ├─ Vẽ sao vàng
  └─ Đếm ngược 2s
```

---

### **PHẦN 4: UI & MENU**

| # | File | Loại | Chức Năng | Hàm Chính | Dòng Code |
|---|------|------|----------|-----------|-----------|
| 12 | `UIManager.h` | Header | Menu, HUD, dialog, input | 15 functions | 100+ |
| 13 | `UIManager.cpp` | Source | Render menu, HUD, save dialog | `UIManager_RenderMenu()`, `UIManager_RenderHUD()` | 400+ |
| 14 | `AudioManager.h` | Header | Khai báo âm thanh | 5 functions | 30 |
| 15 | `AudioManager.cpp` | Source | SDL_mixer wrapper (SFX, BGM) | `AudioManager_PlaySFX()`, `AudioManager_PlayBGM()` | 100+ |

#### 🎵 Audio Events

```cpp
enum SFXEvent {
    SFX_MOVE,        // Tiếng gỗ clack khi đặt quân
    SFX_WIN,         // Jingle vui khi thắng
    SFX_DRAW,        // Jingle trung tính khi hòa
    SFX_MENU_HOVER,  // Hover menu item
    SFX_MENU_SELECT  // Chọn menu item
};
```

---

### **PHẦN 5: AI**

| # | File | Loại | Chức Năng | Hàm Chính | Dòng Code |
|---|------|------|----------|-----------|-----------|
| 16 | `AIPlayer.h` | Header | Khai báo Minimax algorithm | 1 function | 10 |
| 17 | `AIPlayer.cpp` | Source | Minimax + Alpha-Beta Pruning | `AI_FindBestMove()`, `Minimax()`, `EvaluateBoard()` | 300+ |

#### 🤖 AI Algorithm

```
AI_FindBestMove(state)
  ↓
GetCandidates() → Danh sách ~20 ô hợp lệ
  ↓
For each candidate:
  - Đặt quân AI tạm thời
  - Gọi Minimax(depth, alpha, beta, false)
    ├─ Đệ quy: AI (max) ↔ Human (min)
    ├─ Kiểm tra terminal (ai thắng?)
    ├─ Alpha-Beta Pruning (cắt nhánh)
    └─ EvaluateBoard() (tính score)
  - Hoàn tác quân
  - Ghi nhận best move
  ↓
Return best move
```

**Độ khó:**
- `AI_EASY = 2`: Nhìn trước 2 nước (instant)
- `AI_MEDIUM = 4`: Nhìn trước 4 nước (<100ms)
- `AI_HARD = 6`: Nhìn trước 6 nước (1-2s)

---

## 🔄 Flow Chính

### 🎬 Flow Tổng Quát

```
┌─────────────────────────────────────┐
│         main.cpp                    │
│    (1) App_Init()                   │
│    (2) App_Run()                    │
│    (3) App_Shutdown()               │
└────────────┬────────────────────────┘
             │
             ↓
┌─────────────────────────────────────┐
│  App_Init()                         │
│  ✓ SDL_Init()                       │
│  ✓ Tạo window (1100x780)            │
│  ✓ Tạo renderer                     │
│  ✓ Load font, sprites               │
│  ✓ Khởi tạo Animation               │
│  ✓ Khởi tạo AudioManager            │
└────────────┬────────────────────────┘
             │
             ↓
┌─────────────────────────────────────┐
│  App_Run() [MAIN LOOP]              │
│                                     │
│  WHILE appState != STATE_EXIT:      │
│  ┌─────────────────────────────┐    │
│  │ (1) App_HandleEvents()      │    │
│  │ • Mouse/Keyboard input      │    │
│  │ • Menu click                │    │
│  │ • Window close              │    │
│  └────────────┬────────────────┘    │
│               │                     │
│  ┌────────────↓────────────────┐    │
│  │ (2) App_Update()            │    │
│  │ • Animation_Update()        │    │
│  │ • AI_FindBestMove()         │    │
│  │ • Model logic               │    │
│  │ • UIManager updates         │    │
│  └────────────┬────────────────┘    │
│               │                     │
│  ┌────────────↓────────────────┐    │
│  │ (3) App_Render()            │    │
│  │ • Clear screen              │    │
│  │ • Renderer_DrawBG()         │    │
│  │ • Renderer_DrawBoard()      │    │
│  │ • Renderer_DrawPieces()     │    │
│  │ • Animation_Render()        │    │
│  │ • UIManager render          │    │
│  │ • SDL_RenderPresent()       │    │
│  └─────────────────────────────┘    │
│                                     │
└─────────────────────────────────────┘
             │
             ↓
┌─────────────────────────────────────┐
│  App_Shutdown()                     │
│  ✓ Giải phóng renderer              │
│  ✓ Đóng window                      │
│  ✓ SDL_Quit()                       │
└─────────────────────────────────────┘
```

---

### 🎮 Flow Khi Chơi Game

```
USER INTERACTION:
┌─────────────────────────────┐
│ Người chơi di chuột          │
│ App_PixelToCell()            │
│ → hoveredRow, hoveredCol     │
└─────────────┬───────────────┘
              │
              ↓
┌─────────────────────────────┐
│ Renderer_DrawHover()        │
│ → Vẽ ô highlight vàng       │
└─────────────┬───────────────┘
              │
              ↓
┌─────────────────────────────┐
│ Người chơi click ô          │
│ → Mouse event detected      │
│ → App_PlacePiece() trigger? │
│    ❌ Animation đang chơi?   │
│       → Khóa input          │
│    ✅ Animation idle?        │
│       → OK để click         │
└─────────────┬───────────────┘
              │
              ↓
┌─────────────────────────────┐
│ Animation_StartMove()       │
│ • targetRow = hoveredRow    │
│ • targetCol = hoveredCol    │
│ • anim = WALK_TO_CELL       │
│ • Tính toán direction       │
└─────────────┬───────────────┘
              │
              ↓
┌─────────────────────────────┐
│ Animation_Update() [WALK]   │
│ • Di chuyển (420px/s)       │
│ • Cập nhật direction        │
│ • Khi gần đích:             │
│   anim = PLACING            │
│   placeTimer = 0.22s        │
└─────────────┬───────────────┘
              │
              ↓
┌─────────────────────────────┐
│ Animation_Update() [PLACING]│
│ • placeTimer -= dt          │
│ • Vẽ hiệu ứng "thả quân"    │
│ • Khi placeTimer <= 0:      │
│   App_PlacePiece()          │
└─────────────┬───────────────┘
              │
              ↓
┌─────────────────────────────┐
│ App_PlacePiece()            │
│ • CheckBoard() → ghi quân   │
│ • TestBoard() → kiểm tra    │
│ • winCheck() → có thắng?    │
└─────────────┬───────────────┘
              │
       ┌──────┴──────┐
       │             │
       ↓             ↓
   ┌───────┐   ┌────────────┐
   │THẮNG! │   │TIẾP CHƠI   │
   │ • Win │   │ • Lượt sau │
   │ • +1  │   │ • PVE:     │
   │ • Ask │   │   App_Trig-│
   │  new  │   │   gerAITurn│
   │ ├─────┤   │ • PVP:     │
   │ │YES  │   │   Chờ user │
   │ │→ NEW│   └────────────┘
   │ │GAME │
   │ │NO   │
   │ │→EXIT│
   └───────┘
```

---

## 🎯 Phần Cốt Lõi Game Logic

### 🏛️ Model.cpp - 6 Hàm Chính

| # | Hàm | Tham Số | Trả Về | Chức Năng |
|---|-----|--------|--------|----------|
| 1 | `NewSession()` | `state` | `void` | Reset ván mới (giữ điểm cũ, reset board) |
| 2 | `ResetData()` | `state` | `void` | Reset toàn bộ (điểm, tên, board) |
| 3 | `CheckBoard()` | `state, row, col` | `int` (-1/1/0) | Đặt quân, kiểm tra hợp lệ |
| 4 | `isBoardFull()` | `state` | `bool` | Kiểm tra board có đầy không |
| 5 | `winCheck()` | `state, i, j` | `bool` | Kiểm tra vị trí (i,j) có thắng |
| 6 | `TestBoard()` | `state` | `int` | Kiểm tra trạng thái game (đang chơi/thắng/hòa) |

#### 📝 Hàm winCheck() - Chi Tiết

```cpp
bool winCheck(_GAMESTATE& state, int i, int j) {
    int mark = state._BOARD[i][j].c;  // -1 (X) hoặc 1 (O)
    if (mark == 0) return false;

    // 4 hướng: ngang, dọc, chéo/, chéo\
    int dx[] = {0, 1, 1, 1};
    int dy[] = {1, 0, 1, -1};

    for (int dir = 0; dir < 4; dir++) {
        int count = 1;

        // Đi hướng thuận
        for (int step = 1; step < WIN_COUNT; step++) {
            int ni = i + dx[dir] * step;
            int nj = j + dy[dir] * step;
            if (ni < 0 || ni >= BOARD_SIZE || nj < 0 || nj >= BOARD_SIZE) break;
            if (state._BOARD[ni][nj].c == mark) count++;
            else break;
        }

        // Đi hướng đảo
        for (int step = 1; step < WIN_COUNT; step++) {
            int ni = i - dx[dir] * step;
            int nj = j - dy[dir] * step;
            if (ni < 0 || ni >= BOARD_SIZE || nj < 0 || nj >= BOARD_SIZE) break;
            if (state._BOARD[ni][nj].c == mark) count++;
            else break;
        }

        if (count >= WIN_COUNT) return true;
    }
    return false;
}
```

#### 📝 Hàm CheckBoard() - Chi Tiết

```cpp
int CheckBoard(_GAMESTATE& state, int row, int col) {
    // Kiểm tra bounds
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;  // Out of bounds
    }

    // Kiểm tra ô đã có quân
    if (state._BOARD[row][col].c != 0) {
        return 0;  // Ô đã có quân
    }

    // Xác định mark của người chơi hiện tại
    int mark = state.turn ? -1 : 1;  // true=P1(X)=-1, false=P2(O)=1

    // Đặt quân
    state._BOARD[row][col].c = mark;
    state._LastI = row;
    state._LastJ = col;
    state.totalMoves++;

    return mark;  // Thành công
}
```

---

## 🎨 Phần Đồ Họa & UI

### 🖼️ Renderer.cpp - 5 Layer Vẽ

```cpp
// Layer 1: Background (cỏ, cây)
Renderer_DrawBackground(renderer);

// Layer 2: Bàn cờ (gỗ) + lưới
Renderer_DrawBoard(renderer, state);

// Layer 3: Quân cờ đã đặt
Renderer_DrawPieces(renderer, state);

// Layer 4: Hover effect (ô được trỏ)
Renderer_DrawHover(renderer, state);

// Layer 5: Hiệu ứng chiến thắng (5 ô nhấp nháy)
Renderer_DrawWinCells(renderer, state);
```

#### 🎨 Màu Sắc

| Component | Màu RGB |
|-----------|---------|
| P1 (X) | Đỏ: (210, 60, 60) |
| P2 (O) | Xanh: (60, 100, 210) |
| Celebrate | Vàng: (240, 200, 40) |
| Board | Nâu: (139, 90, 43) |
| Grid | Xám: (200, 200, 200) |
| Hover | Vàng mờ: (255, 255, 0, 100) |
| Skin | Da: (235, 195, 155) |

---

### 📊 UIManager.cpp - Menu States

```
┌────────────────────────────────────┐
│        STATE_MENU                  │
│   (Main Menu)                      │
│                                    │
│  ┌─ PLAY                           │
│  ├─ LOAD GAME                      │
│  └─ EXIT                           │
└────────────┬───────────────────────┘
     │       │       │
     │       │       └─→ STATE_EXIT
     │       │
     │       └─→ STATE_LOAD_GAME
     │           (Chọn file save)
     │
     └─→ STATE_NAME_INPUT
         ├─ Nhập tên P1
         ├─ Nhập tên P2
         ├─ Chọn Mode
         │  ├─ PVP
         │  └─ PVE
         │     ├─ Chọn Difficulty
         │     │  ├─ EASY
         │     │  ├─ MEDIUM
         │     │  └─ HARD
         │
         └─→ STATE_PLAYING
             (Game chính)
             └─→ Lượt người chơi
                 ├─ Click ô
                 ├─ Animation
                 ├─ App_PlacePiece()
                 │
                 ├─ Win → ShowResult()
                 ├─ Draw → ShowResult()
                 └─ Continue → Next turn
```

---

## 🤖 Phần AI

### 🧠 AI Algorithm Overview

```
AI_FindBestMove(state)
├─ Copy state để không modify original
├─ Get depth từ state.difficulty
├─ GetCandidates() → Danh sách ~20 ô hợp lệ (tối ưu)
│
├─ FOR each candidate move:
│  ├─ Đặt quân AI tạm thời (c=1)
│  ├─ Minimax(state, depth-1, -∞, +∞, false, row, col)
│  │  │
│  │  ├─ CheckTerminal() → Có ai thắng?
│  │  │  ├─ Có → return INT_MAX/2 hoặc INT_MIN/2
│  │  │  └─ Không → tiếp
│  │  │
│  │  ├─ depth == 0?
│  │  │  ├─ Có → return EvaluateBoard(state)
│  │  │  └─ Không → tiếp
│  │  │
│  │  ├─ IsMaximising (AI)?
│  │  │  ├─ Yes → Tìm MAX
│  │  │  │  ├─ best = -∞
│  │  │  │  ├─ FOR move: score = Minimax(..., false)
│  │  │  │  ├─ best = max(best, score)
│  │  │  │  ├─ alpha = max(alpha, best)
│  │  │  │  ├─ IF beta ≤ alpha: BREAK (prune)
│  │  │  │  └─ return best
│  │  │  │
│  │  │  └─ No → Tìm MIN
│  │  │     ├─ best = +∞
│  │  │     ├─ FOR move: score = Minimax(..., true)
│  │  │     ├─ best = min(best, score)
│  │  │     ├─ beta = min(beta, best)
│  │  │     ├─ IF beta ≤ alpha: BREAK (prune)
│  │  │     └─ return best
│  │
│  ├─ Hoàn tác quân (undo)
│  ├─ Ghi nhận score
│  │
│  └─ IF score > bestScore:
│     └─ bestScore = score, best.row/col = row/col
│
└─ Return best (Move { row, col })
```

### 📊 Scoring System (SCORES[])

```cpp
SCORES[] = { 0, 10, 100, 1000, 100000, 10000000 }
           // 0  1    2     3      4        5
```

**CountLine()** tính điểm dựa trên:
1. Số quân liên tiếp (count)
2. Có bao nhiêu đầu mở (open ends)

```
Ví dụ:
_ O O O _    → 3 quân, 2 đầu mở → SCORES[3] = 1000
X O O O _    → 3 quân, 1 đầu mở → SCORES[3] / 2 = 500
X O O O X    → 3 quân, 0 đầu mở → SCORES[3] / 2 = 500
```

---

## 💾 Phần Save/Load

### 💿 FileHandling.cpp

#### Save Format (Binary)

```
File structure:
├─ Board data (15x15 cells)
│  └─ Mỗi cell: struct _CELL { x, y, c }
├─ Player 1 data
│  └─ struct _PLAYER { name[30], moves, wins, losses, draws, mark }
├─ Player 2 data
│  └─ struct _PLAYER { ... }
├─ Game status
│  └─ int gameStatus
├─ Last move
│  └─ int _LastI, _LastJ
└─ (Còn lại: tùy implement)
```

#### 📝 SaveGame() - Chi Tiết

```cpp
bool SaveGame(const _GAMESTATE& state, const std::string& filename) {
    FILE* f = fopen(filename.c_str(), "wb");
    if (!f) return false;

    // Ghi board
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            fwrite(&state._BOARD[i][j], sizeof(_CELL), 1, f);
        }
    }

    // Ghi players
    fwrite(&state.players, sizeof(_PLAYER) * 2, 1, f);

    // Ghi game state
    int status = state.gameStatus;
    fwrite(&status, sizeof(int), 1, f);

    int lastI = state._LastI, lastJ = state._LastJ;
    fwrite(&lastI, sizeof(int), 1, f);
    fwrite(&lastJ, sizeof(int), 1, f);

    fclose(f);
    return true;
}
```

#### 📝 LoadGame() - Chi Tiết

```cpp
bool LoadGame(_GAMESTATE& state, const std::string& filename) {
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) return false;

    // Đọc board
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            fread(&state._BOARD[i][j], sizeof(_CELL), 1, f);
        }
    }

    // Đọc players
    fread(&state.players, sizeof(_PLAYER) * 2, 1, f);

    // Đọc game state
    int status;
    fread(&status, sizeof(int), 1, f);
    state.gameStatus = status;

    int lastI, lastJ;
    fread(&lastI, sizeof(int), 1, f);
    fread(&lastJ, sizeof(int), 1, f);
    state._LastI = lastI;
    state._LastJ = lastJ;

    fclose(f);
    return true;
}
```

---

## 🔀 State Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  ┌────────────┐      ┌────────────┐      ┌────────────┐        │
│  │ STATE_MENU │──────│STATE_NAME_ │──────│ STATE_     │        │
│  │            │      │   INPUT    │      │ PLAYING    │        │
│  │ • Main     │      │            │      │            │        │
│  │   menu     │      │ • Input    │      │ • Game     │        │
│  │   render   │      │   names    │      │   loop     │        │
│  │ • Button   │      │ • Select   │      │ • User     │        │
│  │   click    │      │   mode/AI  │      │   input    │        │
│  └────┬───────┘      └────┬───────┘      └────┬───────┘        │
│       │                   │                    │                │
│       │                   └────────────────────┘                │
│       │                   (START GAME)                          │
│       │                                                         │
│       │  ┌────────────────┐                                    │
│       │  │STATE_LOAD_GAME│                                    │
│       │  │                │                                    │
│       │  │ • List saves   │                                    │
│       │  │ • Select file  │                                    │
│       │  │ • LoadGame()   │                                    │
│       └──┤                │                                    │
│          │ (LOAD GAME)    │                                    │
│          └────┬───────────┘                                    │
│               │                                                 │
│               └─────────────────┬─────────────────┐            │
│                                 │                 │            │
│                          ┌──────↓──────┐   ┌─────↓────┐       │
│                          │  Continue   │   │ STATE_   │       │
│                          │  PLAYING    │   │   EXIT   │       │
│                          └─────────────┘   │          │       │
│                                            │ • Cleanup│       │
│                                            │ • Quit   │       │
│                                            └──────────┘       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📈 Bảng Hàm Có Thể Viết

### Độ Khó & Độ Phức Tạp

| # | File | Hàm | Độ Khó | Dòng Code | Mô Tả |
|---|------|-----|--------|-----------|-------|
| 1 | Model.cpp | `isBoardFull()` | ⭐ Dễ | 10-15 | Duyệt board, tìm ô trống |
| 2 | Model.cpp | `CheckBoard()` | ⭐ Dễ | 20-30 | Đặt quân, kiểm tra hợp lệ |
| 3 | Model.cpp | `winCheck()` | ⭐⭐ Trung | 40-50 | Kiểm tra 4 hướng (WIN_COUNT=5) |
| 4 | Model.cpp | `TestBoard()` | ⭐⭐ Trung | 30-40 | Kiểm tra trạng thái game |
| 5 | Renderer.cpp | `Renderer_DrawBoard()` | ⭐⭐ Trung | 30-40 | Vẽ lưới 15x15 |
| 6 | Renderer.cpp | `Renderer_DrawPieces()` | ⭐⭐ Trung | 25-35 | Vẽ quân cờ từ board |
| 7 | Renderer.cpp | `Renderer_DrawBackground()` | ⭐⭐ Trung | 15-25 | Vẽ nền (hình chữ nhật xanh) |
| 8 | UIManager.cpp | `UIManager_RenderHUD()` | ⭐⭐ Trung | 50-60 | Vẽ HUD (tên, điểm, lượt) |
| 9 | FileHandling.cpp | `SaveGame()` | ⭐⭐⭐ Khó | 40-50 | Lưu game ra binary file |
| 10 | FileHandling.cpp | `LoadGame()` | ⭐⭐⭐ Khó | 40-50 | Tải game từ binary file |
| 11 | Animation.cpp | Hiệu ứng mới | ⭐⭐ Trung | 20-30 | Thêm animation ANIM_IDLE_THINKING |
| 12 | AIPlayer.cpp | Scoring tối ưu | ⭐⭐⭐ Khó | 30-40 | Thay SCORES[] cho AI mạnh hơn |

---

## 🎓 Hướng Dẫn Viết Code

### Step 1: Hiểu Logic

**Ví dụ: isBoardFull()**

```
Mục đích: Kiểm tra bàn cờ có còn ô trống không?

Thuật toán:
1. Duyệt từng ô trên board (15x15)
2. Nếu tìm thấy ô trống (c==0) → return false
3. Nếu duyệt hết không tìm → return true
```

### Step 2: Viết Pseudocode

```cpp
isBoardFull() {
    for i = 0 to 14 {
        for j = 0 to 14 {
            if board[i][j] is empty {
                return false
            }
        }
    }
    return true
}
```

### Step 3: Viết Code C++

```cpp
bool isBoardFull(_GAMESTATE& state) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state._BOARD[i][j].c == 0) {
                return false;  // Tìm thấy ô trống
            }
        }
    }
    return true;  // Không có ô trống
}
```

### Step 4: Compile & Test

```bash
# Compile
cd CaroGameSDL2
cmake -B build
cmake --build build

# Run
./build/CaroGameSDL2
```

---

## 📚 Tài Liệu Tham Khảo

### Minimax Algorithm
- 👉 Xem file: `src/ai/README_AIPlayer.md`
- Chi tiết Alpha-Beta Pruning, Scoring System

### Cấu Trúc Dữ Liệu
- `GameDef.h`: Tất cả constants, enums, structs

### Flow Game
- `APP.h`: Vòng lặp chính, event handling

---

## 🚀 Quick Start

### Để Hiểu Project

1. ✅ Đọc **file này** (README.md)
2. ✅ Đọc `GameDef.h` (constants, structs)
3. ✅ Đọc `APP.h` (main loop)
4. ✅ Đọc `Model.h` (game logic)
5. ✅ Đọc `AIPlayer.h` (AI algorithm)

### Để Viết Code

1. ✅ Chọn 1 hàm đơn giản (ví dụ: `isBoardFull()`)
2. ✅ Hiểu logic của hàm
3. ✅ Viết pseudocode
4. ✅ Chuyển thành C++
5. ✅ Compile & test

### Để Cải Thiện Project

1. ✅ Tăng AI difficulty (từ 6 lên 8)
2. ✅ Thêm animation mới
3. ✅ Thêm sound effects
4. ✅ Thêm feature lưu/tải game
5. ✅ Thêm network multiplayer

---

## 📞 Liên Hệ & Hỗ Trợ

- **Language**: C++
- **Graphics Library**: SDL2
- **Font Rendering**: SDL_ttf
- **Audio**: SDL_mixer
- **Image Loading**: SDL_image

---

**Biên soạn bởi:** AI Architecture  
**Phần mềm:** Caro Game SDL2  
**Ngôn ngữ:** C++ (SDL2)  
**Phiên bản:** v1.0  
**Cập nhật lần cuối:** 2024
