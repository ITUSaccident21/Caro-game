#pragma once
#include <SDL.h>
#include "../game/GameDef.h"

// ================================================================
//  WinEffect.h — Hiệu ứng ăn mừng chiến thắng 4 giai đoạn
//
//  GLOW (1.2s)  → CONVERGE (0.9s) → ZOOM (0.7s) → ANNOUNCE (∞)
//
//  Gọi WinEffect_Start() ngay khi phát hiện thắng.
//  WinEffect_IsAnimating() = true trong 3 giai đoạn đầu → khóa input.
//  WinEffect_IsPlaying()   = true cả 4 giai đoạn.
//  WinEffect_Reset()       khi bắt đầu ván mới.
// ================================================================

void WinEffect_Start(const _GAMESTATE& state);
void WinEffect_Update(float dt);
void WinEffect_Render(SDL_Renderer* r);
void WinEffect_Reset();

bool WinEffect_IsPlaying();    // true: đang ở bất kỳ giai đoạn nào
bool WinEffect_IsAnimating();  // true: GLOW / CONVERGE / ZOOM — chặn R/ESC
