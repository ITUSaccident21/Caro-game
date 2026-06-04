# DR-004 — Selective Sorting theo Depth

**Ngày đề xuất:** 2026-05-24
**Ngày implement:** 2026-05-24
**Trạng thái:** ✅ Hoàn thành
**Liên quan:** [AIPlayer.cpp](../../CaroGameSDL2/src/ai/AIPlayer.cpp)
**Baseline:** DR-003 → HARD ~25,000ms, nodes ~4,368,338

---

## Vấn Đề

DR-003 giảm nodes 35x nhưng thời gian chỉ giảm 11x.
Nguyên nhân: `SortCandidates` được gọi tại **mọi node** trong cây đệ quy —
kể cả các node ở tầng sâu (depth=1,2) nơi số node nhiều nhất.

```
Cây depth=6, branching≈30:

Tầng (depth còn lại)   Số nodes (ước tính sau pruning)
  depth=6 (root)    :         1  node  → 1 lần sort
  depth=5           :        30  nodes → 30 lần sort
  depth=4           :       300  nodes → 300 lần sort
  depth=3           :     2,000  nodes → 2,000 lần sort
  depth=2           :    10,000  nodes → 10,000 lần sort  ← tốn nhất
  depth=1           :    40,000  nodes → 40,000 lần sort  ← tốn nhất
```

~95% chi phí sort nằm ở depth=1 và depth=2 — nhưng lợi ích pruning ở đó rất nhỏ
vì alpha/beta đã bị giới hạn chặt từ các tầng trên.

---

## Ý Tưởng

Thêm ngưỡng depth: chỉ sort khi `depth >= SORT_THRESHOLD`.

```
Không sort depth 1,2  →  bỏ ~95% chi phí sort
Vẫn sort depth 3,4,5  →  giữ ~80% lợi ích pruning
```

---

## Tại Sao Sort Ở Tầng Sâu Ít Có Giá Trị?

Tại depth=1 (một bước trước leaf):
- Alpha và beta đã được siết chặt từ các tầng trên
- Phần lớn candidates đã bị cắt trước khi đến tầng này
- Số candidates còn lại ít → sort gain nhỏ
- Nhưng số nodes nhiều → sort cost cao

Tại depth=5 (gần root):
- Alpha/beta còn rộng → nhiều khả năng cắt sau sort
- Ít nodes → sort cost thấp
- **Đây là nơi sort có lợi nhất**

---

## Quyết Định: Threshold = 3

```cpp
// Trong Minimax() — thêm điều kiện:
if (depth >= 3)
    SortCandidates(state, cands, nCands, color);
```

**Tại sao 3?**
- depth >= 4: an toàn, chắc chắn có lợi
- depth >= 3: vẫn có lợi, cắt được ~85% chi phí sort
- depth >= 2: tiết kiệm ít hơn, lợi ích pruning giảm nhanh
- Có thể tune lại dựa trên benchmark thực tế

---

## Kết Quả Kỳ Vọng

| Chỉ tiêu | DR-003 | DR-004 kỳ vọng |
|---|---|---|
| Sort calls (HARD) | ~4,000,000 | ~52,000 (~98% ít hơn) |
| HARD timeMs | ~25,000ms | < 3,000ms |
| pruneRatio | 5–8% | Tương đương hoặc cao hơn |
| Chất lượng nước đi | Không đổi | Không đổi |

---

## Câu Hỏi Cần Trả Lời Được Khi Bảo Vệ

- Tại sao sort ở tầng depth thấp (gần leaf) ít có giá trị?
  → Alpha/beta đã siết chặt từ tầng trên, ít candidates còn lại, lợi ích nhỏ
- Threshold=3 chọn như thế nào — có phải con số ma thuật không?
  → Không — là điểm cân bằng giữa sort cost và pruning benefit, có thể tune
- Bỏ sort ở depth=1,2 có làm AI yếu hơn không?
  → Không — kết quả minimax không đổi, chỉ nhanh hơn vì bỏ overhead vô ích

---

## Liên Kết

- Tiền đề: [DR-003](DR-003-move-ordering.md)
- Journal: [2026-05-24](../journal/2026-05-24.md)
