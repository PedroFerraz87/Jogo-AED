#ifndef UTILS_H
#define UTILS_H

#include <time.h>  // Para time() usado na inicialização do gerador aleatório


void utils_clear_screen(void); // Limpa a tela do terminal

/**
 * @param min_value Valor mínimo (inclusivo)
 * @param max_value Valor máximo (inclusivo)
 * @return Número aleatório no intervalo [min_value, max_value]
 * Descrição: Gera um número inteiro aleatório em um intervalo
 * A semente do gerador aleatório é inicializada automaticamente na primeira chamada.
 */
int utils_random_int(int min_value, int max_value);

#endif 