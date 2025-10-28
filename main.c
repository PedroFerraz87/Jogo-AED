#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "game.h"
#include "ranking.h"
#include "raylib_view.h"

#define RANKING_FILE "ranking.txt"

static void wait_any_key(void) {
    printf("Pressione qualquer tecla para continuar...\n");
    utils_enable_raw_mode();
    while (utils_read_key_nonblock() == -1) {
        utils_sleep_ms(16);
    }
    utils_disable_raw_mode();
}

static void run_game_loop(Ranking *ranking) {
    GameState state;
    game_init(&state, MAP_WIDTH);

    utils_enable_raw_mode();
    int running = 1;
    while (running) {
        int key = utils_read_key_nonblock();
        if (key == 'Q') {
            state.game_over = 1;
        } else if (key != -1) {
            game_handle_input(&state, key);
        }

        game_update(&state);
        game_render(&state);
        if (state.game_over) {
            utils_disable_raw_mode();
            printf("\nGame Over! Sua pontua\xC3\xA7\xC3\xA3o: %d\n", state.score);
            char nome[64];
            printf("Digite seu nome (max %d chars): ", MAX_NAME_LEN);
            if (fgets(nome, sizeof(nome), stdin)) {
                size_t len = strlen(nome);
                if (len > 0 && (nome[len - 1] == '\n' || nome[len - 1] == '\r')) nome[len - 1] = '\0';
            } else {
                strcpy(nome, "Player");
            }
            ranking_add_and_sort(ranking, nome, state.score);
            ranking_save(ranking, RANKING_FILE);

            int opcao = 0;
            printf("\nJogar novamente? (1 = sim, 0 = n\xC3\xA3o): ");
            scanf("%d", &opcao);
            getchar(); // consume newline
            if (opcao == 1) {
                game_reset(&state);
                utils_enable_raw_mode();
                continue;
            } else {
                running = 0;
                break;
            }
        }
        utils_sleep_ms(80);
    }
    utils_disable_raw_mode();
}

static void show_menu(void) {
    printf("==============================\n");
    printf("   Nova(Velha) Inf\xC3\xA2ncIA - Crossy\n");
    printf("==============================\n");
    printf("1) Jogar (Terminal)\n");
    printf("3) Jogar (Raylib)\n");
    printf("2) Ver ranking\n");
    printf("0) Sair\n");
    printf("Escolha: ");
}

int main(void) {
    Ranking ranking;
    ranking_load(&ranking, RANKING_FILE);

    int opcao = -1;
    while (1) {
        utils_clear_screen();
        show_menu();
        if (scanf("%d", &opcao) != 1) {
            // invalid input
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}

        if (opcao == 1) {
            run_game_loop(&ranking);
        } else if (opcao == 3) {
            raylib_run_game(&ranking);
        } else if (opcao == 2) {
            utils_clear_screen();
            ranking_insertion_sort(&ranking);
            ranking_print(&ranking, 20);
            printf("\n");
            wait_any_key();
        } else if (opcao == 0) {
            utils_clear_screen();
            printf("Saindo...\n");
            break;
        }
    }
    return 0;
}



