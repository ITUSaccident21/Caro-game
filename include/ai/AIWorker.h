#pragma once
#include "game/GameDef.h"
#include "ai/AIPlayer.h"

// Non-blocking AI search worker: a tiny single-job "message passing" facade over

void AIWorker_Request(const _GAMESTATE& state);
bool AIWorker_Poll(Move& outMove);
void AIWorker_Cancel();
void AIWorker_Shutdown();
