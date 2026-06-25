#pragma once
#include "game/GameDef.h"

struct Move {
    int row = -1;
    int col = -1;
};

struct AIBenchResult {
    float timeMs            = 0.0f;
    int   nodesVisited      = 0;
    int   nodesPruned       = 0;
    float pruneRatio        = 0.0f;
    int   sortCalls         = 0;
    int   candidatesSkipped = 0;
    float realPruneRatio    = 0.0f;
    int   depth             = 0;
    int   rootCandidates    = 0;
    int   moveNumber        = 0;
};

Move             AI_FindBestMove(const _GAMESTATE& state);
AIBenchResult    AI_GetLastBenchmark();
void             AI_SetMoveNumber(int n);
