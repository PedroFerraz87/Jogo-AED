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

static void show_menu(void) {
    printf("==============================\n");
    printf("   Nova(Velha) Infancia - Crossy Road\n");
    printf("==============================\n");
    printf("1) Jogar \n");
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
            raylib_run_game(&ranking);
        } else if (opcao == 2) {
            utils_clear_screen();
            printf("===== RANKING =====\n");
            for (int i = 0; i < ranking.count && i < 20; ++i) {
                printf("%2d) %-20s %6d\n", i + 1, ranking.items[i].name, ranking.items[i].score);
            }
            if (ranking.count == 0) {
                printf("(vazio)\n");
            }
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