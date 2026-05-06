# ⚡ QUICK ACTION GUIDE - Performance Issues & Evaluation

> Bạn cần làm gì ngay bây giờ?

---

## 🔴 **VẤN ĐỀ 1: Lag khi Player vs AI (Ưu Tiên CAO)**

### **Tức Thì (5 phút)**

1. **Thêm logging vào App.cpp:**

```cpp
// Trong App_TriggerAITurn()
auto start = std::chrono::high_resolution_clock::now();
Move best = AI_FindBestMove(state);
auto end = std::chrono::high_resolution_clock::now();
auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

std::cout << "⏱️  AI Time: " << timeMs << "ms (Difficulty: " << state.difficulty << ")" << std::endl;
```

2. **Chạy game & ghi lại AI Time cho mỗi difficulty**

```
EASY (2):   ___ ms
MEDIUM (4): ___ ms
HARD (6):   ___ ms
```

**Kỳ vọng:**
- EASY < 100ms ✅
- MEDIUM < 500ms ✅
- HARD < 3000ms ✅

**Nếu vượt:** → Bước 2

---

### **Bước 2: Giảm Depth (15 phút)**

```cpp
// GameDef.h
enum AIDifficulty { 
    AI_EASY = 2,      // ← Thử 1 hoặc 2
    AI_MEDIUM = 3,    // ← Thử 2 hoặc 3 (từ 4)
    AI_HARD = 4       // ← Thử 3 hoặc 4 (từ 6)
};
```

**Test:**
- Compile & chạy lại
- Có mượt hơn không?
- YES ✅ → Vấn đề xong!
- NO ❌ → Bước 3

---

### **Bước 3: Async AI (Nâng Cao - 1 giờ)**

**Tạo thread để AI chạy song song:**

```cpp
// App.h
std::thread* aiThread = nullptr;

// App.cpp - App_TriggerAITurn()
aiThread = new std::thread([&state]() {
    best = AI_FindBestMove(state);
});

// App.cpp - App_Update_Playing()
if (aiThread && aiThread->joinable()) {
    aiThread->join();
    delete aiThread;
    aiThread = nullptr;
    // Apply move
}
```

---

## 🔴 **VẤN ĐỀ 2: AI Miss Winning Move (Ưu Tiên CAO)**

### **Tức Thì (10 phút)**

1. **Tạo test scenario:**

```cpp
void TestAIWinning() {
    _GAMESTATE state = {};
    ResetData(state);

    // O O O O _ ← Position (0,4) winning
    state._BOARD[0][0].c = 1;
    state._BOARD[0][1].c = 1;
    state._BOARD[0][2].c = 1;
    state._BOARD[0][3].c = 1;

    state.mode = MODE_PVE;
    state.difficulty = AI_HARD;
    state.turn = false;  // AI turn

    Move best = AI_FindBestMove(state);

    std::cout << "AI Move: (" << best.row << ", " << best.col << ")\n";
    std::cout << "Expected: (0, 4)\n";

    if (best.row == 0 && best.col == 4) {
        std::cout << "✅ AI FOUND IT!\n";
    } else {
        std::cout << "❌ AI MISSED!\n";
    }
}
```

2. **Chạy test & xem kết quả**

---

### **Bước 2: Debug CheckTerminal (20 phút)**

```cpp
// Trong AIPlayer.cpp - CheckTerminal()
// Thêm logging:

if (cnt >= WIN_COUNT) {
    std::cout << "🎯 Found " << cnt << " in a row at (" 
              << lastR << ", " << lastC << ")\n";
    return (col == 1) ? INT_MAX / 2 : INT_MIN / 2;
}
```

**Chạy test lại → có log không?**
- YES ✅ → CheckTerminal works
- NO ❌ → Có bug ở đây

---

### **Bước 3: Test CountLine (20 phút)**

```cpp
void TestCountLine() {
    _GAMESTATE state = {};
    ResetData(state);

    // _ O O O _ ← 3 quân, 2 open ends
    state._BOARD[0][1].c = 1;
    state._BOARD[0][2].c = 1;
    state._BOARD[0][3].c = 1;

    int score = CountLine(state, 0, 2, 0, 1, 1);

    std::cout << "Score: " << score << " (Expected: 1000)\n";
}
```

---

### **Bước 4: Disable Alpha-Beta (15 phút)**

```cpp
// Minimax() - tạm thời disable pruning:
// if (beta <= alpha) break;  ← Comment out

// Nếu AI thông minh hơn → Pruning cắt bỏ good moves
```

---

## 📊 **ĐÁNH GIÁ DỰ ÁN TOÀN DIỆN**

### **Sử dụng PROJECT_EVALUATION.md (30 phút)**

```
1. Mở PROJECT_EVALUATION.md
2. Chơi game 10 trận
3. Điền điểm cho mỗi category
4. Tính tổng điểm
5. Xem grade (90+ = Excellent)
```

**Checklist:**
- [ ] Functionality: ___/40
- [ ] Performance: ___/25
- [ ] Code Quality: ___/20
- [ ] Documentation: ___/15
- [ ] **TOTAL: ___/100**

---

## ⚡ **BENCHMARK CHI TIẾT**

### **Sử dụng BENCHMARK_GUIDE.md (45 phút)**

1. **Thêm profiling vào code**
2. **Chạy game ở mỗi difficulty**
3. **Ghi lại metrics:**
   - Frame time
   - AI time
   - Max frame spike
   - Winning move detection

```
EASY:
  AI Time: ___ ms
  Frame Time: ___ ms
  FPS: ___
  Smooth? YES/NO

MEDIUM:
  AI Time: ___ ms
  Frame Time: ___ ms
  FPS: ___
  Smooth? YES/NO

HARD:
  AI Time: ___ ms
  Frame Time: ___ ms
  FPS: ___
  Smooth? YES/NO
```

---

## 🔧 **FIX ISSUES**

### **Sử dụng TROUBLESHOOTING.md**

```
1. Xác định vấn đề (lag/AI miss/crash)
2. Tìm section tương ứng
3. Làm theo step-by-step
4. Test lại
5. Nếu không xong, jump lên "Proper Fix" (lâu hơn)
```

---

## 📋 **COMMIT CHANGES**

```powershell
# 1. Add profiling files
git add "CaroGameSDL2/src/sdl/Profiler.h"
git add "CaroGameSDL2/src/ai/AIBenchmark.h"

# 2. Commit
git commit -m "tools: Thêm Performance Profiler & AI Benchmark"

# 3. Push
git push origin master
```

---

## 🎯 **TIMELINE**

| Task | Time | Done |
|------|------|------|
| Add logging | 5 min | [ ] |
| Benchmark EASY | 5 min | [ ] |
| Benchmark MEDIUM | 10 min | [ ] |
| Benchmark HARD | 15 min | [ ] |
| Analyze results | 10 min | [ ] |
| Fix issues (if any) | 30-120 min | [ ] |
| Full evaluation | 30 min | [ ] |
| **TOTAL** | **75-180 min** | |

---

## ✨ **EXPECTED RESULTS**

### **Nếu không có vấn đề:**
```
✅ FPS = 60 stable
✅ AI Time < 3s ở HARD
✅ Animation smooth
✅ AI finds winning move
✅ Project Score > 80
```

### **Nếu có vấn đề:**
```
❌ FPS < 30 → Reduce AI depth
❌ AI Time > 5s → Async AI needed
❌ Animation lag → Check fixed timestep
❌ AI miss win → Debug CheckTerminal()
```

---

## 📞 **CẦN GIÚP?**

- 📖 Xem **TROUBLESHOOTING.md** để debug
- 📊 Xem **BENCHMARK_GUIDE.md** để đo lường
- 📋 Xem **PROJECT_EVALUATION.md** để đánh giá
- 💡 Xem **Profiler.h** để profile code

---

**Let's make the game smooth!** 🚀
