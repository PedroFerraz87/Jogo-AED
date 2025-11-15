#include "raylib.h"
#include "sound.h"
#include <stdio.h>
#include <stdbool.h>

static Music actionMusic;
static bool musicLoaded = false;
static bool musicEnabled = true;  // Estado da música (ligada/desligada)

void sound_init(void) {
    InitAudioDevice();

    // Aguarda um pequeno tempo para inicializar corretamente o driver de áudio
    SetExitKey(0);
    WaitTime(0.2);

    // Caminho certo
    actionMusic = LoadMusicStream("assets/sounds/music.ogg");

    if (actionMusic.stream.buffer == NULL) {
        musicLoaded = false;
    } else {
        PlayMusicStream(actionMusic);
        SetMusicVolume(actionMusic, 0.8f); 
        musicLoaded = true;
    }
}

void sound_update(void) {
    if (musicLoaded && musicEnabled) {
        UpdateMusicStream(actionMusic);
    }
}

void sound_toggle(void) {
    if (!musicLoaded) return;
    
    musicEnabled = !musicEnabled;
    
    if (musicEnabled) {
        ResumeMusicStream(actionMusic);
    } else {
        PauseMusicStream(actionMusic);
    }
}

bool sound_is_enabled(void) {
    return musicEnabled;
}

void sound_close(void) {
    if (musicLoaded) {
        StopMusicStream(actionMusic);
        UnloadMusicStream(actionMusic);
    }
    CloseAudioDevice();
}
