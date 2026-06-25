#pragma once
#include <SDL.h>
#include "game/GameDef.h"

//  Particle.h — lightweight data-driven particle subsystem.

//  Tách khỏi gameplay & renderer: chỉ cần Init → Update(dt) mỗi frame →

void Particle_Init(SDL_Renderer* renderer);
void Particle_Shutdown();
void Particle_Update(float dt);
void Particle_Render(SDL_Renderer* renderer);
void Particle_Clear();

// Reduce Motion (Settings → Display, V3): when OFF, all Burst*/ShimmerAt

void Particle_SetEnabled(bool on);
bool Particle_IsEnabled();

void Particle_BurstDirt(float x, float y);
void Particle_BurstHarvest(float x, float y, int faction);
void Particle_ShimmerAt(float x, float y, int faction);

