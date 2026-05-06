# 🔧 Troubleshooting Guide - Caro Game SDL2

> Hướng dẫn chẩn đoán và sửa các vấn đề phổ biến

---

## 🔴 **Problem 1: Lag khi Player vs AI**

### **Triệu chứng**
- ❌ Màn hình đứng 1-2 giây khi AI thinking
- ❌ Animation bị cắt ngang
- ❌ Không thể click gì trong lúc AI move
- ❌ FPS drop đột ngột

### **Nguyên Nhân Gốc Rễ**

```
AI_FindBestMove() chạy trên main thread
↓
Minimax recursive chặn game loop
↓
Animation không update
↓
Render không được gọi
↓
Người dùng thấy màn hình đứng
```

### **Giải Pháp (Priority Order)**

#### **✅ Quick Fix (Ngay Lập Tức)**

**1. Giảm AI Depth**

```cpp
// Trong GameDef.h
enum AIDifficulty { 
    AI_EASY = 2,      // ← Thử 2 thay vì 6
    AI_MEDIUM = 3,    // ← Thử 3 thay vì 4
    AI_HARD = 4       // ← Thử 4 thay vì 6
};
```

**Test**: Có mượt hơn không?
- ✅ Nếu YES → Vấn đề là AI depth quá sâu
- ❌ Nếu NO → Tiếp tục bước 2

---

**2. Kiểm Tra Frame Rate**

```cpp
// Trong App.cpp - App_Update()
static auto lastTime = std::chrono::high_resolution_clock::now();
static int frameCount = 0;

auto now = std::chrono::high_resolution_clock::now();
auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
lastTime = now;

frameCount++;
if (frameCount % 60 == 0) {
    std::cout << "Frame time: " << deltaTime.count() << "ms" << std::endl;
    std::cout << "FPS: " << (1000.0 / deltaTime.count()) << std::endl;
}
```

**Kỳ vọng**: Frame time < 20ms, FPS > 50

---

**3. Kiểm Tra AI Time**

```cpp
// Trong App_TriggerAITurn()
auto start = std::chrono::high_resolution_clock::now();
Move best = AI_FindBestMove(state);
auto end = std::chrono::high_resolution_clock::now();
auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

std::cout << "🤖 AI took: " << timeMs.count() << "ms" << std::endl;

if (timeMs.count() > 1000) {
    std::cout << "⚠️  WARNING: AI too slow!" << std::endl;
}
```

**Kỳ vọng**:
- EASY: < 50ms
- MEDIUM: < 200ms
- HARD: < 2000ms

---

#### **⭐ Proper Fix (Dài Hạn)**

**4. Async AI (Dùng Thread)**

```cpp
// Trong App.h
#include <thread>
#include <atomic>

struct AppContext {
    // ... existing fields
    std::thread* aiThread = nullptr;
    std::atomic<bool> aiDone = false;
    Move aiResult = {-1, -1};
};

// Trong App_TriggerAITurn()
void App_TriggerAITurn(_GAMESTATE& state) {
    state.aiThinking = true;

    // Spawn thread
    ctx.aiThread = new std::thread([&state]() {
        ctx.aiResult = AI_FindBestMove(state);
        ctx.aiDone = true;
    });
}

// Trong App_Update_Playing()
if (state.mode == MODE_PVE && state.turn == false && 
    !Animation_IsPlaying() && ctx.aiDone) {

    Move best = ctx.aiResult;
    ctx.aiThread->join();
    delete ctx.aiThread;
    ctx.aiDone = false;

    // Place move...
    state.aiThinking = false;
}
```

**Lợi ích**: Game không bị lock, FPS stable

---

### **Diagnostic Checklist**

- [ ] Depth = 2: Có lag không? (YES = lag, NO = không phải AI)
- [ ] Frame time < 20ms? (NO = render issue)
- [ ] AI time < 100ms ở depth=2? (NO = Minimax bug)
- [ ] Animation smooth? (NO = animation frame rate issue)

---

## 🔴 **Problem 2: AI Miss Winning Move**

### **Triệu Chứng**
- ❌ AI còn 1 nước nữa là thắng nhưng đánh chỗ khác
- ❌ AI không block opponent's winning move
- ❌ AI chơi random

### **Nguyên Nhân Tiềm Ẩn**

```
Khả năng xảy ra (theo thứ tự):
1. CheckTerminal() không detect ✅ winning move
2. CountLine() sai logic
3. GetCandidates() không include winning move
4. Alpha-Beta pruning cắt bỏ winning move
5. Move ordering tồi
```

### **Giải Pháp**

#### **Step 1: Test CheckTerminal()**

```cpp
// Tạo test function
void TestCheckTerminal() {
    _GAMESTATE state = {};
    ResetData(state);

    // Set up: O O O O _ (O có 4 liên tiếp, vị trí (0,4) winning)
    state._BOARD[0][0].c = 1;
    state._BOARD[0][1].c = 1;
    state._BOARD[0][2].c = 1;
    state._BOARD[0][3].c = 1;
    state._BOARD[0][4].c = 0;  // Empty

    // Simulate AI placing at (0,4)
    state._BOARD[0][4].c = 1;

    int result = CheckTerminal(state, 0, 4);

    std::cout << "Result: " << result << std::endl;
    std::cout << "Expected: " << (INT_MAX/2) << " (AI wins)" << std::endl;

    if (result == INT_MAX/2) {
        std::cout << "✅ CheckTerminal works!\n";
    } else {
        std::cout << "❌ CheckTerminal BUG!\n";
        // Debug: print what was found
    }
}
```

**Run this test** → Nếu fail = bug ở CheckTerminal()

---

#### **Step 2: Test CountLine()**

```cpp
void TestCountLine() {
    _GAMESTATE state = {};
    ResetData(state);

    // Set up: _ O O O _
    state._BOARD[0][0].c = 0;
    state._BOARD[0][1].c = 1;
    state._BOARD[0][2].c = 1;
    state._BOARD[0][3].c = 1;
    state._BOARD[0][4].c = 0;

    // CountLine should be 3 quân, 2 open ends
    int score = CountLine(state, 0, 2, 0, 1, 1);  // Check horizontal

    std::cout << "Score: " << score << std::endl;
    std::cout << "Expected: 1000 (SCORES[3])" << std::endl;

    if (score == 1000) {
        std::cout << "✅ CountLine works!\n";
    } else {
        std::cout << "❌ CountLine BUG! Got " << score << "\n";
    }
}
```

---

#### **Step 3: Disable Alpha-Beta Pruning**

```cpp
// Trong AIPlayer.cpp - Minimax()
// Thay:
// if (beta <= alpha) break;

// Bằng:
// if (beta <= alpha) break;  // Disable temporarily
// if (false && beta <= alpha) break;  // Test without pruning

// Nếu AI bỗng nhiên thông minh hơn → Pruning cắt bỏ good moves
```

---

#### **Step 4: Check GetCandidates()**

```cpp
void DebugCandidates(const _GAMESTATE& state) {
    int cands[BOARD_SIZE * BOARD_SIZE][2];
    int count = 0;
    GetCandidates(state, cands, count);

    std::cout << "Total candidates: " << count << "\n";

    // Nếu winning move không trong list
    // ❌ Bug ở GetCandidates() - quá hạn chế
}
```

---

### **Diagnostic Checklist**

- [ ] CheckTerminal() returns INT_MAX/2 cho winning move? (NO = fix)
- [ ] CountLine() returns SCORES[5] = 10000000? (NO = fix)
- [ ] Winning move trong GetCandidates()? (NO = expand radius)
- [ ] Winning move survive alpha-beta? (NO = disable pruning)

---

## 🟡 **Problem 3: Animation Stutters**

### **Triệu Chứng**
- ❌ Animation giật từng bước thay vì mượt mà
- ❌ Nhân vật nhảy cóp chiêng
- ❌ FPS không ổn định

### **Giải Pháp**

**1. Check Fixed Timestep**

```cpp
// Trong App.cpp
static const float FIXED_STEP = 1.0f / FPS;  // Should be 1/60 = 0.01667

std::cout << "Fixed step: " << FIXED_STEP << "s\n";  // Should be ~0.01667
```

**2. Check Animation Update**

```cpp
// Trong Animation.cpp
void Animation_Update(float dt, _GAMESTATE& state) {
    // dt should be FIXED_STEP, not raw frame delta
    std::cout << "Anim dt: " << dt << "\n";  // Should be 0.01667
}
```

**3. Cap dt to prevent spiral of death**

```cpp
// Trong App_Run()
if (dt > 0.25f) dt = 0.25f;  // Already there?
```

---

## 🟡 **Problem 4: Crashes**

### **Common Causes**

```cpp
// 1. Null pointer in Renderer
if (!renderer) return;  // Add checks

// 2. Array out of bounds
if (row < 0 || row >= BOARD_SIZE) return;

// 3. Memory leak in SDL
if (window) SDL_DestroyWindow(window);

// 4. Dangling pointer
ptr = nullptr;  // After delete
```

### **Fix**

```cpp
// Add guards everywhere
bool App_PixelToCell(int px, int py, int& outRow, int& outCol) {
    if (px < 0 || py < 0) return false;  // Guard

    int col = (px - BOARD_OFFSET_X) / CELL_SIZE;
    int row = (py - BOARD_OFFSET_Y) / CELL_SIZE;

    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }
    outRow = row;
    outCol = col;
    return true;
}
```

---

## 🟢 **Verification Checklist**

### **Before Deployment**

- [ ] All tests pass
- [ ] FPS > 50 consistently
- [ ] AI finds winning move
- [ ] AI blocks opponent
- [ ] No crashes
- [ ] Animation smooth
- [ ] Save/Load works
- [ ] All difficulties work

### **During Testing**

- [ ] Play 10 full games (PVP)
- [ ] Play 10 games vs EASY
- [ ] Play 5 games vs HARD
- [ ] Test save/load
- [ ] Test mode switching
- [ ] Monitor memory (no growth)
- [ ] Monitor FPS (no drops)

---

## 📞 **When All Else Fails**

```cpp
// Add comprehensive logging
#define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", __VA_ARGS__)

// Log every important event
DEBUG_LOG("AI starting, difficulty=%d", state.difficulty);
DEBUG_LOG("Best move: (%d, %d) score=%d", best.row, best.col, bestScore);
DEBUG_LOG("Frame time: %.2fms", frameTime);

// Use git bisect to find when it broke
git bisect start
git bisect bad HEAD
git bisect good <commit>
```

---

**Good luck debugging!** 🚀
