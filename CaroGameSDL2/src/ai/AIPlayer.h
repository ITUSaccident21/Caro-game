#pragma once
#include "../game/GameDef.h"

struct Move {
    int row = -1;
    int col = -1;
};

Move AI_FindBestMove(const _GAMESTATE& state);
