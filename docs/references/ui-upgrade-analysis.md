# Phân Tích & Đề Xuất Nâng Cấp UI — 2026-06-02

> Dựa trên 2 mẫu tham khảo trong thư mục `tham khảo/`.
> Người dùng thiên về Mẫu 2 (Pixel Art Nhật Bản).

---

## Nhận Xét Cốt Lõi

Mẫu 2 hơn Mẫu 1 không phải vì pixel art đẹp hơn, mà vì **design consistency**:
mọi màn hình dùng cùng font, màu, border, mascot — kể cùng một câu chuyện từ đầu đến cuối.

---

## Phân Tích Từng Màn Hình

### Main Menu
- Mẫu 2: background phong cảnh, title lớn, 4 nút gỗ dọc
- Mẫu 1: tương tự nhưng 5 nút (có How to play, Sound riêng)
- **Cần thêm:** nút Manual + Settings + About vào menu hiện tại

### Màn Hình Chờ / Splash
- Mẫu 2: visual novel — chữ chạy trên background trước khi vào menu
- **Đơn giản hoá:** hiện logo + loading bar vài giây → fade sang menu

### Chọn Chế Độ + Luật Chơi
- Mẫu 2: 3 thẻ Tarot — hover → hiện mô tả luật ngay trên thẻ
- Màn hình độc đáo nhất của Mẫu 2
- **Cần tách** khỏi name input thành màn hình riêng

### Game Screen
| Yếu tố | Mẫu 2 | Hiện tại |
|---|---|---|
| Bàn cờ | **Trong suốt**, checkerboard | Gỗ đặc |
| HUD | **2 sidebar** trái/phải có avatar | Chưa rõ |
| Header | "RENJU — VÁN 1" căn giữa | Không có |
| Nút in-game | Pause (II) + Save (📷) góc phải | Chỉ phím tắt |
| Lượt chơi | Hiển thị trên header | Không hiển thị |

**3 thay đổi impact cao nhất:** bàn cờ trong suốt, 2 sidebar, header bar.

### Win / Lose / Tie
- Mẫu 2: mascot quả đào nở tung + chữ "X THẮNG!" lớn trên bàn cờ
- Nút THOÁT + CHƠI TIẾP inline trên header, không che toàn màn hình
- Hiện tại đã có WinEffect tốt — chỉ cần thêm nút kết quả rõ hơn

### Escape / Pause Menu
- Mẫu 2: scroll dọc hiện giữa màn hình khi ESC — 4 lựa chọn:
  Tiếp tục / Chơi lại / Lưu / Thoát
- **Hiện tại không có** — ESC thoát thẳng ra menu
- Đây là UX cơ bản cần thêm

### Settings (4 tab)
- Mẫu 2: **ÂM THANH | PHÍM CHƠI | LUẬT CHƠI | THÔNG TIN**
- 1 panel, 4 tab, mỗi tab 1 nội dung — gọn, không lộn xộn
- **Nên copy gần như nguyên xi** — layout hoàn toàn làm được bằng SDL2 primitives

### Load / Save
- Mẫu 2: panel tối, danh sách file, 3 nút (TẢI / ĐỔI TÊN / XÓA)
- Save dialog: popup nhỏ nhập tên file
- Đã có cơ sở — chỉ cần redesign visual

---

## Phân Biệt Feasibility

| Phần | Mô tả | Khả năng |
|---|---|---|
| **Layout & cấu trúc** | Sidebar, tab, transparent board, escape menu... | ✅ 100% bằng SDL2 code |
| **Pixel art visual** | Background Fuji, avatar, mascot quả đào, tarot card | ⚠️ Cần file PNG |

Bạn đã có `assets/sprites/` — nếu chuẩn bị thêm PNG đơn giản, toàn bộ layout Mẫu 2 đều làm được.

---

## Đề Xuất Thứ Tự Thực Hiện

### Giai đoạn 1 — Layout (không cần assets)
1. Transparent/semi-transparent board
2. 2 sidebar HUD (trái/phải) với tên, điểm, lượt
3. Escape/Pause menu overlay (scroll dọc)
4. Settings 4-tab panel

### Giai đoạn 2 — Screens mới (không cần assets)
5. Splash screen (logo + fade)
6. Mode selection screen — tách khỏi name input, dạng 3 lựa chọn
7. Load/Save redesign theo layout Mẫu 2

### Giai đoạn 3 — Visual polish (cần assets PNG)
8. Background image
9. Avatar/mascot sprites
10. Pixel font TTF

---

## App State Machine Mở Rộng (Mục Tiêu)

```
STATE_SPLASH
  ↓
STATE_MAIN_MENU
  ├── [Mới]    → STATE_NAME_INPUT → STATE_MODE_SELECT → STATE_PLAYING
  ├── [Tải]    → STATE_LOAD_GAME
  ├── [Cài đặt]→ STATE_SETTINGS
  ├── [Manual] → (tab trong Settings — LUẬT CHƠI + PHÍM CHƠI)
  └── [Thoát]  → STATE_EXIT

STATE_PLAYING
  └── [ESC]   → STATE_PAUSE_MENU (overlay)
                  ├── Tiếp tục → STATE_PLAYING
                  ├── Chơi lại → reset STATE_PLAYING
                  ├── Lưu      → save dialog
                  └── Thoát    → STATE_MAIN_MENU
```

---

## Câu Hỏi Còn Mở (chưa quyết định)
- Assets PNG đã có chưa? (background, avatar, font pixel art)
- Avatar selection: chọn màu quân cờ hay chọn nhân vật có icon riêng?
- Có đồng hồ đếm ngược không? (Mẫu 1 có, Mẫu 2 không)
- Visual Novel intro có muốn làm không?
