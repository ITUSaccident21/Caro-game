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
void     UIManager_ShowMenu();
AppState UIManager_UpdateMenu(float dt);   // trả về state khi bounce xong → chuyển màn hình
void     UIManager_RenderMenu(SDL_Renderer* renderer);
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

// ── Splash screen (màn hình chờ khi khởi động) ──────────────────
void     UIManager_ShowSplash();
AppState UIManager_UpdateSplash(float dt);   // tự động chuyển sang STATE_MENU sau ~2.5s
void     UIManager_RenderSplash(SDL_Renderer* r);

// ── Header bar trong game (vẽ trên cùng của STATE_PLAYING) ──────
void UIManager_RenderGameHeader(SDL_Renderer* r, const _GAMESTATE& state);

// ── Pause overlay (ESC trong game) ──────────────────────────────
// PauseAction: kết quả người chơi chọn trong menu dừng
enum PauseAction { PAUSE_NONE = 0, PAUSE_RESUME, PAUSE_RESTART, PAUSE_SAVE, PAUSE_QUIT };
void        UIManager_ShowPause();
void        UIManager_HidePause();
bool        UIManager_IsPauseShown();
PauseAction UIManager_HandlePauseEvent(const SDL_Event& e, _GAMESTATE& state);
void        UIManager_RenderPauseOverlay(SDL_Renderer* r);

// ── Settings screen ─────────────────────────────────────────────
void     UIManager_ShowSettings();
AppState UIManager_HandleSettingsEvent(const SDL_Event& e);
void     UIManager_RenderSettings(SDL_Renderer* r);

// ── Text helper dùng chung (vd. WinEffect) ──────────────────────
// fontSize: 0=16pt, 1=22pt, 2=34pt
void UIManager_RenderText(SDL_Renderer* r, const char* text,
                          int x, int y, SDL_Color color,
                          bool center, int fontSize);