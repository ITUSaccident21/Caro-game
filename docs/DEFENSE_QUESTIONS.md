# Câu Hỏi Ôn Tập Bảo Vệ Đồ Án

> Mỗi câu hỏi gắn với một Decision Record cụ thể.
> Mục tiêu: giải thích được bằng lời, vẽ được sơ đồ nếu cần.

---

## DR-001 — Texture Cache (Renderer)

1. `SDL_Surface` vs `SDL_Texture` khác nhau thế nào? Cái nào nằm trên CPU, cái nào trên GPU?
2. Tại sao phải dùng `SDL_TEXTUREACCESS_TARGET` thay vì `SDL_TEXTUREACCESS_STATIC`?
3. `SDL_SetRenderTarget(r, NULL)` làm gì? Điều gì xảy ra nếu quên gọi nó?
4. Tại sao `SDL_CreateRenderer` cần flag `SDL_RENDERER_TARGETTEXTURE`?
5. Lợi ích hiệu năng cụ thể: bao nhiêu phép tính `sqrt` được tiết kiệm mỗi frame?

---

## DR-002 — Benchmark System (AI)

6. `pruneRatio` cũ đo gì? Tại sao nó không phản ánh đúng hiệu quả pruning?
7. `realPruneRatio` = `candidatesSkipped / (nodesVisited + candidatesSkipped)` — giải thích ý nghĩa.
8. Tại sao `candidatesSkipped` chính xác hơn `nodesPruned` (break events)?
9. Log benchmark có format như thế nào? Đọc một dòng log và giải thích từng field.

---

## DR-003 — Move Ordering

10. Move ordering ảnh hưởng thế nào đến Alpha-Beta? Giải thích bằng ví dụ số (có trong DR-003).
11. `QuickScore(r, c, color)` tính gì? Tại sao cộng cả điểm tấn công lẫn điểm phòng thủ?
12. Tại sao dùng `QuickScore` (local) thay vì `EvaluateBoard` (global) để sort?
13. Tại sao phải sort trong **cả hai** `Minimax()` và `AI_FindBestMove()`? Bỏ một chỗ có sao không?
14. Trước DR-003: HARD ~858s. Sau: ~25s. Giải thích tại sao 35x nodes giảm.

---

## DR-004 — Selective Sorting

15. Tại sao chỉ sort khi `depth >= 3`, bỏ qua depth nhỏ hơn?
16. "Sort overhead" là gì? Nó ăn vào hiệu năng như thế nào?
17. Trade-off của selective sorting: mất gì, được gì?
18. `s_sortCalls` giảm 200-6000x nghĩa là gì về mặt thực tế?

---

## DR-005 — Bottleneck Analysis & Fixes

19. Hai điểm nghẽn chính của Minimax hiện tại là gì?
20. `MAX_CANDS = 20` nghĩa là gì? Tại sao 20 mà không phải 10 hay 50?
21. Candidate cap ảnh hưởng thế nào đến **chất lượng** AI? (Có thể bỏ lỡ nước hay không?)
22. Time limit hoạt động thế nào? Khi hết time, AI chọn nước gì?
23. `EvaluateBoard` bottleneck: tại sao dùng cửa sổ `lastR±4` thay vì quét toàn bàn?
24. Priority 1 (win) và Priority 2 (block) — tại sao không dùng MAX_CANDS cap ở đây?

---

## DR-006 — UI Overhaul

25. **Painter's Algorithm** là gì? Liệt kê 5 layer trong `App_Render()` theo đúng thứ tự.
26. **Alpha Blending**: `SDL_BLENDMODE_BLEND` vs `SDL_BLENDMODE_NONE` — khác nhau thế nào?
27. **Stroke effect** (B): tại sao vẽ lớp tối/lớn hơn TRƯỚC rồi vẽ màu thật LÊN TRÊN?
28. **Ghost piece** (A): `SDL_SetTextureAlphaMod(tex, 75)` làm gì? Tại sao phải restore về 255?
29. **Sin wave animation** (C): viết công thức tạo alpha pulsing. Tại sao dùng sin thay if/else?
30. **Deferred transition** (menu bounce + splash): giải thích cơ chế. Tại sao không chuyển màn ngay?
31. **Checkerboard board**: tại sao alpha ~155 thay vì 255 (đặc)? Background "thấu qua" bao nhiêu %?
32. **Corner brackets** (D): tại sao 4 góc L-shape trông tốt hơn ô đặc?

---

## Câu Hỏi Tổng Hợp

33. Vẽ sơ đồ kiến trúc module: ai phụ thuộc vào ai? (`game/` ↔ `sdl/` ↔ `ai/`)
34. Vòng lặp Fixed Timestep hoạt động thế nào? Tại sao không dùng vòng lặp thường?
35. AI chạy trên thread riêng bằng `std::async` — tại sao cần điều này?
36. `_GAMESTATE` encoding: `c=-1` là gì, `c=1` là gì, `turn=true` là gì?
37. Save/Load dùng binary format — tại sao không dùng text/JSON?
38. Animation state machine: IDLE → WALK → PLACING → CELEBRATE. Ai gọi `App_PlacePiece`?

---

*Gợi ý ôn: giải thích từng câu bằng lời trước gương, không nhìn code.*
