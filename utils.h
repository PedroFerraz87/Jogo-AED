#ifndef UTILS_H
#define UTILS_H

#include <time.h>

// Cross-platform utilities for input, timing and screen control.

void utils_sleep_ms(int ms);

void utils_clear_screen(void);

// Initialize terminal for non-blocking, no-echo keyboard input.
void utils_enable_raw_mode(void);

// Restore terminal to normal mode.
void utils_disable_raw_mode(void);

// Returns -1 if no key available; otherwise returns uppercase ASCII of the key.
int utils_read_key_nonblock(void);

// Returns integer in [min, max].
int utils_random_int(int min_value, int max_value);

#endif // UTILS_H





