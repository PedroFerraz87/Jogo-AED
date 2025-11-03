#include "raylib.h" 
#include "game.h"
#include "ranking.h"
#include "utils.h"
#include <string.h>

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
    GAME_START_SCREEN,
    GAME_PLAYING,
    GAME_OVER_SCREEN
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
    
    // Desenha árvores na grama (aleatoriamente)
   // if (row->type == ROW_GRASS) {
    //    for (int x = 0; x < MAP_WIDTH; x += 4) {
      //      if (utils_random_int(0, 3) == 0) { // 25% chance de árvore
      //          int tree_x = start_x + x * CELL_SIZE;
      //          draw_tree_voxel(tree_x, start_y);
      //      }
      //  }
      //}
    
    // Desenha jogador se estiver nesta linha
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
    
    
    
    // Controles
    DrawText("WASD or Arrow Keys to move", MARGIN, SCREEN_HEIGHT - 60, 16, WHITE);
    DrawText("Q to quit", MARGIN, SCREEN_HEIGHT - 40, 16, WHITE);
}

// Renderiza tela de início
static void render_start_screen() {
    ClearBackground(BLACK);
    
    // Título
    DrawText("Nova(Velha) InfanCIA", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 100, 40, YELLOW);
    DrawText("Crossy Road", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 30, WHITE);
    
    // Instruções
    DrawText("Use WASD ou as setas para mover", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2, 20, WHITE);
    DrawText("Chegue ao topo para fazer o mundo rolar!", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 + 30, 20, GREEN);
    DrawText("Pressione ESPAÇO para iniciar", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 80, 24, YELLOW);
    DrawText("Pressione Q para sair", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 110, 20, GRAY);
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
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Nova(Velha) Infancia - Crossy Road");
    SetTargetFPS(60);
    
    GameState state;
    GameScreen current_screen = GAME_START_SCREEN;
    char player_name[50] = "Player";
    
    // Inicializa ranking
    Ranking ranking;
    ranking_load(&ranking, "ranking.txt");
    
    while (!WindowShouldClose()) {
        // Input handling
        if (IsKeyPressed(KEY_Q)) {
            break;
        }
        
        switch (current_screen) {
            case GAME_START_SCREEN:
                if (IsKeyPressed(KEY_SPACE)) {
                    game_init(&state, MAP_WIDTH);
                    current_screen = GAME_PLAYING;
                }
                break;
                
            case GAME_PLAYING:
                // Input do jogador
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
                    game_handle_input(&state, 'W');
                }
                if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
                    game_handle_input(&state, 'S');
                }
                if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
                    game_handle_input(&state, 'A');
                }
                if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
                    game_handle_input(&state, 'D');
                }
                
                // Atualiza jogo
                game_update(&state);
                
                // Verifica game over
                if (state.game_over) {
                    // Salva score
                    ranking_add_and_sort(&ranking, player_name, state.score);
                    ranking_save(&ranking, "ranking.txt");
                    current_screen = GAME_OVER_SCREEN;
                }
                break;
                
            case GAME_OVER_SCREEN:
                if (IsKeyPressed(KEY_R)) {
                    game_init(&state, MAP_WIDTH);
                    current_screen = GAME_PLAYING;
                }
                if (IsKeyPressed(KEY_M)) {
                    current_screen = GAME_START_SCREEN;
                }
                break;
        }
        
        // Renderização
        BeginDrawing();
        
        switch (current_screen) {
            case GAME_START_SCREEN:
                render_start_screen();
                break;
            case GAME_PLAYING:
                render_game(&state);
                break;
            case GAME_OVER_SCREEN:
                render_game_over_screen(&state, player_name);
                break;
        }
        
        EndDrawing();
    }
    
    CloseWindow();
}