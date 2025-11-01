# Crossy Road 

## Autores:
- Pedro Valença Ferraz - pvf@cesar.school
- Caio Sena Santos - css4@cesar.school

## Requisitos
- compilador C (gcc)
-   Raylib para interface gráfica

## Como compilar (já com a biblioteca Raylib instalada)
1. cd /c/Users/Lenovo/Documents/GitHub/Jogo-AED   
2. gcc -Wall -std=c99 -DENABLE_RAYLIB main.c game.c fila.c ranking.c utils.c raylib_view.c -lraylib -lopengl32 -lgdi32 -lwinmm -o crossy.exe
3. ./crossy.exe


## Arquivos importantes
- main.c -> menu principal
- game.c / game.h -> lógica do jogo
- fila.c / fila.h -> fila circular (estrutura de dados central)
- ranking.c / ranking.h -> ranking e insertion sort (algoritmo de ordenação)
- utils.c / utils.h -> utilitários (entrada não bloqueante, sleep, clear)
- ranking.txt -> arquivo onde o ranking é salvo

## Como jogar
- Escolha a opção `Jogar` no menu e siga as instruções na tela, movendo seu boneco usando "WASD" ou as setas.
- Ao finalizar, seu score será salvo em ranking.txt e você deve se desafiar a si mesmo para pontuar o máximo possível.
