#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

static int utils_seeded = 0;

#ifndef _WIN32
static struct termios orig_termios;
#endif

void utils_sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)(ms * 1000));
#endif
}

void utils_clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    // ANSI clear
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

void utils_enable_raw_mode(void) {
#ifndef _WIN32
    struct termios raw;
    if (tcgetattr(0, &orig_termios) == -1) return;
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &raw);
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
#endif
}

void utils_disable_raw_mode(void) {
#ifndef _WIN32
    tcsetattr(0, TCSANOW, &orig_termios);
#endif
}

int utils_read_key_nonblock(void) {
#ifdef _WIN32
    if (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            // Arrow keys prefix, read and map to WASD equivalents if desired
            int ext = _getch();
            switch (ext) {
                case 72: return 'W'; // up
                case 80: return 'S'; // down
                case 75: return 'A'; // left
                case 77: return 'D'; // right
                default: return -1;
            }
        }
        return toupper(ch);
    }
    return -1;
#else
    unsigned char ch;
    int n = (int)read(0, &ch, 1);
    if (n == 1) return toupper(ch);
    return -1;
#endif
}

int utils_random_int(int min_value, int max_value) {
    if (!utils_seeded) {
        utils_seeded = 1;
        srand((unsigned int)time(NULL));
    }
    if (max_value <= min_value) return min_value;
    int span = max_value - min_value + 1;
    return min_value + (rand() % span);
}





