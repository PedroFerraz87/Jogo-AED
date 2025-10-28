#include "game.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// Creates an easier pattern ensuring gaps of at least two cells.
static void fill_row_with_gaps(CircularQueue *q, char obstacle,
                               int obsMin, int obsMax,
                               int gapMin, int gapMax) {
    if (!q || q->length <= 0) return;
    int i = 0;
    while (i < q->length) {
        int obsLen = utils_random_int(obsMin, obsMax);
        int gapLen = utils_random_int(gapMin, gapMax);

        // place obstacles
        for (int k = 0; k < obsLen && i < q->length; ++k) {
            queue_set_cell(q, i++, obstacle);
        }
        // place gaps (spaces)
        for (int k = 0; k < gapLen && i < q->length; ++k) {
            queue_set_cell(q, i++, ' ');
        }
    }
}

// Declarações de funções estáticas
static RowType generate_random_row_type(int world_y);
static void create_playable_obstacles(CircularQueue *queue, RowType type);
static void generate_playable_row(Row *row, int world_y);
static void ensure_safe_spawn_area(GameState *state);

// Cria obstáculos com gaps garantidos para jogabilidade
static void create_playable_obstacles(CircularQueue *queue, RowType type) {
    if (type == ROW_GRASS) {
        // Grama: apenas espaços vazios
        queue_fill_pattern(queue, ' ', 1, ' ', 1);
    } else if (type == ROW_ROAD) {
        // Estrada: carros com gaps de pelo menos 3 espaços
        fill_row_with_gaps(queue, CHAR_CAR,
                           1, 2,   /* carros curtos (1-2 blocos) */
                           3, 6);  /* gaps grandes (3-6 blocos) */
    } else if (type == ROW_RIVER) {
        // Rio: troncos com gaps de pelo menos 2 espaços
        fill_row_with_gaps(queue, CHAR_LOG,
                           2, 4,   /* troncos (2-4 blocos) */
                           2, 5);  /* gaps de água (2-5 blocos) */
    }
}

static void row_init(Row *row, RowType type, int width) {
    row->type = type;
    row->queue = queue_create(width);
    row->direction = utils_random_int(0, 1) == 0 ? -1 : 1;
    // Rows move MUITO mais devagar para facilitar
    row->speed_ticks = utils_random_int(5, 8);
    row->tick_counter = 0;
    
    // Usa a função para criar obstáculos jogáveis
    create_playable_obstacles(row->queue, type);
}

static void row_destroy(Row *row) {
    if (row->queue) {
        queue_destroy(row->queue);
        row->queue = NULL;
    }
}

// Declarações de funções estáticas
static RowType generate_random_row_type(int world_y);
static void generate_playable_row(Row *row, int world_y);
static void ensure_safe_spawn_area(GameState *state);

// Gera uma nova linha aleatória com gaps garantidos
static RowType generate_random_row_type(int world_y) {
    // Sempre grama nas primeiras linhas (área segura inicial)
    if (world_y < 3) return ROW_GRASS;
    
    // Gera tipo baseado em posição e aleatoriedade
    int roll = utils_random_int(0, 99);
    
    // 50% grama, 25% estrada, 25% rio (mais grama para facilitar)
    if (roll < 50) return ROW_GRASS;
    if (roll < 75) return ROW_ROAD;
    return ROW_RIVER;
}

// Gera uma linha com obstáculos jogáveis (sempre com gaps)
static void generate_playable_row(Row *row, int world_y) {
    RowType type = generate_random_row_type(world_y);
    row->type = type;
    row->queue = queue_create(MAP_WIDTH);
    row->direction = utils_random_int(0, 1) == 0 ? -1 : 1;
    row->speed_ticks = utils_random_int(5, 8);
    row->tick_counter = 0;
    
    create_playable_obstacles(row->queue, type);
}

// Garante que as últimas linhas sejam sempre grama (área segura)
static void ensure_safe_spawn_area(GameState *state) {
    // Últimas 3 linhas sempre grama para garantir área segura
    for (int y = MAP_HEIGHT - 3; y < MAP_HEIGHT; ++y) {
        if (state->rows[y].type != ROW_GRASS) {
            row_destroy(&state->rows[y]);
            row_init(&state->rows[y], ROW_GRASS, MAP_WIDTH);
        }
    }
}

void game_init(GameState *state, int width) {
    if (!state) return;
    
    // Inicializa todas as linhas com obstáculos jogáveis
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        generate_playable_row(&state->rows[y], y);
    }
    
    // SEMPRE garante área segura para spawn (últimas 3 linhas = grama)
    ensure_safe_spawn_area(state);
    
    // Garante que o jogador sempre nasça na grama (última linha sempre grama)
    state->player_x = width / 2;
    state->player_y = MAP_HEIGHT - 1;  // sempre na última linha (grama garantida)
    state->score = 0;
    state->game_over = 0;
    state->world_offset = 0;
}

void game_reset(GameState *state) {
    if (!state) return;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        row_destroy(&state->rows[y]);
    }
    game_init(state, MAP_WIDTH);
    
    // Garante novamente que o jogador nasça na grama após reset
    ensure_safe_spawn_area(state);
    state->player_x = MAP_WIDTH / 2;
    state->player_y = MAP_HEIGHT - 1;  // sempre na última linha (grama)
}

static void scroll_world_down(GameState *state) {
    // Move todas as linhas para baixo (exceto a última)
    for (int y = MAP_HEIGHT - 1; y > 0; --y) {
        // Destroi a linha atual e move a de cima para baixo
        row_destroy(&state->rows[y]);
        state->rows[y] = state->rows[y - 1];
    }
    
    // Gera nova linha no topo com obstáculos jogáveis
    generate_playable_row(&state->rows[0], state->world_offset + MAP_HEIGHT);
    
    state->world_offset++;
    state->score++; // incrementa pontuação quando mundo rola
    
    // Garante que sempre mantenha área segura (últimas 3 linhas = grama)
    ensure_safe_spawn_area(state);
    
    // Jogador não se move automaticamente - ele controla seu próprio movimento
}

static void move_rows(GameState *state) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        Row *row = &state->rows[y];
        if (row->type == ROW_GRASS) continue;
        row->tick_counter++;
        if (row->tick_counter >= row->speed_ticks) {
            row->tick_counter = 0;
            if (row->direction < 0) {
                queue_rotate_left(row->queue);
            } else {
                queue_rotate_right(row->queue);
            }
        }
    }
}


static void apply_river_push(GameState *state) {
    Row *row = &state->rows[state->player_y];
    // SÓ aplica empurrão se estiver realmente no rio E em um tronco
    if (row->type != ROW_RIVER) return;
    
    char cell = queue_get_cell(row->queue, state->player_x);
    if (cell == CHAR_LOG && row->tick_counter == 0) {
        // Empurra o jogador na direção do tronco
        state->player_x += (row->direction < 0 ? -1 : 1);
        // Limita movimento horizontal em vez de game over
        if (state->player_x < 0) state->player_x = 0;
        if (state->player_x >= row->queue->length) state->player_x = row->queue->length - 1;
    }
}

static void check_collision(GameState *state) {
    // Verifica se jogador saiu dos limites (paredes invisíveis)
    if (state->player_x < 0 || state->player_x >= MAP_WIDTH) {
        state->game_over = 1;
        return;
    }
    
    Row *row = &state->rows[state->player_y];
    if (row->type == ROW_ROAD) {
        if (queue_get_cell(row->queue, state->player_x) == CHAR_CAR) {
            state->game_over = 1;
        }
    } else if (row->type == ROW_RIVER) {
        if (queue_get_cell(row->queue, state->player_x) != CHAR_LOG) {
            state->game_over = 1; // fell into water
        }
    }
}

void game_update(GameState *state) {
    if (!state || state->game_over) return;
    
    // Scroll do mundo para baixo (movimento automático MUITO mais lento)
    static int scroll_timer = 0;
    scroll_timer++;
    if (scroll_timer >= 120) { // scroll a cada 120 frames (extremamente lento)
        scroll_timer = 0;
        scroll_world_down(state);
    }
    
    move_rows(state);
    apply_river_push(state);
    check_collision(state);
}

void game_render(const GameState *state) {
    utils_clear_screen();
    // top border
    for (int i = 0; i < MAP_WIDTH + 2; ++i) putchar('#');
    putchar('\n');

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        putchar('#');
        const Row *row = &state->rows[y];
        for (int x = 0; x < MAP_WIDTH; ++x) {
            char ch = ' ';
            if (state->player_x == x && state->player_y == y) {
                ch = CHAR_PLAYER;
            } else if (row->type == ROW_GRASS) {
                ch = ' ';
            } else if (row->type == ROW_ROAD) {
                ch = queue_get_cell(row->queue, x) == CHAR_CAR ? CHAR_CAR : CHAR_ROAD;
            } else if (row->type == ROW_RIVER) {
                ch = queue_get_cell(row->queue, x) == CHAR_LOG ? CHAR_LOG : ' ';
            }
            putchar(ch);
        }
        putchar('#');
        putchar('\n');
    }

    // bottom border
    for (int i = 0; i < MAP_WIDTH + 2; ++i) putchar('#');
    putchar('\n');
    printf("Score: %d    Controls: W/A/S/D to move, Q to quit\n", state->score);
}

void game_handle_input(GameState *state, int key) {
    if (!state || state->game_over) return;
    
    // Movimento horizontal e vertical
    if (key == 'W') {
        // Tenta subir (mas só se não estiver no topo)
        if (state->player_y > 0) {
            state->player_y--;
            state->score++; // pontuação ao subir
        }
    } else if (key == 'S') {
        // Tenta descer (mas só se não estiver embaixo)
        if (state->player_y < MAP_HEIGHT - 1) {
            state->player_y++;
        }
    } else if (key == 'A') {
        state->player_x--;
    } else if (key == 'D') {
        state->player_x++;
    }

    // Limita movimento horizontal
    if (state->player_x < 0) state->player_x = 0;
    if (state->player_x >= MAP_WIDTH) state->player_x = MAP_WIDTH - 1;

    // Check collision imediatamente após movimento
    check_collision(state);
}



