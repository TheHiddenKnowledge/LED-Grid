#pragma once

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// Initializes timer registers
void f_init_millis(void);

// Resets and starts the timer
void f_start_millis(void);

// Stops the timer
void f_stop_millis(void);

// Gets the current timer value
uint16_t f_get_millis(void);

// Return true if timer has elapsed specified time
bool f_has_elapsed(uint16_t a_start, uint16_t a_limit);

