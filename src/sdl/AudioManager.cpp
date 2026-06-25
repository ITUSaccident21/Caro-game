#include "sdl/AudioManager.h"
#include <SDL.h>

//  AudioManager.cpp — SDL_mixer wrapper with graceful no-mixer fallback

#if __has_include(<SDL_mixer.h>)
#  include <SDL_mixer.h>
#  define HAS_MIXER 1
#else
#  define HAS_MIXER 0
#endif

#if HAS_MIXER

static Mix_Music* s_bgm = nullptr;
static Mix_Chunk* s_sfx[5] = {};
static bool       s_audioReady = false;
static bool       s_muted      = false;
static int        s_savedMusicVol = 128;
static int        s_savedSFXVol   = 128;

static const char* SFX_PATHS[] = {
    "assets/sounds/move.wav",
    "assets/sounds/win.wav",
    "assets/sounds/draw.wav",
    "assets/sounds/hover.wav",
    "assets/sounds/select.wav",
};

bool AudioManager_Init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio failed (running silently): %s", Mix_GetError());
        return true;
    }
    s_audioReady = true;
    for (int i = 0; i < 5; i++)
        s_sfx[i] = Mix_LoadWAV(SFX_PATHS[i]);
    return true;
}

void AudioManager_Shutdown() {
    if (!s_audioReady) return;
    for (int i = 0; i < 5; i++)
        if (s_sfx[i]) { Mix_FreeChunk(s_sfx[i]); s_sfx[i] = nullptr; }
    if (s_bgm) { Mix_FreeMusic(s_bgm); s_bgm = nullptr; }
    Mix_CloseAudio();
    s_audioReady = false;
}

void AudioManager_PlaySFX(SFXEvent sfx) {
    if (!s_audioReady) return;
    int idx = static_cast<int>(sfx);
    if (idx >= 0 && idx < 5 && s_sfx[idx])
        Mix_PlayChannel(-1, s_sfx[idx], 0);
}

void AudioManager_PlayBGM(const char* path) {
    if (!s_audioReady) return;
    if (s_bgm) { Mix_FreeMusic(s_bgm); s_bgm = nullptr; }
    s_bgm = Mix_LoadMUS(path);
    if (s_bgm) Mix_PlayMusic(s_bgm, -1);
}

void AudioManager_StopBGM() { if (s_audioReady) Mix_HaltMusic(); }

void AudioManager_SetVolume(int vol) {
    if (!s_audioReady) return;
    if (s_muted) { s_savedMusicVol = vol; s_savedSFXVol = vol; return; }
    Mix_Volume(-1, vol); Mix_VolumeMusic(vol);
}
void AudioManager_SetMusicVolume(int vol) {
    if (!s_audioReady) return;
    if (s_muted) { s_savedMusicVol = vol; return; }
    Mix_VolumeMusic(vol);
}
void AudioManager_SetSFXVolume(int vol) {
    if (!s_audioReady) return;
    if (s_muted) { s_savedSFXVol = vol; return; }
    Mix_Volume(-1, vol);
}
void AudioManager_SetMuted(bool muted) {
    s_muted = muted;
    if (!s_audioReady) return;
    if (muted) {
        s_savedMusicVol = Mix_VolumeMusic(-1);
        s_savedSFXVol   = Mix_Volume(-1, -1);
        Mix_VolumeMusic(0);
        Mix_Volume(-1, 0);
    } else {
        Mix_VolumeMusic(s_savedMusicVol);
        Mix_Volume(-1, s_savedSFXVol);
    }
}
bool AudioManager_IsMuted() { return s_muted; }

#else

bool AudioManager_Init()               { return true; }
void AudioManager_Shutdown()           {}
void AudioManager_PlaySFX(SFXEvent)    {}
void AudioManager_PlayBGM(const char*) {}
void AudioManager_StopBGM()            {}
void AudioManager_SetVolume(int)       {}
void AudioManager_SetMusicVolume(int)  {}
void AudioManager_SetSFXVolume(int)    {}
void AudioManager_SetMuted(bool)       {}
bool AudioManager_IsMuted()            { return false; }

#endif
