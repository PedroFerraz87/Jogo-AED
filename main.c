#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "game.h"
#include "ranking.h"
#include "raylib_view.h"
#include "sound.h"

#define RANKING_FILE "ranking.txt"

static void show_menu(void) {
    printf("=================\n");
    printf(" Crossy Road\n");
    printf("=================\n");
    printf("Escolha uma das alternativas abaixo: \n");
    printf("1) Jogar\n");
    printf("0) Sair\n");
}

int main(void) {
    
    Ranking ranking;
    ranking_load(&ranking, RANKING_FILE);
    
    int opcao = -1;
    char buffer[32];   
    while (1) {
        utils_clear_screen();
        show_menu();

        printf("Número: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            continue;
        }
        if (sscanf(buffer, "%d", &opcao) != 1) {
            continue;
        }

        if (opcao == 1) {
            raylib_run_game(&ranking);
        } else if (opcao == 0) {
            utils_clear_screen();
            printf("Saindo...\n");
            break;
        }
    }
    return 0;
}