# DR-001 — Pre-render X/O lên SDL_Texture

**Ngày đề xuất:** 2026-05-22
**Ngày implement:** 2026-05-29
**Trạng thái:** ✅ Hoàn thành
**Liên quan:** [Renderer.cpp](../../CaroGameSDL2/src/sdl/Renderer.cpp)

---

## Vấn Đề

Hiện tại `DrawX()` và `DrawO()` được gọi **mỗi frame** (60 FPS) cho mỗi quân cờ trên bảng.
Trong worst case (bảng gần đầy ~200 quân), hệ thống gọi hàm scanline 200 lần/frame.

```cpp
// Renderer.cpp — được gọi lại từng frame
static void DrawFilledCircle(SDL_Renderer* r, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrt(radius*radius - dy*dy);  // sqrt mỗi scanline!
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}
```

**Hệ quả:** CPU phải tính lại scanline + sqrt cho các quân cờ không thay đổi.

---

## Các Lựa Chọn Đã Xem Xét

### Lựa Chọn A — Giữ nguyên (scanline mỗi frame)
- **Pros:** Đơn giản, không cần quản lý texture lifecycle
- **Cons:** CPU bị chiếm dụng không cần thiết, không scale tốt
- **Đánh giá:** Không phù hợp khi muốn thêm visual effects phức tạp hơn

### Lựa Chọn B — Pre-render lên SDL_Texture *(được chọn)*
- **Pros:** Vẽ scanline 1 lần lúc init → mỗi frame chỉ `SDL_RenderCopy()` (GPU blit)
- **Cons:** Cần quản lý `SDL_Texture*` lifecycle (tạo/destroy)
- **Đánh giá:** Đúng pattern của SDL2, mở đường cho glow/shadow effects

### Lựa Chọn C — Dùng SDL_image để load sprite PNG
- **Pros:** Chất lượng hình cao nhất, artist-friendly
- **Cons:** Cần file asset bên ngoài, dependency thêm
- **Đánh giá:** Phù hợp cho dự án lớn hơn, overkill cho hiện tại

---

## Quyết Định: Lựa Chọn B

**Lý do:**
1. SDL_Texture được xử lý bởi GPU — CPU chỉ gọi lệnh blit, không tính toán pixel
2. Pattern chuẩn trong SDL2 game development
3. Không thay đổi logic game, chỉ thay đổi cách render
4. Mở đường cho alpha blending (glow, shadow) dễ dàng hơn

---

## Kế Hoạch Implement

```cpp
// Renderer.h — thêm
bool  Renderer_CreatePieceTextures();
void  Renderer_DestroyPieceTextures();
void  Renderer_DrawCachedX(int cx, int cy, int size);
void  Renderer_DrawCachedO(int cx, int cy, int size);

// Renderer.cpp — logic mới
// Init: tạo SDL_Texture, set render target, vẽ X/O một lần, restore target
// Per-frame: SDL_RenderCopy với dst rect tính từ cx/cy/size
```

---

## Khái Niệm Cần Hiểu

**SDL_Texture vs SDL_Surface:**
- `SDL_Surface` = buffer trong RAM (CPU)
- `SDL_Texture` = buffer trong VRAM (GPU)
- `SDL_RenderCopy()` = GPU copy texture → framebuffer, cực nhanh

**Render Target Texture:**
```
SDL_SetRenderTarget(renderer, myTexture);  // redirect output lên texture
// ... vẽ bất cứ thứ gì ...
SDL_SetRenderTarget(renderer, NULL);       // restore về screen
```

---

## Kết Quả Mong Đợi

- CPU usage giảm đáng kể khi bảng có nhiều quân
- Mở đường để vẽ shadow/glow bằng cách tạo thêm texture biến thể
- FPS ổn định hơn khi thêm visual effects

---

## Liên Kết

- Concept: [SDL2 Rendering Pipeline](../concepts/sdl2-rendering-pipeline.md)
- Journal: [2026-05-22](../journal/2026-05-22.md)
