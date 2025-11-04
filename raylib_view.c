#include "raylib_view.h"
#include "raylib.h"
#include "game.h"
#include "ranking.h"
#include "utils.h"
#include <string.h>

// ---- FORWARD DECLARATIONS (prototipos) ----
static void render_menu_screen(int menu_index, const char** options, int count);
static void render_help_screen(void);
static void render_ranking_screen(const Ranking *ranking);

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 25
#define MARGIN 50
#define RANKING_FILE "ranking.txt"

// Cores no estilo voxel/Crossy Road
#define COLOR_GRASS (Color){76, 175, 80, 255}
#define COLOR_ROAD  (Color){97, 97, 97, 255}
#define COLOR_RIVER (Color){33, 150, 243, 255}
#define COLOR_CAR   (Color){244, 67, 54, 255}
#define COLOR_LOG   (Color){121, 85, 72, 255}
#define COLOR_PLAYER (Color){255, 193, 7, 255}
#define COLOR_TREE (Color){76, 175, 80, 255}
#define COLOR_TREE_TRUNK (Color){121, 85, 72, 255}
// === 2 PLAYER MODE ===
// Cores dos jogadores para diferenciá-los visualmente
#define COLOR_PLAYER1 (Color){255, 193, 7, 255}    // Amarelo - Jogador 1 (P1)
#define COLOR_PLAYER2 (Color){0, 200, 255, 255}    // Ciano - Jogador 2 (P2)

// Estados do jogo
typedef enum {
    GAME_START_SCREEN,
    GAME_NAME_INPUT_SCREEN,
    GAME_NAME_INPUT_SCREEN_P2,  // === 2 PLAYER MODE === Tela para input do nome do Jogador 2
    GAME_PLAYING,
    GAME_OVER_SCREEN,
    GAME_HELP_SCREEN,
    GAME_RANKING_SCREEN
} GameScreen;

// ----- Desenho voxel -----
/**
 * Desenha um jogador no estilo voxel com cor customizável
 * === 2 PLAYER MODE === Versão atualizada para suportar cores diferentes
 * @param x Posição X em pixels
 * @param y Posição Y em pixels
 * @param player_color Cor do jogador (amarelo para P1, ciano para P2)
 */
static void draw_player_voxel(int x, int y, Color player_color) {
    // Corpo principal (bloco maior)
    DrawRectangle(x + 2, y + 8, CELL_SIZE - 4, CELL_SIZE - 8, player_color);
    // Cabeça (bloco menor no topo)
    DrawRectangle(x + 4, y + 2, CELL_SIZE - 8, CELL_SIZE - 12, player_color);
    // Olhos (dois pequenos quadrados pretos)
    DrawRectangle(x + 6, y + 4, 3, 3, BLACK);
    DrawRectangle(x + 15, y + 4, 3, 3, BLACK);
    // Crista (pequeno retângulo vermelho no topo)
    DrawRectangle(x + 8, y, CELL_SIZE - 16, 4, RED);
}

/**
 * Função de compatibilidade: mantém função original usando cor padrão
 * Usada no modo 1 jogador para manter compatibilidade
 */
static void draw_player_voxel_old(int x, int y) {
    draw_player_voxel(x, y, COLOR_PLAYER);
}

static void draw_car_voxel(int x, int y, Color color) {
    DrawRectangle(x + 1, y + 12, CELL_SIZE - 2, CELL_SIZE - 8, color);
    DrawRectangle(x + 3, y + 6, CELL_SIZE - 6, CELL_SIZE - 12, WHITE);
    DrawRectangle(x + 2, y + 14, 4, 3, YELLOW);
    DrawRectangle(x + CELL_SIZE - 6, y + 14, 4, 3, YELLOW);
    DrawRectangle(x + 4, y + 8, CELL_SIZE - 8, 4, (Color){200, 200, 255, 255});
}

static void draw_log_voxel(int x, int y) {
    DrawRectangle(x + 2, y + 8, CELL_SIZE - 4, CELL_SIZE - 8, COLOR_LOG);
    DrawRectangle(x + 4, y + 10, CELL_SIZE - 8, 2, (Color){101, 67, 33, 255});
    DrawRectangle(x + 4, y + 14, CELL_SIZE - 8, 2, (Color){101, 67, 33, 255});
    DrawRectangle(x + 4, y + 18, CELL_SIZE - 8, 2, (Color){101, 67, 33, 255});
}

// ----- Render de linhas e jogo -----
static void render_row(const Row *row, int y, int player_x, int player_y) {
    int start_x = MARGIN;
    int start_y = MARGIN + y * CELL_SIZE;

    Color bg_color;
    switch (row->type) {
        case ROW_GRASS: bg_color = COLOR_GRASS; break;
        case ROW_ROAD:  bg_color = COLOR_ROAD;  break;
        case ROW_RIVER: bg_color = COLOR_RIVER; break;
    }

    DrawRectangle(start_x, start_y, MAP_WIDTH * CELL_SIZE, CELL_SIZE, bg_color);

    if (row->type == ROW_ROAD) {
        for (int x = 0; x < MAP_WIDTH * CELL_SIZE; x += CELL_SIZE * 2) {
            DrawRectangle(start_x + x, start_y + CELL_SIZE/2 - 1, CELL_SIZE, 2, WHITE);
        }
    }

    if (row->queue) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            char cell = queue_get_cell(row->queue, x);
            int cell_x = start_x + x * CELL_SIZE;

            if (cell == CHAR_CAR)      draw_car_voxel(cell_x, start_y, COLOR_CAR);
            else if (cell == CHAR_LOG) draw_log_voxel(cell_x, start_y);
        }
    }

    if (player_y == y) {
        int player_x_pos = start_x + player_x * CELL_SIZE;
        draw_player_voxel_old(player_x_pos, start_y);
    }
}

static void render_game(const GameState *state) {
    ClearBackground(BLACK);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        render_row(&state->rows[y], y, state->player_x, state->player_y);
    }

    DrawRectangleLines(MARGIN, MARGIN, MAP_WIDTH * CELL_SIZE, MAP_HEIGHT * CELL_SIZE, WHITE);

    DrawText(TextFormat("Score: %d", state->score), MARGIN, 10, 20, WHITE);
    DrawText(TextFormat("Pos: (%d,%d)", state->player_x, state->player_y), MARGIN, 55, 14, YELLOW);

    if (state->player_y >= 0 && state->player_y < MAP_HEIGHT) {
        const Row *row = &state->rows[state->player_y];
        const char* row_type = (row->type == ROW_GRASS) ? "GRASS" :
                               (row->type == ROW_ROAD)  ? "ROAD"  : "RIVER";
        DrawText(TextFormat("Row Type: %s", row_type), MARGIN, 75, 14, GREEN);

        if (row->type == ROW_RIVER && row->queue) {
            char cell = queue_get_cell(row->queue, state->player_x);
            if (cell == CHAR_LOG) DrawText("ON LOG - SAFE!", MARGIN, 95, 16, GREEN);
            else                  DrawText("IN WATER - DANGER!", MARGIN, 95, 16, RED);
        }
    }
}

// === 2 PLAYER MODE ===
/**
 * Renderiza uma linha do mapa no modo 2 jogadores
 * Similar a render_row, mas desenha ambos os jogadores (P1 e P2) se estiverem nesta linha
 * @param row Ponteiro para a linha do mapa a renderizar
 * @param y Índice Y da linha (0 a MAP_HEIGHT-1)
 * @param p1_x, p1_y Posição do Jogador 1
 * @param p2_x, p2_y Posição do Jogador 2
 * @param p1_alive Flag: 1 se P1 está vivo, 0 se morto
 * @param p2_alive Flag: 1 se P2 está vivo, 0 se morto
 */
static void render_row_two(const Row *row, int y, int p1_x, int p1_y, int p2_x, int p2_y, int p1_alive, int p2_alive) {
    int start_x = MARGIN;
    int start_y = MARGIN + y * CELL_SIZE;

    // Define cor de fundo baseada no tipo de linha
    Color bg_color;
    switch (row->type) {
        case ROW_GRASS: bg_color = COLOR_GRASS; break;
        case ROW_ROAD:  bg_color = COLOR_ROAD;  break;
        case ROW_RIVER: bg_color = COLOR_RIVER; break;
    }

    // Desenha fundo da linha
    DrawRectangle(start_x, start_y, MAP_WIDTH * CELL_SIZE, CELL_SIZE, bg_color);

    // Desenha linhas da estrada (faixas brancas)
    if (row->type == ROW_ROAD) {
        for (int x = 0; x < MAP_WIDTH * CELL_SIZE; x += CELL_SIZE * 2) {
            DrawRectangle(start_x + x, start_y + CELL_SIZE/2 - 1, CELL_SIZE, 2, WHITE);
        }
    }

    // Desenha obstáculos (carros e troncos)
    if (row->queue) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            char cell = queue_get_cell(row->queue, x);
            int cell_x = start_x + x * CELL_SIZE;

            if (cell == CHAR_CAR)      draw_car_voxel(cell_x, start_y, COLOR_CAR);
            else if (cell == CHAR_LOG) draw_log_voxel(cell_x, start_y);
        }
    }

    // Desenha P1 se estiver nesta linha e vivo (cor amarela)
    if (p1_y == y && p1_alive) {
        int p1_x_pos = start_x + p1_x * CELL_SIZE;
        draw_player_voxel(p1_x_pos, start_y, COLOR_PLAYER1);
    }
    
    // Desenha P2 se estiver nesta linha e vivo (cor ciano)
    if (p2_y == y && p2_alive) {
        int p2_x_pos = start_x + p2_x * CELL_SIZE;
        draw_player_voxel(p2_x_pos, start_y, COLOR_PLAYER2);
    }
}

/**
 * Renderiza o jogo completo no modo 2 jogadores
 * Mostra ambos os jogadores, suas pontuações individuais e status (vivo/morto)
 * === 2 PLAYER MODE ===
 */
static void render_game_two(const GameState *state) {
    ClearBackground(BLACK);

    // Obtém posições e estados de ambos os jogadores
    int p1_x, p1_y, p2_x, p2_y;
    game_get_player_pos(state, 1, &p1_x, &p1_y);
    game_get_player_pos(state, 2, &p2_x, &p2_y);
    int p1_alive = game_is_player_alive(state, 1);
    int p2_alive = game_is_player_alive(state, 2);
    int p1_score = game_get_player_score(state, 1);
    int p2_score = game_get_player_score(state, 2);

    // Renderiza todas as linhas do mapa (incluindo ambos os jogadores)
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        render_row_two(&state->rows[y], y, p1_x, p1_y, p2_x, p2_y, p1_alive, p2_alive);
    }

    // Desenha bordas do mapa
    DrawRectangleLines(MARGIN, MARGIN, MAP_WIDTH * CELL_SIZE, MAP_HEIGHT * CELL_SIZE, WHITE);

    // === HUD: Informações dos jogadores ===
    // Pontuação do Jogador 1 (amarelo)
    DrawText(TextFormat("P1 Score: %d", p1_score), MARGIN, 10, 20, COLOR_PLAYER1);
    // Pontuação do Jogador 2 (ciano)
    DrawText(TextFormat("P2 Score: %d", p2_score), MARGIN, 35, 20, COLOR_PLAYER2);
    
    // Status de vida do Jogador 1
    if (p1_alive) {
        DrawText("P1: VIVO", MARGIN, 60, 16, GREEN);
    } else {
        DrawText("P1: MORTO", MARGIN, 60, 16, RED);
    }
    
    // Status de vida do Jogador 2
    if (p2_alive) {
        DrawText("P2: VIVO", MARGIN, 80, 16, GREEN);
    } else {
        DrawText("P2: MORTO", MARGIN, 80, 16, RED);
    }
}

// ----- Telas -----
static void render_menu_screen(int menu_index, const char** options, int count)
{
    ClearBackground(BLACK);

    DrawText("Nova(Velha) InfancIA", SCREEN_WIDTH/2 - 220, 120, 40, YELLOW);
    DrawText("Crossy Road",           SCREEN_WIDTH/2 - 100, 170, 30, WHITE);
    DrawText("Use UP/DOWN para navegar, ENTER para selecionar",
             SCREEN_WIDTH/2 - 280, 220, 18, GRAY);

    int base_y = 270;
    for (int i = 0; i < count; ++i) {
        Color c = (i == menu_index) ? YELLOW : WHITE;
        if (i == menu_index) DrawText(">", SCREEN_WIDTH/2 - 140, base_y + i*40, 26, c);
        DrawText(options[i],  SCREEN_WIDTH/2 - 110, base_y + i*40, 26, c);
    }
}

/**
 * Renderiza a tela de Game Over
 * === 2 PLAYER MODE === Adaptada para mostrar resultados de ambos os jogadores
 * @param state Estado do jogo
 * @param player_name Nome do Jogador 1 (ou único jogador no modo 1P)
 * @param player2_name Nome do Jogador 2 (NULL no modo 1P)
 * @param two_players Flag: 1 = modo 2 jogadores, 0 = modo 1 jogador
 */
static void render_game_over_screen(const GameState *state, const char *player_name, const char *player2_name, int two_players) {
    ClearBackground(BLACK);
    DrawText("GAME OVER!", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 150, 40, RED);
    
    if (two_players) {
        // === 2 PLAYER MODE === Mostra resultados de ambos os jogadores
        int p1_score = game_get_player_score(state, 1);
        int p2_score = game_get_player_score(state, 2);
        
        // Mostra pontuação do Jogador 1 (amarelo)
        DrawText(TextFormat("P1 (%s): %d", player_name ? player_name : "Player1", p1_score), 
                 SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 90, 22, COLOR_PLAYER1);
        // Mostra pontuação do Jogador 2 (ciano)
        DrawText(TextFormat("P2 (%s): %d", player2_name ? player2_name : "Player2", p2_score), 
                 SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 60, 22, COLOR_PLAYER2);
        
        // Verifica se houve empate ou vencedor
        if (p1_score == p2_score) {
            // Empate: ambos têm a mesma pontuação
            DrawText(TextFormat("EMPATE! Ambos com %d pontos!", p1_score), 
                     SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 20, 24, YELLOW);
        } else {
            // Determina vencedor (maior pontuação)
            int winner_score = (p1_score > p2_score) ? p1_score : p2_score;
            const char *winner_name = (p1_score > p2_score) ? player_name : player2_name;
            // Mostra vencedor em destaque (amarelo)
            DrawText(TextFormat("Vencedor: %s com %d pontos!", winner_name, winner_score), 
                     SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 20, 24, YELLOW);
        }
    } else {
        // Modo 1 jogador: mostra pontuação final normal
        DrawText(TextFormat("Final Score: %d", state->score), SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 24, WHITE);
        DrawText(TextFormat("Player: %s", player_name), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 10, 20, GREEN);
    }
    
    // Instruções para o jogador
    DrawText("Press R to play again", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, WHITE);
    DrawText("Press M to return to menu", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 80, 20, WHITE);
}

/**
 * Renderiza a tela de input de nome
 * === 2 PLAYER MODE === Adaptada para aceitar título customizado (P1 ou P2)
 * @param buffer Buffer com o texto digitado
 * @param letterCount Número de letras digitadas
 * @param title Título da tela (ex: "Digite o nome do Jogador 1:" ou "Digite o nome do Jogador 2:")
 */
static void render_name_input_screen(const char *buffer, int letterCount, const char *title) {
    ClearBackground((Color){30, 30, 30, 255});
    
    // Título da tela (customizável para P1 ou P2)
    DrawText(title ? title : "Digite seu nome:", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 80, 30, WHITE);
    
    // Caixa de texto (fundo escuro)
    DrawRectangle(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 30, 400, 50, (Color){60, 60, 60, 255});
    DrawRectangleLines(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 30, 400, 50, WHITE);
    
    // Texto digitado
    DrawText(buffer, SCREEN_WIDTH/2 - 190, SCREEN_HEIGHT/2 - 15, 24, WHITE);

    // Cursor piscante (mostra posição de digitação)
    if (((int)(GetTime() * 2)) % 2 == 0) {
        int textWidth = MeasureText(buffer, 24);
        DrawText("_", SCREEN_WIDTH/2 - 190 + textWidth, SCREEN_HEIGHT/2 - 15, 24, WHITE);
    }
    
    // Instruções
    DrawText("Pressione ENTER para continuar", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 + 50, 20, GRAY);
    if (letterCount == 0) {
        DrawText("(ou ESC para cancelar)", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 80, 18, DARKGRAY);
    }
}

// ----- Loop principal com RenderTexture (resolução virtual) -----
void raylib_run_game(Ranking *ranking) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Nova (Velha) Infancia - Crossy Road");
    SetTargetFPS(60);

    // Alvo de renderização virtual 800x600
    const int VIRTUAL_W = SCREEN_WIDTH, VIRTUAL_H = SCREEN_HEIGHT;
    RenderTexture2D target = LoadRenderTexture(VIRTUAL_W, VIRTUAL_H);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    GameState state;
    GameScreen current_screen = GAME_START_SCREEN;
    char player_name[MAX_NAME_LEN] = {0};
    char player2_name[MAX_NAME_LEN] = {0};  // === 2 PLAYER MODE ===
    int two_players_mode = 0;  // === 2 PLAYER MODE ===

    int name_input_letterCount = 0;
    char name_input_buffer[MAX_NAME_LEN] = {0};

    ranking_load(ranking, RANKING_FILE);

    int menu_index = 0;
    const char* MENU_OPTS[] = { "Aprender a jogar", "Jogar (1 Jogador)", "Jogar (2 Jogadores)", "Ver ranking", "Voltar" };
    const int MENU_COUNT = 5;

    int exit_requested = 0;

    while (!WindowShouldClose()) {
        // Alternar fullscreen
        if (IsKeyPressed(KEY_F11) ||
            (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            ToggleFullscreen();
            int mon = GetCurrentMonitor();
            SetWindowSize(GetMonitorWidth(mon), GetMonitorHeight(mon));
        }

        // ----- UPDATE -----
        switch (current_screen) {
            case GAME_START_SCREEN: {
                if (IsKeyPressed(KEY_DOWN)) menu_index = (menu_index + 1) % MENU_COUNT;
                if (IsKeyPressed(KEY_UP))   menu_index = (menu_index - 1 + MENU_COUNT) % MENU_COUNT;
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    if (menu_index == 0)      current_screen = GAME_HELP_SCREEN;
                    else if (menu_index == 1) { // Jogar (1 Jogador)
                        two_players_mode = 0;
                        name_input_letterCount = 0; name_input_buffer[0] = '\0';
                        current_screen = GAME_NAME_INPUT_SCREEN;
                    } else if (menu_index == 2) { // === 2 PLAYER MODE === Jogar (2 Jogadores)
                        two_players_mode = 1;
                        name_input_letterCount = 0; name_input_buffer[0] = '\0';
                        player2_name[0] = '\0';
                        current_screen = GAME_NAME_INPUT_SCREEN;
                    } else if (menu_index == 3) current_screen = GAME_RANKING_SCREEN;
                    else if (menu_index == 4) exit_requested = 1;
                }
            } break;

            case GAME_NAME_INPUT_SCREEN: {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (name_input_letterCount < MAX_NAME_LEN - 1)) {
                        name_input_buffer[name_input_letterCount++] = (char)key;
                        name_input_buffer[name_input_letterCount] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (name_input_letterCount > 0) name_input_letterCount--;
                    name_input_buffer[name_input_letterCount] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER) && name_input_letterCount > 0) {
                    strncpy(player_name, name_input_buffer, MAX_NAME_LEN - 1);
                    player_name[MAX_NAME_LEN - 1] = '\0';
                    if (two_players_mode) {
                        // === 2 PLAYER MODE === Pede nome do P2
                        name_input_letterCount = 0; name_input_buffer[0] = '\0';
                        current_screen = GAME_NAME_INPUT_SCREEN_P2;
                    } else {
                        game_init(&state, MAP_WIDTH);
                        game_set_two_players(&state, 0);
                        current_screen = GAME_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_ESCAPE)) current_screen = GAME_START_SCREEN;
            } break;

            // === 2 PLAYER MODE ===
            case GAME_NAME_INPUT_SCREEN_P2: {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (name_input_letterCount < MAX_NAME_LEN - 1)) {
                        name_input_buffer[name_input_letterCount++] = (char)key;
                        name_input_buffer[name_input_letterCount] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (name_input_letterCount > 0) name_input_letterCount--;
                    name_input_buffer[name_input_letterCount] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER) && name_input_letterCount > 0) {
                    strncpy(player2_name, name_input_buffer, MAX_NAME_LEN - 1);
                    player2_name[MAX_NAME_LEN - 1] = '\0';
                    game_init(&state, MAP_WIDTH);
                    game_set_two_players(&state, 1);
                    current_screen = GAME_PLAYING;
                }
                if (IsKeyPressed(KEY_ESCAPE)) current_screen = GAME_START_SCREEN;
            } break;

            case GAME_PLAYING: {
                if (two_players_mode) {
                    // === 2 PLAYER MODE ===
                    // P1: WASD
                    if (IsKeyPressed(KEY_W)) game_handle_input_player(&state, 1, 'W');
                    if (IsKeyPressed(KEY_S)) game_handle_input_player(&state, 1, 'S');
                    if (IsKeyPressed(KEY_A)) game_handle_input_player(&state, 1, 'A');
                    if (IsKeyPressed(KEY_D)) game_handle_input_player(&state, 1, 'D');
                    
                    // P2: Setas
                    if (IsKeyPressed(KEY_UP))    game_handle_input_player(&state, 2, 'W');
                    if (IsKeyPressed(KEY_DOWN))  game_handle_input_player(&state, 2, 'S');
                    if (IsKeyPressed(KEY_LEFT))  game_handle_input_player(&state, 2, 'A');
                    if (IsKeyPressed(KEY_RIGHT)) game_handle_input_player(&state, 2, 'D');
                } else {
                    // Modo 1 jogador
                    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    game_handle_input(&state, 'W');
                    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  game_handle_input(&state, 'S');
                    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  game_handle_input(&state, 'A');
                    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) game_handle_input(&state, 'D');
                }

                game_update(&state);

                if (state.game_over) {
                    if (two_players_mode) {
                        // === 2 PLAYER MODE === Salva apenas o melhor score
                        int p1_score = game_get_player_score(&state, 1);
                        int p2_score = game_get_player_score(&state, 2);
                        if (p1_score > p2_score) {
                            ranking_add(ranking, player_name, p1_score, RANKING_FILE);
                        } else {
                            ranking_add(ranking, player2_name, p2_score, RANKING_FILE);
                        }
                    } else {
                        ranking_add(ranking, player_name, state.score, RANKING_FILE);
                    }
                    current_screen = GAME_OVER_SCREEN;
                }
                if (IsKeyPressed(KEY_M)) current_screen = GAME_START_SCREEN;
            } break;

            case GAME_OVER_SCREEN: {
                if (IsKeyPressed(KEY_R)) {
                    name_input_letterCount = 0; name_input_buffer[0] = '\0';
                    if (two_players_mode) {
                        player2_name[0] = '\0';
                    }
                    current_screen = GAME_NAME_INPUT_SCREEN;
                }
                if (IsKeyPressed(KEY_M)) current_screen = GAME_START_SCREEN;
            } break;

            case GAME_HELP_SCREEN: {
                if (IsKeyPressed(KEY_M)) current_screen = GAME_START_SCREEN;
            } break;

            case GAME_RANKING_SCREEN: {
                if (IsKeyPressed(KEY_M)) current_screen = GAME_START_SCREEN;
            } break;
        }

        if (exit_requested) break;

        // ----- DRAW to virtual target (800x600) -----
        BeginTextureMode(target);
        {
            // desenha como antes, usando SCREEN_WIDTH/HEIGHT fixos
            switch (current_screen) {
                case GAME_START_SCREEN:
                    render_menu_screen(menu_index, MENU_OPTS, MENU_COUNT);
                    break;
                case GAME_NAME_INPUT_SCREEN:
                    render_name_input_screen(name_input_buffer, name_input_letterCount, 
                                             two_players_mode ? "Digite o nome do Jogador 1:" : "Digite seu nome:");
                    break;
                case GAME_NAME_INPUT_SCREEN_P2:
                    render_name_input_screen(name_input_buffer, name_input_letterCount, "Digite o nome do Jogador 2:");
                    break;
                case GAME_PLAYING:
                    if (two_players_mode) {
                        render_game_two(&state);
                    } else {
                        render_game(&state);
                    }
                    break;
                case GAME_OVER_SCREEN:
                    render_game_over_screen(&state, player_name, player2_name, two_players_mode);
                    break;
                case GAME_HELP_SCREEN:
                    render_help_screen();
                    break;
                case GAME_RANKING_SCREEN:
                    render_ranking_screen(ranking);
                    break;
            }
        }
        EndTextureMode();

        // ----- DRAW target scaled to screen (letterbox) -----
        BeginDrawing();
        ClearBackground(BLACK);

        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        float scale = (sw / VIRTUAL_W < sh / VIRTUAL_H) ? (sw / VIRTUAL_W) : (sh / VIRTUAL_H);
        float dw = VIRTUAL_W * scale;
        float dh = VIRTUAL_H * scale;
        float dx = (sw - dw) * 0.5f;
        float dy = (sh - dh) * 0.5f;

        DrawTexturePro(
            target.texture,
            (Rectangle){0, 0, (float)VIRTUAL_W, -(float)VIRTUAL_H},  // altura negativa = corrige flip
            (Rectangle){dx, dy, dw, dh},
            (Vector2){0, 0},
            0.0f,
            WHITE
        );

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
}

// ----- Telas auxiliares -----
static void render_help_screen(void)
{
    ClearBackground(BLACK);
    DrawText("Como jogar", SCREEN_WIDTH/2 - 100, 80, 32, YELLOW);

    int x = 120, y = 150, lh = 28;
    DrawText("Modo 1 Jogador:", x, y, 22, YELLOW); y += lh;
    DrawText("- W / S / A / D ou Setas: mover", x + 20, y, 22, WHITE); y += lh;
    DrawText("Modo 2 Jogadores:", x, y, 22, YELLOW); y += lh;
    DrawText("- Jogador 1 (Amarelo): W / A / S / D", x + 20, y, 22, COLOR_PLAYER1); y += lh;
    DrawText("- Jogador 2 (Ciano): Setas", x + 20, y, 22, COLOR_PLAYER2); y += lh + 10;
    DrawText("- Evite carros na estrada", x, y, 22, WHITE); y += lh;
    DrawText("- No rio, fique em cima dos troncos", x, y, 22, WHITE); y += lh;
    DrawText("- O mapa sobe automaticamente a cada intervalo", x, y, 22, WHITE); y += lh;
    DrawText("- Pontue ao alcançar linhas ainda não visitadas", x, y, 22, WHITE); y += lh + 10;
    DrawText("'M' para voltar ao menu", x, y, 22, GRAY);
}

static void render_ranking_screen(const Ranking *ranking)
{
    ClearBackground(BLACK);
    DrawText("Ranking", SCREEN_WIDTH/2 - 70, 80, 32, YELLOW);
    DrawText("'M' para voltar ao menu", SCREEN_WIDTH/2 - 120, 120, 20, GRAY);

    int x = SCREEN_WIDTH/2 - 200;
    int y = 170;
    int lh = 35;  // Aumentado o espaçamento entre linhas

    int maxShow = 10;
    int n = 0;

    // Cabeçalho das colunas
    DrawText("Pos", x, y, 22, LIGHTGRAY);
    DrawText("Nome", x + 80, y, 22, LIGHTGRAY);
    DrawText("Pontuação", x + 280, y, 22, LIGHTGRAY);
    y += lh + 5;  // Espaço extra após o cabeçalho
    
    // Linha separadora
    DrawLine(x - 10, y - 5, x + 380, y - 5, GRAY);

    for (int i = 0; i < ranking->count && n < maxShow; ++i) {
        const char* name = ranking->items[i].name;
        int score = ranking->items[i].score;

        // Posição (mais espaçada)
        DrawText(TextFormat("%2d.", i + 1), x, y, 24, WHITE);
        
        // Nome (com mais espaço)
        DrawText(name, x + 80, y, 24, WHITE);
        
        // Pontuação (alinhada à direita, mais espaçada)
        const char* scoreText = TextFormat("%d", score);
        int scoreWidth = MeasureText(scoreText, 24);
        DrawText(scoreText, x + 380 - scoreWidth, y, 24, YELLOW);
        
        y += lh;
        n++;
    }

    if (n == 0) {
        DrawText("Nenhum registro encontrado ainda.", SCREEN_WIDTH/2 - 180, y, 22, GRAY);
    }
}

