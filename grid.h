#pragma once

#include <xc.h> 
#include <stdint.h>
#include <stdbool.h>
#include "chars.h"

// RGB structure
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} s_rgb; 

// Initializes the PIC for control of the LED grid
void f_init_grid(void);

// Sets the GRID array character and background
void f_set_grid(char a_char, s_rgb a_char_rgb, s_rgb a_bg_rgb);

// Writes RGB data to LED data line 
void f_write_col(uint8_t a_col, bool a_clr); 

// Checks the scanning line timer
bool f_can_scan_led(void);

// Performs scanning line write to LED
void f_scan_led(void);
