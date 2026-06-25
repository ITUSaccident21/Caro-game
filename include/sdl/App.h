#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "game/GameDef.h"

//    App_Init()  →  App_Run()  →  App_Shutdown()

struct AppContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool          running = false;

    // Fixed timestep
    Uint64 prevTicks = 0;
    float  accumulator = 0.0f;
};

bool App_Init(AppContext& ctx, _GAMESTATE& state, AppState& appState);

void App_Run(AppContext& ctx, _GAMESTATE& state, AppState& appState);

void App_Shutdown(AppContext& ctx);

void App_HandleEvents(AppContext& ctx, _GAMESTATE& state, AppState& appState);

void App_Update(float dt, _GAMESTATE& state, AppState& appState);

void App_Render(AppContext& ctx, _GAMESTATE& state, AppState& appState);

void App_OnEvent_Menu(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_NameInput(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_Playing(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_LoadGame(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_Settings(const SDL_Event& e, _GAMESTATE& state, AppState& appState);

void App_Update_Menu(float dt, _GAMESTATE& state, AppState& appState);
void App_Update_Splash(float dt, _GAMESTATE& state, AppState& appState);
void App_Update_Playing(float dt, _GAMESTATE& state, AppState& appState);

void App_TransitionTo(AppState newState, AppState& appState, _GAMESTATE& state);
void App_StartNewGame(_GAMESTATE& state);
void App_StartNewSession(_GAMESTATE& state);

void App_TriggerAITurn(_GAMESTATE& state);

void App_PlacePiece(_GAMESTATE& state, int row, int col);

bool App_PixelToCell(int px, int py, int& outRow, int& outCol);

void App_CellToPixel(int row, int col, int& outX, int& outY);