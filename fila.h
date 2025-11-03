#ifndef FILA_H
#define FILA_H

#include <stddef.h>

// Circular queue used to represent one scrolling row of the map.
// The queue stores ASCII cells and supports rotation to simulate movement.
// Implementada como lista encadeada circular com nós.

// Nó da lista encadeada circular
typedef struct Node {
    char data;              // caractere armazenado
    struct Node *next;      // ponteiro para próximo nó
} Node;

// Fila circular implementada com lista encadeada
typedef struct CircularQueue {
    int length;       // number of cells (columns)
    Node *head;       // ponteiro para o primeiro nó (lista circular)
} CircularQueue;

// Creates a queue with given length, initializes all cells to ' '.
CircularQueue *queue_create(int length);

// Frees queue memory.
void queue_destroy(CircularQueue *queue);

// Sets a specific cell (0..length-1).
void queue_set_cell(CircularQueue *queue, int index, char value);

// Gets a specific cell. Returns ' ' if index is out of range.
char queue_get_cell(const CircularQueue *queue, int index);

// Fills the queue with a repeating pattern defined by two characters and their run lengths.
// Example: patternA='=', runA=2, patternB=' ', runB=4 will create waves of cars separated by gaps.
void queue_fill_pattern(CircularQueue *queue, char patternA, int runA, char patternB, int runB);

// Rotates the queue by one cell to the left (index 0 goes to end).
void queue_rotate_left(CircularQueue *queue);

// Rotates the queue by one cell to the right (last index goes to 0).
void queue_rotate_right(CircularQueue *queue);

// Counts how many cells match a given character.
int queue_count_char(const CircularQueue *queue, char ch);

#endif // FILA_H





