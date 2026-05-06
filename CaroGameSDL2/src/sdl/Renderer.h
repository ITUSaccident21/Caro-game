#pragma once
#include <SDL.h>
#include "../game/GameDef.h"

// ================================================================
//  Renderer.h — Vẽ bàn cờ, quân cờ, background, hover effect
//  Tất cả render đều KHÔNG thay đổi _GAMESTATE (const &)
// ================================================================

bool Renderer_Init(SDL_Renderer* renderer);
void Renderer_Shutdown();

// Layer 1: Cỏ, đất, cây, hàng rào xung quanh bàn cờ
void Renderer_DrawBackground(SDL_Renderer* renderer);

// Layer 2: Bề mặt bàn cờ (tile gỗ/đá) + đường kẻ lưới
void Renderer_DrawBoard(SDL_Renderer* renderer, const _GAMESTATE& state);

// Layer 3: Quân cờ đã đặt (stone/seed sprite theo màu người chơi)
void Renderer_DrawPieces(SDL_Renderer* renderer, const _GAMESTATE& state);

// Layer 4: Highlight ô đang hover (ô vuông mờ màu vàng)
void Renderer_DrawHover(SDL_Renderer* renderer, const _GAMESTATE& state);

// Layer 5: Hiệu ứng nhấp nháy 5 ô chiến thắng
void Renderer_DrawWinCells(SDL_Renderer* renderer, const _GAMESTATE& state);