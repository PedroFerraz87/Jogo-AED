#include "ranking.h"
#include <stdio.h>
#include <string.h>

static void safe_strcpy(char *dst, const char *src, int maxlen) {
    if (maxlen <= 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, (size_t)maxlen);
    dst[maxlen] = '\0';
}

// Função principal: adiciona pontuação, ordena com insertion sort e salva automaticamente
void ranking_add(Ranking *ranking, const char *name, int score, const char *filepath) {
    if (!ranking) return;
    
    // Adiciona nova pontuação
    if (ranking->count < MAX_SCORES) {
        safe_strcpy(ranking->items[ranking->count].name, name ? name : "Player", MAX_NAME_LEN);
        ranking->items[ranking->count].score = score;
        ranking->count++;
    } else {
        // Substitui o pior se a nova pontuação for melhor
        int worst_index = ranking->count - 1;
        if (score > ranking->items[worst_index].score) {
            safe_strcpy(ranking->items[worst_index].name, name ? name : "Player", MAX_NAME_LEN);
            ranking->items[worst_index].score = score;
        }
    }
    
    // Insertion Sort (ordena decrescente por pontuação, alfabético em empate)
    for (int i = 1; i < ranking->count; ++i) {
        ScoreEntry key = ranking->items[i];
        int j = i - 1;
        while (j >= 0) {
            int swap = 0;
            if (ranking->items[j].score < key.score) {
                swap = 1;
            } else if (ranking->items[j].score == key.score) {
                if (strcmp(ranking->items[j].name, key.name) > 0) swap = 1;
            }
            if (!swap) break;
            ranking->items[j + 1] = ranking->items[j];
            j--;
        }
        ranking->items[j + 1] = key;
    }
    
    // Salva automaticamente no arquivo
    if (filepath) {
        FILE *f = fopen(filepath, "w");
        if (f) {
            for (int i = 0; i < ranking->count; ++i) {
                fprintf(f, "%s;%d\n", ranking->items[i].name, ranking->items[i].score);
            }
            fclose(f);
        }
    }
}

// Carrega ranking do arquivo
void ranking_load(Ranking *ranking, const char *filepath) {
    if (!ranking) return;
    ranking->count = 0;
    FILE *f = fopen(filepath, "r");
    if (!f) return;
    char name[128];
    int score;
    while (ranking->count < MAX_SCORES && fscanf(f, "%127[^;];%d\n", name, &score) == 2) {
        safe_strcpy(ranking->items[ranking->count].name, name, MAX_NAME_LEN);
        ranking->items[ranking->count].score = score;
        ranking->count++;
    }
    fclose(f);
}
