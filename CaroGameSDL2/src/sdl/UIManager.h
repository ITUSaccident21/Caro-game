#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "../game/GameDef.h"

// ================================================================
//  UIManager.h — Menu, HUD, text input, load screen
//  Dùng SDL_ttf để render font pixel
// ================================================================

bool UIManager_Init(SDL_Renderer* renderer);
void UIManager_Shutdown();

// ── Menu chính ──────────────────────────────────────────────────
void UIManager_ShowMenu();
void UIManager_UpdateMenu(float dt);       // rainbow animation tick
void UIManager_RenderMenu(SDL_Renderer* renderer);
// Trả về AppState mới nếu người dùng chọn, STATE_MENU nếu chưa
AppState UIManager_HandleMenuEvent(const SDL_Event& e);

// ── Nhập tên người chơi + chọn mode/difficulty ──────────────────
void UIManager_ShowNameInput(_GAMESTATE& state);
void UIManager_RenderNameInput(SDL_Renderer* renderer);
AppState UIManager_HandleNameInputEvent(const SDL_Event& e, _GAMESTATE& state);

// ── HUD trong game ───────────────────────────────────────────────
void UIManager_RenderHUD(SDL_Renderer* renderer, const _GAMESTATE& state);

// ── Load game screen ────────────────────────────────────────────
void UIManager_ShowLoadScreen();
void UIManager_RenderLoadScreen(SDL_Renderer* renderer);
AppState UIManager_HandleLoadEvent(const SDL_Event& e, _GAMESTATE& state);

// ── Save dialog (khi nhấn L trong game) ─────────────────────────
void UIManager_ShowSaveDialog(_GAMESTATE& state);

// ── Kết quả ván (win/draw dialog) ───────────────────────────────
void UIManager_ShowResult(const _GAMESTATE& state, int result);
void UIManager_RenderResult(SDL_Renderer* renderer);
void UIManager_HideResult();   // gọi khi bắt đầu ván mới trong cùng session

// ── Text helper dùng chung (vd. WinEffect) ──────────────────────
// fontSize: 0=16pt, 1=22pt, 2=34pt
void UIManager_RenderText(SDL_Renderer* r, const char* text,
                          int x, int y, SDL_Color color,
                          bool center, int fontSize);