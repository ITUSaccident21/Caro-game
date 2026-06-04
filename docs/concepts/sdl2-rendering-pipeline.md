# Khái Niệm: SDL2 Rendering Pipeline

> Tài liệu này giải thích cách SDL2 render frame — nền tảng để hiểu mọi
> quyết định về performance và visual quality trong dự án.

---

## Luồng Render Cơ Bản

```
Game Loop (60 FPS)
    │
    ├─ SDL_RenderClear()        ← xóa frame cũ
    │
    ├─ [Draw calls...]          ← gọi các hàm vẽ
    │   ├─ SDL_RenderFillRect()
    │   ├─ SDL_RenderDrawLine()
    │   └─ SDL_RenderCopy()     ← copy texture lên screen
    │
    └─ SDL_RenderPresent()      ← flip buffer, hiển thị lên màn hình
```

---

## CPU vs GPU — Điều Quan Trọng Nhất

| | CPU path | GPU path |
|---|---|---|
| **Cách vẽ** | Tính pixel thủ công (scanline loop) | Gọi lệnh blit, GPU xử lý |
| **Ví dụ trong code** | `DrawFilledCircle()`, `DrawO()` | `SDL_RenderCopy()` |
| **Tốc độ** | Chậm hơn, tốn CPU | Nhanh, song song |
| **Khi nào dùng** | Prototype, primitive đơn giản | Production, texture-based |

**Quy tắc vàng:** Vẽ 1 lần lên texture (CPU), hiển thị nhiều lần bằng GPU.

---

## SDL_Surface vs SDL_Texture

```
SDL_Surface (RAM — CPU)
├── Có thể đọc/ghi pixel trực tiếp
├── Dùng để xử lý ảnh, load file
└── Phải convert sang Texture trước khi render

SDL_Texture (VRAM — GPU)
├── Không đọc/ghi pixel trực tiếp được
├── Render cực nhanh qua SDL_RenderCopy
└── Tạo từ: SDL_CreateTexture() hoặc SDL_CreateTextureFromSurface()
```

---

## Render Target Texture — Kỹ Thuật Pre-render

Cho phép "redirect" output của renderer vào một texture thay vì màn hình:

```cpp
// Bước 1: Tạo texture có thể làm render target
SDL_Texture* tex = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_TARGET,   // ← quan trọng
    width, height
);

// Bước 2: Redirect renderer vào texture
SDL_SetRenderTarget(renderer, tex);
SDL_RenderClear(renderer);

// Bước 3: Vẽ bất cứ thứ gì — output vào texture, không phải màn hình
DrawX(renderer, cx, cy, size);

// Bước 4: Restore về màn hình
SDL_SetRenderTarget(renderer, NULL);

// Sau đó mỗi frame chỉ cần:
SDL_RenderCopy(renderer, tex, NULL, &dstRect);  // GPU blit
```

---

## Alpha Blending — Nền Tảng Cho Visual Effects

```cpp
// Bật alpha blending cho renderer
SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

// Vẽ màu với alpha (A = 0 trong suốt, 255 đục)
SDL_SetRenderDrawColor(renderer, 255, 200, 0, 80);  // vàng mờ
SDL_RenderFillRect(renderer, &rect);
```

**Ứng dụng:**
- Hover highlight: vẽ rect trắng alpha 60 lên ô chuột đang trỏ
- Drop shadow: vẽ symbol đen alpha 100, offset +3,+3 trước symbol gốc
- Glow effect: vẽ nhiều vòng tròn, alpha giảm dần ra ngoài

---

## Double Buffering — Tại Sao Không Bị Nhấp Nháy

SDL2 dùng **double buffer** theo mặc định:
- `Back buffer`: nơi đang vẽ (không nhìn thấy)
- `Front buffer`: đang hiển thị trên màn hình

`SDL_RenderPresent()` swap hai buffer → không bao giờ thấy frame chưa vẽ xong.

---

## Liên Kết

- Decision: [DR-001 Pre-render Texture](../decisions/DR-001-prerender-texture.md)
- Decision: [DR-002 Drop Shadow Effect](../decisions/DR-002-drop-shadow-effect.md) *(sắp có)*
