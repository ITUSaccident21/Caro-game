# Render Pipeline — 5 Layers

## Thứ Tự Vẽ (mỗi frame)

```
SDL_RenderClear()              // xóa buffer, màu nền đen

Layer 1: Renderer_DrawBackground()    // nền xanh đậm + viền bàn cờ
Layer 2: Renderer_DrawBoard()         // bề mặt gỗ + lưới + star points
Layer 3: Renderer_DrawPieces()        // quân cờ đã đặt (GPU texture blit)
Layer 4: Renderer_DrawWinCells()      // highlight 5 ô thắng (blink)
         hoặc WinEffect_Render()      // overlay hiệu ứng chiến thắng
Layer 5: Renderer_DrawHover()         // ô vàng mờ tại con trỏ chuột
         Animation_Render()           // nhân vật đi bộ
         UIManager_RenderHUD()        // tên, điểm, lượt chơi

SDL_RenderPresent()            // flip buffer lên màn hình
```

---

## Texture Cache (DR-001)

Trước DR-001: `DrawX()` / `DrawO()` tính `sqrt` + scanline mỗi frame cho mỗi quân.

Sau DR-001:
```
Init (1 lần):
  SDL_CreateTexture(TEXTUREACCESS_TARGET)
  SDL_SetRenderTarget(renderer, s_texX)
  DrawX(...)                   // vẽ vào texture, không ra màn hình
  SDL_SetRenderTarget(renderer, NULL)

Per-frame:
  SDL_RenderCopy(r, s_texX, NULL, &dst)   // GPU blit, không tính toán
```

Module state:
```cpp
static SDL_Texture* s_texX = nullptr;    // texture quân X đỏ
static SDL_Texture* s_texO = nullptr;    // texture quân O xanh
static int          s_pieceSize = 0;     // = CELL_SIZE
```

**Yêu cầu:** renderer phải tạo với flag `SDL_RENDERER_TARGETTEXTURE`.

---

## Màu Sắc Chuẩn

| Thành phần | RGB |
|---|---|
| Background | (28, 48, 28) — xanh đậm |
| Viền bàn cờ | (90, 60, 25) — nâu |
| Bề mặt bàn cờ | (212, 172, 105) — vàng gỗ |
| Lưới | (95, 65, 28) — nâu đậm |
| Star points | (60, 40, 18) — nâu đen |
| Quân X | (210, 55, 55) — đỏ |
| Quân O | (55, 100, 210) — xanh dương |
| Hover | (255, 240, 0, 75) — vàng mờ |
| Win cells | (255, 220, 0, 170) — vàng sáng |

---

## WinEffect Overlay

Chạy sau khi AI/người chơi thắng, phủ lên toàn màn hình:

```
GLOW     → 5 ô thắng phát sáng
  ↓
CONVERGE → các ô bay vào giữa màn hình
  ↓
ZOOM     → phóng to với easing EaseOutBack
  ↓
ANNOUNCE → hiển thị tên người thắng
```

WinEffect dùng `Renderer_DrawSymbolX/O` trực tiếp (không qua texture cache) vì cần scale tùy ý.

---

## Liên Kết

- [concepts/sdl2-rendering-pipeline.md](../concepts/sdl2-rendering-pipeline.md) — SDL2 CPU vs GPU chi tiết
- [decisions/DR-001-prerender-texture.md](../decisions/DR-001-prerender-texture.md) — quyết định texture cache
