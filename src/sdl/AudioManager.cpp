#include "Audiomanager.h"

// ================================================================
//  AudioManager.cpp — SDL_mixer wrapper with graceful no-mixer fallback
//
//  If SDL2_mixer is not installed, all functions become no-ops.
//  The #if guard makes this compile cleanly either way.
// ================================================================

#if __has_include(<SDL_mixer.h>)
#  include <SDL_mixer.h>
#  define HAS_MIXER 1
#else
#  define HAS_MIXER 0
#endif

#if HAS_MIXER

static Mix_Music* s_bgm = nullptr;
static Mix_Chunk* s_sfx[5] = {};

static const char* SFX_PATHS[] = {
    "assets/sounds/move.wav",
    "assets/sounds/win. ",
    "assets/sounds/draw.wav",
    "assets/sounds/hover.wav",
    "assets/sounds/select.wav",
};

bool AudioManager_Init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
        return false;
    }
    for (int i = 0; i < 5; i++) {
        s_sfx[i] = Mix_LoadWAV(SFX_PATHS[i]);
        // Missing SFX files are non-fatal — just leave slot null
    }
    return true;
}

void AudioManager_Shutdown() {
    for (int i = 0; i < 5; i++) {
        if (s_sfx[i]) { Mix_FreeChunk(s_sfx[i]); s_sfx[i] = nullptr; }
    }
    if (s_bgm) { Mix_FreeMusic(s_bgm); s_bgm = nullptr; }
    Mix_CloseAudio();
}

void AudioManager_PlaySFX(SFXEvent sfx) {
    int idx = static_cast<int>(sfx);
    if (idx >= 0 && idx < 5 && s_sfx[idx])
        Mix_PlayChannel(-1, s_sfx[idx], 0);
}

void AudioManager_PlayBGM(const char* path) {
    if (s_bgm) { Mix_FreeMusic(s_bgm); s_bgm = nullptr; }
    s_bgm = Mix_LoadMUS(path);
    if (s_bgm) Mix_PlayMusic(s_bgm, -1);
}

void AudioManager_StopBGM() {
    Mix_HaltMusic();
}

void AudioManager_SetVolume(int vol) {
    Mix_Volume(-1, vol);
    Mix_VolumeMusic(vol);
}

#else // no SDL_mixer

bool AudioManager_Init()               { return true; }
void AudioManager_Shutdown()           {}
void AudioManager_PlaySFX(SFXEvent)    {}
void AudioManager_PlayBGM(const char*) {}
void AudioManager_StopBGM()            {}
void AudioManager_SetVolume(int)       {}

#endif
