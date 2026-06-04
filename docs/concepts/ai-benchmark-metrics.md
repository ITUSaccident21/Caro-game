# Khái Niệm: AI Benchmark Metrics

> Giải thích ý nghĩa của từng chỉ tiêu đo lường AI,
> và tại sao chúng phản ánh chất lượng thuật toán.

---

## 1. Thời Gian Phản Hồi (timeMs)

**Đo bằng:** `std::chrono::high_resolution_clock`

```cpp
auto t0 = std::chrono::high_resolution_clock::now();
// ... AI tính toán ...
auto t1 = std::chrono::high_resolution_clock::now();
float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
```

**Tại sao quan trọng:** Người dùng cảm nhận trực tiếp. Nếu AI HARD mất > 1 giây,
trải nghiệm chơi bị phá vỡ — cảm giác game "đứng hình".

**Các yếu tố ảnh hưởng:**
- Depth: tăng 1 depth → time tăng ~b lần (b = branching factor)
- Board state: mid-game có nhiều candidates nhất → chậm nhất
- Move ordering: nước tốt được xét trước → pruning nhiều hơn → nhanh hơn

---

## 2. Số Nodes Đã Xét (nodesVisited)

**Đo bằng:** Counter tăng mỗi lần `Minimax()` được gọi

```
Cây Minimax depth=3, branching=25:
  Tầng 0 (root):    1 node
  Tầng 1:          25 nodes
  Tầng 2:         625 nodes
  Tầng 3:      15,625 nodes
  Tổng không pruning: ~16,276 nodes
```

**Tại sao quan trọng:** Nodes là đơn vị công việc thực sự.
Thời gian = nodes × thời gian/node. Giảm nodes → giảm thời gian tuyến tính.

**Khác nhau theo pha game:**
```
Early game (< 10 quân): ~15 candidates → ít nodes
Mid game (15-25 quân):  ~30-40 candidates → nhiều nodes nhất
Late game (> 35 quân):  candidates giảm lại → ít nodes hơn
```

---

## 3. Số Lần Cắt Tỉa (nodesPruned)

**Đo bằng:** Counter tăng mỗi lần `if (beta <= alpha) break;` được kích hoạt

**Ý nghĩa:** Mỗi lần pruning = cả một nhánh con không cần xét.
Nếu nhánh con có 100 nodes → tiết kiệm 100 lần tính toán.

**Lý thuyết Alpha-Beta:**
```
Không pruning:    b^d     nodes  (b=branching, d=depth)
Pruning tối ưu:   b^(d/2) nodes  → tương đương tăng gấp đôi depth!
Pruning thực tế:  b^(d×0.75) nodes (phụ thuộc move ordering)
```

---

## 4. Tỷ Lệ Cắt Tỉa (pruneRatio)

```
pruneRatio = nodesPruned / (nodesVisited + nodesPruned)
```

**Đây là chỉ tiêu quan trọng nhất** vì nó đo hiệu quả của Alpha-Beta
**độc lập với depth và board state**.

| pruneRatio | Ý nghĩa kỹ thuật |
|---|---|
| ~0% | Move order ngẫu nhiên hoàn toàn — worst case Alpha-Beta |
| 20-40% | Random order nhưng Alpha-Beta vẫn có tác dụng |
| 40-65% | Move ordering bắt đầu có hiệu quả |
| 65-80% | Gần với lý thuyết optimal ordering |
| > 80% | Rất tốt — cần move ordering thực sự thông minh |

**Tại sao pruneRatio thấp ở code hiện tại:**
`GetCandidates` trả về theo thứ tự quét bảng (row-major), không ưu tiên nước tốt.
Nước tốt nhất thường được xét ở giữa hoặc cuối → Alpha-Beta không kịp cắt.

---

## 5. Mối Liên Hệ Giữa Các Chỉ Tiêu

```
pruneRatio cao
    → nodesVisited thấp
        → timeMs thấp
            → UX tốt hơn
                → có thể tăng depth
                    → AI mạnh hơn
```

Đây là **vòng lặp cải tiến**. Benchmark giúp biết đang ở đâu trong vòng lặp này.

---

## 6. Cách Đọc Log

```
[2026-05-23 10:30:15] HARD d=6 move#12 | 143ms | 87,432 nodes | 5,201 pruned | 37.2% | root=28
```

| Field | Giá trị | Nhận xét |
|---|---|---|
| HARD d=6 | depth=6 | Chế độ khó nhất |
| move#12 | Nước thứ 12 | Mid-game — candidates nhiều nhất |
| 143ms | 143 milliseconds | Chấp nhận được (< 200ms) |
| 87,432 nodes | Nodes đã xét | Tốt cho depth 6 mid-game |
| 5,201 pruned | Nodes đã cắt | |
| 37.2% | pruneRatio | Cần cải thiện move ordering |
| root=28 | 28 candidates ở root | Bình thường cho mid-game |

---

## Liên Kết

- Decision: [DR-002 Benchmark System](../decisions/DR-002-ai-benchmark-system.md)
- Decision: [DR-003 Move Ordering](../decisions/DR-003-move-ordering.md) *(sắp có)*
- Source: [AIPlayer.cpp](../../CaroGameSDL2/src/ai/AIPlayer.cpp)
