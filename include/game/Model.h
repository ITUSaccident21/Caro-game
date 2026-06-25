#pragma once
#include "game/GameDef.h"

void NewSession(_GAMESTATE& state);

void ResetData(_GAMESTATE& state);

int CheckBoard(_GAMESTATE& state, int row, int col);

bool isBoardFull(_GAMESTATE& state);

bool winCheck(_GAMESTATE& state, int i, int j);

int TestBoard(_GAMESTATE& state);

int ProcessFinish(_GAMESTATE& state, int pWhoWin);