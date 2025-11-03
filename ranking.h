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

// Loads ranking from file. If file missing, starts empty.
void ranking_load(Ranking *ranking, const char *filepath);

// Saves ranking to file.
void ranking_save(const Ranking *ranking, const char *filepath);

// Adds new score (name truncated if needed), then sorts.
void ranking_add_and_sort(Ranking *ranking, const char *name, int score);

// Insertion sort (descending by score). Stable by name when equal.
void ranking_insertion_sort(Ranking *ranking);

// Displays ranking to terminal.
void ranking_print(const Ranking *ranking, int max_rows);

#endif // RANKING_H