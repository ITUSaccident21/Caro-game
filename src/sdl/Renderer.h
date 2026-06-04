#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include "../game/GameDef.h"

// ================================================================
//  Renderer.h — Vẽ bàn cờ, quân cờ, background, hover effect
//  Tất cả render đều KHÔNG thay đổi _GAMESTATE (const &)
// ================================================================

bool Renderer_Init(SDL_Renderer* renderer);
void Renderer_Shutdown();

// Layer 1: Background toàn màn hình
// isGameScreen=true → bg_blue.png, false → bg_yellow.png (menu/ui)
// Fallback procedural nếu texture không load được
void Renderer_DrawBackground(SDL_Renderer* renderer, bool isGameScreen = false);

// Board skin random — gọi khi bắt đầu ván mới
void Renderer_RandomizeBoard();

// Layer 2: Bề mặt bàn cờ (texture JPG hoặc checkerboard fallback) + lưới
void Renderer_DrawBoard(SDL_Renderer* renderer, const _GAMESTATE& state);

// Layer 3: Quân cờ đã đặt (stone/seed sprite theo màu người chơi)
void Renderer_DrawPieces(SDL_Renderer* renderer, const _GAMESTATE& state);

// Layer 4: Highlight ô đang hover (ô vuông mờ màu vàng)
void Renderer_DrawHover(SDL_Renderer* renderer, const _GAMESTATE& state);

// Layer 5: Hiệu ứng nhấp nháy 5 ô chiến thắng
void Renderer_DrawWinCells(SDL_Renderer* renderer, const _GAMESTATE& state);

// Primitive ký hiệu — dùng bởi WinEffect để vẽ lại có scale tùy ý
// Màu do caller set trước bằng SDL_SetRenderDrawColor
void Renderer_DrawSymbolX(SDL_Renderer* r, int cx, int cy, int half, int thickness);
void Renderer_DrawSymbolO(SDL_Renderer* r, int cx, int cy, int radius, int thickness);

// Lifecycle texture cache
bool Renderer_CreatePieceTextures(SDL_Renderer* renderer);
void Renderer_DestroyPieceTextures();

