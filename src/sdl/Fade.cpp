#include "sdl/Fade.h"
#include "sdl/Renderer.h"
#include "sdl/AudioManager.h"
#include "sdl/UIManager.h"
#include "sdl/HarvestResult.h"
#include "sdl/Particle.h"
#include "game/Model.h"
#include "ai/AIWorker.h"
#include <SDL.h>

static float    s_fadeAlpha  = 0.0f;
static float    s_fadeDir    = 0.0f;
static AppState s_fadeTarget = STATE_MENU;
static _GAMESTATE* s_fadeStatePtr = nullptr;
static const float FADE_SPEED = 600.0f;

void Fade_Start(AppState target, _GAMESTATE* statePtr) {
    s_fadeTarget   = target;
    s_fadeStatePtr = statePtr;
    s_fadeDir      = 1.0f;
    if (s_fadeAlpha <= 0.0f) s_fadeAlpha = 0.0f;
}

void Fade_Update(float dt, AppState& appState, _GAMESTATE& state) {
    if (s_fadeDir == 0.0f) return;

    s_fadeAlpha += s_fadeDir * FADE_SPEED * dt;

    if (s_fadeDir > 0.0f && s_fadeAlpha >= 255.0f) {

        s_fadeAlpha = 255.0f;
        s_fadeDir   = -1.0f;

        appState = s_fadeTarget;
        switch (s_fadeTarget) {
        case STATE_MENU:
            AIWorker_Cancel();
            UIManager_ShowMenu();
            AudioManager_PlayBGM("assets/sounds/menu_bgm.ogg");
            break;
        case STATE_NAME_INPUT:
            if (s_fadeStatePtr) UIManager_ShowNameInput(*s_fadeStatePtr);
            break;
        case STATE_PLAYING:
            if (s_fadeStatePtr) {
                ResetData(*s_fadeStatePtr);
                s_fadeStatePtr->hoveredRow = BOARD_SIZE / 2;
                s_fadeStatePtr->hoveredCol = BOARD_SIZE / 2;
                s_fadeStatePtr->gameStatus = CHUA_KET_THUC;
                AIWorker_Cancel();
                s_fadeStatePtr->aiThinking = false;
                Renderer_ResetCellAnims();
                HarvestResult_Reset();
                Particle_Clear();
                AudioManager_PlayBGM("assets/sounds/game_bgm.ogg");
            }
            break;
        case STATE_LOAD_GAME:
            UIManager_ShowLoadScreen();
            break;
        case STATE_SETTINGS:
            UIManager_ShowSettings();
            break;
        default: break;
        }
    }

    if (s_fadeDir < 0.0f && s_fadeAlpha <= 0.0f) {
        s_fadeAlpha = 0.0f;
        s_fadeDir   = 0.0f;
    }
}

void Fade_Render(SDL_Renderer* r) {
    if (s_fadeAlpha <= 0.0f) return;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)s_fadeAlpha);
    SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
