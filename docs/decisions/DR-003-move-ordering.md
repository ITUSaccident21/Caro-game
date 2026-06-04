54# DR-003 — Move Ordering cho Alpha-Beta Pruning

**Ngày đề xuất:** 2026-05-23
**Ngày implement:** 2026-05-24
**Trạng thái:** ✅ Hoàn thành — hiệu năng cải thiện rõ rệt
**Liên quan:** [AIPlayer.cpp](../../CaroGameSDL2/src/ai/AIPlayer.cpp)
**Baseline (trước khi implement):** HARD ~858,000ms, pruneRatio ~2%

---

## Vấn Đề

Alpha-Beta pruning chỉ hiệu quả khi **nước tốt được xét trước**.
Hiện tại `GetCandidates` trả về theo thứ tự row-major (quét bảng từ trên-trái),
hoàn toàn không liên quan đến chất lượng nước cờ.

Kết quả: Alpha-Beta gần như là Minimax thuần → 379 triệu nodes → 14 phút/nước.

---

## Ý Tưởng Cốt Lõi

Trước khi đưa danh sách candidates vào vòng lặp Minimax,
**chấm điểm nhanh từng candidate** rồi **sort giảm dần**.

```
GetCandidates() → [A, B, C, D, E, F]  (row-major, ngẫu nhiên về chất lượng)
     ↓
ScoreAndSort()  → [D, B, F, A, C, E]  (D tốt nhất, E tệ nhất)
     ↓
Minimax duyệt theo thứ tự mới → alpha tăng nhanh → pruning xảy ra sớm
```

---

## Hàm Cần Viết

### Hàm 1: `QuickScore(state, r, c, color)`

Chấm điểm **nhanh** cho nước đặt tại (r,c) với màu `color`.
"Nhanh" nghĩa là KHÔNG gọi `EvaluateBoard` toàn bảng — chỉ xét xung quanh (r,c).

**Pseudocode:**
```
int QuickScore(state, r, c, color):
    score = 0
    for mỗi hướng trong 4 hướng (ngang, dọc, 2 chéo):
        score += CountLine(state, r, c, hướng, color)      // lợi cho AI
        score += CountLine(state, r, c, hướng, -color)     // đe dọa từ đối thủ
    return score
```

Gợi ý: `CountLine` đã có sẵn trong file, dùng lại được.
`-color` là màu đối thủ (AI=1 thì đối thủ=-1, và ngược lại).

**Tại sao cộng cả điểm đối thủ?**
Nước cờ tốt = vừa tạo mối đe dọa cho mình, vừa phá mối đe dọa của đối thủ.
Một nước chỉ tốt cho mình mà không chặn đối thủ → không phải nước tốt nhất.

---

### Hàm 2: `SortCandidates(state, cands, nCands, color)`

Sort mảng `cands[nCands][2]` theo `QuickScore` giảm dần.

**Pseudocode:**
```
void SortCandidates(state, cands[][2], nCands, color):
    // Tạo mảng tạm: scores[nCands]
    for i = 0 to nCands-1:
        scores[i] = QuickScore(state, cands[i][0], cands[i][1], color)

    // Sort: hoán vị cands[i] và scores[i] cùng nhau
    // Dùng selection sort hoặc std::sort với custom comparator
    // Mục tiêu: cands[0] có scores[0] cao nhất
```

**Gợi ý dùng std::sort:**
```cpp
// Tạo struct tạm để sort cùng nhau
struct ScoredMove { int r, c, score; };
ScoredMove tmp[BOARD_SIZE * BOARD_SIZE];

for (int i = 0; i < nCands; i++)
    tmp[i] = { cands[i][0], cands[i][1],
               QuickScore(state, cands[i][0], cands[i][1], color) };

std::sort(tmp, tmp + nCands,
    [](const ScoredMove& a, const ScoredMove& b){ return a.score > b.score; });

// Copy lại vào cands
for (int i = 0; i < nCands; i++) {
    cands[i][0] = tmp[i].r;
    cands[i][1] = tmp[i].c;
}
```

---

## Nơi Gọi Hàm Mới

### Trong `Minimax()` — dòng sau `GetCandidates`:

```cpp
// Hiện tại:
GetCandidates(state, cands, nCands);
if (nCands == 0) return 0;
int color = isMaximising ? 1 : -1;
// ... vòng lặp for ...

// Sau khi thêm:
GetCandidates(state, cands, nCands);
if (nCands == 0) return 0;
int color = isMaximising ? 1 : -1;
SortCandidates(state, cands, nCands, color);   // ← thêm dòng này
// ... vòng lặp for ... (không thay đổi gì khác)
```

### Trong `AI_FindBestMove()` — dòng sau `GetCandidates` ở Priority 3:

```cpp
// Hiện tại:
GetCandidates(working, cands, nCands);
// ... Priority 1, Priority 2 checks ...
// Priority 3 loop:

// Sau khi thêm:
// Sort trước Priority 3 loop (AI đang maximise → color=1)
SortCandidates(working, cands, nCands, 1);     // ← thêm dòng này
for (int i = 0; i < nCands; i++) { ... }
```

---

## Tại Sao Chỉ Thêm 2 Dòng Mà Hiệu Quả Cao?

```
Không sort: Alpha tăng chậm
  Nước 1: score=50  → alpha=50
  Nước 2: score=45  → alpha=50  (không tăng)
  Nước 3: score=80  → alpha=80  ← mới tăng, nhưng đã xét 2 nước vô ích trước
  Nước 4: score=30  → CẮT (30 < alpha=80) ← mới pruning
  Nước 5,6,7...     → tiếp tục phải xét vì alpha tăng muộn

Có sort: Alpha tăng ngay từ đầu
  Nước 1: score=80  → alpha=80  ← ngay lập tức
  Nước 2: score=50  → alpha=80
  Nước 3: score=45  → CẮT (45 < alpha=80) ← pruning sớm!
  Nước 4,5,6,7...   → BỊ CẮT TOÀN BỘ
```

---

## Kết Quả Thực Tế (Benchmark 4 — 2026-05-24)

| Chỉ tiêu | Trước (B3) | Sau (B4) | Cải thiện |
|---|---|---|---|
| HARD time (root≈37) | 285,844ms | 25,289ms | **11x** |
| HARD nodes (root≈37) | 155,198,597 | 4,368,338 | **35x** |
| pruneRatio | 2.1% | 5–7.6% | Tăng nhẹ |

**Ghi chú quan trọng:** pruneRatio thấp là do metric đang đếm *break events*,
không đếm *candidates skipped*. Số nodes giảm 35x mới phản ánh đúng hiệu quả thực.

**Vẫn còn hạn chế:** HARD ~25 giây vẫn SLOW do SortCandidates được gọi
tại mọi node đệ quy — sort overhead ăn vào một phần lợi ích pruning.
→ Xem DR-004 (selective sorting theo depth).

---

## Checklist Tự Kiểm Tra

- [ ] `QuickScore` trả về số dương (càng cao càng tốt cho `color`)
- [ ] `SortCandidates` sort **giảm dần** (index 0 = nước tốt nhất)
- [ ] Gọi `SortCandidates` trong **cả 2 nơi**: `Minimax()` và `AI_FindBestMove()` Priority 3
- [ ] Game vẫn chạy đúng — AI không đặt sai ô, không crash
- [ ] Log benchmark mới và ghi vào journal so sánh với baseline

---

## Câu Hỏi Cần Trả Lời Được Khi Bảo Vệ

- Move ordering ảnh hưởng thế nào đến Alpha-Beta — giải thích bằng ví dụ số?
- Tại sao QuickScore cộng cả điểm đối thủ chứ không chỉ điểm AI?
- Tại sao sort trong cả Minimax() lẫn AI_FindBestMove() — bỏ một chỗ có sao không?
- Tại sao dùng QuickScore (local) thay vì EvaluateBoard (global) để sort?

---

## Liên Kết

- Baseline: [DR-002](DR-002-ai-benchmark-system.md)
- Concept: [ai-benchmark-metrics.md](../concepts/ai-benchmark-metrics.md)
- Journal: [2026-05-23](../journal/2026-05-23.md)
