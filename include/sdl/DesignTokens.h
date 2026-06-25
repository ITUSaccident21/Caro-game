#pragma once
#include <SDL.h>

static constexpr int DT_XS  =  4;
static constexpr int DT_S   =  8;
static constexpr int DT_M   = 16;
static constexpr int DT_L   = 24;
static constexpr int DT_XL  = 40;
static constexpr int DT_XXL = 64;

// (App_Init) so scaled glyph edges stay smooth instead of jagged.
static constexpr int DT_T1 = 58;
static constexpr int DT_T2 = 36;
static constexpr int DT_T3 = 30;
static constexpr int DT_T4 = 22;

static constexpr SDL_Color DT_DEEP   = { 28,  18, 10, 255};
static constexpr SDL_Color DT_GROUND = { 46,  28, 14, 255};
static constexpr SDL_Color DT_WOOD   = { 74,  44, 20, 255};
static constexpr SDL_Color DT_LINE   = {122,  78, 40, 255};
static constexpr SDL_Color DT_WARM   = {212, 166, 84, 255};
static constexpr SDL_Color DT_LIGHT  = {255, 248,232, 255};

static constexpr SDL_Color DT_BTN_NORMAL  = {158, 116, 56, 255};
static constexpr SDL_Color DT_BTN_HOVER   = {186, 146, 76, 255};
static constexpr SDL_Color DT_BTN_PRESSED = {110,  76, 28, 255};
static constexpr SDL_Color DT_TEXT_BTN    = {255, 244, 218, 255};

static constexpr SDL_Color DT_STONE_B = { 26,  26, 46, 255};
static constexpr SDL_Color DT_STONE_W = {240, 234,214, 255};

static constexpr SDL_Color DT_WIN    = {126, 184, 126, 255};
static constexpr SDL_Color DT_THREAT = {200,  90,  58, 255};
static constexpr SDL_Color DT_OFF    = {138, 122, 106, 255};

