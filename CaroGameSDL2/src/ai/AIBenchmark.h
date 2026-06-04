#pragma once
#include "../game/GameDef.h"
#include <chrono>
#include <vector>

// ================================================================
//  AIBenchmark.h — AI performance testing
//  Đo lường tốc độ AI, chất lượng nước đi, và phát hiện bugs
// ================================================================

struct AITestResult {
    int depth;
    int nodesExplored;
    double timeMs;
    int moveRow, moveCol;
    int score;
    bool isWinningMove;
};

class AIBenchmark {
public:
    // Test AI tốc độ ở các depth khác nhau
    static void BenchmarkAI(_GAMESTATE& state);

    // Test xem AI có phát hiện winning move không
    static void TestWinningMoveDection(_GAMESTATE& state);

    // Test xem AI có chặn threatening move của opponent không
    static void TestDefensivePlay(_GAMESTATE& state);

    // In chi tiết về board state hiện tại
    static void PrintBoardState(const _GAMESTATE& state);

    // Kiểm tra xem move có phải winning move không
    static bool IsWinningMove(const _GAMESTATE& state, int row, int col);
};

// ================================================================
//  Performance Metrics
// ================================================================

class PerformanceMetrics {
public:
    struct FrameMetrics {
        double frameTime;      // ms
        double updateTime;     // ms
        double renderTime;     // ms
        double aiTime;         // ms
        int fps;
    };

    static FrameMetrics currentFrame;

    static void StartFrame() {
        frameStart = std::chrono::high_resolution_clock::now();
    }

    static void EndFrame() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - frameStart);
        currentFrame.frameTime = duration.count() / 1000.0;
        currentFrame.fps = static_cast<int>(1000.0 / currentFrame.frameTime);
    }

    static void PrintMetrics() {
        std::cout << "📊 FRAME METRICS:\n";
        std::cout << "  Frame Time: " << currentFrame.frameTime << "ms\n";
        std::cout << "  Update Time: " << currentFrame.updateTime << "ms\n";
        std::cout << "  Render Time: " << currentFrame.renderTime << "ms\n";
        std::cout << "  AI Time: " << currentFrame.aiTime << "ms\n";
        std::cout << "  FPS: " << currentFrame.fps << "\n";
    }

private:
    static std::chrono::high_resolution_clock::time_point frameStart;
};

// ================================================================
//  Memory Profiler
// ================================================================

class MemoryProfiler {
public:
    static void PrintMemoryUsage() {
        // TODO: Platform-specific memory query
        std::cout << "💾 Memory Usage: [TBD]\n";
    }
};
