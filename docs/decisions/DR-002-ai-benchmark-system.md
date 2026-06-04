# DR-002 — Hệ Thống Benchmark AI

**Ngày đề xuất:** 2026-05-23
**Ngày implement:** 2026-05-23
**Trạng thái:** Hoàn thành
**Liên quan:** [AIPlayer.cpp](../../CaroGameSDL2/src/ai/AIPlayer.cpp), [AIPlayer.h](../../CaroGameSDL2/src/ai/AIPlayer.h)

---

## Vấn Đề

Hiện tại không có cách nào đo được AI đang hoạt động tốt hay tệ.
Trước khi cải tiến bất cứ điều gì, cần có **baseline** — con số đo được để so sánh
sau khi thay đổi.

Không có benchmark → không biết cải tiến có thực sự tốt hơn không.

---

## Các Lựa Chọn Đã Xem Xét

### Lựa Chọn A — Chỉ đo thời gian (đơn giản)
- **Pros:** Dễ implement, dễ hiểu
- **Cons:** Không giải thích được TẠI SAO nhanh hay chậm
- **Đánh giá:** Không đủ để học và cải tiến

### Lựa Chọn B — Đo đầy đủ: time + nodes + pruning *(được chọn)*
- **Pros:** Hiểu được internals của thuật toán, so sánh trước/sau meaningful
- **Cons:** Thêm ~30 dòng code, static counter cần reset đúng chỗ
- **Đánh giá:** Phù hợp với mục tiêu học sâu

### Lựa Chọn C — Profiler ngoài (VS Profiler, gprof)
- **Pros:** Rất chi tiết, không cần sửa code
- **Cons:** Overhead tool, không integrate vào log workflow của dự án
- **Đánh giá:** Overkill cho scope này

---

## Quyết Định: Lựa Chọn B

**4 chỉ tiêu đo:**

| Chỉ tiêu | Ý nghĩa |
|---|---|
| `timeMs` | Thời gian wall-clock cho 1 nước AI |
| `nodesVisited` | Số lần Minimax được gọi đệ quy |
| `nodesPruned` | Số lần cắt Alpha-Beta thực sự xảy ra |
| `pruneRatio` | nodesPruned / tổng nodes — hiệu quả pruning |

**Thêm context để log có nghĩa:**

| Context | Ý nghĩa |
|---|---|
| `depth` | Độ sâu tìm kiếm (2/4/6) |
| `rootCandidates` | Số nước xét ở tầng gốc |
| `moveNumber` | Nước thứ bao nhiêu (early/mid/late game) |

---

## Ngưỡng Đánh Giá

### Thời gian phản hồi (timeMs)

| Difficulty | Tệ | Chấp nhận được | Tốt | Xuất sắc |
|---|---|---|---|---|
| EASY (depth 2) | ≥ 20ms | < 20ms | < 5ms | < 1ms |
| MEDIUM (depth 4) | ≥ 150ms | < 150ms | < 50ms | < 15ms |
| HARD (depth 6) | ≥ 500ms | < 500ms | < 200ms | < 80ms |

> Ngưỡng "chấp nhận được" = game không lag với mắt thường.
> Ngưỡng "xuất sắc" = người dùng không cảm nhận được delay.

### Hiệu quả pruning (pruneRatio)

| Mức | Ngưỡng | Ý nghĩa |
|---|---|---|
| Tệ | < 20% | Alpha-Beta gần như không pruning được — candidate order ngẫu nhiên hoàn toàn |
| Chấp nhận | 20–45% | Pruning cơ bản hoạt động |
| Tốt | 45–65% | Move ordering có tác dụng |
| Xuất sắc | > 65% | Gần với lý thuyết tối ưu (70–80%) |

### Nodes visited (HARD depth 6, mid-game ~20 quân)

| Mức | Ngưỡng |
|---|---|
| Tệ | > 500,000 nodes |
| Chấp nhận | 100,000–500,000 |
| Tốt | 20,000–100,000 |
| Xuất sắc | < 20,000 |

---

## Cách Theo Dõi

**Mỗi lần AI tính nước:**
1. In ra console (debug mode) theo format chuẩn
2. Append vào `Log/ai_benchmark.log` với timestamp

**Format log:**
```
[2026-05-23 10:30:15] HARD d=6 move#12 | 143ms | 87,432 nodes | 5,201 pruned | 37.2% | root=28
```

**Mỗi lần cải tiến thuật toán:**
1. Chạy vài ván, ghi lại log
2. Copy kết quả vào journal ngày đó
3. So sánh với baseline trong PROGRESS.md

---

## Khái Niệm Cần Hiểu

Xem [concepts/ai-benchmark-metrics.md](../concepts/ai-benchmark-metrics.md)

---

## Kết Quả Mong Đợi

Sau khi implement, mỗi nước AI sẽ tự động log:
- Phát hiện được bottleneck ở depth nào
- Thấy pruneRatio thấp → biết cần move ordering
- So sánh trước/sau khi cải tiến có số liệu cụ thể

---

## Liên Kết

- Journal: [2026-05-23](../journal/2026-05-23.md)
- Concept: [ai-benchmark-metrics.md](../concepts/ai-benchmark-metrics.md)
- DR tiếp theo: [DR-003](DR-003-move-ordering.md) *(dự kiến)*
