# DR-005 — Nâng cấp AI: Negamax + Iterative Deepening

**Ngày đề xuất:** 2026-05-24
**Trạng thái:** Sẵn sàng implement
**Liên quan:** [AIPlayer.cpp](../../CaroGameSDL2/src/ai/AIPlayer.cpp)

---

## Phần 1 — Báo Cáo Model Hiện Tại

### Số Liệu Thực Tế Qua Các Phiên Bản

#### HARD (depth=6) — Timeline cải tiến

| Phiên bản | Root≈8 | Root≈38 | Root≈49 | Root≈55 | realPrune | sortCalls |
|---|---|---|---|---|---|---|
| **Baseline** (B3, trước DR-003) | 611ms | 285,844ms | 162,539ms | 858,010ms | ~2% | N/A |
| **+DR-003** Move ordering (B4) | — | 25,289ms | 25,041ms | — | ~7%* | ~4,000,000 |
| **+DR-004** Selective sort (B5) | 437ms ✅ | 5,190ms | 6,077ms | 16,485ms | **63–76%** | 683–19,613 |

> \* pruneRatio B4 thấp do metric cũ đo sai (break events, không phải candidates skipped).
> B5 dùng metric mới `realPruneRatio = candidatesSkipped / (nodes + skipped)` — chính xác hơn.

#### HARD — Cải thiện tổng thể (Baseline → B5)

| Chỉ tiêu | Trước | Sau | Cải thiện |
|---|---|---|---|
| Worst case time | 858,010ms | 16,485ms | **52x nhanh hơn** |
| Mid-game (root≈38) | 285,844ms | 5,190ms | **55x nhanh hơn** |
| First move (root≈8) | 611ms | 437ms | **1.4x** |
| realPruneRatio | ~2% | 63–76% | |
| sortCalls | N/A | 683–19,613 | Giảm ~200–6,000x vs B4 |

#### EASY (depth=2) — Baseline B2

| Move# | Time | Nodes | Rating |
|---|---|---|---|
| Đầu ván | 0.1ms | 137 | EXCELLENT |
| Cuối ván | 77.7ms | 8,162 | SLOW |

> EASY bị chậm cuối ván do candidate generation tăng — depth=2 không đủ để pruning có tác dụng mạnh.

#### MEDIUM (depth=4) — Baseline B1

| Move# | Time | Nodes | Rating |
|---|---|---|---|
| Đầu ván | 286ms | 179,882 | SLOW |
| Mid ván | 2,829ms | ~1.6M | SLOW |
| Cuối ván | 9,913ms | ~5.4M | SLOW |

> MEDIUM baseline đã chậm ngay từ đầu — cùng lý do: không có move ordering.

---

### Những gì đang hoạt động tốt

| Thành phần | Đánh giá | Ghi chú |
|---|---|---|
| Alpha-Beta pruning | ✅ Đúng về logic | realPruneRatio đạt 63–76% sau DR-003/004 |
| Priority 1 & 2 | ✅ Tốt | Thắng ngay / chặn ngay, bypass minimax |
| Candidate generation | ✅ Hợp lý | Radius=2, không xét toàn bảng |
| Move ordering (DR-003) | ✅ Hiệu quả | Nodes giảm 35x, pruneRatio tăng mạnh |
| Selective sort (DR-004) | ✅ Giảm overhead | sortCalls giảm 200–6,000x, time giảm 5x |
| Benchmark system | ✅ Đầy đủ | timeMs, nodes, sortCalls, candidatesSkipped, realPruneRatio |

---

### Những gì đang có vấn đề

#### Vấn đề 1: Code Minimax hai nhánh — dư thừa và khó bảo trì

```cpp
// Hiện tại: phải viết 2 case riêng biệt
if (isMaximising) {
    best  = std::max(best, val);
    alpha = std::max(alpha, best);
} else {
    best  = std::min(best, val);
    beta  = std::min(beta, best);
}
// + truyền bool isMaximising xuống đệ quy
// + EvaluateBoard phải biết "ai đang xem" → sign logic rải khắp nơi
```

**Hệ quả:** Mỗi lần thêm feature (killer moves, history heuristic...) phải viết 2 lần.

---

#### Vấn đề 2: Depth cố định — không thể kiểm soát thời gian

```cpp
int depth = static_cast<int>(state.difficulty);  // 2, 4, hoặc 6 — không thay đổi
```

Mid-game (root≈50): HARD depth=6 → 25,000ms. Không có cách nào dừng sớm hơn.
Nếu muốn đảm bảo AI phản hồi trong 1 giây → phải giảm depth → mất chất lượng.

**Hệ quả:** Không có cân bằng tốt giữa chất lượng và thời gian.

---

#### Vấn đề 3: Kết quả depth trước bị bỏ phí

Khi chạy depth=6, toàn bộ kết quả từ depth=1,2,3,4,5 bị bỏ qua.
Nhưng **nước tốt nhất ở depth=5 gần như chắc chắn vẫn là nước tốt nhất ở depth=6**.
Nếu đặt nó đầu tiên trong danh sách candidates → alpha tăng ngay → pruning tối đa.

**Cơ hội bị bỏ lỡ:** Move ordering tốt nhất có thể đến từ chính kết quả của lần search trước.

---

#### Vấn đề 4: EvaluateBoard quét toàn bảng — tốn kém tại mọi leaf

```cpp
// EvaluateBoard: O(15×15×4) = 900 operations mỗi lần gọi
// Được gọi ở mọi leaf node — hàng triệu lần
for (int r = 0; r < BOARD_SIZE; r++)
    for (int c = 0; c < BOARD_SIZE; c++)
        for (auto& d : dirs)
            score += sign * CountLine(...);
```

**Hướng sửa:** Incremental evaluation — chỉ tính lại vùng xung quanh nước vừa đặt.
*(Không include trong DR-005 — để dành DR sau)*

---

#### Vấn đề 5: Không có time limit — AI có thể "đứng hình"

Không có cơ chế dừng search khi vượt quá thời gian cho phép.
HARD mid-game: người dùng phải chờ 25 giây — trải nghiệm tệ.

---

### Tóm tắt điểm số model hiện tại

| Tiêu chí | Điểm | Lý do |
|---|---|---|
| Correctness (đúng) | 9/10 | Logic đúng, priority đúng |
| Performance | 6/10 | 52x cải thiện nhưng HARD vẫn 5–16s mid-game |
| Pruning efficiency | 7/10 | realPruneRatio 63–76% — tốt, còn room |
| Code quality | 6/10 | Dual-branch verbose, depth cứng |
| Extensibility | 4/10 | Khó thêm feature mới |
| Time management | 2/10 | Không có — worst case 16,485ms không kiểm soát được |

**Kết luận:** Model đã tốt về correctness và pruning efficiency, nhưng **thiếu hoàn toàn khả năng kiểm soát thời gian**. HARD worst case 16 giây là không chấp nhận được trong game thực. Negamax + IDDFS giải quyết đúng điểm yếu này.

---

## Phần 2 — Kế Hoạch Nâng Cấp

### Model mới: Negamax + Iterative Deepening

---

### Khái niệm 1: Negamax

**Insight cốt lõi:** Từ góc nhìn của người đang đến lượt,
điểm tốt cho mình = điểm tệ cho đối thủ:

```
max(a, b) = -min(-a, -b)
```

Thay vì hai nhánh MAX và MIN, chỉ có một nhánh duy nhất:
**luôn maximize từ góc nhìn của người hiện tại**, và phủ định kết quả khi trả về.

```
Minimax hiện tại:
  Tầng AI   (MAX): chọn nước cao nhất
  Tầng Human(MIN): chọn nước thấp nhất

Negamax:
  Mọi tầng: chọn nước cao nhất (cho người đang đến lượt)
  Trả về:   -Negamax(...)  ← đảo dấu → đối thủ thấy điểm ngược lại
```

**Pseudocode Negamax:**
```
int Negamax(state, depth, alpha, beta, color, lastR, lastC):

    // Nếu nước vừa rồi kết thúc game → người hiện tại THUA
    if CheckTerminal(state, lastR, lastC) != 0:
        return -(WIN_SCORE + depth)   // thua sớm = tệ hơn thua muộn
                                      // depth cao hơn → bị trừ ít hơn → ưu tiên kéo dài

    if depth == 0:
        return color * EvaluateBoard(state)
        // color=1: AI đang xem → dương là tốt
        // color=-1: Human đang xem → âm là tốt cho human → nhân -1 để flip

    GetCandidates(...)
    if depth >= 3: SortCandidates(..., color)

    best = -∞
    for candidate (r, c):
        state._BOARD[r][c].c = color
        val = -Negamax(state, depth-1, -beta, -alpha, -color, r, c)
              ↑ đảo dấu + đảo alpha/beta
        state._BOARD[r][c].c = 0

        best  = max(best, val)
        alpha = max(alpha, best)
        if alpha >= beta: PRUNE; break

    return best
```

**Tại sao đảo `-alpha, -beta` khi đệ quy?**
```
Góc nhìn A (hiện tại):   alpha=30, beta=80 (A muốn score trong [30,80])
Góc nhìn B (đối thủ):    A muốn B bị hạn chế trong [-80, -30]
                          → B nhận alpha=-80, beta=-30
                          → B tìm được val → A nhìn thấy -val
```

---

### Khái niệm 2: Iterative Deepening (IDDFS)

Thay vì nhảy thẳng vào depth=6, tìm kiếm lần lượt từ depth=1:

```
Lần 1: Negamax(depth=1) → bestMove_d1  (rất nhanh, <1ms)
Lần 2: Negamax(depth=2) → bestMove_d2  (nhanh)
         đặt bestMove_d1 đầu tiên trong candidates → pruning tốt hơn
Lần 3: Negamax(depth=3) → bestMove_d3
         đặt bestMove_d2 đầu tiên → pruning tốt hơn nữa
...
Lần 6: Negamax(depth=6) → bestMove_d6
         đặt bestMove_d5 đầu tiên → pruning tốt nhất có thể
         HOẶC nếu hết giờ → return bestMove_d5 (đã có sẵn)
```

**Tại sao overhead nhỏ?**
```
Tổng nodes 1+b+b²+...+b^(d-1) = b^d/(b-1)
Với b=30, d=6: overhead = 30^6/29 ≈ 0.9M
Depth=6 alone: 30^6 ≈ 729M (worst case)
→ Overhead chỉ ~0.1% của worst case, đổi lại được time management và move ordering tốt hơn
```

**Pseudocode Iterative Deepening:**
```
Move AI_FindBestMove(state, timeLimitMs):

    Move bestMove = center  // fallback
    auto t0 = now()

    for d = 1 to MAX_DEPTH:
        // Đặt bestMove từ depth trước vào đầu candidates
        // → alpha tăng ngay → pruning tối đa ngay từ candidate đầu tiên
        Move candidate = SearchNegamax(state, d, bestMove_prev)

        elapsed = now() - t0
        if elapsed > timeLimitMs:
            break  // hết giờ → dùng kết quả depth trước
        bestMove = candidate

    return bestMove
```

**Lợi ích kép của Iterative Deepening:**
1. **Time management:** AI luôn trả về trong giới hạn thời gian
2. **Move ordering tốt nhất:** bestMove từ depth d-1 làm candidate đầu tiên ở depth d
   → alpha=best_possible ngay từ đầu → cắt được tối đa

---

### So Sánh: Minimax cũ vs Negamax + IDDFS mới

| Tiêu chí | Minimax cũ | Negamax + IDDFS |
|---|---|---|
| Số dòng code search | ~35 dòng | ~25 dòng |
| Dual-branch logic | Có | Không |
| Kiểm soát thời gian | Không | Có (time limit) |
| Move ordering từ depth trước | Không | Có (tự động) |
| Luôn có kết quả | Không đảm bảo | Luôn có (depth trước) |
| Correctness | Đúng | Đúng (tương đương) |
| Kỳ vọng HARD timeMs | ~25,000ms | < 500ms |

---

### Kế Hoạch Implement

**Bước 1:** Viết `Negamax()` thay thế `Minimax()`
- Bỏ tham số `bool isMaximising`
- Thêm tham số `int color` (+1 hoặc -1)
- Đổi `!isMaximising` thành `-color`
- Đổi `INT_MIN/INT_MAX` thành `-INF/+INF` nhất quán
- Logic alpha-beta giữ nguyên, bỏ if/else MAX/MIN

**Bước 2:** Viết `SearchAtDepth(state, depth, firstMove)`
- Wrapper gọi Negamax với một depth cụ thể
- Nhận `firstMove` (best move từ depth trước)
- Nếu `firstMove` valid: đặt nó đầu candidates trước khi sort
- Trả về best move tìm được

**Bước 3:** Viết `AI_FindBestMove()` dùng IDDFS
- Time limit theo difficulty: EASY=50ms, MEDIUM=300ms, HARD=1000ms
- Loop depth từ 1 đến MAX_DEPTH
- Check thời gian sau mỗi depth
- Return kết quả depth cuối cùng hoàn thành

**Bước 4:** Cập nhật benchmark
- Thêm field `depthReached` (IDDFS có thể dừng sớm)
- Thêm field `timeLimitMs` (giới hạn đang dùng)

---

### Checklist Sau Implement

- [ ] AI vẫn chọn nước thắng ngay (Priority 1)
- [ ] AI vẫn chặn nước thua ngay (Priority 2)
- [ ] HARD trả về trong < 1,000ms
- [ ] Kết quả không tệ hơn Minimax cũ (thử chơi thực tế)
- [ ] Benchmark log thêm `depthReached`

---

### Câu Hỏi Cần Trả Lời Được Khi Bảo Vệ

- Tại sao Negamax tương đương Minimax về kết quả?
  → `max(a,b) = -min(-a,-b)` — toán học thuần túy, không mất thông tin
- Iterative Deepening có lãng phí không khi search lại từ đầu mỗi depth?
  → Overhead ~0.1% worst case; đổi lại time management + move ordering tốt hơn
- Tại sao đảo dấu alpha/beta khi đệ quy Negamax?
  → Góc nhìn đối thủ là phủ định góc nhìn hiện tại
- Time limit chọn như thế nào?
  → Dựa trên UX: người chơi chờ bao lâu thì thấy "đứng hình"?

---

## Liên Kết

- Tiền đề: [DR-003](DR-003-move-ordering.md), [DR-004](DR-004-selective-sorting.md)
- Concept: [minimax-ai.md](../concepts/minimax-ai.md)
- Journal: [2026-05-24](../journal/2026-05-24.md)
