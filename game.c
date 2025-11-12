// game.c
#include "game.h"
#include "utils.h"
#include "raylib.h"  // Para GetFrameTime()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    // Para ceil()

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
static void generate_row(Row *row, int world_position, GameState *state);
static void row_destroy(Row *row);
static void ensure_safe_area(GameState *state);
static void scroll_world_down(GameState *state);
static void move_rows(GameState *state);
static void check_collision(GameState *state);
// === 2 PLAYER MODE ===
static void check_collision_player(GameState *state, Player *player);
// Sistema de vidas
static void handle_death(GameState *state);
static void collect_life_power(GameState *state);

/* -------------------------------------------------------
   PREENCHIMENTO COM OBSTÁCULOS + GAPSF
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
static void generate_row(Row *row, int world_position, GameState *state)
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
    
    // Sistema de vidas: gera poder de vida a cada 25 linhas (apenas modo 1 jogador)
    if (state && !state->two_players && world_position > 0 && 
        world_position % 25 == 0 && world_position != state->life_power_spawned) {
        // Verifica se já existe um poder de vida no mapa
        int has_life_power = 0;
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            if (state->rows[y].queue) {
                for (int x = 0; x < MAP_WIDTH; ++x) {
                    if (queue_get_cell(state->rows[y].queue, x) == CHAR_LIFE) {
                        has_life_power = 1;
                        break;
                    }
                }
                if (has_life_power) break;
            }
        }
        
        // Se não há poder de vida no mapa, cria um em posição aleatória
        if (!has_life_power) {
            int life_x = utils_random_int(0, MAP_WIDTH - 1);
            queue_set_cell(row->queue, life_x, CHAR_LIFE);
            state->life_power_spawned = world_position;
        }
    }
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
   ÁREA SEGURA (APENAS NO INÍCIO)
   - início: 3 linhas seguras (apenas nas primeiras 20 linhas)
   - depois: NENHUMA área segura - mapa desce normalmente
 ------------------------------------------------------- */
static void ensure_safe_area(GameState *state)
{
    if (!state) return;

    // Área segura apenas no início do jogo (primeiras 20 linhas)
    if (state->world_position < 20) {
        int safe_lines = 3;
        
        // Força grama nas últimas 3 linhas apenas no início
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
    // Depois das primeiras 20 linhas: NÃO faz nada - mapa desce normalmente
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

    // --- DESCE TODAS AS LINHAS (SCROLL) ---
    for (int y = MAP_HEIGHT - 1; y > 0; --y) {
        row_destroy(&state->rows[y]);          // libera memória da linha atual
        state->rows[y] = state->rows[y - 1];   // copia a linha de cima para baixo
        state->rows[y - 1].queue = NULL;       // evita ponteiro duplicado (double free)
    }

    // --- GERA UMA NOVA LINHA NO TOPO ---
    row_destroy(&state->rows[0]);              // libera topo antigo (se houver)
    generate_row(&state->rows[0], state->world_position, state); // cria nova linha de mundo

    // --- ZERA FLAGS DE MOVIMENTO (scroll não conta como "mover linha") ---
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        state->rows[y].moved_this_tick = 0;
    }

    ensure_safe_area(state);                   // mantém as áreas seguras (grama)

    // --- AVANÇO LÓGICO DO MUNDO ---
    state->world_position++;                   // contador global de linhas geradas
    state->world_head++;                       // uma nova linha "absoluta" nasceu no topo

    // --- AJUSTE VISUAL DO PLAYER ---
    // O mapa sobe => o player "desce" 1 linha visualmente
    state->player_y++;
    
    // === 2 PLAYER MODE ===
    // No modo 2 jogadores, atualiza ambos os jogadores durante o scroll
    if (state->two_players) {
        // Ajusta posição Y de ambos (mapa sobe = jogadores descem visualmente)
        state->p1.y++;
        state->p2.y++;
        
        // Sincroniza progresso absoluto de ambos (+2 porque scroll adiciona +2 no abs)
        state->p1.last_abs += 2;
        state->p2.last_abs += 2;
        state->p1.min_abs_reached += 2;  // Move o limite de progresso também
        state->p2.min_abs_reached += 2;
        
        // Zera flag de avanço (scroll não conta como avanço manual)
        state->p1.advanced_this_tick = 0;
        state->p2.advanced_this_tick = 0;
        
        // Game over se AMBOS os jogadores saírem da tela (sairam por baixo)
        if (state->p1.y >= MAP_HEIGHT && state->p2.y >= MAP_HEIGHT) {
            state->game_over = 1;
        }
        
        // Se apenas um jogador saiu da tela, marca como morto
        // O outro jogador pode continuar até morrer ou sair também
        if (state->p1.y >= MAP_HEIGHT && state->p1.alive) {
            state->p1.alive = 0;  // P1 morreu
        }
        if (state->p2.y >= MAP_HEIGHT && state->p2.alive) {
            state->p2.alive = 0;  // P2 morreu
        }
    } else {
        // Modo 1 jogador
        state->last_abs += 2;           // mantém o histórico alinhado
        state->min_abs_reached += 2;    // também move o limite de progresso
        state->advanced_this_tick = 0;  // neste frame, o player não subiu manualmente

        // --- GAME OVER SE SAIU DA TELA ---
        if (state->player_y >= MAP_HEIGHT) {
            state->game_over = 1;
            return;
        }
    }

    // --- FLAG DE RESPIRO ---
    state->just_scrolled = 1;       // evita empurrão do rio neste frame
}

/* -------------------------------------------------------
   INIT / RESET
 ------------------------------------------------------- */
 void game_init(GameState *state, int width)
 {
     if (!state) return;
 
     state->world_position = 0;
 
     // Gera o buffer inicial de linhas visíveis
     for (int y = 0; y < MAP_HEIGHT; ++y) {
         generate_row(&state->rows[y], y, state);
     }
 
     ensure_safe_area(state);
 
     // Posição inicial do player (centralizado no X, 1 acima do rodapé no Y)
     state->player_x = width / 2;
     state->player_y = MAP_HEIGHT - 2;
 
     // Estado base do jogo
     state->score        = 0;
     state->world_head   = 0;  // 0 linhas absolutas "nascidas" no topo ainda
     state->game_over    = 0;
 
     // ===== Referenciais de progresso =====
     // Posição absoluta inicial do player no mundo
     int abs0 = state->world_head + state->player_y;
 
     state->min_abs_reached   = abs0;  // melhor (menor) linha absoluta já alcançada
     state->last_abs          = abs0;  // histórico p/ sincronizar com scroll
     state->advanced_this_tick = 0;    // neste frame ainda não avançou verticalmente
 
     // Outros controles
     state->world_position = MAP_HEIGHT; // mantém seu comportamento original
     state->just_scrolled  = 0;          // nenhum scroll ocorreu ainda
     
    // Sistema de vidas
    state->vidas = 1;                    // Começa com 1 vida
    state->renascendo = 0;               // Não está renascendo
    state->renascer_timer = 0.0f;        // Timer zerado
    state->life_power_spawned = -25;     // Inicializa para permitir primeiro spawn
     
    // === 2 PLAYER MODE ===
    // Inicializa modo 1 jogador por padrão (two_players = 0)
    // Mesmo assim, inicializa ambos os jogadores para facilitar transição
    state->two_players = 0;
    
    // Inicializa Jogador 1 (P1) - mesma posição do player principal
    state->p1.x = width / 2;
    state->p1.y = MAP_HEIGHT - 2;
    state->p1.alive = 1;              // Começa vivo
    state->p1.score = 0;              // Pontuação inicial zero
    state->p1.min_abs_reached = abs0; // Progresso inicial
    state->p1.last_abs = abs0;        // Histórico inicial
    state->p1.advanced_this_tick = 0; // Não avançou ainda
    
    // Inicializa Jogador 2 (P2) - mesma posição inicial
    state->p2.x = width / 2;
    state->p2.y = MAP_HEIGHT - 2;
    state->p2.alive = 1;              // Começa vivo
    state->p2.score = 0;              // Pontuação inicial zero
    state->p2.min_abs_reached = abs0; // Progresso inicial
    state->p2.last_abs = abs0;        // Histórico inicial
    state->p2.advanced_this_tick = 0; // Não avançou ainda
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
   COLETA PODER DE VIDA
   - Verifica se o jogador está em uma célula com CHAR_LIFE
   - Adiciona +1 vida (máximo 5)
------------------------------------------------------- */
static void collect_life_power(GameState *state)
{
    // Sistema de vidas só funciona no modo 1 jogador
    if (!state || state->renascendo || state->two_players) return;
    
    // Modo 1 jogador apenas
    if (state->player_y >= 0 && state->player_y < MAP_HEIGHT) {
        Row *row = &state->rows[state->player_y];
        if (row->queue) {
            char cell = queue_get_cell(row->queue, state->player_x);
            if (cell == CHAR_LIFE) {
                // Coleta o poder de vida
                if (state->vidas < 5) {
                    state->vidas++;
                }
                // Remove o poder do mapa
                queue_set_cell(row->queue, state->player_x, ' ');
                state->life_power_spawned = -1; // Permite gerar novo poder
            }
        }
    }
}

/* -------------------------------------------------------
   GERENCIA MORTE E RENASCIMENTO
   - Reduz uma vida
   - Se ainda tem vidas, inicia processo de renascimento
   - Se não tem mais vidas, termina o jogo
------------------------------------------------------- */
static void handle_death(GameState *state)
{
    // Sistema de vidas só funciona no modo 1 jogador
    if (!state || state->two_players) {
        // No modo 2 jogadores, termina o jogo imediatamente
        if (state) state->game_over = 1;
        return;
    }
    
    // Reduz uma vida
    state->vidas--;
    
    // Se não tem mais vidas, termina o jogo
    if (state->vidas <= 0) {
        state->game_over = 1;
        return;
    }
    
    // Inicia processo de renascimento
    state->renascendo = 1;
    state->renascer_timer = 3.0f; // 3 segundos
    
    // Reposiciona o jogador no centro inferior (posição segura)
    state->player_x = MAP_WIDTH / 2;
    state->player_y = MAP_HEIGHT - 2;
    
    // Atualiza a posição absoluta para permitir pontuação imediata após renascer
    int abs_now = state->world_head + state->player_y;
    state->min_abs_reached = abs_now;  // Reseta o melhor progresso para a posição atual
    state->last_abs = abs_now;         // Atualiza histórico
    state->advanced_this_tick = 0;     // Reseta flag de avanço
}

/* -------------------------------------------------------
   COLISÕES (robustas)
   - Estrada: não-espaço => carro => morre
   - Rio: espaço => água => morre
   - Verifica coleta de poder de vida
------------------------------------------------------- */
static void check_collision(GameState *state)
{
    if (!state || state->game_over) return;
    
    // Sistema de renascimento só funciona no modo 1 jogador
    if (state->renascendo && !state->two_players) return;

    // Verifica coleta de poder de vida primeiro (apenas modo 1 jogador)
    if (!state->two_players) {
        collect_life_power(state);
    }

    if (state->two_players) {
        // Modo 2 jogadores: verifica ambos
        if (state->p1.alive) check_collision_player(state, &state->p1);
        if (state->p2.alive) check_collision_player(state, &state->p2);
        
        // Game over se ambos morreram
        if (!state->p1.alive && !state->p2.alive) {
            state->game_over = 1;
        }
        return;
    }

    // Modo 1 jogador (compatibilidade)
    if (state->player_x < 0 || state->player_x >= MAP_WIDTH) { 
        handle_death(state);
        return; 
    }
    if (state->player_y < 0 || state->player_y >= MAP_HEIGHT) { 
        handle_death(state);
        return; 
    }

    Row *row = &state->rows[state->player_y];
    if (row->type == ROW_GRASS) return;
    if (!row->queue) return;

    char cell = queue_get_cell(row->queue, state->player_x);

    if (row->type == ROW_ROAD) {
        if (cell != ' ') {
            handle_death(state);
        }
        return;
    }

    if (row->type == ROW_RIVER) {
        // Se a célula atual é água
        if (cell == ' ') {
            // exceção: se o tronco acabou de "sair" pela direita e o player está na borda
            if (row->moved_this_tick && row->direction > 0 && state->player_x == MAP_WIDTH - 1) {
                return; // protege o jogador do falso negativo
            }
            handle_death(state);
        }
        return;
    }
}

// === 2 PLAYER MODE ===
/**
 * Verifica colisão de um jogador específico com obstáculos
 * Similar à check_collision, mas para um único jogador
 * @param state Estado do jogo
 * @param player Ponteiro para o jogador a verificar
 */
static void check_collision_player(GameState *state, Player *player)
{
    // Validações: estado válido, jogador válido e vivo
    if (!state || !player || !player->alive) return;

    // Verifica se jogador saiu dos limites do mapa (horizontal)
    if (player->x < 0 || player->x >= MAP_WIDTH) { 
        player->alive = 0;  // Morreu por sair horizontalmente
        return; 
    }
    // Verifica se jogador saiu dos limites do mapa (vertical)
    if (player->y < 0 || player->y >= MAP_HEIGHT) { 
        player->alive = 0;  // Morreu por sair verticalmente
        return; 
    }

    // Obtém a linha onde o jogador está
    Row *row = &state->rows[player->y];
    
    // Grama é sempre segura (sem obstáculos)
    if (row->type == ROW_GRASS) return;
    if (!row->queue) return;

    // Verifica o que está na célula onde o jogador está
    char cell = queue_get_cell(row->queue, player->x);

    // Estrada: se não for espaço vazio, há um carro = colisão fatal
    if (row->type == ROW_ROAD) {
        if (cell != ' ') {
            player->alive = 0;  // Morreu por colisão com carro
        }
        return;
    }

    // Rio: se for espaço vazio, está na água = colisão fatal
    if (row->type == ROW_RIVER) {
        if (cell == ' ') {
            // Exceção: proteção contra falso negativo quando tronco sai pela direita
            // Evita que o jogador morra quando o tronco que ele está desaparece pela borda
            if (row->moved_this_tick && row->direction > 0 && player->x == MAP_WIDTH - 1) {
                return;  // Seguro, está no tronco que acabou de sair pela direita
            }
            player->alive = 0;  // Morreu por cair na água
        }
        // Se cell == CHAR_LOG, o jogador está seguro em cima do tronco
        return;
    }
}

/* -------------------------------------------------------
   UPDATE: SCROLL (tempo), MOVE (carros/troncos), PUSH, COLLIDE
------------------------------------------------------- */
 void game_update(GameState *state)
 {
     if (!state || state->game_over) return;
     
     // Sistema de renascimento: pausa o jogo durante o renascimento (apenas modo 1 jogador)
     if (state->renascendo && !state->two_players) {
         // Decrementa o timer
         state->renascer_timer -= GetFrameTime();
         
         // Quando o timer chega a 0, termina o renascimento
         if (state->renascer_timer <= 0.0f) {
             state->renascendo = 0;
             state->renascer_timer = 0.0f;
             // Jogador já foi reposicionado em handle_death()
             // Garante que a posição absoluta está atualizada para pontuação imediata
             int abs_now = state->world_head + state->player_y;
             state->min_abs_reached = abs_now;
             state->last_abs = abs_now;
         }
         // Durante o renascimento, não atualiza nada mais
         return;
     }
 
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
     // === 2 PLAYER MODE ===
     if (state->two_players) {
         // Modo 2 jogadores: verifica empurrão de troncos para ambos os jogadores
         int will_push_p1 = 0, push_dir_p1 = 0;  // Flags para P1
         int will_push_p2 = 0, push_dir_p2 = 0;  // Flags para P2
         
         // Só verifica empurrão se não acabou de fazer scroll (evita conflito)
         if (!state->just_scrolled) {
             // Verifica se P1 está em tronco que vai se mover
             if (state->p1.alive && state->p1.y >= 0 && state->p1.y < MAP_HEIGHT) {
                 Row *prow = &state->rows[state->p1.y];
                 if (prow->type == ROW_RIVER && prow->queue) {
                     // Verifica se o tronco vai rotacionar neste frame
                     int will_move = (prow->tick_counter + 1 >= prow->speed_ticks);
                     // Verifica o que está embaixo do P1 ANTES da rotação
                     char under_pre = queue_get_cell(prow->queue, state->p1.x);
                     if (will_move && under_pre == CHAR_LOG) {
                         // P1 será empurrado junto com o tronco
                         will_push_p1 = 1;
                         push_dir_p1 = (prow->direction < 0) ? -1 : +1;  // Esquerda ou direita
                     }
                 }
             }
             // Verifica se P2 está em tronco que vai se mover
             if (state->p2.alive && state->p2.y >= 0 && state->p2.y < MAP_HEIGHT) {
                 Row *prow = &state->rows[state->p2.y];
                 if (prow->type == ROW_RIVER && prow->queue) {
                     int will_move = (prow->tick_counter + 1 >= prow->speed_ticks);
                     char under_pre = queue_get_cell(prow->queue, state->p2.x);
                     if (will_move && under_pre == CHAR_LOG) {
                         // P2 será empurrado junto com o tronco
                         will_push_p2 = 1;
                         push_dir_p2 = (prow->direction < 0) ? -1 : +1;
                     }
                 }
             }
         }
         
         // 2) Agora mova todas as linhas (rotaciona carros/troncos)
         move_rows(state);
         
         // 3) Empurra ambos os jogadores se necessário (após a rotação)
         if (will_push_p1 && state->p1.alive) {
             state->p1.x += push_dir_p1;
             // Wrap-around nas bordas (se sair por um lado, aparece do outro)
             if (state->p1.x < 0) state->p1.x = MAP_WIDTH - 1;
             else if (state->p1.x >= MAP_WIDTH) state->p1.x = 0;
         }
         if (will_push_p2 && state->p2.alive) {
             state->p2.x += push_dir_p2;
             // Wrap-around nas bordas
             if (state->p2.x < 0) state->p2.x = MAP_WIDTH - 1;
             else if (state->p2.x >= MAP_WIDTH) state->p2.x = 0;
         }
     } else {
         // Modo 1 jogador (compatibilidade)
         int will_push = 0;
         int push_dir  = 0;
         if (!state->just_scrolled &&
             state->player_y >= 0 && state->player_y < MAP_HEIGHT)
         {
             Row *prow = &state->rows[state->player_y];
             if (prow->type == ROW_RIVER && prow->queue) {
                 int will_move = (prow->tick_counter + 1 >= prow->speed_ticks);
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
     }
 
     // 4) Colisão do frame
     check_collision(state);
 
     // Libera o “respiro” para os próximos frames
     state->just_scrolled = 0;
 }

/* -------------------------------------------------------
   INPUT / CONTROLES
   - Player livre (sem ancoragem/câmera).
 ------------------------------------------------------- */
 void game_handle_input(GameState *state, int key)
{
    if (!state || state->game_over || state->renascendo) return;      // Sem estado, jogo encerrado ou renascendo: não faz nada.

    int old_x = state->player_x;                 // Guarda posição anterior para desfazer se morrer.
    int old_y = state->player_y;

    // --- MOVIMENTO (WASD) ---
    if (key == 'W') {                            // Sobe 1 linha (se não está no topo visível).
        if (state->player_y > 0) state->player_y--;
    } else if (key == 'S') {                     // Desce 1 linha (se não está no rodapé visível).
        if (state->player_y < MAP_HEIGHT - 1) state->player_y++;
    } else if (key == 'A') {                     // Esquerda (se não está na borda).
        if (state->player_x > 0) state->player_x--;
    } else if (key == 'D') {                     // Direita (se não está na borda).
        if (state->player_x < MAP_WIDTH - 2) state->player_x++;
    } else if (key == 'Q') {                     // Sair.
        state->game_over = 1;
    }

    // --- LIMITES DA TELA (clamp) ---
    if (state->player_x < 0)               state->player_x = 0;
    if (state->player_x >= MAP_WIDTH)      state->player_x = MAP_WIDTH - 1;
    if (state->player_y < 0)               state->player_y = 0;
    if (state->player_y >= MAP_HEIGHT)     state->player_y = MAP_HEIGHT - 1;

    // --- COLISÃO APÓS MOVER ---
    check_collision(state);

    // --- FLAG: avançou verticalmente para cima neste frame? ---
    // (Só consideramos "avanço" se o player realmente subiu 1 linha: y diminuiu.)
    state->advanced_this_tick = (!state->game_over && state->player_y < old_y) ? 1 : 0;

    if (!state->game_over) {
        // Linha absoluta atual do player no mundo (independe do scroll visual).
        int abs_now = state->world_head + state->player_y;

        // --- PONTUAÇÃO ROBUSTA ---
        // Pontua SOMENTE se houve avanço para cima neste frame E a linha absoluta é inédita
        // (abs_now menor que o melhor já alcançado).
        if (state->advanced_this_tick && abs_now < state->min_abs_reached) {
            state->score++;
            state->min_abs_reached = abs_now;
        }

        // Atualiza histórico para próximos frames (mantido alinhado no scroll).
        state->last_abs = abs_now;
    } else {
        // Morreu: restaura posição visual para não "teleportar" o cadáver.
        state->player_x = old_x;
        state->player_y = old_y;
        state->advanced_this_tick = 0;           // Não houve avanço válido.
    }
}

// === 2 PLAYER MODE ===
/**
 * Ativa ou desativa o modo 2 jogadores
 * Quando ativado, sincroniza os jogadores com o estado atual do jogo
 */
void game_set_two_players(GameState *state, int enabled)
{
    if (!state) return;
    state->two_players = enabled ? 1 : 0;
    
    if (enabled) {
        // Sincroniza P1 com o estado atual do player principal (para transição suave)
        state->p1.x = state->player_x;
        state->p1.y = state->player_y;
        state->p1.alive = 1;                           // Começa vivo
        state->p1.score = state->score;                // Herda pontuação atual
        state->p1.min_abs_reached = state->min_abs_reached;  // Herda progresso
        state->p1.last_abs = state->last_abs;          // Herda histórico
        state->p1.advanced_this_tick = 0;              // Reseta flag
        
        // P2 começa ligeiramente à direita de P1 (3 células) para evitar sobreposição
        state->p2.x = (state->player_x + 3) % MAP_WIDTH;  // Wrap-around se necessário
        state->p2.y = state->player_y;                    // Mesma linha Y
        state->p2.alive = 1;                              // Começa vivo
        state->p2.score = 0;                              // Pontuação inicial zero
        int abs0 = state->world_head + state->p2.y;       // Calcula posição absoluta inicial
        state->p2.min_abs_reached = abs0;                // Progresso inicial
        state->p2.last_abs = abs0;                        // Histórico inicial
        state->p2.advanced_this_tick = 0;                 // Reseta flag
    }
}

/**
 * Processa input de movimento de um jogador específico no modo 2 jogadores
 * Similar a game_handle_input, mas para um jogador individual
 */
void game_handle_input_player(GameState *state, int player_id, char key)
{
    // Validações: estado válido, jogo não acabou, modo 2P ativo, não está renascendo
    if (!state || state->game_over || !state->two_players || state->renascendo) return;
    
    // Seleciona o jogador correto (P1 ou P2)
    Player *player = (player_id == 1) ? &state->p1 : &state->p2;
    
    // Se o jogador já está morto, não processa input
    if (!player->alive) return;
    
    // Guarda posição anterior para restaurar se morrer
    int old_x = player->x;
    int old_y = player->y;
    
    // Processa movimento baseado na tecla
    // W = cima, S = baixo, A = esquerda, D = direita
    if (key == 'W') {
        if (player->y > 0) player->y--;  // Sobe (diminui Y)
    } else if (key == 'S') {
        if (player->y < MAP_HEIGHT - 1) player->y++;  // Desce (aumenta Y)
    } else if (key == 'A') {
        if (player->x > 0) player->x--;  // Esquerda (diminui X)
    } else if (key == 'D') {
        if (player->x < MAP_WIDTH - 1) player->x++;  // Direita (aumenta X)
    }
    
    // Garante que o jogador não saiu dos limites do mapa
    if (player->x < 0) player->x = 0;
    if (player->x >= MAP_WIDTH) player->x = MAP_WIDTH - 1;
    if (player->y < 0) player->y = 0;
    if (player->y >= MAP_HEIGHT) player->y = MAP_HEIGHT - 1;
    
    // Verifica colisão após mover (carros, rio, bordas)
    check_collision_player(state, player);
    
    // Verifica se o jogador avançou verticalmente (subiu = Y diminuiu)
    // Isso é usado para calcular pontuação
    player->advanced_this_tick = (player->alive && player->y < old_y) ? 1 : 0;
    
    if (player->alive) {
        // Jogador ainda vivo: atualiza pontuação se avançou para nova linha
        int abs_now = state->world_head + player->y;  // Posição absoluta atual
        
        // Pontua apenas se avançou E alcançou uma linha absoluta nunca visitada antes
        if (player->advanced_this_tick && abs_now < player->min_abs_reached) {
            player->score++;                          // Incrementa pontuação
            player->min_abs_reached = abs_now;        // Atualiza melhor progresso
        }
        player->last_abs = abs_now;  // Atualiza histórico
    } else {
        // Jogador morreu: restaura posição anterior para não "teleportar" o cadáver
        player->x = old_x;
        player->y = old_y;
        player->advanced_this_tick = 0;  // Reseta flag
    }
}

/**
 * Obtém a posição atual de um jogador
 * Compatível com modo 1P e 2P
 */
void game_get_player_pos(const GameState *state, int player_id, int *x, int *y)
{
    if (!state || !x || !y) return;
    
    if (state->two_players) {
        // Modo 2P: retorna posição do jogador específico
        const Player *player = (player_id == 1) ? &state->p1 : &state->p2;
        *x = player->x;
        *y = player->y;
    } else {
        // Modo 1P: retorna posição do player principal
        *x = state->player_x;
        *y = state->player_y;
    }
}

/**
 * Verifica se um jogador está vivo
 * Compatível com modo 1P e 2P
 */
int game_is_player_alive(const GameState *state, int player_id)
{
    if (!state) return 0;
    
    if (state->two_players) {
        // Modo 2P: verifica flag alive do jogador específico
        const Player *player = (player_id == 1) ? &state->p1 : &state->p2;
        return player->alive ? 1 : 0;
    }
    
    // Modo 1P: vivo se o jogo não acabou
    return (!state->game_over) ? 1 : 0;
}

/**
 * Obtém a pontuação de um jogador
 * Compatível com modo 1P e 2P
 */
int game_get_player_score(const GameState *state, int player_id)
{
    if (!state) return 0;
    
    if (state->two_players) {
        // Modo 2P: retorna pontuação individual do jogador
        const Player *player = (player_id == 1) ? &state->p1 : &state->p2;
        return player->score;
    }
    
    // Modo 1P: retorna pontuação do player principal
    return state->score;
}