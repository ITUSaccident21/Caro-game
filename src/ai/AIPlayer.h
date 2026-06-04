#pragma once
#include "../game/GameDef.h"

struct Move {
    int row = -1;
    int col = -1;
};

struct AIBenchResult {
    float timeMs            = 0.0f;
    int   nodesVisited      = 0;
    int   nodesPruned       = 0;     // số lần break xảy ra (break events)
    float pruneRatio        = 0.0f;  // cũ: nodesPruned / (visited + pruned)
    int   sortCalls         = 0;     // DR-004: số lần SortCandidates thực sự chạy
    int   candidatesSkipped = 0;     // số candidates thực sự bị bỏ qua do cắt tỉa
    float realPruneRatio    = 0.0f;  // candidatesSkipped / (visited + skipped) — đúng hơn
    int   depth             = 0;
    int   rootCandidates    = 0;
    int   moveNumber        = 0;
};

Move             AI_FindBestMove(const _GAMESTATE& state);
AIBenchResult    AI_GetLastBenchmark();
void             AI_SetMoveNumber(int n);  // gọi từ App khi đặt quân
