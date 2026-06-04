#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "../game/GameDef.h"

// ================================================================
//  App.h — Vòng lặp chính SDL2, hoàn toàn thủ tục
//  Không class, không OOP. Chỉ struct thuần + free functions.
//
//  Luồng gọi từ main.cpp:
//    App_Init()  →  App_Run()  →  App_Shutdown()
// ================================================================

// ─── AppContext ──────────────────────────────────────────────────
// Thay thế cho "class App" — đóng gói SDL2 handles và
// trạng thái vòng lặp vào một struct thuần dữ liệu
struct AppContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool          running = false;

    // Fixed timestep
    Uint64 prevTicks = 0;
    float  accumulator = 0.0f;
};

// ─── Public API ──────────────────────────────────────────────────

// Khởi tạo SDL2, tạo window + renderer, load tất cả subsystems
// Trả về false nếu thất bại
bool App_Init(AppContext& ctx, _GAMESTATE& state, AppState& appState);

// Vòng lặp chính — chạy cho đến khi appState == STATE_EXIT
void App_Run(AppContext& ctx, _GAMESTATE& state, AppState& appState);

// Dọn dẹp toàn bộ SDL2 resources
void App_Shutdown(AppContext& ctx);

// ─── Game loop phases ────────────────────────────────────────────

// Xử lý toàn bộ SDL event queue trong một frame
void App_HandleEvents(AppContext& ctx, _GAMESTATE& state, AppState& appState);

// Cập nhật logic game (fixed step ~16.67ms)
void App_Update(float dt, _GAMESTATE& state, AppState& appState);

// Vẽ toàn bộ frame lên renderer
void App_Render(AppContext& ctx, _GAMESTATE& state, AppState& appState);

// ─── Event handlers theo từng AppState ──────────────────────────
void App_OnEvent_Menu(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_NameInput(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_Playing(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_LoadGame(const SDL_Event& e, _GAMESTATE& state, AppState& appState);
void App_OnEvent_Settings(const SDL_Event& e, _GAMESTATE& state, AppState& appState);

// ─── Update handlers theo từng AppState ─────────────────────────
void App_Update_Menu(float dt, _GAMESTATE& state, AppState& appState);
void App_Update_Splash(float dt, _GAMESTATE& state, AppState& appState);
void App_Update_Playing(float dt, _GAMESTATE& state, AppState& appState);

// ─── Transitions ─────────────────────────────────────────────────
void App_TransitionTo(AppState newState, AppState& appState, _GAMESTATE& state);
void App_StartNewGame(_GAMESTATE& state);  // ResetData + init SDL cursor
void App_StartNewSession(_GAMESTATE& state);  // NewSession (giữ điểm)

// ─── AI turn ─────────────────────────────────────────────────────
// Gọi AI_FindBestMove, apply move, trigger animation
void App_TriggerAITurn(_GAMESTATE& state);

// ─── Place piece ─────────────────────────────────────────────────
// Ghi quân vào board, kiểm tra thắng/hòa
// Được gọi bởi Animation khi hoạt ảnh PLACING kết thúc
void App_PlacePiece(_GAMESTATE& state, int row, int col);

// ─── Coordinate conversion ───────────────────────────────────────
// Mouse pixel → chỉ số ô bàn cờ (trả về false nếu ngoài bàn)
bool App_PixelToCell(int px, int py, int& outRow, int& outCol);

// Chỉ số ô → tọa độ pixel tâm ô (để đặt sprite, nhân vật)
void App_CellToPixel(int row, int col, int& outX, int& outY);