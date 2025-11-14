#ifndef GAME_H
#define GAME_H

#include "lista.h"

// Map configuration
#define MAP_WIDTH  28  // Reduzido de 31 para 28 para caber na tela (28*25 + 50*2 = 800px)
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
#define CHAR_LIFE   '+'    // Poder de vida

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

// === 2 PLAYER MODE ===
/**
 * Estrutura que representa um jogador individual no modo 2 jogadores
 * Cada jogador mantém seu próprio estado de posição, vida, pontuação e progresso
 */
typedef struct Player {
    int x;                      // Posição X no mapa (0 a MAP_WIDTH-1)
    int y;                      // Posição Y no mapa (0 a MAP_HEIGHT-1)
    int alive;                  // 1 = vivo, 0 = morto
    int score;                  // Pontuação individual do jogador
    int min_abs_reached;        // Menor posição absoluta já alcançada (melhor progresso)
    int last_abs;               // Última posição absoluta registrada (histórico)
    int advanced_this_tick;     // 1 se avançou verticalmente neste frame (para pontuação)
} Player;

typedef struct GameState {
    Row rows[MAP_HEIGHT];
    int player_x;  
    int player_y;  
    int score;    
    int game_over;

    int world_position;   
    int just_scrolled;    

    int world_head;       // Quantas linhas já nasceram no topo (quantos scrolls)
    int min_abs_reached;  // Menor índice absoluto já alcançado (melhor progresso)
    int last_abs;          // Abs do frame anterior (usado p/ detectar avanço real)
    int advanced_this_tick; // 1 se subiu y neste frame (player_y diminuiu)
    
    // Sistema de vidas
    int vidas;              // Vidas atuais do jogador
    int renascendo;         // 1 se está em processo de renascimento
    float renascer_timer;   // Tempo restante do renascimento (em segundos)
    int life_power_spawned; // Posição do mundo onde o último poder de vida foi gerado
    
    // === 2 PLAYER MODE ===
    Player p1, p2;              // Estruturas dos jogadores (P1 e P2)
    int two_players;            // Flag: 1 = modo 2 jogadores ativo, 0 = modo 1 jogador
} GameState;

void game_init(GameState *state, int width);
void game_reset(GameState *state);
void game_update(GameState *state);
void game_render(const GameState *state);
void game_handle_input(GameState *state, int key);

// === 2 PLAYER MODE ===
/**
 * Ativa ou desativa o modo 2 jogadores
 * @param state Estado do jogo
 * @param enabled 1 para ativar modo 2 jogadores, 0 para modo 1 jogador
 */
void game_set_two_players(GameState *state, int enabled);

/**
 * Processa input de movimento de um jogador específico
 * @param state Estado do jogo
 * @param player_id ID do jogador (1 = P1, 2 = P2)
 * @param key Tecla pressionada ('W', 'S', 'A', 'D')
 */
void game_handle_input_player(GameState *state, int player_id, char key);

/**
 * Obtém a posição atual de um jogador
 * @param state Estado do jogo
 * @param player_id ID do jogador (1 = P1, 2 = P2)
 * @param x Ponteiro para retornar posição X
 * @param y Ponteiro para retornar posição Y
 */
void game_get_player_pos(const GameState *state, int player_id, int *x, int *y);

/**
 * Verifica se um jogador está vivo
 * @param state Estado do jogo
 * @param player_id ID do jogador (1 = P1, 2 = P2)
 * @return 1 se vivo, 0 se morto
 */
int game_is_player_alive(const GameState *state, int player_id);

/**
 * Obtém a pontuação de um jogador
 * @param state Estado do jogo
 * @param player_id ID do jogador (1 = P1, 2 = P2)
 * @return Pontuação do jogador
 */
int game_get_player_score(const GameState *state, int player_id);

#endif // GAME_H