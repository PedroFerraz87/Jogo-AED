#ifndef RANKING_H
#define RANKING_H

#define MAX_NAME_LEN 31
#define MAX_SCORES 200

typedef struct ScoreEntry {
    char name[MAX_NAME_LEN + 1];
    int score;
} ScoreEntry;

typedef struct Ranking {
    ScoreEntry items[MAX_SCORES];
    int count;
} Ranking;

// Carrega ranking do arquivo
void ranking_load(Ranking *ranking, const char *filepath);

// Função principal: adiciona pontuação, ordena com insertion sort e salva automaticamente
void ranking_add(Ranking *ranking, const char *name, int score, const char *filepath);

#endif // RANKING_H
