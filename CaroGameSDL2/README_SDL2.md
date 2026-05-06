# Caro Game — SDL2 Edition

## Cấu trúc thư mục

```
CaroGame_SDL2/
├── main.cpp                    ← Entry point (SDL2 yêu cầu int main(int, char**))
├── src/
│   ├── game/                   ← GIỮ NGUYÊN từ phiên bản console
│   │   ├── GameDef.h           ← Structs + enums (có thêm field SDL2/AI)
│   │   ├── Model.cpp/h         ← Logic: winCheck, TestBoard, ProcessFinish
│   │   └── FileHandling.cpp/h  ← Save/Load binary
│   ├── ai/                     ← MỚI
│   │   └── AIPlayer.cpp/h      ← Minimax + Alpha-Beta + Evaluation
│   └── sdl/                    ← MỚI — thay thế toàn bộ console layer
│       ├── App.cpp/h           ← Vòng lặp chính + state machine
│       ├── Renderer.cpp/h      ← Vẽ board, pieces, background
│       ├── Animation.cpp/h     ← Nhân vật pixel art + state machine
│       ├── AudioManager.cpp/h  ← SDL_mixer: SFX + BGM
│       └── UIManager.cpp/h     ← Menu, HUD, text input
└── assets/
    ├── sprites/                ← Sprite sheet nhân vật (LPC format)
    │   ├── player1.png         ← Walk cycle 4 hướng, 8 frame mỗi hướng
    │   └── player2.png
    ├── tiles/                  ← Tileset bàn cờ + cảnh quan
    │   ├── ground.png          ← Tile đất/gỗ cho bàn cờ
    │   └── nature.png          ← Cỏ, cây, hoa, hàng rào
    ├── sounds/
    │   ├── place.wav
    │   ├── win.wav
    │   ├── draw.wav
    │   ├── menu_hover.wav
    │   ├── menu_select.wav
    │   ├── menu_bgm.ogg
    │   └── game_bgm.ogg
    └── fonts/
        └── pixel.ttf           ← Pixel font (ví dụ: Press Start 2P)

```

## Files giữ nguyên (không đổi)
- `src/game/Model.cpp` — winCheck, countDir, TestBoard, ProcessFinish, NewSession, ResetData
- `src/game/FileHandling.cpp` — SaveGame, LoadGame, InputFileName

## Files thay thế (console layer → SDL2)
| File cũ (console)   | File mới (SDL2)         |
|---------------------|-------------------------|
| View.cpp            | Renderer.cpp            |
| HUD.cpp             | UIManager.cpp (HUD)     |
| Menu.cpp            | UIManager.cpp (Menu)    |
| Control.cpp         | App.cpp                 |
| main.cpp            | main.cpp (SDL2 version) |

## Thứ tự implement (2 tuần)

### Tuần 1
1. **App.cpp**: Window + game loop chạy được (màn đen OK)
2. **Renderer.cpp**: Vẽ bàn cờ tile đơn giản + quân cờ
3. **UIManager.cpp**: HUD cơ bản, menu đơn giản
4. **AudioManager.cpp**: SFX đặt quân + BGM
5. **FileHandling**: Cải thiện save game

### Tuần 2
6. **Animation.cpp**: Walk animation nhân vật
7. **AIPlayer.cpp**: Minimax depth=2 trước, tune sau
8. **UIManager**: Name input + difficulty selection
9. **Renderer**: Background cảnh quan (cây, cỏ, hoa)
10. Polish + test toàn bộ

## Coordinate system
- Ô [row][col] → tâm pixel tại:
  - x = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE/2
  - y = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE/2
- Mouse (px, py) → ô:
  - col = (px - BOARD_OFFSET_X) / CELL_SIZE
  - row = (py - BOARD_OFFSET_Y) / CELL_SIZE

## AI Architecture
- Candidate moves: bán kính 2 quanh quân đã đánh (~20-30 moves)
- Evaluation: pattern scoring (5,4-open,4-closed,3-open...)
- Alpha-Beta pruning: giảm ~90% nodes phải duyệt
- Depth: Easy=2, Medium=4, Hard=6
- AI color = 1 (O = Player 2), Human color = -1 (X = Player 1)

## Asset sources
- Nhân vật: https://liberatedpixelcup.github.io/Universal-LPC-Spritesheet-Character-Generator/
- Tileset: https://kenney.nl hoặc https://opengameart.org
- Font: https://fonts.google.com/specimen/Press+Start+2P (free, OFL)
- Sound: https://kenney.nl/assets/category:Audio hoặc https://freesound.org
