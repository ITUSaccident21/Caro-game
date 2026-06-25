#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "game/GameDef.h"

bool UIManager_Init(SDL_Renderer* renderer);
void UIManager_Shutdown();
void UIManager_BeginFrame();

void     UIManager_ShowMenu();
AppState UIManager_UpdateMenu(float dt);
void     UIManager_RenderMenu(SDL_Renderer* renderer);
AppState UIManager_HandleMenuEvent(const SDL_Event& e);

void UIManager_ShowNameInput(_GAMESTATE& state);
void UIManager_RenderNameInput(SDL_Renderer* renderer);
AppState UIManager_HandleNameInputEvent(const SDL_Event& e, _GAMESTATE& state);

void UIManager_RenderHUD(SDL_Renderer* renderer, const _GAMESTATE& state);

void UIManager_ShowLoadScreen();
void UIManager_RenderLoadScreen(SDL_Renderer* renderer);
AppState UIManager_HandleLoadEvent(const SDL_Event& e, _GAMESTATE& state);

void UIManager_ShowSaveNameDialog();
void UIManager_HideSaveNameDialog();
bool UIManager_IsSaveNameDialogOpen();
void UIManager_RenderSaveNameDialog(SDL_Renderer* r);
int  UIManager_HandleSaveNameDialogEvent(const SDL_Event& e, _GAMESTATE& state);

void UIManager_ShowResult(const _GAMESTATE& state, int result);
void UIManager_RenderResult(SDL_Renderer* renderer);
void UIManager_HideResult();

void     UIManager_ShowSplash();
AppState UIManager_UpdateSplash(float dt);
void     UIManager_RenderSplash(SDL_Renderer* r);
void     UIManager_QA_FreezeSplash();
void     UIManager_QA_OpenCredits();
void     UIManager_QA_OpenHowTo();
void     UIManager_QA_FakeSaves(int n);

enum PauseAction { PAUSE_NONE = 0, PAUSE_RESUME, PAUSE_RESTART, PAUSE_SAVE, PAUSE_QUIT };
void        UIManager_ShowPause();
void        UIManager_HidePause();
bool        UIManager_IsPauseShown();
PauseAction UIManager_HandlePauseEvent(const SDL_Event& e, _GAMESTATE& state);
void        UIManager_RenderPauseOverlay(SDL_Renderer* r);

void     UIManager_ShowSettings();
AppState UIManager_HandleSettingsEvent(const SDL_Event& e);
void     UIManager_RenderSettings(SDL_Renderer* r);

void UIManager_RenderText(SDL_Renderer* r, const char* text,
                          int x, int y, SDL_Color color,
                          bool center, int fontSize);