# Crossy Road 
## Como jogar
- No menu, escolha a opção `Jogar (1 jogador)` ou `Jogar (2 jogadores)` para jogar sozinho ou contra um colega, respectivamente e mova seu boneco usando "WASD" ou as setas (↑, ↓, ←, →).
- Seu objetivo é não colidir com a "base" da tela, que sobe de acordo com o tempo, com nenhum carro e nem cair na água, assim, subindo o mais longe possível no mapa, se autodesafiando para conseguir uma pontuação cada vez mais alta.
- Ao ser eliminado, a tela de game over mostrará seu score, o do seu colega, caso esteja no modo multiplayer, e as pontuações serão salvas no arquivo "ranking.txt".

## Autores:
- Pedro Valença Ferraz - pvf@cesar.school
- Caio Sena Santos - css4@cesar.school

## Como compilar (já com a biblioteca Raylib instalada e compilador em C (gcc))
1. cd /c/Users/"seu_caminho..."/Jogo-AED   
2. gcc -Wall -std=c99 -DENABLE_RAYLIB main.c sound.c game.c lista.c ranking.c utils.c raylib_view.c -lraylib -lopengl32 -lgdi32 -lwinmm -o crossy.exe
3. ./crossy.exe

## Arquivos importantes
- main.c -> menu principal
- game.c / game.h -> lógica do jogo
- lista.c / lista.h -> lista simplesmente circular (estrutura de dados central)
- ranking.c / ranking.h -> ranking e insertion sort (algoritmo de ordenação)
- utils.c / utils.h -> utilitários (entrada não bloqueante, sleep, clear)
- ranking.txt -> arquivo onde o ranking é salvo
