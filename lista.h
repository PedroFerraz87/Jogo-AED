#ifndef FILA_H
#define FILA_H

#include <stddef.h>

// Fila circular usada para representar uma linha rolante do mapa.
// A fila armazena células ASCII e suporta rotação para simular movimento.
// Implementada como lista encadeada circular com nós.

// Nó da lista encadeada circular
typedef struct Node {
    char data;              // caractere armazenado
    struct Node *next;      // ponteiro para próximo nó
} Node;

// Fila circular implementada com lista encadeada
typedef struct CircularQueue {
    int length;       // número de células (colunas)
    Node *head;       // ponteiro para o primeiro nó (lista circular)
} CircularQueue;

// Cria uma fila com o comprimento dado, inicializa todas as células com ' '.
CircularQueue *queue_create(int length);

// Libera a memória da fila.
void queue_destroy(CircularQueue *queue);

// Define uma célula específica (0..length-1).
void queue_set_cell(CircularQueue *queue, int index, char value);

// Obtém uma célula específica. Retorna ' ' se o índice estiver fora do intervalo.
char queue_get_cell(const CircularQueue *queue, int index);

// Preenche a fila com um padrão repetitivo definido por dois caracteres e seus comprimentos.
// Exemplo: patternA='=', runA=2, patternB=' ', runB=4 criará ondas de carros separados por espaços.
void queue_fill_pattern(CircularQueue *queue, char patternA, int runA, char patternB, int runB);

// Rotaciona a fila uma célula para a esquerda (índice 0 vai para o final).
void queue_rotate_left(CircularQueue *queue);

// Rotaciona a fila uma célula para a direita (último índice vai para 0).
void queue_rotate_right(CircularQueue *queue);

// Conta quantas células correspondem a um caractere dado.
int queue_count_char(const CircularQueue *queue, char ch);

#endif /* FILA_H */