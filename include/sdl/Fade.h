#pragma once
#include <SDL.h>
#include "game/GameDef.h"

void Fade_Start(AppState target, _GAMESTATE* statePtr);
void Fade_Update(float dt, AppState& appState, _GAMESTATE& state);
void Fade_Render(SDL_Renderer* r);
