# DR-007 — Cozy Style: Tích Hợp Assets & Visual Overhaul

**Ngày implement:** 2026-06-04
**Trạng thái:** ✅ Hoàn thành
**Liên quan:** Renderer.cpp, Renderer.h, UIManager.cpp, App.cpp

---

## Bối Cảnh

Sau khi phân tích 2 mẫu tham khảo và style Stardew Valley, quyết định chuyển toàn bộ visual sang **Cozy Pixel Art** style.
Assets thu thập từ 4 open source packs + 1 board.jpg do AI generate.

---

## Những Gì Đã Thực Hiện

### A — Background PNG System (Renderer.cpp)

Thay solid color fill bằng texture PNG:
- `assets/sprites/bg_menu.png` (Background_Yellow) → menu/settings/name input
- `assets/sprites/bg_game.png` (Background_Blue) → game screen
- Fallback procedural nếu file không load được

`Renderer_DrawBackground(bool isGameScreen)` — truyền bool để chọn đúng background.
Gọi từ `App_Render` cho **tất cả state** (trừ splash tự vẽ riêng).

Decoration sprites từ Cozy Lands trang trí 4 góc sau background.

### B — Random Board Skin System (Renderer.cpp)

- Load tối đa 4 `board_0N.jpg` trong `Renderer_Init()`
- `Renderer_RandomizeBoard()` — `rand() % s_boardCount`, gọi trong `App_StartNewGame/Session()`
- Board texture blit 740×740 tại offset (-40,-40) → frame ảnh hiện ra ngoài vùng chơi
- Panel HUD vẽ sau nên che phần nhô ra
- Fallback → checkerboard procedural nếu không có file

**board.jpg** (AI generated, 764×764): Pixel art wood board với frame mahogany + golden accents.

### C — Cozy Color Palette (UIManager.cpp)

```cpp
COZY_PARCHMENT  = {212, 180, 134, 225}  // panel background
COZY_WOOD_DARK  = {107,  56,  24, 255}  // border, title text
COZY_WOOD_MID   = {160,  90,  35, 255}  // secondary text
COZY_TEXT_DARK  = { 72,  36,   8, 255}  // body text on parchment
COZY_TEXT_LIGHT = {240, 218, 175, 255}  // text on dark bg
COZY_GOLD       = {232, 198,  58, 255}  // selected/hover state
COZY_RED        = {178,  50,  24, 255}  // P1 accent
COZY_GREEN      = { 68, 140,  52, 255}  // Easy difficulty, success
```

Tất cả SDL_RenderClear dark navy được xóa khỏi UIManager render functions.
Helper `DrawCozyPanel(r, x, y, w, h, highlighted)` chuẩn hóa panel style.

### D — Icon System (UIManager.cpp)

11 icon textures load từ Garden Cozy Icons Pack:
```
ICON_SETTINGS, ICON_X, ICON_ARROW_L/R, ICON_PAUSE,
ICON_BACK, ICON_TRASH, ICON_TROPHY, ICON_SOUND, ICON_MUTE, ICON_BOOK
```

`DrawIcon(r, IconID, cx, cy, size, alpha=255)` — replace text placeholders:
- Nút X → `DrawIcon(ICON_X, ...)`
- Arrow selector difficulty → `DrawIcon(ICON_ARROW_L/R, ...)`
- Pause button header → `DrawIcon(ICON_PAUSE, ...)`
- Load screen: icon per action button

### E — Character Portrait System (UIManager.cpp)

Free Base by Cozy Fae pack — sprite sheet 2×3 frames (idle).
Frame 0 = góc trên-trái, dùng `SDL_QueryTexture` để tính kích thước.

**P1 (X/đỏ):** skin03 + red top + brown pants + short brown hair + blue eyes
**P2 (O/xanh):** skin07 + blue top + blue pants + medium yellow hair + green eyes

`DrawCharPortrait(r, layers[5], x, y, size)` — composite 5 layers bằng cách gọi `SDL_RenderCopy` 5 lần với cùng src/dst rect.

Fallback về colored square nếu sprite chưa load.

---

## Assets Sử Dụng

| Source Pack | Files | Dùng cho |
|---|---|---|
| Garden Cozy Icons | bg_menu/game.png, 11 icons | Background + UI buttons |
| Cozy Lands FREE | deco_tree/bush/flower.png | Background decorations |
| Kenney Input Prompts | keyboard_map.png | Settings > Controls tab |
| Free Base by Cozy Fae | 10 char sprite layers | Avatar portrait trong panels |
| AI Generated | board_01.jpg | Board surface texture |

---

## Kỹ Thuật Tổng Hợp

| Kỹ thuật | Áp dụng |
|---|---|
| `IMG_LoadTexture()` | Load PNG/JPG trực tiếp thành texture |
| Graceful degradation | Mọi texture đều có fallback procedural |
| `rand() % N` + `srand(time)` | Random board skin per ván |
| Layer compositing | 5 `SDL_RenderCopy` calls tạo character portrait |
| `SDL_QueryTexture` | Tính frame size từ sprite sheet runtime |
| Painter's Algorithm | Renderer_DrawBackground → Board → Pieces → UI → Overlay |

---

## Câu Hỏi Bảo Vệ

- `IMG_LoadTexture` vs `SDL_CreateTextureFromSurface` — khác nhau thế nào?
- Tại sao cần `srand(time(NULL))` khi khởi động?
- Graceful degradation là gì? Tại sao quan trọng trong game dev?
- Sprite compositing: tại sao 5 layer được vẽ lên cùng 1 dst rect cho ra kết quả đúng?
- Tại sao xóa `SDL_RenderClear` khỏi UIManager render functions?
