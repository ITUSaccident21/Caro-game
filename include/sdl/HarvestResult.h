#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include "game/GameDef.h"

enum HarvestPhase {
    RESULT_NONE = 0,
    RESULT_HARVEST_WIN_LINE,
    RESULT_FRUIT_LIFT,
    RESULT_BASKET_DROP,
    RESULT_BASKET_IDLE
};

enum HarvestResultType {
    STRAWBERRY_RESULT = 0,
    BLUEBERRY_RESULT,
    DRAW_RESULT
};

void HarvestResult_Init(SDL_Renderer* r);
void HarvestResult_Shutdown();

void HarvestResult_Start(const _GAMESTATE& state, int gameResult);

void HarvestResult_Update(float dt);
void HarvestResult_Render(SDL_Renderer* r, const _GAMESTATE& state);
void HarvestResult_Reset();

bool HarvestResult_IsActive();
bool HarvestResult_IsAnimating();
HarvestPhase      HarvestResult_Phase();
HarvestResultType HarvestResult_Type();

bool HarvestResult_ShouldHideCell(int row, int col);

