# Kiến Trúc Tổng Quan — CaroGame SDL2

## Mô Tả

Gomoku (Cờ Caro) 5-in-a-row viết bằng C++/SDL2. Kiến trúc thủ tục thuần túy — không class, không OOP, chỉ free functions phân chia theo module.

---

## Cấu Trúc Thư Mục

```
CaroGameSDL2/
├── main.cpp                    Điểm vào: khởi tạo, chạy, dọn dẹp
└── src/
    ├── game/                   Logic game (không phụ thuộc SDL2)
    │   ├── GameDef.h           Hằng số, enum, struct dùng chung
    │   ├── Model.h / .cpp      Đặt quân, kiểm tra thắng, reset
    │   └── FileHandling.h/.cpp Lưu/tải game (binary)
    ├── sdl/                    Tầng hiển thị và tương tác
    │   ├── App.h / App.cpp     Vòng lặp chính, điều phối state
    │   ├── Renderer.h / .cpp   Vẽ bàn cờ, quân cờ (GPU texture)
    │   ├── Animation.h / .cpp  State machine nhân vật
    │   ├── WinEffect.h / .cpp  Hiệu ứng kết thúc ván
    │   ├── UIManager.h / .cpp  Menu, HUD, dialog
    │   └── AudioManager.h/.cpp BGM + SFX (SDL_mixer)
    └── ai/
        ├── AIPlayer.h          API: AI_FindBestMove()
        └── AIPlayer.cpp        Minimax + Alpha-Beta + benchmark
```

---

## Bản Đồ Module

```
main.cpp
  └── App (App.h/App.cpp)          — điều phối toàn bộ
        ├── Renderer                — vẽ board + pieces
        ├── Animation               — di chuyển nhân vật
        ├── WinEffect               — overlay khi thắng
        ├── UIManager               — menu, HUD, load screen
        ├── AudioManager            — âm thanh
        └── AIPlayer                — chạy trên background thread
              └── Model (dùng chung)
```

**Quy tắc phụ thuộc:**
- `game/` không biết SDL2 tồn tại — pure logic
- `sdl/` phụ thuộc vào `game/` nhưng không ngược lại
- `ai/` phụ thuộc vào `game/GameDef.h`, không phụ thuộc `sdl/`
- `App.cpp` là nơi duy nhất biết về tất cả module khác

---

## Luồng Khởi Động

```
main()
  → App_Init()
      SDL_Init() + SDL_CreateWindow() + SDL_CreateRenderer()
      Renderer_Init() → Renderer_CreatePieceTextures()
      AudioManager_Init()
      UIManager_Init()
      Animation_Init()
      ResetData(state)
  → App_Run()          ← vòng lặp chính
  → App_Shutdown()
      Renderer_DestroyPieceTextures() → Renderer_Shutdown()
      AudioManager/UIManager/Animation _Shutdown()
      SDL_DestroyRenderer() → SDL_DestroyWindow() → SDL_Quit()
```

---

## Liên Kết

- [game-loop.md](game-loop.md) — Fixed timestep loop, App state machine
- [data-model.md](data-model.md) — Structs, enums, constants
- [render-layers.md](render-layers.md) — 5 render layers, texture cache
