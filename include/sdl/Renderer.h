#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include "game/GameDef.h"

bool Renderer_Init(SDL_Renderer* renderer);
void Renderer_Shutdown();

// Draw order: background, board, pieces, markers, then foreground foliage.
void Renderer_DrawMatchBackground(SDL_Renderer* renderer);
void Renderer_DrawBoard(SDL_Renderer* renderer);
void Renderer_DrawPieces(SDL_Renderer* renderer, const _GAMESTATE& state);
void Renderer_DrawMarkers(SDL_Renderer* renderer, const _GAMESTATE& state);
void Renderer_DrawForeground(SDL_Renderer* renderer);

int         Renderer_GetFactionPieceSize(int factionIdx);

void        Renderer_SetPieceReadability(int preset);
void        Renderer_CyclePieceReadability();
const char* Renderer_GetPieceReadabilityName();
bool        Renderer_IsPieceDebugShown();

bool Renderer_CreatePieceTextures(SDL_Renderer* renderer);
void Renderer_DestroyPieceTextures();

SDL_Texture* Renderer_GetPieceTexture(int playerIdx);
SDL_Texture* Renderer_GetGrowthFinalTexture(int factionIdx);

void Renderer_TriggerPiecePop(int row, int col);
void Renderer_UpdatePop(float dt);
void Renderer_ResetCellAnims();
