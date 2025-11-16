/*
 * ============================================================================
 * ARQUIVO: utils.c
 * DESCRIÇÃO: Implementação de funções utilitárias multiplataforma
 * 
 * Este arquivo implementa funções que funcionam tanto no Windows quanto
 * em sistemas Unix/Linux, usando compilação condicional (#ifdef).
 * ============================================================================
 */

 #include "utils.h"
 #include <stdio.h>   // Para printf, system
 #include <stdlib.h>  // Para srand, rand
 
 // Variável estática: controla se o gerador aleatório já foi inicializado
 static int utils_seeded = 0;
 
 void utils_clear_screen(void) {
 #ifdef _WIN32
     system("cls");
 #else
     // ANSI clear
     printf("\033[2J\033[H");
     fflush(stdout);
 #endif
 }
 
 int utils_random_int(int min_value, int max_value) {
     if (!utils_seeded) {
         utils_seeded = 1;
         srand((unsigned int)time(NULL));
     }
     if (max_value <= min_value) return min_value;
     int span = max_value - min_value + 1;
     return min_value + (rand()%span);
 }