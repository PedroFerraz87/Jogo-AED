#ifndef GAME_H
#define GAME_H

#include "fila.h"

// Map configuration
#define MAP_WIDTH  31
#define MAP_HEIGHT 20  // mais linhas para melhor visualização
// PLAYER_ROW não é usado no modelo atual de scroll livre, mas pode ficar
#define PLAYER_ROW (MAP_HEIGHT - 1)

// Symbols
#define CHAR_PLAYER 'O'
#define CHAR_CAR    '='
#define CHAR_LOG    '~'
#define CHAR_ROAD   '-'
#define CHAR_RIVER  '\xE1' /* fallback se '≈' não for suportado */
#define CHAR_GRASS  '#'

typedef enum RowType {
    ROW_GRASS = 0,
    ROW_ROAD  = 1,
    ROW_RIVER = 2
} RowType;

typedef struct Row {
    RowType type;
    CircularQueue *queue;   // obstáculos/móvel (ROAD/RIVER) ou espaços (GRASS)
    int direction;          // -1 left, +1 right, 0 parado
    int speed_ticks;        // a cada N ticks a linha rotaciona
    int tick_counter;       // contador interno (0..speed_ticks-1)
    int moved_this_tick;    // NEW: 1 se rotacionou neste frame; 0 caso contrário
} Row;

typedef struct GameState {
    Row rows[MAP_HEIGHT];
    int player_x;
    int player_y;
    int score;
    int game_over;

    int world_position;   // já existia
    int just_scrolled;    // já existia

    int world_head;       // NOVO: quantas linhas já nasceram no topo (quantos scrolls)
    int min_abs_reached;  // NOVO: menor índice absoluto já alcançado (melhor progresso)
} GameState;

void game_init(GameState *state, int width);
void game_reset(GameState *state);
void game_update(GameState *state);
void game_render(const GameState *state);
void game_handle_input(GameState *state, int key);

#endif // GAME_H
