#include "raylib_view.h"
#include "game.h"
#include "utils.h"
#include <string.h>

#ifdef ENABLE_RAYLIB
#include <raylib.h>

static Color color_for_row(RowType type) {
    if (type == ROW_GRASS) return (Color){ 90, 160, 60, 255 };
    if (type == ROW_ROAD)  return (Color){ 60, 60, 60, 255 };
    return (Color){ 40, 110, 180, 255 }; // rio
}

void raylib_run_game(Ranking *ranking) {
    const int cell = 24;
    const int margin = 20;
    const int W = MAP_WIDTH * cell + margin * 2;
    const int H = MAP_HEIGHT * cell + margin * 2 + 40;
    InitWindow(W, H, "Nova(Velha) InfanCIA - Crossy");
    SetTargetFPS(60);

    GameState state;
    game_init(&state, MAP_WIDTH);
    
    // Estados do jogo
    bool game_started = false;
    bool game_over_screen = false;
    char player_name[64] = "Player";
    int name_cursor = 0;
    int game_over_option = 0; // 0 = nome, 1 = jogar novamente, 2 = voltar menu

    while (!WindowShouldClose()) {
        // Tela de início
        if (!game_started && !game_over_screen) {
            BeginDrawing();
            ClearBackground((Color){ 30, 30, 30, 255 });
            DrawText("Nova(Velha) InfanCIA", W/2 - 120, H/2 - 60, 24, RAYWHITE);
            DrawText("Crossy Road - Modo Infinito", W/2 - 140, H/2 - 30, 18, YELLOW);
            DrawText("Use WASD ou setas para mover", W/2 - 120, H/2, 16, RAYWHITE);
            DrawText("Evite carros e fique nos troncos", W/2 - 130, H/2 + 20, 16, RAYWHITE);
            DrawText("Pressione ESPACO para começar", W/2 - 130, H/2 + 50, 16, GREEN);
            DrawText("Q para sair", W/2 - 50, H/2 + 70, 14, GRAY);
            EndDrawing();
            
            if (IsKeyPressed(KEY_SPACE)) {
                game_started = true;
            }
            if (IsKeyPressed(KEY_Q)) {
                CloseWindow();
                return;
            }
            continue;
        }

        // Tela de Game Over
        if (game_over_screen) {
            BeginDrawing();
            ClearBackground((Color){ 30, 30, 30, 255 });
            DrawRectangle(0, 0, W, H, (Color){0,0,0,180});
            DrawText("GAME OVER", W/2 - 120, H/2 - 80, 32, RED);
            DrawText(TextFormat("Score: %d", state.score), W/2 - 60, H/2 - 40, 20, RAYWHITE);
            
            // Input do nome
            if (game_over_option == 0) {
                DrawText("Digite seu nome:", W/2 - 80, H/2 - 10, 16, YELLOW);
                DrawText(player_name, W/2 - 60, H/2 + 10, 18, RAYWHITE);
                DrawText("_", W/2 - 60 + MeasureText(player_name, 18), H/2 + 10, 18, YELLOW);
                
                // Input de texto
                int key = GetCharPressed();
                if (key >= 32 && key <= 126 && name_cursor < 63) {
                    player_name[name_cursor] = (char)key;
                    player_name[name_cursor + 1] = '\0';
                    name_cursor++;
                }
                if (IsKeyPressed(KEY_BACKSPACE) && name_cursor > 0) {
                    name_cursor--;
                    player_name[name_cursor] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    game_over_option = 1;
                }
            }
            
            // Opções do menu
            if (game_over_option >= 1) {
                DrawText("1) Jogar Novamente", W/2 - 100, H/2 + 20, 16, game_over_option == 1 ? GREEN : RAYWHITE);
                DrawText("2) Voltar ao Menu", W/2 - 80, H/2 + 40, 16, game_over_option == 2 ? GREEN : RAYWHITE);
                
                if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_DOWN)) {
                    game_over_option = 1;
                }
                if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_UP)) {
                    game_over_option = 2;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    if (game_over_option == 1) {
                        // Salva pontuação e reinicia
                        ranking_add_and_sort(ranking, player_name, state.score);
                        ranking_save(ranking, "ranking.txt");
                        game_reset(&state);
                        game_started = true;
                        game_over_screen = false;
                        strcpy(player_name, "Player");
                        name_cursor = 0;
                        game_over_option = 0;
                    } else {
                        // Volta ao menu
                        ranking_add_and_sort(ranking, player_name, state.score);
                        ranking_save(ranking, "ranking.txt");
                        CloseWindow();
                        return;
                    }
                }
            }
            EndDrawing();
            continue;
        }

        // Input do jogo
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) game_handle_input(&state, 'W');
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) game_handle_input(&state, 'S');
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) game_handle_input(&state, 'A');
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) game_handle_input(&state, 'D');
        if (IsKeyPressed(KEY_Q)) {
            game_over_screen = true;
            continue;
        }

        // Update (simula ~8 ticks/s - mais lento)
        static float acc = 0.0f;
        acc += GetFrameTime();
        if (acc >= 0.125f) { // mais lento: 8 ticks por segundo
            game_update(&state);
            acc = 0.0f;
            
            // Verifica se game over
            if (state.game_over) {
                game_over_screen = true;
                continue;
            }
        }

        BeginDrawing();
        ClearBackground((Color){ 30, 30, 30, 255 });
        // Desenha mapa
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            Color bg = color_for_row(state.rows[y].type);
            DrawRectangle(margin, margin + y * cell, MAP_WIDTH * cell, cell, bg);
            if (state.rows[y].type == ROW_ROAD) {
                // faixa
                for (int x = 0; x < MAP_WIDTH; x += 4) {
                    DrawRectangle(margin + x * cell, margin + y * cell + cell/2 - 2, cell*2, 4, (Color){220,220,220,255});
                }
            }
            for (int x = 0; x < MAP_WIDTH; ++x) {
                char c = queue_get_cell(state.rows[y].queue, x);
                if (c == CHAR_CAR) {
                    DrawRectangle(margin + x * cell + 2, margin + y * cell + 4, cell - 4, cell - 8, RED);
                } else if (c == CHAR_LOG) {
                    DrawRectangle(margin + x * cell + 1, margin + y * cell + 6, cell - 2, cell - 12, BROWN);
                }
            }
        }
        // Jogador
        DrawCircle(margin + state.player_x * cell + cell/2, margin + state.player_y * cell + cell/2, cell*0.4f, YELLOW);
        DrawText(TextFormat("Score: %d  (WASD ou setas, Q sai)", state.score), margin, H - 32, 18, RAYWHITE);
        DrawText(TextFormat("Pos: (%d,%d) Tipo: %d", state.player_x, state.player_y, state.rows[state.player_y].type), margin, H - 50, 14, YELLOW);

        EndDrawing();
    }
    CloseWindow();
}

#else
void raylib_run_game(Ranking *ranking) {
    (void)ranking;
    printf("Raylib não habilitado na compilação. Recompile com -DENABLE_RAYLIB e linke -lraylib.\n");
}
#endif



