#include "App.h"
#include "Renderer.h"
#include "AudioManager.h"
#include "UIManager.h"
#include "Animation.h"
#include "../game/Model.h"
#include "../game/FileHandling.h"
#include "../ai/AIPlayer.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cstring>

// ================================================================
//  App.cpp — Vòng lặp chính SDL2, hoàn toàn thủ tục
//  Không class, không method. Chỉ free functions.
// ================================================================

// ─── Hằng số vòng lặp ────────────────────────────────────────────
static const float FIXED_STEP = 1.0f / FPS;   // ~0.01667s

// ─── App_Init ────────────────────────────────────────────────────
bool App_Init(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    // 1. Khởi tạo SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init that bai: %s", SDL_GetError());
        return false;
    }

    // 2. Tạo cửa sổ
    ctx.window = SDL_CreateWindow(
        "Co Caro — Gomoku",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!ctx.window) {
        SDL_Log("SDL_CreateWindow that bai: %s", SDL_GetError());
        return false;
    }

    // 3. Tạo renderer (hardware accelerated + vsync)
    ctx.renderer = SDL_CreateRenderer(ctx.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer) {
        SDL_Log("SDL_CreateRenderer that bai: %s", SDL_GetError());
        return false;
    }

    // 4. Khởi tạo các subsystem theo thứ tự phụ thuộc
    if (!Renderer_Init(ctx.renderer))  return false;
    if (!AudioManager_Init())          return false;
    if (!UIManager_Init(ctx.renderer)) return false;
    Animation_Init();

    // 5. Dữ liệu game ban đầu
    ResetData(state);
    // Tên mặc định — UIManager sẽ ghi đè khi user nhập qua STATE_NAME_INPUT
    strcpy_s(state.players[0].name, sizeof(state.players[0].name), "Player 1");
    strcpy_s(state.players[1].name, sizeof(state.players[1].name), "Player 2");
    state.hoveredRow = BOARD_SIZE / 2;
    state.hoveredCol = BOARD_SIZE / 2;
    state.gameStatus = CHUA_KET_THUC;

    // 6. Trạng thái bắt đầu
    appState = STATE_MENU;
    ctx.running = true;
    ctx.prevTicks = SDL_GetTicks64();

    AudioManager_PlayBGM("assets/sounds/menu_bgm.ogg");
    return true;
}

// ─── App_Run — Fixed Timestep Loop ───────────────────────────────
void App_Run(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    while (ctx.running && appState != STATE_EXIT) {
        Uint64 now = SDL_GetTicks64();
        float dt = (now - ctx.prevTicks) / 1000.0f;
        ctx.prevTicks = now;

        // Giới hạn dt: tránh "spiral of death" khi bị lag
        if (dt > 0.25f) dt = 0.25f;
        ctx.accumulator += dt;

        // Xử lý events mỗi frame (không phụ thuộc timestep)
        App_HandleEvents(ctx, state, appState);

        // Logic update theo bước cố định
        while (ctx.accumulator >= FIXED_STEP) {
            App_Update(FIXED_STEP, state, appState);
            ctx.accumulator -= FIXED_STEP;
        }

        // Render
        App_Render(ctx, state, appState);
    }
}

// ─── App_Shutdown ────────────────────────────────────────────────
void App_Shutdown(AppContext& ctx) {
    Animation_Shutdown();
    UIManager_Shutdown();
    AudioManager_Shutdown();
    Renderer_Shutdown();

    if (ctx.renderer) {
        SDL_DestroyRenderer(ctx.renderer);
        ctx.renderer = nullptr;
    }
    if (ctx.window) {
        SDL_DestroyWindow(ctx.window);
        ctx.window = nullptr;
    }
    SDL_Quit();
}

// ─── App_HandleEvents ────────────────────────────────────────────
void App_HandleEvents(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        // Thoát bất kể đang ở state nào
        if (e.type == SDL_QUIT) {
            ctx.running = false;
            return;
        }

        // Routing đến handler của state hiện tại
        switch (appState) {
        case STATE_MENU:        App_OnEvent_Menu(e, state, appState); break;
        case STATE_NAME_INPUT:  App_OnEvent_NameInput(e, state, appState); break;
        case STATE_PLAYING:     App_OnEvent_Playing(e, state, appState); break;
        case STATE_LOAD_GAME:   App_OnEvent_LoadGame(e, state, appState); break;
        default: break;
        }
    }
}

// ─── App_Update ──────────────────────────────────────────────────
void App_Update(float dt, _GAMESTATE& state, AppState& appState) {
    switch (appState) {
    case STATE_MENU:    App_Update_Menu(dt, state);             break;
    case STATE_PLAYING: App_Update_Playing(dt, state, appState);   break;
    default: break;
    }
}

// ─── App_Render ──────────────────────────────────────────────────
void App_Render(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    // Nền đen trước khi vẽ
    SDL_SetRenderDrawColor(ctx.renderer, 20, 15, 10, 255);
    SDL_RenderClear(ctx.renderer);

    switch (appState) {
    case STATE_MENU:
        UIManager_RenderMenu(ctx.renderer);
        break;

    case STATE_NAME_INPUT:
        UIManager_RenderNameInput(ctx.renderer);
        break;

    case STATE_PLAYING:
        // Thứ tự layers: background → board → pieces → hover → character → HUD
        Renderer_DrawBackground(ctx.renderer);
        Renderer_DrawBoard(ctx.renderer, state);
        Renderer_DrawPieces(ctx.renderer, state);
        Renderer_DrawHover(ctx.renderer, state);
        Animation_Render(ctx.renderer, state);
        UIManager_RenderHUD(ctx.renderer, state);
        break;

    case STATE_LOAD_GAME:
        UIManager_RenderLoadScreen(ctx.renderer);
        break;

    default: break;
    }

    SDL_RenderPresent(ctx.renderer);
}

// ─── Update Handlers ─────────────────────────────────────────────
void App_Update_Menu(float dt, _GAMESTATE& state) {
    UIManager_UpdateMenu(dt);   // cập nhật rainbow animation
}

void App_Update_Playing(float dt, _GAMESTATE& state, AppState& appState) {
    // Cập nhật animation nhân vật
    Animation_Update(dt, state);

    // Nếu là lượt AI, animation đã xong, game chưa kết thúc
    if (state.mode == MODE_PVE
        && state.turn == false              // false = Player 2 = AI
        && !state.aiThinking
        && !Animation_IsPlaying()
        && state.gameStatus == CHUA_KET_THUC) {
        App_TriggerAITurn(state);
    }
}

// ─── Event Handlers ──────────────────────────────────────────────
void App_OnEvent_Menu(const SDL_Event& e, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_HandleMenuEvent(e);
    if (next != STATE_MENU) {
        App_TransitionTo(next, appState, state);
    }
}

void App_OnEvent_NameInput(const SDL_Event& e, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_HandleNameInputEvent(e, state);
    if (next != STATE_NAME_INPUT) {
        App_TransitionTo(next, appState, state);
    }
}

void App_OnEvent_Playing(const SDL_Event& e, _GAMESTATE& state, AppState& appState) {
    // Khóa input khi animation đang chạy hoặc AI đang tính
    if (Animation_IsPlaying() || state.aiThinking) return;

    // ESC và R luôn hoạt động kể cả khi game kết thúc
    if (e.type == SDL_KEYDOWN) {
        if (state.gameStatus != CHUA_KET_THUC) {
            switch (e.key.keysym.sym) {
            case SDLK_r:
                App_StartNewSession(state);
                return;
            case SDLK_ESCAPE:
                App_TransitionTo(STATE_MENU, appState, state);
                return;
            default:
                return;  // ignore all other input after game ends
            }
        }
        // Game still in progress: ESC goes to menu
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            App_TransitionTo(STATE_MENU, appState, state);
            return;
        }
    }

    // Game is over — ignore mouse input too
    if (state.gameStatus != CHUA_KET_THUC) return;

    // Chỉ nhận input khi là lượt người chơi
    bool isHumanTurn = (state.mode == MODE_PVP)
        || (state.mode == MODE_PVE && state.turn == true);
    if (!isHumanTurn) return;

    if (e.type == SDL_MOUSEMOTION) {
        App_PixelToCell(e.motion.x, e.motion.y,
            state.hoveredRow, state.hoveredCol);
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int row, col;
        if (App_PixelToCell(e.button.x, e.button.y, row, col)) {
            if (state._BOARD[row][col].c == 0) {
                // Trigger animation đi đến ô — board chỉ được ghi khi animation xong
                state.selectedRow = row;
                state.selectedCol = col;
                Animation_StartMove(state, row, col, 0);
                AudioManager_PlaySFX(SFX_MOVE);
            }
        }
    }
    else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            if (state.hoveredRow > 0)             state.hoveredRow--; break;
        case SDLK_s:
            if (state.hoveredRow < BOARD_SIZE - 1) state.hoveredRow++; break;
        case SDLK_a:
            if (state.hoveredCol > 0)             state.hoveredCol--; break;
        case SDLK_d:
            if (state.hoveredCol < BOARD_SIZE - 1) state.hoveredCol++; break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: {
            int r = state.hoveredRow, c = state.hoveredCol;
            if (r >= 0 && c >= 0 && state._BOARD[r][c].c == 0) {
                state.selectedRow = r;
                state.selectedCol = c;
                Animation_StartMove(state, r, c, 0);
                AudioManager_PlaySFX(SFX_MOVE);
            }
            break;
        }
        case SDLK_l: // Lưu game
            UIManager_ShowSaveDialog(state); break;
        case SDLK_t: // Tải game
            App_TransitionTo(STATE_LOAD_GAME, appState, state); break;
        default: break;
        }
    }
}

void App_OnEvent_LoadGame(const SDL_Event& e, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_HandleLoadEvent(e, state);
    if (next != STATE_LOAD_GAME) {
        App_TransitionTo(next, appState, state);
    }
}

// ─── AI Turn ─────────────────────────────────────────────────────
void App_TriggerAITurn(_GAMESTATE& state) {
    state.aiThinking = true;
    Move best = AI_FindBestMove(state);
    state.aiThinking = false;

    if (best.row >= 0 && best.col >= 0) {
        state.selectedRow = best.row;
        state.selectedCol = best.col;
        Animation_StartMove(state, best.row, best.col, 1); // 1 = AI character
        AudioManager_PlaySFX(SFX_MOVE);
    }
}

// ─── Place Piece ─────────────────────────────────────────────────
// Hàm này được gọi bởi Animation_Update khi hoạt ảnh PLACING kết thúc
void App_PlacePiece(_GAMESTATE& state, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    if (state._BOARD[row][col].c != 0) return;

    // Ghi quân vào board
    int color = state.turn ? -1 : 1;   // turn=true → X(-1), turn=false → O(1)
    state._BOARD[row][col].c = color;
    state._LastI = row;
    state._LastJ = col;
    state.players[state.turn ? 0 : 1].moves++;

    // Kiểm tra kết quả — tái sử dụng Model.cpp cũ
    int result = TestBoard(state);
    ProcessFinish(state, result);

    if (result == P1_THANG || result == P2_THANG) {
        Animation_StartCelebrate(state, state.turn ? 0 : 1);
        AudioManager_PlaySFX(SFX_WIN);
        UIManager_ShowResult(state, result);
    }
    else if (result == HOA) {
        AudioManager_PlaySFX(SFX_DRAW);
        UIManager_ShowResult(state, result);
    }
}

// ─── Transitions ─────────────────────────────────────────────────
void App_TransitionTo(AppState newState, AppState& appState, _GAMESTATE& state) {
    appState = newState;
    switch (newState) {
    case STATE_MENU:
        AudioManager_PlayBGM("assets/sounds/menu_bgm.ogg");
        UIManager_ShowMenu();
        break;
    case STATE_NAME_INPUT:
        UIManager_ShowNameInput(state);
        break;
    case STATE_PLAYING:
        App_StartNewGame(state);
        AudioManager_PlayBGM("assets/sounds/game_bgm.ogg");
        break;
    case STATE_LOAD_GAME:
        UIManager_ShowLoadScreen();
        break;
    case STATE_EXIT:
        break;
    }
}

void App_StartNewGame(_GAMESTATE& state) {
    ResetData(state);
    state.hoveredRow = BOARD_SIZE / 2;
    state.hoveredCol = BOARD_SIZE / 2;
    state.gameStatus = CHUA_KET_THUC;
    state.aiThinking = false;
    Animation_Reset();
}

void App_StartNewSession(_GAMESTATE& state) {
    NewSession(state);
    state.hoveredRow = BOARD_SIZE / 2;
    state.hoveredCol = BOARD_SIZE / 2;
    state.gameStatus = CHUA_KET_THUC;
    state.aiThinking = false;
    Animation_Reset();
    UIManager_HideResult();
}

// ─── Coordinate Conversion ────────────────────────────────────────
bool App_PixelToCell(int px, int py, int& outRow, int& outCol) {
    int col = (px - BOARD_OFFSET_X) / CELL_SIZE;
    int row = (py - BOARD_OFFSET_Y) / CELL_SIZE;

    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        outRow = -1;
        outCol = -1;
        return false;
    }
    outRow = row;
    outCol = col;
    return true;
}

void App_CellToPixel(int row, int col, int& outX, int& outY) {
    outX = BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2;
    outY = BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2;
}