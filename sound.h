#ifndef SOUND_H
#define SOUND_H
#include <stdbool.h>

void sound_init(void);
void sound_update(void);
void sound_close(void);
void sound_toggle(void);  // Alterna entre ligar/desligar música
bool sound_is_enabled(void);  // Retorna true se a música está ligada

#endif
