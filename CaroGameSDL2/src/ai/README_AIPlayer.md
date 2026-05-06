# 🤖 AIPlayer.cpp - Minimax AI Caro Game

## 📋 Giới thiệu

**AIPlayer.cpp** chứa logic AI để chơi Caro (Gomoku - 5 quân liên tiếp). AI sử dụng **thuật toán Minimax với Alpha-Beta Pruning** để tìm nước đi tốt nhất.

**Mục đích**: Cung cấp nước đi chiến thuật cho chế độ PVE (Player vs Environment).

---

## 🎯 Thuật Toán Minimax Là Gì?

### Ý tưởng chính
```
AI (Maximizing)           Human (Minimizing)
      ↓                           ↓
    +10                         -15
   /   \                        /   \
  +5   +10                    -10   -15
 / \   / \                    / \   / \
```

- **Maximizing** (AI): Tìm nước đi giúp điểm số cao nhất
- **Minimizing** (Human): Tìm nước đi giúp AI có điểm thấp nhất
- **Độ sâu (depth)**: Bao nhiêu nước trước được tính toán
  - `AI_EASY = 2`: Nhìn trước 2 nước
  - `AI_MEDIUM = 4`: Nhìn trước 4 nước
  - `AI_HARD = 6`: Nhìn trước 6 nước

### Alpha-Beta Pruning
Tối ưu hóa: **Cắt bỏ (prune) những nhánh không cần xem**
```
Nếu ta tìm thấy nước tốt (α >= β), dừng tìm kiếm ngay
→ Giảm từ O(b^d) xuống O(b^(d/2)) phép tính
```

---

## 📊 Bảng Scoring System

### Score Constants
```cpp
SCORES[] = { 0, 10, 100, 1000, 100000, 10000000 }
           // 0  1    2     3      4        5
```

| Lượng quân liên tiếp | Mở 2 đầu | Mở 1 đầu | Điểm |
|----------------------|----------|----------|------|
| 0 quân              | N/A      | N/A      | 0    |
| 1 quân              | Có       | Có       | 10   |
| 2 quân              | Có       | Có       | 100  |
| 3 quân              | Có       | Có       | 1000 |
| 4 quân              | Có       | Có       | 100000 |
| **5 quân (Thắng)** | N/A      | N/A      | 10000000 |

**"Mở đầu"** = ô trống liền kế (có thể mở rộng)

**Ví dụ:**
```
Bàn cờ:          Score:
O O O _ X        O O O: 3 mở 1 đầu = 1000/2 = 500
                 O O O _ trống + X chặn = mở 1 đầu

O O O O _        O O O O: 4 mở 1 đầu = 100000/2 = 50000
                 Cực kỳ nguy hiểm, AI sẽ chọn
```

---

## 🔧 Các Hàm Chính

### 1️⃣ `CountLine(state, r, c, dr, dc, color)`

**Chức năng:** Đếm quân liên tiếp từ vị trí (r,c) theo hướng (dr,dc)

**Tham số:**
- `r, c`: Vị trí bắt đầu
- `dr, dc`: Hướng (1,0)=ngang, (0,1)=dọc, (1,1)=chéo/, (1,-1)=chéo\
- `color`: Loại quân (-1=X, 1=O)

**Quá trình:**
```
1. Bắt đầu từ (r,c) → count=1
2. Đi hướng xuôi (r+dr, c+dc, ...) → đếm quân cùng màu
3. Đi hướng ngược (-dr, -dc, ...) → đếm quân cùng màu
4. Kiểm tra ô kế tiếp có trống không (open ends)
5. Trả về score dựa trên bảng SCORES[]
```

**Ví dụ:**
```cpp
// Bàn cờ: [_, O, O, O, _]
CountLine(state, 1, (0,1), color=1)  
// → count=3, open=2 (cả 2 đầu mở)
// → Score = SCORES[3] = 1000 (mở 2 đầu)
```

---

### 2️⃣ `EvaluateBoard(state)`

**Chức năng:** Tính điểm toàn bộ bàn cờ từ quan điểm của AI

**Quá trình:**
```
1. Duyệt tất cả ô trên bàn cờ (15x15)
2. Nếu ô trống → bỏ qua
3. Nếu ô có quân:
   - AI (c=1) → điểm dương (+)
   - Opponent (c=-1) → điểm âm (-)
4. Kiểm tra 4 hướng (→, ↓, ↘, ↙)
5. Cộng tất cả điểm
6. Trả về tổng điểm
```

**Công thức:**
```
score = Σ (sign * CountLine)
sign = +1 nếu là quân AI (c=1)
sign = -1 nếu là quân đối thủ (c=-1)
```

**Ví dụ:**
```
Bàn cờ:      Điểm:
X X X        -1000 (3 quân X liên tiếp → nguy hiểm!)
O O          +100 (2 quân O liên tiếp)
_
=> Total = -1000 + 100 = -900 (AI đang thua)
=> Cần chặn X hoặc phát triển O
```

---

### 3️⃣ `CheckTerminal(state, lastR, lastC)`

**Chức năng:** Kiểm tra game có kết thúc không (ai thắng?)

**Tham số:**
- `lastR, lastC`: Nước đi gần nhất (chỉ cần kiểm tra từ đây)

**Trả về:**
- `INT_MAX / 2` → **AI thắng** ✓
- `INT_MIN / 2` → **Người chơi thắng** ✗
- `0` → **Chưa kết thúc** (tiếp tục chơi)

**Quá trình:**
```
1. Lấy màu quân tại (lastR, lastC)
2. Kiểm tra 4 hướng từ vị trí này
3. Đếm tổng quân cùng màu (2 chiều)
4. Nếu ≥ 5 quân → Người chơi đó thắng
5. Return INT_MAX/2 hoặc INT_MIN/2
```

---

### 4️⃣ `GetCandidates(state, out[], count)`

**Chức năng:** Tìm danh sách nước đi hợp lệ (tối ưu hóa tìm kiếm)

**Chiến lược Smart Move Generation:**
```
❌ Xấu:      Kiểm tra tất cả 225 ô (15x15)
✅ Tốt:      Chỉ xem ô trống gần quân đã đặt ≤ 2 ô
```

**Quá trình:**
```
1. Duyệt tất cả ô có quân (placement)
2. Với mỗi ô có quân:
   - Kiểm tra vùng 5x5 xung quanh (dr,dc: -2 to 2)
   - Nếu ô trống → thêm vào danh sách (dùng visited để tránh trùng)
3. Nếu board trống (count=0):
   - Mặc định chơi ở giữa board (7,7)
```

**Ví dụ:**
```
Bàn cờ:              Candidates (O là quân, ? là ứng viên):
_ _ _ _ _
_ _ ? ? _
_ ? O ? _
_ ? ? ? _
_ _ _ _ _

→ Chỉ cần kiểm tra ~10-20 ô thay vì 225 ô
→ Tăng tốc độ AI lên 10x!
```

---

### 5️⃣ `Minimax(state, depth, alpha, beta, isMaximising, lastR, lastC)`

**Chức năng:** Thuật toán đệ quy tìm nước đi tốt nhất

**Tham số:**
- `depth`: Độ sâu còn lại (bao nhiêu nước tiếp theo)
- `alpha`: Điểm tốt nhất mà Maximizer tìm được
- `beta`: Điểm tốt nhất mà Minimizer tìm được
- `isMaximising`: `true` = AI's turn, `false` = Human's turn

**Quá trình:**
```
1. Base cases:
   - Nếu game kết thúc → trả về INT_MAX/2 hoặc INT_MIN/2
   - Nếu depth=0 → trả về điểm đánh giá board (EvaluateBoard)

2. Lấy danh sách nước đi ứng viên (GetCandidates)

3. Recursive search:
   - Nếu isMaximising (AI):
     - Thử từng nước → gọi Minimax(..., !isMaximising)
     - Chọn nước có điểm CAO NHẤT (max)
     - Cập nhật alpha
   - Nếu !isMaximising (Human):
     - Thử từng nước → gọi Minimax(..., !isMaximising)
     - Chọn nước có điểm THẤP NHẤT (min)
     - Cập nhật beta

4. Alpha-Beta Pruning:
   - Nếu beta ≤ alpha → break (cắt nhánh)
   - Vì: nhánh này không thể tốt hơn

5. Hoàn tác nước đi (undo) trước khi return
```

**Pseudocode:**
```cpp
if (terminal or depth == 0) return score;

if (isMaximising) {
    best = -∞
    for each move:
        score = Minimax(..., depth-1, alpha, beta, false)
        best = max(best, score)
        alpha = max(alpha, best)
        if (beta <= alpha) break  // ← Pruning!
    return best
} else {
    best = +∞
    for each move:
        score = Minimax(..., depth-1, alpha, beta, true)
        best = min(best, score)
        beta = min(beta, best)
        if (beta <= alpha) break  // ← Pruning!
    return best
}
```

---

### 6️⃣ `AI_FindBestMove(state)`

**Chức năng:** Điểm vào công khai - tìm nước đi tốt nhất cho AI

**Quá trình:**
```
1. Copy state để không thay đổi original
2. Lấy depth từ state.difficulty
3. Lấy danh sách nước ứng viên
4. Với mỗi nước:
   - Đặt quân AI (c=1) tạm thời
   - Gọi Minimax(..., depth-1, INT_MIN, INT_MAX, false)
   - Hoàn tác nước đi
   - Nếu score > best → cập nhật
5. Trả về Move { row, col } tốt nhất
```

**Ví dụ:**
```
Nước 1: score = +500  ← Best so far
Nước 2: score = +450
Nước 3: score = +800  ← New best!
Nước 4: score = +600
...
→ Return nước 3
```

---

## 🌳 Ví Dụ Flow Minimax (Depth=2)

```
                    MAX (AI's turn)
                    /              \
               Move A            Move B
              /       \          /      \
            MIN      MIN       MIN      MIN
           / | \    / | \     / | \    / | \
          +10 +5 -20 +0 +8 -15 +12 +0 -5 +20
          ↓   ↓  ↓   ↓  ↓  ↓   ↓   ↓  ↓  ↓
        MIN = -20 (chọn score thấp nhất cho human)

                    MIN = -15

        MAX = max(-20, -15) = -15
        Move B được chọn (ít tổn thất)
```

---

## 📈 Độ Khó (Difficulty Levels)

| Level | Depth | Nước tìm trước | Tốc độ | Chiến thuật |
|-------|-------|----------------|--------|-----------|
| **EASY** | 2 | 2 nước | Nhanh | Chơi lỏng lẻo, dễ thua |
| **MEDIUM** | 4 | 4 nước | Bình thường | Cân bằng offense/defense |
| **HARD** | 6 | 6 nước | Chậm (1-2s) | Rất mạnh, khó thắng |

---

## ⚡ Optimizations (Tối ưu hóa)

### 1. **Smart Move Generation**
```cpp
// ❌ Cũ: Duyệt 225 ô
for (int i = 0; i < 15; i++)
    for (int j = 0; j < 15; j++)
        if (board[i][j] == 0) candidates.push(i,j);

// ✅ Mới: Chỉ ~20 ô gần quân đã đặt
GetCandidates(state, cands, nCands);  // nCands ≈ 20
```

### 2. **Alpha-Beta Pruning**
```cpp
// Cắt nhánh không cần xem
if (beta <= alpha) break;  // → Tiết kiệm 50% tính toán
```

### 3. **Last Move Checking**
```cpp
// Chỉ kiểm tra 4 hướng từ lastR,C thay vì duyệt board
int terminal = CheckTerminal(state, lastR, lastC);
```

---

## 🎮 Cách Sử Dụng

### Gọi từ App.cpp
```cpp
// Khi chế độ PVE + lượt AI
Move bestMove = AI_FindBestMove(state);
Animation_StartMove(state, bestMove.row, bestMove.col, 1);  // charIdx=1 (AI)
```

### Từ GameDef.h
```cpp
enum AIDifficulty { 
    AI_EASY = 2,     // Nhìn trước 2 nước
    AI_MEDIUM = 4,   // Nhìn trước 4 nước
    AI_HARD = 6      // Nhìn trước 6 nước
};
```

---

## 📊 Độ Phức Tạp (Complexity)

### Time Complexity
```
T = O(b^d / 2^d)  with Alpha-Beta Pruning
  = O(b^(d/2))

b = branching factor (~20 candidates)
d = depth

EASY (d=2):     O(20^1) = 20        (instant)
MEDIUM (d=4):   O(20^2) = 400       (< 100ms)
HARD (d=6):     O(20^3) = 8000      (1-2s)
```

### Space Complexity
```
O(d) for recursion stack
→ Negligible
```

---

## 🔍 Debug Tips

### 1. Thêm logging để theo dõi
```cpp
// Trong AI_FindBestMove
std::cout << "Candidate " << i << ": (" << r << ", " << c << ") = " << score << "\n";
```

### 2. Kiểm tra score của moves
```cpp
// Move nào có score cao nhất?
if (score > bestScore) {
    std::cout << "New best: " << r << "," << c << " score=" << score << "\n";
}
```

### 3. Simulate play
```cpp
// Chạy game AI vs AI để kiểm tra
// Xem AI có thực sự chiến thắng không?
```

---

## 🚀 Cách Cải Thiện AI

| Cải Thiện | Dòng Code | Hiệu ứng |
|-----------|-----------|---------|
| Tăng depth | `AI_HARD = 8` | Mạnh hơn, chậm hơn |
| Bảng scoring tốt hơn | Thay `SCORES[]` | Chiến thuật khác |
| Transposition Table (cache) | Lưu kết quả đã tính | Nhanh gấp 2-3x |
| Killer Move Heuristic | Ưu tiên moves tốt | Pruning tốt hơn |
| Opening Book | Nước mở tiêu chuẩn | Mở bài tốt hơn |

---

## 📝 Tóm Tắt

| Yếu tố | Mô tả |
|--------|-------|
| **Thuật toán** | Minimax + Alpha-Beta Pruning |
| **Encoding** | AI=O(c=1), Human=X(c=-1) |
| **Depth** | AI_EASY=2, AI_MEDIUM=4, AI_HARD=6 |
| **Move Generation** | Smart: chỉ ô gần quân đã đặt |
| **Scoring** | Pattern-based (1-5 quân liên tiếp) |
| **Terminal Check** | Kiểm tra thắng từ lastMove |
| **Performance** | EASY=instant, MEDIUM=<100ms, HARD=1-2s |

---

## ✅ Checklist Hiểu Được

- [ ] Minimax là gì?
- [ ] Alpha-Beta Pruning giảm tính toán như thế nào?
- [ ] Scoring system dựa trên pattern nào?
- [ ] CountLine() tính điểm thế nào?
- [ ] GetCandidates() tối ưu hóa tìm kiếm?
- [ ] Hàm nào là entry point?
- [ ] Depth nhạo ảnh hưởng đến tốc độ?

---

**Viết bởi:** AI Architecture  
**Phần mềm:** Caro Game SDL2  
**Ngôn ngữ:** C++  
**Thư viện:** Minimax Algorithm
