#include "raylib.h"
#include "sound.h"
#include <stdio.h>
#include <stdbool.h>

static Music actionMusic;
static bool musicLoaded = false;

void sound_init(void) {
    InitAudioDevice();

    // Aguarda um pequeno tempo para inicializar corretamente o driver de áudio
    SetExitKey(0);
    WaitTime(0.2);

    // Caminho certo
    actionMusic = LoadMusicStream("assets/sounds/music.ogg");

    if (actionMusic.stream.buffer == NULL) {
        printf("⚠️  ERRO: não foi possível carregar assets/sounds/music.ogg\n");
        musicLoaded = false;
    } else {
        PlayMusicStream(actionMusic);
        SetMusicVolume(actionMusic, 0.8f); // volume confortável
        musicLoaded = true;
        printf("🎵 Música carregada com sucesso!\n");
    }
}

void sound_update(void) {
    if (musicLoaded) UpdateMusicStream(actionMusic);
}

void sound_close(void) {
    if (musicLoaded) {
        StopMusicStream(actionMusic);
        UnloadMusicStream(actionMusic);
    }
    CloseAudioDevice();
}
