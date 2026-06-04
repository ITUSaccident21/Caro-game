# Tiến Độ Phát Triển — CaroGame SDL2

> Tài liệu này tổng hợp toàn bộ quá trình học, quyết định kỹ thuật và cải tiến.
> Dùng để ôn tập trước khi bảo vệ đồ án.

---

## Trạng Thái Hệ Thống (cập nhật 2026-06-02)

| Module | Trạng thái | Ghi chú |
|---|---|---|
| Game Logic (Model) | ✅ Hoàn thiện | CheckBoard, WinCheck, NewSession |
| AI (Minimax) | ✅ Hoạt động | EASY/MEDIUM/HARD, Alpha-Beta + move ordering + time limit |
| Rendering (SDL2) | ✅ Visual polish | Texture cache, stroke, ghost piece, win line, hover brackets |
| Animation | ✅ Hoạt động | State machine IDLE→WALK→PLACE→CELEBRATE |
| WinEffect | ✅ Hoạt động | GLOW→CONVERGE→ZOOM→ANNOUNCE |
| Audio | ✅ Cơ bản | 1 layer, chưa có variation |
| Save/Load | ✅ Hoàn thiện | File-based |
| UI/Menu | ✅ Hoạt động | |

---

## Danh Sách Decision Records

| ID | Chủ đề | Ngày | Trạng thái |
|---|---|---|---|
| [DR-001](decisions/DR-001-prerender-texture.md) | Pre-render X/O lên SDL_Texture | 2026-05-29 | ✅ Hoàn thành |
| [DR-002](decisions/DR-002-ai-benchmark-system.md) | Hệ thống benchmark AI | 2026-05-23 | ✅ Hoàn thành |
| [DR-003](decisions/DR-003-move-ordering.md) | Move ordering cho Alpha-Beta | 2026-05-23 | ✅ Hoàn thành |
| [DR-004](decisions/DR-004-selective-sorting.md) | Selective sorting theo depth | 2026-05-24 | ✅ Hoàn thành |
| [DR-005](decisions/DR-005-negamax-iterative-deepening.md) | Negamax + Iterative Deepening | 2026-05-24 | Sẵn sàng implement |
| [DR-006](decisions/DR-006-ui-overhaul.md) | UI Overhaul: 7 cải tiến visual + 4 màn hình mới | 2026-06-02 | ✅ Hoàn thành |
| [DR-007](decisions/DR-007-cozy-style-assets.md) | Cozy Style: tích hợp assets + cozy palette + icons + avatars | 2026-06-04 | ✅ Hoàn thành |

---

## Nhật Ký Phát Triển

| Ngày | File | Tóm tắt |
|---|---|---|
| 2026-05-22 | [journal/2026-05-22.md](journal/2026-05-22.md) | Đánh giá hệ thống, lên kế hoạch nâng cấp visual |
| 2026-05-23 | [journal/2026-05-23.md](journal/2026-05-23.md) | Phân tích AI, implement benchmark system |
| 2026-05-24 | [journal/2026-05-24.md](journal/2026-05-24.md) | Benchmark DR-003/004: nodes 35x, sort 200-6000x, realPrune 63-76%, viết báo cáo model |
| 2026-05-29 | [journal/2026-05-29.md](journal/2026-05-29.md) | Implement DR-001: texture cache cho quân cờ, bắt đầu nâng cấp visual |
| 2026-06-02 | [journal/2026-06-02.md](journal/2026-06-02.md) | DR-006: 7 cải tiến visual (ghost, stroke, win line, hover, splash, load, save) + 4 screens mới |

---

## Kiến Trúc Hệ Thống

| File | Nội dung |
|---|---|
| [architecture/overview.md](architecture/overview.md) | Module map, folder structure, dependency rules |
| [architecture/game-loop.md](architecture/game-loop.md) | Fixed timestep loop, App state machine, AI threading |
| [architecture/data-model.md](architecture/data-model.md) | Structs, enums, constants, encoding quy ước |
| [architecture/render-layers.md](architecture/render-layers.md) | 5 render layers, texture cache, WinEffect pipeline |

---

## Gợi Ý Cải Tiến Tiếp Theo

### AI — Kỹ thuật nâng cao (tham khảo [references/ai-worker-v2.js](references/ai-worker-v2.js))

| Kỹ thuật | Hiệu quả mong đợi | Độ khó |
|---|---|---|
| **Transposition Table** (Zobrist hash) | Tránh tính lại vị trí đã xét → giảm nodes đáng kể ở endgame | Cao |
| **Killer Moves** | Nhớ nước gây cắt tỉa tốt ở cùng depth → pruning sớm hơn | Trung bình |
| **PVS (Principal Variation Search)** | Window hẹp cho non-PV nodes → ít nodes hơn alpha-beta thông thường | Cao |
| **Iterative Deepening** | Tìm kiếm depth 1→2→...→N với time limit → luôn có nước tốt nhất kịp thời | Trung bình |

> File tham khảo JavaScript `ai-worker-v2.js` implement đủ 4 kỹ thuật trên. Đọc để hiểu ý tưởng trước khi port sang C++.

### Visual — Còn lại từ kế hoạch

| Hướng | Ghi chú |
|---|---|
| Hover ghost piece | Hiển thị quân mờ khi di chuột — ~10 dòng |
| Glow/shadow quân cờ | Cần texture biến thể với alpha blending |
| Win cells pulse glow | Thay blink bằng animation sáng lan ra |

---

## Khái Niệm Kỹ Thuật Cần Nắm

| File | Nội dung |
|---|---|
| [concepts/sdl2-rendering-pipeline.md](concepts/sdl2-rendering-pipeline.md) | Cách SDL2 render frame, CPU vs GPU |
| [concepts/animation-state-machine.md](concepts/animation-state-machine.md) | State machine cho animation nhân vật |
| [concepts/minimax-ai.md](concepts/minimax-ai.md) | Thuật toán AI Minimax trong game cờ |
| [concepts/ai-benchmark-metrics.md](concepts/ai-benchmark-metrics.md) | Chỉ tiêu đo lường AI, ngưỡng đánh giá, cách đọc log |
