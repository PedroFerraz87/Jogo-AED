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

// Cores no estilo voxel/Crossy Road
#define COLOR_GRASS (Color){76, 175, 80, 255}      // Verde grama vibrante
#define COLOR_ROAD (Color){97, 97, 97, 255}        // Cinza estrada
#define COLOR_RIVER (Color){33, 150, 243, 255}     // Azul água vibrante
#define COLOR_CAR (Color){244, 67, 54, 255}        // Vermelho carro vibrante
#define COLOR_LOG (Color){121, 85, 72, 255}        // Marrom tronco
#define COLOR_PLAYER (Color){255, 193, 7, 255}     // Amarelo jogador
#define COLOR_TREE (Color){76, 175, 80, 255}       // Verde árvore
#define COLOR_TREE_TRUNK (Color){121, 85, 72, 255}  // Marrom tronco árvore

// Estados do jogo
typedef enum {
    GAME_START_SCREEN,   // usamos como "MENU"
    GAME_PLAYING,
    GAME_OVER_SCREEN,
    GAME_HELP_SCREEN,    // NOVO: "Aprender a jogar"
    GAME_RANKING_SCREEN  // NOVO: "Ver ranking"
} GameScreen;
// Funções para desenhar elementos no estilo voxel
static void draw_player_voxel(int x, int y) {
    // Corpo principal (bloco maior)
    DrawRectangle(x + 2, y + 8, CELL_SIZE - 4, CELL_SIZE - 8, COLOR_PLAYER);
    // Cabeça (bloco menor no topo)
    DrawRectangle(x + 4, y + 2, CELL_SIZE - 8, CELL_SIZE - 12, COLOR_PLAYER);
    // Olhos
    DrawRectangle(x + 6, y + 4, 3, 3, BLACK);
    DrawRectangle(x + 15, y + 4, 3, 3, BLACK);
    // Crista (pequeno retângulo vermelho)
    DrawRectangle(x + 8, y, CELL_SIZE - 16, 4, RED);
}

static void draw_car_voxel(int x, int y, Color color) {
    // Base do carro
    DrawRectangle(x + 1, y + 12, CELL_SIZE - 2, CELL_SIZE - 8, color);
    // Teto do carro
    DrawRectangle(x + 3, y + 6, CELL_SIZE - 6, CELL_SIZE - 12, WHITE);
    // Faróis
    DrawRectangle(x + 2, y + 14, 4, 3, YELLOW);
    DrawRectangle(x + CELL_SIZE - 6, y + 14, 4, 3, YELLOW);
    // Janelas
    DrawRectangle(x + 4, y + 8, CELL_SIZE - 8, 4, (Color){200, 200, 255, 255});
}

static void draw_log_voxel(int x, int y) {
    // Tronco principal
    DrawRectangle(x + 2, y + 8, CELL_SIZE - 4, CELL_SIZE - 8, COLOR_LOG);
    // Detalhes da madeira (linhas)
    DrawRectangle(x + 4, y + 10, CELL_SIZE - 8, 2, (Color){101, 67, 33, 255});
    DrawRectangle(x + 4, y + 14, CELL_SIZE - 8, 2, (Color){101, 67, 33, 255});
    DrawRectangle(x + 4, y + 18, CELL_SIZE - 8, 2, (Color){101, 67, 33, 255});
}

static void draw_tree_voxel(int x, int y) {
    // Tronco da árvore
    DrawRectangle(x + 8, y + 12, CELL_SIZE - 16, CELL_SIZE - 8, COLOR_TREE_TRUNK);
    // Copa da árvore (bloco maior)
    DrawRectangle(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 8, COLOR_TREE);
    // Detalhes da copa
    DrawRectangle(x + 4, y + 4, CELL_SIZE - 8, 4, (Color){56, 142, 60, 255});
    DrawRectangle(x + 4, y + 12, CELL_SIZE - 8, 4, (Color){56, 142, 60, 255});
}

// Renderiza uma linha do jogo
static void render_row(const Row *row, int y, int player_x, int player_y) {
    int start_x = MARGIN;
    int start_y = MARGIN + y * CELL_SIZE;
    
    // Cor de fundo da linha
    Color bg_color;
    switch (row->type) {
        case ROW_GRASS: bg_color = COLOR_GRASS; break;
        case ROW_ROAD: bg_color = COLOR_ROAD; break;
        case ROW_RIVER: bg_color = COLOR_RIVER; break;
    }
    
    // Desenha fundo da linha
    DrawRectangle(start_x, start_y, MAP_WIDTH * CELL_SIZE, CELL_SIZE, bg_color);
    
    // Desenha linhas da estrada se for estrada
    if (row->type == ROW_ROAD) {
        for (int x = 0; x < MAP_WIDTH * CELL_SIZE; x += CELL_SIZE * 2) {
            DrawRectangle(start_x + x, start_y + CELL_SIZE/2 - 1, CELL_SIZE, 2, WHITE);
        }
    }
    
    // Desenha obstáculos se houver
    if (row->queue) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            char cell = queue_get_cell(row->queue, x);
            int cell_x = start_x + x * CELL_SIZE;
            
            if (cell == CHAR_CAR) {
                draw_car_voxel(cell_x, start_y, COLOR_CAR);
            } else if (cell == CHAR_LOG) {
                draw_log_voxel(cell_x, start_y);
            }
        }
    }
    
    if (player_y == y) {
        int player_x_pos = start_x + player_x * CELL_SIZE;
        draw_player_voxel(player_x_pos, start_y);
    }
}

// Renderiza o jogo principal
static void render_game(const GameState *state) {
    ClearBackground(BLACK);
    
    // Desenha todas as linhas
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        render_row(&state->rows[y], y, state->player_x, state->player_y);
    }
    
    // Bordas
    DrawRectangleLines(MARGIN, MARGIN, MAP_WIDTH * CELL_SIZE, MAP_HEIGHT * CELL_SIZE, WHITE);
    
    // Informações do jogo
    DrawText(TextFormat("Score: %d", state->score), MARGIN, 10, 20, WHITE);
    DrawText(TextFormat("World: %d", state->world_position), MARGIN, 35, 16, YELLOW);
    DrawText(TextFormat("Pos: (%d,%d)", state->player_x, state->player_y), MARGIN, 55, 14, YELLOW);
    
    // Debug: tipo da linha atual
    if (state->player_y >= 0 && state->player_y < MAP_HEIGHT) {
        const Row *row = &state->rows[state->player_y];
        const char* row_type = (row->type == ROW_GRASS) ? "GRASS" : 
                              (row->type == ROW_ROAD) ? "ROAD" : "RIVER";
        DrawText(TextFormat("Row Type: %s", row_type), MARGIN, 75, 14, GREEN);
        
        // Debug: se está em tronco
        if (row->type == ROW_RIVER && row->queue) {
            char cell = queue_get_cell(row->queue, state->player_x);
            if (cell == CHAR_LOG) {
                DrawText("ON LOG - SAFE!", MARGIN, 95, 16, GREEN);
            } else {
                DrawText("IN WATER - DANGER!", MARGIN, 95, 16, RED);
            }
        }
    }
}

// Renderiza o menu principal com as opções e destaque na selecionada
static void render_menu_screen(int menu_index, const char** options, int count)
{
    ClearBackground(BLACK);

    DrawText("Nova(Velha) InfancIA", SCREEN_WIDTH/2 - 220, 120, 40, YELLOW);
    DrawText("Crossy Road",           SCREEN_WIDTH/2 - 100, 170, 30, WHITE);
    DrawText("Use UP/DOWN para navegar, ENTER para selecionar",
             SCREEN_WIDTH/2 - 280, 220, 18, GRAY);

    int base_y = 270;
    for (int i = 0; i < count; ++i) {
        Color c = (i == menu_index) ? YELLOW : WHITE; // se selecionado, pinta de amarelo
        if (i == menu_index)
            DrawText(">", SCREEN_WIDTH/2 - 140, base_y + i*40, 26, c); // seta indicadora
        DrawText(options[i],  SCREEN_WIDTH/2 - 110, base_y + i*40, 26, c);
    }
}

// Renderiza tela de game over
static void render_game_over_screen(const GameState *state, const char *player_name) {
    ClearBackground(BLACK);
    
    // Game Over
    DrawText("GAME OVER!", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 100, 40, RED);
    
    // Score
    DrawText(TextFormat("Final Score: %d", state->score), SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 24, WHITE);
    // DrawText(TextFormat("World Reached: %d", state->world_position), SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 20, 20, YELLOW);
    
    // Player name
    DrawText(TextFormat("Player: %s", player_name), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 10, 20, GREEN);
    
    // Opções
    DrawText("Press R to play again", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, WHITE);
    DrawText("Press M to return to menu", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 80, 20, WHITE);
    DrawText("Press Q to quit", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 110, 20, GRAY);
}

// Função principal do jogo Raylib
void raylib_run_game() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Nova (Velha) Infancia - Crossy Road");
    SetTargetFPS(60);

    GameState state;
    GameScreen current_screen = GAME_START_SCREEN; // tela de MENU
    char player_name[50] = "Player";

    // Ranking
    Ranking ranking;
    ranking_load(&ranking, "ranking.txt");

    // Estado do menu
    int menu_index = 0; // 0..3
    const char* MENU_OPTS[] = {
        "Aprender a jogar",
        "Jogar",
        "Ver ranking",
        "Voltar"
    };
    const int MENU_COUNT = 4;

    int exit_requested = 0;

    while (!WindowShouldClose()) {

        // --- INPUT / LÓGICA POR TELA ---
        switch (current_screen) {
            case GAME_START_SCREEN: { // MENU
                if (IsKeyPressed(KEY_DOWN)) {
                    menu_index = (menu_index + 1) % MENU_COUNT;
                }
                if (IsKeyPressed(KEY_UP)) {
                    menu_index = (menu_index - 1 + MENU_COUNT) % MENU_COUNT;
                }
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    if (menu_index == 0) {           // Aprender a jogar
                        current_screen = GAME_HELP_SCREEN;
                    } else if (menu_index == 1) {    // Jogar
                        game_init(&state, MAP_WIDTH);
                        current_screen = GAME_PLAYING;
                    } else if (menu_index == 2) {    // Ver ranking
                        current_screen = GAME_RANKING_SCREEN;
                    } else if (menu_index == 3) {    // Voltar (sair)
                        exit_requested = 1;
                    }
                }
            } break;

            case GAME_PLAYING: {
                // Input do jogador
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    game_handle_input(&state, 'W');
                if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  game_handle_input(&state, 'S');
                if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  game_handle_input(&state, 'A');
                if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) game_handle_input(&state, 'D');

                // Atualiza jogo
                game_update(&state);

                // Game over?
                if (state.game_over) {
                    ranking_add_and_sort(&ranking, player_name, state.score);
                    ranking_save(&ranking, "ranking.txt");
                    current_screen = GAME_OVER_SCREEN;
                }

                // ESC durante o jogo: volta ao MENU
                if (IsKeyPressed(KEY_ESCAPE)) {
                    current_screen = GAME_START_SCREEN;
                }
            } break;

            case GAME_OVER_SCREEN: {
                // R: jogar novamente
                if (IsKeyPressed(KEY_R)) {
                    game_init(&state, MAP_WIDTH);
                    current_screen = GAME_PLAYING;
                }
                // M ou ESC: volta ao MENU
                if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
                    current_screen = GAME_START_SCREEN;
                }
            } break;

            case GAME_HELP_SCREEN: {
                // Apenas instruções; ESC volta ao MENU
                if (IsKeyPressed(KEY_ESCAPE)) {
                    current_screen = GAME_START_SCREEN;
                }
            } break;

            case GAME_RANKING_SCREEN: {
                // Apenas visualização do ranking; ESC volta ao MENU
                if (IsKeyPressed(KEY_ESCAPE)) {
                    current_screen = GAME_START_SCREEN;
                }
            } break;
        }

        if (exit_requested) break;

        // --- RENDER ---
        BeginDrawing();
        ClearBackground(BLACK);

        switch (current_screen) {
            case GAME_START_SCREEN:
                render_menu_screen(menu_index, MENU_OPTS, MENU_COUNT);
                break;
            case GAME_PLAYING:
                render_game(&state);
                break;
            case GAME_OVER_SCREEN:
                render_game_over_screen(&state, player_name);
                break;
            case GAME_HELP_SCREEN:
                render_help_screen();
                break;
            case GAME_RANKING_SCREEN:
                render_ranking_screen(&ranking);
                break;
        }

        // opcional
        DrawFPS(SCREEN_WIDTH - 90, 10);

        EndDrawing();
    }

    CloseWindow();
}


static void render_help_screen(void)
{
    ClearBackground(BLACK);
    DrawText("Como jogar", SCREEN_WIDTH/2 - 100, 80, 32, YELLOW);

    int x = 120, y = 150, lh = 28;
    DrawText("- W / S / A / D ou Setas: mover o personagem", x, y, 22, WHITE); y += lh;
    DrawText("- Evite carros na estrada", x, y, 22, WHITE); y += lh;
    DrawText("- No rio, fique em cima dos troncos", x, y, 22, WHITE); y += lh;
    DrawText("- O mapa sobe automaticamente a cada intervalo", x, y, 22, WHITE); y += lh;
    DrawText("- Pontue ao alcançar linhas ainda não visitadas", x, y, 22, WHITE); y += lh + 10;

    DrawText("ESC para voltar ao menu", x, y, 22, GRAY);
}

static void render_ranking_screen(const Ranking *ranking)
{
    ClearBackground(BLACK);
    DrawText("Ranking", SCREEN_WIDTH/2 - 70, 80, 32, YELLOW);
    DrawText("ESC para voltar ao menu", SCREEN_WIDTH/2 - 120, 120, 20, GRAY);

    int x = SCREEN_WIDTH/2 - 220;
    int y = 170;
    int lh = 28;

    // ===== ajuste aqui conforme sua estrutura =====
    // Exemplo suposto: ranking.count e ranking.items[i].name / .score
    // Mostra top 10
    int maxShow = 10;
    int n = 0;
    // Se o seu tipo não tiver esses campos, troque aqui.
    for (int i = 0; i < /*ranking->count*/ 0 /*<-- troque para ranking->count*/ && n < maxShow; ++i) {
        // Exemplo:
        // const char* name = ranking->items[i].name;
        // int score = ranking->items[i].score;

        // Placeholders (remova após ajustar):
        const char* name = "Player";
        int score = 0;

        DrawText(TextFormat("%2d) %-16s  %6d", i+1, name, score), x, y, 24, WHITE);
        y += lh;
        n++;
    }

    if (n == 0) {
        DrawText("Nenhum registro encontrado ainda.", x, y, 22, GRAY);
    }
}