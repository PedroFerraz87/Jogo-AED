# Crossy Road 
## Como jogar
- Escolha a opção `Jogar` no menu e siga as instruções na tela, movendo seu boneco usando "WASD" ou as setas.
- Seu objetivo é não colidir com nenhum carro e nem cair na água, assim, subindo o mais longe possível no mapa, se autodesafiando para conseguir uma pontuação cada vez mais alta.
- Ao ser eliminado, a tela de game over mostrará seu score, que será salvo em ranking.txt.

## Autores:
- Pedro Valença Ferraz - pvf@cesar.school
- Caio Sena Santos - css4@cesar.school

## Requisitos
- Compilador C (gcc)
- Raylib para interface gráfica

## Como compilar (já com a biblioteca Raylib instalada)
1. cd /seu_caminho.../GitHub/Jogo-AED   
2. gcc -Wall -std=c99 -DENABLE_RAYLIB main.c game.c lista.c ranking.c utils.c raylib_view.c -lraylib -lopengl32 -lgdi32 -lwinmm -o crossy.exe
3. ./crossy.exe

## Arquivos importantes
- main.c -> menu principal
- game.c / game.h -> lógica do jogo
- lista.c / lista.h -> lista simplesmente circular (estrutura de dados central)
- ranking.c / ranking.h -> ranking e insertion sort (algoritmo de ordenação)
- utils.c / utils.h -> utilitários (entrada não bloqueante, sleep, clear)
- ranking.txt -> arquivo onde o ranking é salvo
