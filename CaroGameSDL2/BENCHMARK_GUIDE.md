# 🔍 Caro Game - Performance Diagnostic & Benchmark Guide

## 📊 Vấn Đề Cần Xác Định

### **Issue 1: Lag khi Player vs AI**
- **Triệu chứng**: Màn hình đứng, animation chậm
- **Nguyên nhân tiềm ẩn**: 
  - AI_FindBestMove() blocking main thread
  - Depth quá sâu
  - Frame skip/stuttering

### **Issue 2: AI không thông minh**
- **Triệu chứng**: AI miss winning move
- **Nguyên nhân tiềm ẩn**:
  - CheckTerminal() bug
  - CountLine() sai logic
  - Move ordering tồi

---

## 🛠️ **Cách Benchmark (4 bước)**

### **Step 1: Thêm Profiler vào Code**

**Trong App.cpp - function App_TriggerAITurn():**
```cpp
void App_TriggerAITurn(_GAMESTATE& state) {
    state.aiThinking = true;

    // ⭐ PROFILE AI
    auto aiStart = std::chrono::high_resolution_clock::now();
    Move best = AI_FindBestMove(state);
    auto aiEnd = std::chrono::high_resolution_clock::now();
    auto aiTime = std::chrono::duration_cast<std::chrono::milliseconds>(aiEnd - aiStart);

    std::cout << "⏱️  AI Time: " << aiTime.count() << "ms" << std::endl;

    state.aiThinking = false;

    if (best.row >= 0 && best.col >= 0) {
        state.selectedRow = best.row;
        state.selectedCol = best.col;
        Animation_StartMove(state, best.row, best.col, 1);
        AudioManager_PlaySFX(SFX_MOVE);
    }
}
```

### **Step 2: Thêm Frame Time Tracking**

**Trong App.cpp - function App_Update():**
```cpp
void App_Update(float dt, _GAMESTATE& state, AppState& appState) {
    // ⭐ TRACK FRAME TIME
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
    lastTime = now;

    static int frameCounter = 0;
    frameCounter++;

    if (frameCounter % 60 == 0) {  // In cứ 60 frames
        std::cout << "📊 Frame Time: " << frameTime << "ms" << std::endl;
        frameCounter = 0;
    }

    switch (appState) {
    case STATE_MENU:    App_Update_Menu(dt, state);             break;
    case STATE_PLAYING: App_Update_Playing(dt, state, appState);   break;
    default: break;
    }
}
```

### **Step 3: Test Winning Move Detection**

**Tạo test function:**
```cpp
void TestAIWinningMove() {
    _GAMESTATE state = {};
    ResetData(state);

    // ⭐ SET UP: AI = O (c=1), Human = X (c=-1)
    // Scenario: AI can win in 1 move
    // X X X _ O
    // O O O _ X
    // Vị trí (1, 3) là winning move cho AI

    // Set up board manually
    state._BOARD[0][0].c = -1;  // X
    state._BOARD[0][1].c = -1;  // X
    state._BOARD[0][2].c = -1;  // X
    state._BOARD[0][3].c = 0;   // Empty
    state._BOARD[0][4].c = 1;   // O

    state._BOARD[1][0].c = 1;   // O
    state._BOARD[1][1].c = 1;   // O
    state._BOARD[1][2].c = 1;   // O
    state._BOARD[1][3].c = 0;   // Empty ← WINNING MOVE HERE
    state._BOARD[1][4].c = -1;  // X

    state.mode = MODE_PVE;
    state.difficulty = AI_HARD;
    state.turn = false;  // AI turn

    Move best = AI_FindBestMove(state);

    std::cout << "Best move: (" << best.row << ", " << best.col << ")\n";
    std::cout << "Expected: (1, 3)\n";

    if (best.row == 1 && best.col == 3) {
        std::cout << "✅ AI found winning move!\n";
    } else {
        std::cout << "❌ AI MISSED winning move!\n";
    }
}
```

### **Step 4: Analyze Results**

Chạy game và ghi lại:
```
AI Time vs Difficulty:
- EASY (depth=2):    ? ms
- MEDIUM (depth=4):  ? ms
- HARD (depth=6):    ? ms

Frame Time:
- Before: ? ms
- After AI move: ? ms
- Lag threshold: > 50ms = noticeable lag
```

---

## 📈 **Benchmark Checklist**

### **Performance Metrics**
- [ ] AI Move Time (EASY/MEDIUM/HARD)
- [ ] Frame Time (before/during/after AI)
- [ ] Max Frame Time (detect spikes)
- [ ] Average FPS (should be 60)
- [ ] Memory Usage (before/after game)

### **AI Quality Metrics**
- [ ] Winning Move Detection (yes/no)
- [ ] Defensive Move (block opponent)
- [ ] Move Distribution (smart vs random)
- [ ] First Move (center bias)
- [ ] Win Rate (vs random player)

### **Rendering Metrics**
- [ ] Animation Frame Rate
- [ ] Render Time per layer
- [ ] UI Render Time
- [ ] Total Frame Time

---

## 🔧 **Diagnosis Guide**

### **If FPS < 30:**
```
Suspected Issues:
1. ❌ AI depth too deep → reduce from 6 to 4
2. ❌ Too many candidates → improve GetCandidates()
3. ❌ Memory leak → check for dangling pointers
4. ❌ Renderer bottleneck → profile Renderer_DrawPieces()
```

### **If AI misses winning move:**
```
Suspected Issues:
1. ❌ CheckTerminal() bug → test with manual board
2. ❌ CountLine() overflow → check for WIN_COUNT >= 5
3. ❌ Move not in candidates → debug GetCandidates()
4. ❌ Alpha-Beta pruning too aggressive → test without pruning
```

### **If animation stutters:**
```
Suspected Issues:
1. ❌ Fixed timestep issue → check FIXED_STEP
2. ❌ dt too large → cap dt at 0.25s
3. ❌ Animation update wrong → check Animation_Update()
4. ❌ Render order → verify layer drawing order
```

---

## 📊 **Expected Benchmarks**

### **Good Performance**
| Metric | Expected |
|--------|----------|
| EASY AI Time | < 50ms |
| MEDIUM AI Time | < 200ms |
| HARD AI Time | 1-2s |
| Frame Time | 16.67ms (60 FPS) |
| Animation Time | 2-3s smooth |
| Max Frame Spike | < 100ms |

### **Bad Performance** 🔴
| Symptom | Threshold |
|---------|-----------|
| Lag detected | Frame time > 50ms |
| Stutter visible | FPS drops < 30 |
| AI sluggish | Move time > 5s |
| Memory leak | Growing over time |

---

## 🎯 **Quick Diagnosis Steps**

### **1. Check AI Speed**
```cpp
// Thêm vào AIPlayer.cpp
static int nodeCount = 0;

// Trong Minimax():
nodeCount++;

// Sau AI_FindBestMove():
std::cout << "Nodes explored: " << nodeCount << "\n";
```

### **2. Check Frame Drops**
```cpp
// Thêm vào App.cpp
if (dt > 0.05f) {  // > 50ms
    std::cout << "⚠️  FRAME SPIKE: " << dt*1000 << "ms\n";
}
```

### **3. Check Animation Smooth**
```cpp
// Trong Animation_Update()
static float animTime = 0;
animTime += dt;
std::cout << "Anim Time: " << animTime << "s\n";
```

### **4. Check AI Quality**
```cpp
// Manually test scenarios:
// - AI should find 3-in-a-row threats
// - AI should block opponent 3-in-a-row
// - AI should build toward 4-in-a-row
```

---

## 🚀 **Optimization Tips**

### **Nếu AI quá chậm:**
1. **Giảm depth** từ 6 → 4
2. **Tối ưu GetCandidates()** - limit radius từ 2 → 1.5
3. **Add transposition table** - cache results
4. **Killer move heuristic** - reorder moves
5. **Parallel search** - run on separate thread

### **Nếu animation lag:**
1. **Check FPS** - should be 60
2. **Profile render layers** - find bottleneck
3. **Simplify animation** - fewer state changes
4. **Async AI** - move to thread

### **Nếu AI miss winning move:**
1. **Debug CheckTerminal()** - add print statements
2. **Test CountLine()** - manual verification
3. **Verify GetCandidates()** - check winning move in list
4. **Compare with previous move** - was it pruned?

---

## 📋 **Testing Checklist**

### **AI Quality Tests**
- [ ] Can win in 1 move (EASY)
- [ ] Can block opponent 3-in-a-row (MEDIUM)
- [ ] Builds long sequences (HARD)
- [ ] First move near center
- [ ] Doesn't play random corner moves

### **Performance Tests**
- [ ] EASY AI responds in < 100ms
- [ ] MEDIUM AI responds in < 500ms
- [ ] HARD AI responds in < 3s
- [ ] No frame drops during AI thinking
- [ ] Animation remains smooth at 60 FPS

### **Integration Tests**
- [ ] PVP mode works smoothly
- [ ] PVE mode works (all difficulties)
- [ ] Save/Load preserves state
- [ ] Win detection works
- [ ] Draw detection works

---

## 📞 **Output Format for Analysis**

```
╔════════════════════════════════════════════════════╗
║           CARO GAME BENCHMARK REPORT              ║
╚════════════════════════════════════════════════════╝

📊 PERFORMANCE METRICS:
  Average Frame Time: 16.8ms ✅
  Target Frame Time: 16.67ms (60 FPS)
  Min Frame Time: 15.2ms
  Max Frame Time: 42.1ms ⚠️
  Frame Drops: 2 (> 50ms)

⏱️  AI PERFORMANCE:
  EASY (depth=2): 15ms ✅
  MEDIUM (depth=4): 85ms ✅
  HARD (depth=6): 1850ms ✅

🤖 AI QUALITY:
  Winning Move Detection: ✅
  Defensive Play: ✅
  Strategic Play: ✅
  Win Rate (vs Random): 95%

✨ OVERALL: GOOD ✅
  Recommendations:
  1. HARD difficulty slightly slow (> 2s)
  2. Occasional frame spike detected
  3. Consider thread-based AI for HARD
```

---

**Bắt đầu benchmark ngay!** 🚀
