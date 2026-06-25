#pragma once
#include "game/GameDef.h"

//  AudioManager.h — SDL_mixer wrapper
enum SFXEvent {
    SFX_MOVE,
    SFX_WIN,
    SFX_DRAW,
    SFX_MENU_HOVER,
    SFX_MENU_SELECT
};

bool AudioManager_Init();
void AudioManager_Shutdown();
void AudioManager_PlaySFX(SFXEvent sfx);
void AudioManager_PlayBGM(const char* path);
void AudioManager_StopBGM();
void AudioManager_SetVolume(int vol);
void AudioManager_SetMusicVolume(int vol);
void AudioManager_SetSFXVolume(int vol);
void AudioManager_SetMuted(bool muted);
bool AudioManager_IsMuted();