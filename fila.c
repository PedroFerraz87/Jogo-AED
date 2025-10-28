#include "fila.h"
#include <stdlib.h>
#include <string.h>

CircularQueue *queue_create(int length) {
    if (length <= 0) return NULL;
    CircularQueue *queue = (CircularQueue *)malloc(sizeof(CircularQueue));
    if (!queue) return NULL;
    queue->length = length;
    queue->cells = (char *)malloc((size_t)length);
    if (!queue->cells) {
        free(queue);
        return NULL;
    }
    memset(queue->cells, ' ', (size_t)length);
    return queue;
}

void queue_destroy(CircularQueue *queue) {
    if (!queue) return;
    free(queue->cells);
    free(queue);
}

void queue_set_cell(CircularQueue *queue, int index, char value) {
    if (!queue || !queue->cells) return;
    if (index < 0 || index >= queue->length) return;
    queue->cells[index] = value;
}

char queue_get_cell(const CircularQueue *queue, int index) {
    if (!queue || !queue->cells) return ' ';
    if (index < 0 || index >= queue->length) return ' ';
    return queue->cells[index];
}

void queue_fill_pattern(CircularQueue *queue, char patternA, int runA, char patternB, int runB) {
    if (!queue || !queue->cells || queue->length <= 0) return;
    if (runA <= 0 && runB <= 0) {
        memset(queue->cells, ' ', (size_t)queue->length);
        return;
    }
    int i = 0;
    while (i < queue->length) {
        for (int a = 0; a < runA && i < queue->length; ++a) {
            queue->cells[i++] = patternA;
        }
        for (int b = 0; b < runB && i < queue->length; ++b) {
            queue->cells[i++] = patternB;
        }
    }
}

void queue_rotate_left(CircularQueue *queue) {
    if (!queue || queue->length <= 1) return;
    char first = queue->cells[0];
    memmove(queue->cells, queue->cells + 1, (size_t)(queue->length - 1));
    queue->cells[queue->length - 1] = first;
}

void queue_rotate_right(CircularQueue *queue) {
    if (!queue || queue->length <= 1) return;
    char last = queue->cells[queue->length - 1];
    memmove(queue->cells + 1, queue->cells, (size_t)(queue->length - 1));
    queue->cells[0] = last;
}

int queue_count_char(const CircularQueue *queue, char ch) {
    if (!queue || !queue->cells) return 0;
    int count = 0;
    for (int i = 0; i < queue->length; ++i) {
        if (queue->cells[i] == ch) count++;
    }
    return count;
}





