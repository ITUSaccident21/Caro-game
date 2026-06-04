# DR-006 — UI Overhaul: Visual Polish & New Screens

**Ngày implement:** 2026-06-02
**Trạng thái:** ✅ Hoàn thành
**Liên quan:** Renderer.cpp, UIManager.cpp, UIManager.h, App.cpp, GameDef.h

---

## Bối Cảnh

Sau khi phân tích 2 mẫu tham khảo (Casual Wood và Pixel Art Nhật Bản), xác định được
các cải tiến UI có thể thực hiện thuần code, không cần asset PNG.

---

## Những Gì Đã Thực Hiện

### A — Ghost Piece (Renderer.cpp)
**Kỹ thuật:** `SDL_SetTextureAlphaMod(tex, 75)` trước `SDL_RenderCopy`, restore về 255 sau.
**Kết quả:** Quân cờ mờ hiển thị tại ô hover — preview nước đi trước khi đặt.
**Điều kiện:** Chỉ hiện khi đến lượt người chơi thật (không hiện khi AI đang nghĩ).

### B — Stroke cho quân cờ (Renderer.cpp — CreatePieceTextures)
**Kỹ thuật:** Painter's Algorithm trong texture — vẽ shape tối/lớn hơn TRƯỚC, vẽ màu thật LÊN TRÊN.
Stroke X: `DrawX(half+2, thickness=6)` màu tối → `DrawX(half, thickness=4)` màu đỏ.
**Kết quả:** X và O có viền tối bao quanh — sắc nét hơn trên nền checkerboard.

### C — Win cells: đường thắng pulsing (Renderer.cpp)
**Kỹ thuật:** Thay blink (on/off 4Hz) bằng `DrawThickLine` với alpha dao động:
`alpha = 120 + 135 * sin(t * 2π)` — sóng sin tạo hiệu ứng glow liên tục.
Hai lớp: đường vàng dày (6px) + đường trắng mỏng ở giữa (2px) để có chiều sâu.

### D — Corner brackets cho hover (Renderer.cpp)
**Kỹ thuật:** 8 `SDL_RenderDrawLine` vẽ 4 góc L-shape (mỗi góc 2 đoạn ngắn 8px).
Thay ô đặc màu vàng mờ → tinh tế hơn, không che ghost piece phía dưới.

### E — Splash Screen (UIManager.cpp + App.cpp)
**Kỹ thuật:** Alpha fade 3 phase:
- 0→0.6s: fade in (`alpha = 255 * t/0.6`)
- 0.6→2.0s: hold (`alpha = 255`)
- 2.0→2.6s: fade out (`alpha = 255 * (1 - progress)`)

`App_Init` bây giờ bắt đầu từ `STATE_SPLASH` thay vì `STATE_MENU`.
Bấm phím bất kỳ → bỏ qua splash ngay lập tức.

### F — Load Screen Redesign (UIManager.cpp)
**Cải tiến:**
- Panel danh sách với row highlight + icon ▶ cho file được chọn
- 3 nút bên phải: TẢI GAME | XÓA | QUAY LẠI
- Hover effect trên nút (màu sáng hơn khi di chuột)
- Phím Delete → xóa file đang chọn
- `std::remove()` để xóa file, `GetSaveFiles()` để refresh list

### G — Save Dialog Redesign (UIManager.cpp)
**Cải tiến:**
- Panel lớn hơn, viền đôi (outer + inner)
- Tiêu đề "LUU GAME" màu vàng đất
- Input field có padding rõ ràng hơn

---

## Kỹ Thuật Tổng Hợp Đã Dùng

| Kỹ thuật | Dùng ở đâu |
|---|---|
| Painter's Algorithm | Stroke (B), ghost piece (A), corner brackets (D) |
| Alpha Blending | Tất cả — ghost, stroke, win line, splash fade, panel overlay |
| Sin wave animation | Win line pulse (C), splash fade (E) |
| SDL_SetTextureAlphaMod | Ghost piece (A) — thay đổi alpha texture tạm thời |
| Deferred state transition | Menu bounce (từ DR trước) + Splash auto-advance (E) |

---

## App State Machine Hiện Tại

```
STATE_SPLASH → (2.6s hoặc bấm phím) → STATE_MENU
STATE_MENU → STATE_NAME_INPUT → STATE_PLAYING
          → STATE_LOAD_GAME
          → STATE_SETTINGS
STATE_PLAYING → (ESC) → Pause Overlay
```

---

## Câu Hỏi Bảo Vệ

- `SDL_SetTextureAlphaMod` hoạt động thế nào? Tại sao cần restore về 255?
- Tại sao stroke phải vẽ lớp tối TRƯỚC lớp màu chính?
- Sin wave tạo hiệu ứng pulse như thế nào? Viết công thức.
- Deferred transition là gì? Tại sao cần nó cho bounce animation?
