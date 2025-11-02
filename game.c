// game.c
#include "game.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------
   CONFIG GERAL
   - Mapa desce sozinho a cada SCROLL_TICKS frames.
   - Player se move livre (WASD), não é ancorado.
 ------------------------------------------------------- */
#define SCROLL_TICKS 120   // ajuste a velocidade do scroll vertical

/* -------------------------------------------------------
   FORWARD DECLS
 ------------------------------------------------------- */
static void fill_row_with_gaps(CircularQueue *q, char obstacle,
                               int obsMin, int obsMax,
                               int gapMin, int gapMax);
static RowType generate_row_type(int world_position);
static void create_obstacles(CircularQueue *queue, RowType type);
static void generate_row(Row *row, int world_position);
static void row_destroy(Row *row);
static void ensure_safe_area(GameState *state);
static void scroll_world_down(GameState *state);
static void move_rows(GameState *state);
static void apply_river_push(GameState *state);
static void check_collision(GameState *state);

/* -------------------------------------------------------
   PREENCHIMENTO COM OBSTÁCULOS + GAPS
 ------------------------------------------------------- */
static void fill_row_with_gaps(CircularQueue *q, char obstacle,
                               int obsMin, int obsMax,
                               int gapMin, int gapMax)
{
    if (!q || q->length <= 0) return;
    int i = 0;
    while (i < q->length) {
        int obsLen = utils_random_int(obsMin, obsMax);
        int gapLen = utils_random_int(gapMin, gapMax);

        for (int k = 0; k < obsLen && i < q->length; ++k) {
            queue_set_cell(q, i++, obstacle);
        }
        for (int k = 0; k < gapLen && i < q->length; ++k) {
            queue_set_cell(q, i++, ' ');
        }
    }
}

/* -------------------------------------------------------
   TIPO DE LINHA
 ------------------------------------------------------- */
static RowType generate_row_type(int world_position)
{
    if (world_position < 5) return ROW_GRASS; // respiro inicial
    int roll = utils_random_int(0, 99);
    if (roll < 40) return ROW_GRASS; // 40%
    if (roll < 70) return ROW_ROAD;  // 30%
    return ROW_RIVER;                 // 30%
}

/* -------------------------------------------------------
   GERA OBSTÁCULOS DA LINHA
 ------------------------------------------------------- */
static void create_obstacles(CircularQueue *queue, RowType type)
{
    if (!queue) return;

    if (type == ROW_GRASS) {
        queue_fill_pattern(queue, ' ', 1, ' ', 1);
        return;
    }

    if (type == ROW_ROAD) {
        int pattern = utils_random_int(0, 2);
        if (pattern == 0)      fill_row_with_gaps(queue, CHAR_CAR, 1, 2, 4, 7);
        else if (pattern == 1) fill_row_with_gaps(queue, CHAR_CAR, 2, 3, 3, 5);
        else                   fill_row_with_gaps(queue, CHAR_CAR, 3, 4, 2, 4);
        return;
    }

    if (type == ROW_RIVER) {
        int pattern = utils_random_int(0, 2);
        if (pattern == 0)      fill_row_with_gaps(queue, CHAR_LOG, 2, 3, 3, 5);
        else if (pattern == 1) fill_row_with_gaps(queue, CHAR_LOG, 3, 4, 2, 4);
        else                   fill_row_with_gaps(queue, CHAR_LOG, 4, 5, 1, 3);
        return;
    }
}

/* -------------------------------------------------------
   GERA LINHA COMPLETA
 ------------------------------------------------------- */
static void generate_row(Row *row, int world_position)
{
    if (!row) return;

    RowType type = generate_row_type(world_position);
    row->type = type;
    row->queue = queue_create(MAP_WIDTH);
    if (!row->queue) {
        row->type = ROW_GRASS;
        row->queue = queue_create(MAP_WIDTH);
        if (!row->queue) return;
    }

    row->direction = (utils_random_int(0, 1) == 0) ? -1 : 1;

    int baseMin = 15, baseMax = 25;
    int accel = world_position / 20;     // acelera suave com o progresso
    if (baseMin - accel < 8)  baseMin = 8;
    if (baseMax - accel < 12) baseMax = 12;

    row->speed_ticks = utils_random_int(baseMin, baseMax);
    row->tick_counter = 0;
    row->moved_this_tick = 0;            // <<-- IMPORTANTE: inicia zerado

    create_obstacles(row->queue, type);
}

/* -------------------------------------------------------
   DESTROY
 ------------------------------------------------------- */
static void row_destroy(Row *row)
{
    if (row && row->queue) {
        queue_destroy(row->queue);
        row->queue = NULL;
    }
}

/* -------------------------------------------------------
   ÁREA SEGURA (DINÂMICA)
   - início: 3 linhas seguras
   - depois: 1 linha (só o rodapé)
 ------------------------------------------------------- */
static void ensure_safe_area(GameState *state)
{
    if (!state) return;

    int safe_lines = (state->world_position < 20) ? 3 : 1;

    for (int y = MAP_HEIGHT - safe_lines; y < MAP_HEIGHT; ++y) {
        if (state->rows[y].type != ROW_GRASS) {
            row_destroy(&state->rows[y]);
            state->rows[y].type = ROW_GRASS;
            state->rows[y].queue = queue_create(MAP_WIDTH);
            state->rows[y].direction = 0;
            state->rows[y].speed_ticks = 0;
            state->rows[y].tick_counter = 0;
            state->rows[y].moved_this_tick = 0;
            queue_fill_pattern(state->rows[y].queue, ' ', 1, ' ', 1);
        }
    }

    // Evita rio em cima de rio no começo (depois libera)
    if (state->world_position < 30) {
        for (int y = 0; y < MAP_HEIGHT - 1; ++y) {
            if (state->rows[y].type == ROW_RIVER && state->rows[y + 1].type == ROW_RIVER) {
                row_destroy(&state->rows[y + 1]);
                state->rows[y + 1].type = ROW_GRASS;
                state->rows[y + 1].queue = queue_create(MAP_WIDTH);
                state->rows[y + 1].direction = 0;
                state->rows[y + 1].speed_ticks = 0;
                state->rows[y + 1].tick_counter = 0;
                state->rows[y + 1].moved_this_tick = 0;
                queue_fill_pattern(state->rows[y + 1].queue, ' ', 1, ' ', 1);
            }
        }
    }
}

/* -------------------------------------------------------
   SCROLL DO MUNDO PRA BAIXO (GERA NOVA LINHA NO TOPO)
   - NÃO mexe no player_y (player livre)
   - Evita ponteiro duplicado / vazamento
   - ZERA moved_this_tick (scroll não conta como “mover a linha”)
 ------------------------------------------------------- */
 static void scroll_world_down(GameState *state)
 {
     if (!state) return;
 
     for (int y = MAP_HEIGHT - 1; y > 0; --y) {
         row_destroy(&state->rows[y]);
         state->rows[y] = state->rows[y - 1];
         state->rows[y - 1].queue = NULL;
     }
 
     row_destroy(&state->rows[0]);
     generate_row(&state->rows[0], state->world_position);
 
     // scroll não conta como "mover a linha" para empurrão do rio
     for (int y = 0; y < MAP_HEIGHT; ++y) {
         state->rows[y].moved_this_tick = 0;
     }
 
     ensure_safe_area(state);
 
     state->world_position++;
     state->score++;
 
     // 🔸 Empurra o player PARA BAIXO quando o mapa sobe
     state->player_y++;
 
     // 🔸 Se o mapa "passou" do player (saiu da tela), morre (igual Crossy Road)
     if (state->player_y >= MAP_HEIGHT) {
         state->game_over = 1;
         return;
     }
 
     state->just_scrolled = 1;
 }
 

/* -------------------------------------------------------
   INIT / RESET
 ------------------------------------------------------- */
void game_init(GameState *state, int width)
{
    if (!state) return;

    state->world_position = 0;

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        generate_row(&state->rows[y], y);
    }

    ensure_safe_area(state);

    state->player_x = width / 2;
    state->player_y = MAP_HEIGHT - 2; // começa 1 acima do rodapé
    state->score = 0;
    state->max_row_reached = state->player_y; 
    state->game_over = 0;

    state->world_position = MAP_HEIGHT;
    state->just_scrolled = 0;
}

void game_reset(GameState *state)
{
    if (!state) return;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        row_destroy(&state->rows[y]);
    }
    game_init(state, MAP_WIDTH);
}

/* -------------------------------------------------------
   MOVE AS LINHAS (ROTAÇÃO DAS FILAS)
   - Marca moved_this_tick=1 somente quando rotaciona.
 ------------------------------------------------------- */
static void move_rows(GameState *state)
{
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        Row *row = &state->rows[y];

        if (!row->queue || row->type == ROW_GRASS) {
            row->moved_this_tick = 0;
            continue;
        }

        row->tick_counter++;
        if (row->tick_counter >= row->speed_ticks) {
            row->tick_counter = 0;
            if (row->direction < 0) queue_rotate_left(row->queue);
            else                    queue_rotate_right(row->queue);
            row->moved_this_tick = 1;  // <<-- moveu AGORA
        } else {
            row->moved_this_tick = 0;  // <<-- não moveu
        }
    }
}

/* -------------------------------------------------------
   EMPURRÃO DO RIO: carrega junto SÓ quando a linha moveu
   (e nunca por causa do scroll)
 ------------------------------------------------------- */
static void apply_river_push(GameState *state)
{
    if (!state || state->game_over) return;
    if (state->just_scrolled) return;
    if (state->player_y < 0 || state->player_y >= MAP_HEIGHT) return;

    Row *row = &state->rows[state->player_y];
    if (row->type != ROW_RIVER || !row->queue) return;

    char under = queue_get_cell(row->queue, state->player_x);
    if (under != CHAR_LOG) return; // na água: morte será verificada em seguida

    // Só empurra se a linha REALMENTE moveu neste frame
    if (row->moved_this_tick) {
        if (row->direction < 0) state->player_x--;
        else                    state->player_x++;

        // wrap horizontal
        if (state->player_x < 0)               state->player_x = MAP_WIDTH - 1;
        else if (state->player_x >= MAP_WIDTH) state->player_x = 0;
    }
}

/* -------------------------------------------------------
   COLISÕES (robustas)
   - Estrada: não-espaço => carro => morre
   - Rio: espaço => água => morre
 ------------------------------------------------------- */
static void check_collision(GameState *state)
{
    if (!state || state->game_over) return;

    if (state->player_x < 0 || state->player_x >= MAP_WIDTH) { state->game_over = 1; return; }
    if (state->player_y < 0 || state->player_y >= MAP_HEIGHT){ state->game_over = 1; return; }

    Row *row = &state->rows[state->player_y];
    if (row->type == ROW_GRASS) return;
    if (!row->queue) return;

    char cell = queue_get_cell(row->queue, state->player_x);

    if (row->type == ROW_ROAD) {
        if (cell != ' ') state->game_over = 1;
        return;
    }

    if (row->type == ROW_RIVER) {
        // Se a célula atual é água
        if (cell == ' ') {
            // exceção: se o tronco acabou de "sair" pela direita e o player está na borda
            if (row->moved_this_tick && row->direction > 0 && state->player_x == MAP_WIDTH - 1) {
                return; // protege o jogador do falso negativo
            }
            state->game_over = 1;
        }
        return;
    }
}

/* -------------------------------------------------------
   UPDATE: SCROLL (tempo), MOVE (carros/troncos), PUSH, COLLIDE
 ------------------------------------------------------- */
 void game_update(GameState *state)
 {
     if (!state || state->game_over) return;
 
     // 1) Scroll vertical por tempo
     static int scroll_timer = 0;
     scroll_timer++;
     if (scroll_timer >= SCROLL_TICKS) {
         scroll_timer = 0;
 
         // FAZ O SCROLL
         scroll_world_down(state);
 
         // Checa colisão após reposicionar o mundo
         check_collision(state);
 
         // Damos um frame de respiro: nada de mover linhas/empurrar neste frame
         state->just_scrolled = 1;
         return;
     }
 
     // ======== EMPURRÃO DO RIO CORRETO (pré-checagem) ========
     // Captura o estado ANTES da rotação para decidir se deve carregar o player
     int will_push = 0;      // se a linha do player vai mover neste frame
     int push_dir  = 0;      // direção do empurrão
     if (!state->just_scrolled &&
         state->player_y >= 0 && state->player_y < MAP_HEIGHT)
     {
         Row *prow = &state->rows[state->player_y];
         if (prow->type == ROW_RIVER && prow->queue) {
             // Vai rotacionar neste frame?
             int will_move = (prow->tick_counter + 1 >= prow->speed_ticks);
             // Está em cima de tronco ANTES da rotação?
             char under_pre = queue_get_cell(prow->queue, state->player_x);
             if (will_move && under_pre == CHAR_LOG) {
                 will_push = 1;
                 push_dir  = (prow->direction < 0) ? -1 : +1;
             }
         }
     }
 
     // 2) Agora mova todas as linhas (rotaciona carros/troncos)
     move_rows(state);
 
     // 3) Se precisava empurrar, empurre AGORA (imediatamente após a rotação)
     if (will_push) {
         state->player_x += push_dir;
         if (state->player_x < 0)                state->player_x = MAP_WIDTH - 1;
         else if (state->player_x >= MAP_WIDTH)  state->player_x = 0;
     }
 
     // 4) Colisão do frame
     check_collision(state);
 
     // Libera o “respiro” para os próximos frames
     state->just_scrolled = 0;
 }
 
/* -------------------------------------------------------
   RENDER SIMPLES (ASCII)
 ------------------------------------------------------- */
void game_render(const GameState *state)
{
    utils_clear_screen();

    for (int i = 0; i < MAP_WIDTH + 2; ++i) putchar('#');
    putchar('\n');

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        putchar('#');
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (state->player_x == x && state->player_y == y) {
                putchar(CHAR_PLAYER);
            } else {
                const Row *row = &state->rows[y];
                if (row->type == ROW_GRASS || !row->queue) {
                    putchar(CHAR_GRASS);
                } else {
                    char cell = queue_get_cell(row->queue, x);
                    putchar(cell);
                }
            }
        }
        putchar('#');
        putchar('\n');
    }

    for (int i = 0; i < MAP_WIDTH + 2; ++i) putchar('#');
    putchar('\n');

    printf("Score: %d  World: %d  Controls: W/A/S/D (Q sai)\n",
           state->score, state->world_position);
}

/* -------------------------------------------------------
   INPUT / CONTROLES
   - Player livre (sem ancoragem/câmera).
 ------------------------------------------------------- */
void game_handle_input(GameState *state, int key)
{
    if (!state || state->game_over) return;

    if (key == 'W') {
            state->player_y--;
        if (state->player_y < state->max_row_reached) {
            state->score++;
            state->max_row_reached = state->player_y;
        }
}
           
    } else if (key == 'S') {
        if (state->player_y < MAP_HEIGHT - 1) {
            state->player_y++;
        }
    } else if (key == 'A') {
        state->player_x--;
    } else if (key == 'D') {
        state->player_x++;
    }

    // clamp horizontal
    if (state->player_x < 0)                  state->player_x = 0;
    if (state->player_x >= MAP_WIDTH)         state->player_x = MAP_WIDTH - 1;

    // colisão após movimento manual
    check_collision(state);
}
