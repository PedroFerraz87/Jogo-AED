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

void ranking_save(const Ranking *ranking, const char *filepath) {
    if (!ranking) return;
    FILE *f = fopen(filepath, "w");
    if (!f) return;
    for (int i = 0; i < ranking->count; ++i) {
        fprintf(f, "%s;%d\n", ranking->items[i].name, ranking->items[i].score);
    }
    fclose(f);
}

void ranking_insertion_sort(Ranking *ranking) {
    if (!ranking) return;
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
}

void ranking_add_and_sort(Ranking *ranking, const char *name, int score) {
    if (!ranking) return;
    if (ranking->count < MAX_SCORES) {
        safe_strcpy(ranking->items[ranking->count].name, name ? name : "Player", MAX_NAME_LEN);
        ranking->items[ranking->count].score = score;
        ranking->count++;
    } else {
        // Replace last if better
        int worst_index = ranking->count - 1;
        if (score > ranking->items[worst_index].score) {
            safe_strcpy(ranking->items[worst_index].name, name ? name : "Player", MAX_NAME_LEN);
            ranking->items[worst_index].score = score;
        }
    }
    ranking_insertion_sort(ranking);
}

void ranking_print(const Ranking *ranking, int max_rows) {
    if (!ranking) return;
    int rows = ranking->count < max_rows ? ranking->count : max_rows;
    printf("===== RANKING =====\n");
    for (int i = 0; i < rows; ++i) {
        printf("%2d) %-20s %6d\n", i + 1, ranking->items[i].name, ranking->items[i].score);
    }
    if (rows == 0) {
        printf("(vazio)\n");
    }
}





