#pragma once
#include "../game/GameDef.h"

// ================================================================
//  AudioManager.h — SDL_mixer wrapper
// ================================================================
enum SFXEvent {
    SFX_MOVE,       // đặt quân: tiếng gỗ clack
    SFX_WIN,        // thắng: jingle vui
    SFX_DRAW,       // hòa: jingle trung tính
    SFX_MENU_HOVER, // hover menu item
    SFX_MENU_SELECT // chọn menu item
};

bool AudioManager_Init();
void AudioManager_Shutdown();
void AudioManager_PlaySFX(SFXEvent sfx);
void AudioManager_PlayBGM(const char* path);  // loop vô tận
void AudioManager_StopBGM();
void AudioManager_SetVolume(int vol);          // 0-128