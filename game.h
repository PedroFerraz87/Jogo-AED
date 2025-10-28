#ifndef GAME_H
#define GAME_H

#include "fila.h"

// Map configuration
#define MAP_WIDTH  31
#define MAP_HEIGHT 17
#define PLAYER_ROW (MAP_HEIGHT - 1)  // jogador começa embaixo e sobe

// Symbols
#define CHAR_PLAYER 'O'
#define CHAR_CAR '='
#define CHAR_LOG '~'
#define CHAR_ROAD '-'
#define CHAR_RIVER '\xE1' /* fallback if '≈' isn't supported */
#define CHAR_GRASS '#'

typedef enum RowType {
    ROW_GRASS = 0,
    ROW_ROAD  = 1,
    ROW_RIVER = 2
} RowType;

typedef struct Row {
    RowType type;
    CircularQueue *queue;   // moving obstacles for ROAD/RIVER, or spaces for GRASS
    int direction;          // -1 left, +1 right
    int speed_ticks;        // how many ticks between each movement
    int tick_counter;       // internal counter
} Row;

typedef struct GameState {
    Row rows[MAP_HEIGHT];
    int player_x;           // column
    int player_y;           // row (começa embaixo, sobe conforme progride)
    int score;              // distance progressed
    int game_over;          // boolean
    int world_offset;       // offset do mundo (para scroll infinito)
} GameState;

void game_init(GameState *state, int width);
void game_reset(GameState *state);
void game_update(GameState *state);
void game_render(const GameState *state);
void game_handle_input(GameState *state, int key);

#endif // GAME_H




