#include "raylib.h"

static Music actionMusic; // Música de fundo

// Inicializa o sistema de áudio e carrega a música
void sound_init(void) {
    InitAudioDevice();
    actionMusic = LoadMusicStream("assets/sounds/music.ogg");
    SetMusicVolume(actionMusic, 1.0f);
    PlayMusicStream(actionMusic);
}

// Atualiza o stream de áudio (precisa ser chamado a cada frame)
void sound_update(void) {
    UpdateMusicStream(actionMusic);
}

// Encerra o áudio e libera memória
void sound_close(void) {
    StopMusicStream(actionMusic);
    UnloadMusicStream(actionMusic);
    CloseAudioDevice();
}
