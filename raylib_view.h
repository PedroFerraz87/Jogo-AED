#ifndef RAYLIB_VIEW_H
#define RAYLIB_VIEW_H

#include "ranking.h"

// Executa o jogo usando Raylib. Se Raylib não estiver disponível
// (compilação sem ENABLE_RAYLIB), a função apenas informa ao usuário.
void raylib_run_game(Ranking *ranking);

#endif // RAYLIB_VIEW_H




