#include "sdl/App.h"
#include "sdl/Fade.h"
#include "sdl/Renderer.h"
#include "sdl/AudioManager.h"
#include "sdl/UIManager.h"
#include "sdl/HarvestResult.h"
#include "sdl/Particle.h"
#include "game/Model.h"
#include "game/FileHandling.h"
#include "ai/AIPlayer.h"
#include "ai/AIWorker.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>

static int   s_lastHoverSfxRow = -2;
static int   s_lastHoverSfxCol = -2;
static float s_turnDelay       = 0.0f;
static const float TURN_DELAY_S = 0.42f;

static const float FIXED_STEP = 1.0f / FPS;

// ─── App_Init ────────────────────────────────────────────────────
bool App_Init(AppContext& ctx, _GAMESTATE& state, AppState& appState) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init that bai: %s", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    ctx.window = SDL_CreateWindow(
        "Co Caro — Gomoku",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        0, 0,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    if (!ctx.window) {
        SDL_Log("SDL_CreateWindow that bai: %s", SDL_GetError());
        return false;
    }

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!ctx.renderer) {
        SDL_Log("SDL_CreateRenderer that bai: %s", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(ctx.renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (!Renderer_Init(ctx.renderer))  return false;
    Renderer_CreatePieceTextures(ctx.renderer);
    HarvestResult_Init(ctx.renderer);
    if (!AudioManager_Init())          return false;
    if (!UIManager_Init(ctx.renderer)) return false;
    Particle_Init(ctx.renderer);

    ResetData(state);
    strcpy_s(state.players[0].name, sizeof(state.players[0].name), "Player 1");
    strcpy_s(state.players[1].name, sizeof(state.players[1].name), "Player 2");
    state.hoveredRow = BOARD_SIZE / 2;
    state.hoveredCol = BOARD_SIZE / 2;
    state.gameStatus = CHUA_KET_THUC;

    srand(static_cast<unsigned>(time(nullptr)));
    appState = STATE_SPLASH;
    ctx.running = true;
    ctx.prevTicks = SDL_GetTicks64();
    UIManager_ShowSplash();
    return true;
}

// ─── App_Run — Fixed Timestep Loop ───────────────────────────────
void App_Run(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    while (ctx.running && appState != STATE_EXIT) {
        Uint64 now = SDL_GetTicks64();
        float dt = (now - ctx.prevTicks) / 1000.0f;
        ctx.prevTicks = now;

        if (dt > 0.25f) dt = 0.25f;
        ctx.accumulator += dt;

        App_HandleEvents(ctx, state, appState);

        while (ctx.accumulator >= FIXED_STEP) {
            App_Update(FIXED_STEP, state, appState);
            ctx.accumulator -= FIXED_STEP;
        }

        App_Render(ctx, state, appState);
    }
}

// ─── App_Shutdown ────────────────────────────────────────────────
void App_Shutdown(AppContext& ctx) {
    AIWorker_Shutdown();

    Particle_Shutdown();
    UIManager_Shutdown();
    AudioManager_Shutdown();
    HarvestResult_Shutdown();
    Renderer_DestroyPieceTextures();
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

void App_HandleEvents(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {

        if (e.type == SDL_QUIT) {
            ctx.running = false;
            return;
        }

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F6) {
            Renderer_CyclePieceReadability();
            continue;
        }

        switch (appState) {
        case STATE_SPLASH:
            if (e.type==SDL_KEYDOWN || e.type==SDL_MOUSEBUTTONDOWN)
                App_TransitionTo(STATE_MENU, appState, state);
            break;
        case STATE_MENU:        App_OnEvent_Menu(e, state, appState);     break;
        case STATE_NAME_INPUT:  App_OnEvent_NameInput(e, state, appState); break;
        case STATE_PLAYING:     App_OnEvent_Playing(e, state, appState);   break;
        case STATE_LOAD_GAME:   App_OnEvent_LoadGame(e, state, appState);  break;
        case STATE_SETTINGS:    App_OnEvent_Settings(e, state, appState);  break;
        default: break;
        }
    }
}

void App_Update(float dt, _GAMESTATE& state, AppState& appState) {
    Fade_Update(dt, appState, state);
    switch (appState) {
    case STATE_SPLASH:  App_Update_Splash(dt, state, appState);    break;
    case STATE_MENU:    App_Update_Menu(dt, state, appState);      break;
    case STATE_PLAYING: App_Update_Playing(dt, state, appState);   break;
    default: break;
    }
}

// Layer-ordered compositing for STATE_PLAYING. [paint #N] = real draw order.

static void App_RenderMatchScene(AppContext& ctx, _GAMESTATE& state) {
    SDL_Renderer* r = ctx.renderer;

    Renderer_DrawMatchBackground(r);
    Renderer_DrawBoard(r);
    Renderer_DrawPieces(r, state);
    Renderer_DrawMarkers(r, state);

    Particle_Render(r);

    Renderer_DrawForeground(r);

    UIManager_RenderHUD(r, state);
    HarvestResult_Render(r, state);

    if (Renderer_IsPieceDebugShown()) {
        char dbg[48];
        std::snprintf(dbg, sizeof(dbg), "PIECE: %s", Renderer_GetPieceReadabilityName());
        SDL_Color c = { 255, 0, 255, 255 };
        UIManager_RenderText(r, dbg, WINDOW_WIDTH / 2, 100, c, true, 1);
    }

    UIManager_RenderPauseOverlay(r);
    UIManager_RenderSaveNameDialog(r);
}

void App_Render(AppContext& ctx, _GAMESTATE& state, AppState& appState) {
    UIManager_BeginFrame();
    if (appState == STATE_SPLASH) {
        UIManager_RenderSplash(ctx.renderer);
        SDL_RenderPresent(ctx.renderer);
        return;
    }

    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx.renderer);

    switch (appState) {
    case STATE_MENU:
        UIManager_RenderMenu(ctx.renderer);
        break;

    case STATE_NAME_INPUT:
        UIManager_RenderNameInput(ctx.renderer);
        break;

    case STATE_PLAYING:
        App_RenderMatchScene(ctx, state);
        break;

    case STATE_SETTINGS:
        UIManager_RenderSettings(ctx.renderer);
        break;

    case STATE_LOAD_GAME:
        UIManager_RenderLoadScreen(ctx.renderer);
        break;

    default: break;
    }

    Fade_Render(ctx.renderer);
    SDL_RenderPresent(ctx.renderer);
}

void App_Update_Splash(float dt, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_UpdateSplash(dt);
    if (next != STATE_SPLASH)
        App_TransitionTo(next, appState, state);
}

void App_Update_Menu(float dt, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_UpdateMenu(dt);
    if (next != STATE_MENU)
        App_TransitionTo(next, appState, state);
}

void App_Update_Playing(float dt, _GAMESTATE& state, AppState& appState) {
    if (UIManager_IsPauseShown()) return;
    HarvestResult_Update(dt);
    Renderer_UpdatePop(dt);
    Particle_Update(dt);
    if (s_turnDelay > 0.0f) s_turnDelay -= dt;

    static float s_shimmerT = 0.0f;
    s_shimmerT += dt;
    if (state.gameStatus == CHUA_KET_THUC && s_shimmerT >= 0.2f &&
        state._LastI >= 0 && state._LastI < BOARD_SIZE &&
        state._LastJ >= 0 && state._LastJ < BOARD_SIZE &&
        state._BOARD[state._LastI][state._LastJ].c != 0) {
        s_shimmerT = 0.0f;
        int px, py; App_CellToPixel(state._LastI, state._LastJ, px, py);
        int fac = (state._BOARD[state._LastI][state._LastJ].c == -1) ? 0 : 1;
        Particle_ShimmerAt((float)px, (float)(py - 12), fac);
    }

    if (state.mode == MODE_PVE && state.gameStatus == CHUA_KET_THUC && !state.turn && s_turnDelay <= 0.0f) {
        if (state.aiThinking) {
            Move best;
            if (AIWorker_Poll(best)) {
                state.aiThinking = false;
                if (best.row >= 0 && best.col >= 0) {
                    state.selectedRow = best.row;
                    state.selectedCol = best.col;
                    AudioManager_PlaySFX(SFX_MOVE);
                    App_PlacePiece(state, best.row, best.col);
                } else {
                    state.gameStatus = HOA;
                    AudioManager_PlaySFX(SFX_DRAW);
                    HarvestResult_Start(state, HOA);
                    UIManager_ShowResult(state, HOA);
                }
            }
        } else {
            App_TriggerAITurn(state);
        }
    }
}

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
    if (UIManager_IsSaveNameDialogOpen()) {
        UIManager_HandleSaveNameDialogEvent(e, state);
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        if (state.gameStatus != CHUA_KET_THUC) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            App_TransitionTo(STATE_MENU, appState, state);
            return;
        }
        AudioManager_PlaySFX(SFX_MENU_SELECT);
        if (UIManager_IsPauseShown()) UIManager_HidePause();
        else                          UIManager_ShowPause();
        return;
    }

    if (UIManager_IsPauseShown()) {
        PauseAction action = UIManager_HandlePauseEvent(e, state);
        switch (action) {
        case PAUSE_RESUME:
            UIManager_HidePause();
            break;
        case PAUSE_RESTART:
            UIManager_HidePause();
            App_StartNewSession(state);
            break;
        case PAUSE_SAVE:
            UIManager_ShowSaveNameDialog();
            break;
        case PAUSE_QUIT:
            UIManager_HidePause();
            App_TransitionTo(STATE_MENU, appState, state);
            break;
        default: break;
        }
        return;
    }

    if (state.aiThinking || HarvestResult_IsAnimating() || s_turnDelay > 0.0f) return;

    if (e.type == SDL_KEYDOWN && state.gameStatus != CHUA_KET_THUC) {
        if (e.key.keysym.sym == SDLK_r) {
            AudioManager_PlaySFX(SFX_MENU_SELECT);
            App_StartNewSession(state);
        }
        return;
    }

    if (state.gameStatus != CHUA_KET_THUC) return;

    bool isHumanTurn = (state.mode == MODE_PVP)
        || (state.mode == MODE_PVE && state.turn == true);
    if (!isHumanTurn) return;

    if (e.type == SDL_MOUSEMOTION) {
        int oldRow = state.hoveredRow;
        int oldCol = state.hoveredCol;
        App_PixelToCell(e.motion.x, e.motion.y,
            state.hoveredRow, state.hoveredCol);
        if ((state.hoveredRow != oldRow || state.hoveredCol != oldCol) &&
            state.hoveredRow >= 0 && state.hoveredCol >= 0 &&
            state._BOARD[state.hoveredRow][state.hoveredCol].c == 0 &&
            (state.hoveredRow != s_lastHoverSfxRow || state.hoveredCol != s_lastHoverSfxCol)) {
            AudioManager_PlaySFX(SFX_MENU_HOVER);
            s_lastHoverSfxRow = state.hoveredRow;
            s_lastHoverSfxCol = state.hoveredCol;
        }
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int row, col;
        if (App_PixelToCell(e.button.x, e.button.y, row, col)) {
            if (state._BOARD[row][col].c == 0) {
                state.selectedRow = row;
                state.selectedCol = col;
                AudioManager_PlaySFX(SFX_MOVE);
                App_PlacePiece(state, row, col);
            } else {
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
        } else {
            AudioManager_PlaySFX(SFX_MENU_HOVER);
        }
    }
    else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            if (state.hoveredRow > 0) { state.hoveredRow--; AudioManager_PlaySFX(SFX_MENU_HOVER); } break;
        case SDLK_s:
            if (state.hoveredRow < BOARD_SIZE - 1) { state.hoveredRow++; AudioManager_PlaySFX(SFX_MENU_HOVER); } break;
        case SDLK_a:
            if (state.hoveredCol > 0) { state.hoveredCol--; AudioManager_PlaySFX(SFX_MENU_HOVER); } break;
        case SDLK_d:
            if (state.hoveredCol < BOARD_SIZE - 1) { state.hoveredCol++; AudioManager_PlaySFX(SFX_MENU_HOVER); } break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: {
            int r = state.hoveredRow, c = state.hoveredCol;
            if (r >= 0 && c >= 0 && state._BOARD[r][c].c == 0) {
                state.selectedRow = r;
                state.selectedCol = c;
                AudioManager_PlaySFX(SFX_MOVE);
                App_PlacePiece(state, r, c);
            } else {
                AudioManager_PlaySFX(SFX_MENU_HOVER);
            }
            break;
        }
        case SDLK_l:
            UIManager_ShowSaveNameDialog(); AudioManager_PlaySFX(SFX_MENU_SELECT); break;
        case SDLK_t:
            AudioManager_PlaySFX(SFX_MENU_SELECT); App_TransitionTo(STATE_LOAD_GAME, appState, state); break;
        default: break;
        }
    }
}

void App_OnEvent_LoadGame(const SDL_Event& e, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_HandleLoadEvent(e, state);
    if (next == STATE_PLAYING) {
        AIWorker_Cancel();
        state.aiThinking = false;
        state.hoveredRow = BOARD_SIZE / 2;
        state.hoveredCol = BOARD_SIZE / 2;
        Renderer_ResetCellAnims();
        HarvestResult_Reset();
        Particle_Clear();
        if (state.gameStatus != CHUA_KET_THUC)
            UIManager_ShowResult(state, state.gameStatus);
        else
            UIManager_HideResult();
        appState = STATE_PLAYING;
        AudioManager_PlayBGM("assets/sounds/game_bgm.ogg");
    } else if (next != STATE_LOAD_GAME) {
        App_TransitionTo(next, appState, state);
    }
}

void App_TriggerAITurn(_GAMESTATE& state) {
    state.aiThinking = true;
    AIWorker_Request(state);
}

void App_PlacePiece(_GAMESTATE& state, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    if (state._BOARD[row][col].c != 0) return;

    int color = state.turn ? -1 : 1;
    state._BOARD[row][col].c = color;
    Renderer_TriggerPiecePop(row, col);
    s_turnDelay = TURN_DELAY_S;
    { int px, py; App_CellToPixel(row, col, px, py);
      Particle_BurstDirt((float)px, (float)(py + 16)); }
    state._LastI = row;
    state._LastJ = col;
    state.players[state.turn ? 0 : 1].moves++;

    int result = TestBoard(state);
    ProcessFinish(state, result);

    if (result == P1_THANG || result == P2_THANG) {
        AudioManager_PlaySFX(SFX_WIN);
        int fac = (result == P2_THANG) ? 1 : 0;
        for (int k = 0; k < WIN_COUNT; k++) {
            int wc = state._WIN_CELLS[k].x, wr = state._WIN_CELLS[k].y;
            if (wr < 0 || wr >= BOARD_SIZE || wc < 0 || wc >= BOARD_SIZE) continue;
            int px, py; App_CellToPixel(wr, wc, px, py);
            Particle_BurstHarvest((float)px, (float)py, fac);
        }
        HarvestResult_Start(state, result);
        UIManager_ShowResult(state, result);
    }
    else if (result == HOA) {
        AudioManager_PlaySFX(SFX_DRAW);
        HarvestResult_Start(state, result);
        UIManager_ShowResult(state, result);
    }
}

void App_TransitionTo(AppState newState, AppState& appState, _GAMESTATE& state) {

    if (newState == STATE_SPLASH || newState == STATE_EXIT) {
        appState = newState;
        if (newState == STATE_SPLASH) UIManager_ShowSplash();
        return;
    }

    Fade_Start(newState, &state);
    (void)appState;
}

void App_StartNewGame(_GAMESTATE& state) {
    ResetData(state);
    AIWorker_Cancel();
    state.hoveredRow = BOARD_SIZE / 2;
    state.hoveredCol = BOARD_SIZE / 2;
    state.gameStatus = CHUA_KET_THUC;
    state.aiThinking = false;
    Renderer_ResetCellAnims();
    HarvestResult_Reset();
    Particle_Clear();
}

void App_StartNewSession(_GAMESTATE& state) {
    NewSession(state);
    AIWorker_Cancel();
    state.hoveredRow = BOARD_SIZE / 2;
    state.hoveredCol = BOARD_SIZE / 2;
    state.gameStatus = CHUA_KET_THUC;
    state.aiThinking = false;
    Renderer_ResetCellAnims();
    HarvestResult_Reset();
    Particle_Clear();
    UIManager_HideResult();
}

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

void App_OnEvent_Settings(const SDL_Event& e, _GAMESTATE& state, AppState& appState) {
    AppState next = UIManager_HandleSettingsEvent(e);
    if (next != STATE_SETTINGS)
        App_TransitionTo(next, appState, state);
}
