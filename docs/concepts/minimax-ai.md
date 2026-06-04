# Khái Niệm: Minimax AI

> Giải thích thuật toán Minimax dùng trong AIPlayer.cpp — cách AI "suy nghĩ"
> về nước đi tốt nhất trong game cờ.

---

## Bài Toán AI Trong Game Cờ

AI cần chọn nước đi làm **lợi thế của mình cao nhất** đồng thời **lợi thế đối thủ thấp nhất**.

Đây chính là tên gọi: **MINI**mize (giảm lợi thế đối thủ) + **MAX**imize (tăng lợi thế mình).

---

## Cây Tìm Kiếm

```
         [Trạng thái hiện tại]
                 │
         AI đến lượt (MAX)
        /        |        \
      [A]       [B]       [C]    ← các nước AI có thể đi
       │                   │
  Đối thủ (MIN)        Đối thủ (MIN)
   /    \               /    \
 [A1]  [A2]          [C1]  [C2]  ← đối thủ phản ứng
```

**Nguyên tắc:**
- Tầng MAX: chọn nút con có giá trị **cao nhất** (AI chọn nước tốt nhất cho mình)
- Tầng MIN: chọn nút con có giá trị **thấp nhất** (đối thủ chọn nước tệ nhất cho AI)

---

## Hàm Đánh Giá (Evaluation Function)

Minimax cần "chấm điểm" mỗi trạng thái bảng cờ:

```
Score > 0 : có lợi cho AI
Score = 0 : cân bằng
Score < 0 : có lợi cho đối thủ
Score = +∞: AI thắng
Score = -∞: AI thua
```

**Trong thực tế (AIPlayer.cpp), điểm được tính dựa trên:**
- Chuỗi liên tiếp của AI: 2 quân → 10pts, 3 quân → 100pts, 4 quân → 1000pts, 5 quân → WIN
- Chuỗi liên tiếp của đối thủ: tính ngược dấu

---

## Depth — Độ Sâu Tìm Kiếm

```
EASY:   depth = 1  → chỉ nhìn 1 nước trước
MEDIUM: depth = 3  → nhìn 3 nước
HARD:   depth = 5  → nhìn 5 nước trước
```

**Vấn đề:** Số nút tăng theo cấp số mũ: `(15×15)^depth`
- depth=1: 225 nút
- depth=3: 11 triệu nút
- depth=5: 844 tỷ nút (không thể brute force!)

**Giải pháp thực tế:** Chỉ xem xét các ô **gần quân đã đặt** (candidate moves).

---

## Alpha-Beta Pruning — Tối Ưu Hóa

```
         [MAX: chọn max]
        /              \
    [MIN]             [MIN]
    /   \             /   \
  [3]   [5]        [2]   [?]
              ↑ α=5
              Không cần xét [?] — dù bao nhiêu cũng bị MIN chọn ≤ 2 < 5
```

**Ý tưởng:** Nếu đã tìm được nhánh cho điểm 5, và nhánh khác đang có MIN ≤ 2, thì nhánh đó không bao giờ được MAX chọn → cắt bỏ (prune).

**Hiệu quả:** Giảm số nút cần xét từ O(b^d) xuống O(b^(d/2)) — tương đương tăng depth gấp đôi với cùng thời gian.

---

## Cấu Trúc Trong AIPlayer.cpp

```cpp
// AI tìm nước đi tốt nhất
MoveResult AI_FindBestMove(const _GAMESTATE& state, AIDifficulty diff) {
    int depth = GetDepthForDifficulty(diff);  // 1, 3, hoặc 5
    int bestScore = INT_MIN;
    MoveResult bestMove;

    for (auto& candidate : GetCandidateMoves(state)) {
        // Thử nước đi
        SimulateMove(state, candidate);
        // Đánh giá qua minimax
        int score = Minimax(state, depth-1, false, INT_MIN, INT_MAX);
        // Giữ nước tốt nhất
        if (score > bestScore) { bestScore = score; bestMove = candidate; }
        // Undo nước đi
        UndoMove(state, candidate);
    }
    return bestMove;
}
```

---

## Điểm Yếu Hiện Tại Và Hướng Cải Thiện

| Vấn đề | Nguyên nhân | Giải pháp |
|---|---|---|
| AI HARD còn chậm | Chưa có Alpha-Beta | Implement pruning |
| AI không "nhìn xa" | Depth giới hạn | Iterative deepening |
| Evaluation đơn giản | Chỉ tính chuỗi đơn | Thêm threat patterns |

---

## Câu Hỏi Thường Gặp Khi Bảo Vệ

**H: Tại sao AI EASY dễ đánh thắng?**
A: depth=1 — AI chỉ nhìn 1 nước, không thể lên kế hoạch dài hạn.

**H: Alpha-Beta có thay đổi kết quả không?**
A: Không — kết quả giống hệt Minimax thuần, chỉ nhanh hơn.

**H: Tại sao không dùng depth lớn hơn?**
A: Thời gian tính tăng theo lũy thừa. depth=7 có thể mất vài giây/nước.

---

## Liên Kết

- Source: [AIPlayer.cpp](../../CaroGameSDL2/src/ai/AIPlayer.cpp)
- Journal: [2026-05-22](../journal/2026-05-22.md)
