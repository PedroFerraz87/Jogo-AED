#include "lista.h"
#include <stdlib.h>
#include <string.h>

CircularQueue *queue_create(int length) {
    if (length <= 0) return NULL;
    
    CircularQueue *queue = (CircularQueue *)malloc(sizeof(CircularQueue));
    if (!queue) return NULL;
    
    queue->length = length;
    queue->head = NULL;
    
    // Cria lista encadeada circular
    if (length == 0) {
        return queue;
    }
    
    // Cria o primeiro nó
    Node *first = (Node *)malloc(sizeof(Node));
    if (!first) {
        free(queue);
        return NULL;
    }
    first->data = ' ';
    queue->head = first;
    
    // Cria os nós restantes e conecta em círculo
    Node *current = first;
    for (int i = 1; i < length; i++) {
        Node *new_node = (Node *)malloc(sizeof(Node));
        if (!new_node) {
            // Libera nós já criados
            Node *temp = first;
            for (int j = 0; j < i; j++) {
                Node *next = temp->next;
                free(temp);
                temp = next;
            }
            free(queue);
            return NULL;
        }
        new_node->data = ' ';
        current->next = new_node;
        current = new_node;
    }
    
    // Conecta o último nó ao primeiro (fecha o círculo)
    current->next = first;
    
    return queue;
}

void queue_destroy(CircularQueue *queue) {
    if (!queue || !queue->head) {
        if (queue) free(queue);
        return;
    }
    
    // Libera todos os nós da lista circular
    Node *current = queue->head;
    Node *start = queue->head;
    
    if (current) {
        do {
            Node *next = current->next;
            free(current);
            current = next;
        } while (current != start && current != NULL);
    }
    
    free(queue);
}

void queue_set_cell(CircularQueue *queue, int index, char value) {
    if (!queue || !queue->head) return;
    if (index < 0 || index >= queue->length) return;
    
    // Navega até o nó no índice especificado
    Node *current = queue->head;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    
    current->data = value;
}

char queue_get_cell(const CircularQueue *queue, int index) {
    if (!queue || !queue->head) return ' ';
    if (index < 0 || index >= queue->length) return ' ';
    
    // Navega até o nó no índice especificado
    Node *current = queue->head;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    
    return current->data;
}

void queue_fill_pattern(CircularQueue *queue, char patternA, int runA, char patternB, int runB) {
    if (!queue || !queue->head || queue->length <= 0) return;
    if (runA <= 0 && runB <= 0) {
        // Preenche tudo com espaço
        Node *current = queue->head;
        Node *start = queue->head;
        do {
            current->data = ' ';
            current = current->next;
        } while (current != start);
        return;
    }
    
    // Preenche o padrão
    Node *current = queue->head;
    int i = 0;
    
    while (i < queue->length) {
        // Preenche patternA
        for (int a = 0; a < runA && i < queue->length; a++) {
            current->data = patternA;
            current = current->next;
            i++;
        }
        // Preenche patternB
        for (int b = 0; b < runB && i < queue->length; b++) {
            current->data = patternB;
            current = current->next;
            i++;
        }
    }
}

void queue_rotate_left(CircularQueue *queue) {
    if (!queue || !queue->head || queue->length <= 1) return;
    
    // Rotacionar para esquerda = mover head para o próximo nó
    queue->head = queue->head->next;
}

void queue_rotate_right(CircularQueue *queue) {
    if (!queue || !queue->head || queue->length <= 1) return;
    
    // Rotacionar para direita = mover head para o nó anterior
    // Precisamos encontrar o nó anterior ao head
    Node *current = queue->head;
    for (int i = 0; i < queue->length - 1; i++) {
        current = current->next;
    }
    queue->head = current;
}

int queue_count_char(const CircularQueue *queue, char ch) {
    if (!queue || !queue->head) return 0;
    
    int count = 0;
    Node *current = queue->head;
    Node *start = queue->head;
    
    do {
        if (current->data == ch) {
            count++;
        }
        current = current->next;
    } while (current != start);
    
    return count;
}
