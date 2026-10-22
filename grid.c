#include "grid.h"

// Time counted on TMR0 between scanning operations
#define SCAN_TIME 0x0400

// Array that holds the RGB values of the LED grid 
s_rgb g_grid[COL][ROW];

// Current column being
uint8_t g_current_col = 0;

// Adjusts the RGB values to lower current consumption
inline s_rgb _f_rgb_adjust(s_rgb a_rgb_b)
{
    s_rgb l_rgb_a = 
    {a_rgb_b.r / 3, a_rgb_b.g / 3, a_rgb_b.b / 3};
    return l_rgb_a;
}

void f_init_grid()
{
    // Initializing RGB driver pins 
    PORTC &= _PORTC_RC0_MASK;
    LATC &= _PORTC_RC0_MASK;
    ANSELC &= _PORTC_RC0_MASK;
    TRISC &= _PORTC_RC0_MASK;
    // Initializing Timer1
    T1CON |= _T1CON_RD16_MASK;
    T1CLK = 0x01;
    // CCP1 toggles the output and resets Timer1 on compare
    CCP1CON |= _CCP1CON_EN_MASK | _CCP1CON_MODE0_MASK;
    // Initializing Timer0
    T0CON0 |= _T0CON0_MD16_MASK | _T0CON0_EN_MASK;
    T0CON1 |= _T0CON1_CS1_MASK | _T0CON1_CKPS2_MASK | _T0CON1_CKPS1_MASK;
    // Clearing LED grid
    f_set_grid(' ', (s_rgb){0x00, 0x00, 0x00}, (s_rgb){0x00, 0x00, 0x00});
}

void f_set_grid(char a_char, s_rgb a_char_rgb, s_rgb a_bg_rgb)
{
    // Each character is used, in order, to index the CHAR array
    s_char l_char_map = g_chars[a_char - ' '];
    for(uint8_t i = 0; i < COL; i++)
    {
        for(uint8_t j = 0; j < ROW; j++)
        {
            // Fills in the grid matrix according to the binary data
            if((l_char_map.cols[i] >> (ROW - 1 - j)) & 0x1)
            {
                g_grid[i][j] = _f_rgb_adjust(a_char_rgb);
            }
            else
            {
                g_grid[i][j] = _f_rgb_adjust(a_bg_rgb);
            }
        } 
    } 
}

void f_write_col(uint8_t a_col, bool a_clr)
{
    // Resetting CCPR1 and TMR1 registers
    CCPR1L = 0x69;
    CCPR1H = 0x00;
    TMR1L = 0x00;
    TMR1H = 0x00;
    for(int i = 0; i < ROW; i++)
    {
        // Setting CCP module to output to current port position
        *(&RC5PPS - a_col) = 0x09;
        // Rearranging RGB to GRB per datasheet
        uint8_t l_stream[3] = {0x00, 0x00, 0x00};
        if(!a_clr)
        {
            l_stream[0] = g_grid[a_col][i].g;
            l_stream[1] = g_grid[a_col][i].r;
            l_stream[2] = g_grid[a_col][i].b;
        }
        // Have to assign variables like this to keep linker created symbols
        uint8_t *l_data = 0;
        l_data += (uint16_t)l_stream;
        uint8_t l_bit_count = 0;
        l_bit_count += 8;
        uint8_t l_byte_count = 0;
        l_byte_count += 3;
        // Writing FSR pointer register
        asm("movf " ___mkstr(BANKMASK(f_write_col@l_data)) ", w, c");
        asm("movwf FSR0L, c");
        asm("movf " ___mkstr(BANKMASK(f_write_col@l_data + 1)) ", w, c");
        asm("movwf FSR0H, c");
        asm("BANKSEL CCPR1L");
        // Resetting and enabling timer
        asm("movlw 0x069");
        asm("movwf " ___mkstr(BANKMASK(CCPR1L)) ", b");
        asm("movlw 0x067");
        asm("movwf " ___mkstr(BANKMASK(TMR1L)) ", b");
        asm("bsf T1CON, 0, b");
        asm("small_low_loop2:");
        asm("nop");
        asm("nop");
        asm("nop");
        // .3 us high 
        asm("small_high_loop:");
        asm("rlcf INDF0, f, c");
        asm("bc large_loop");
        asm("movlw 0x06");
        asm("movwf " ___mkstr(BANKMASK(CCPR1L)) ", b");
        asm("nop");
        asm("nop");
        // .9 us high and low
        asm("large_loop:");
        asm("movlw 0x0F");
        asm("movwf " ___mkstr(BANKMASK(CCPR1L)) ", b");
        asm("dcfsnz " ___mkstr(BANKMASK(f_write_col@l_bit_count)) ", f, c");
        asm("goto byte_done");
        asm("bc large_high1");
        asm("goto large_low1");
        asm("byte_done:");
        asm("dcfsnz " ___mkstr(BANKMASK(f_write_col@l_byte_count)) ", f, c");
        asm("nop");
        asm("movlw 0x08");
        asm("movwf " ___mkstr(BANKMASK(f_write_col@l_bit_count)) ", c");
        asm("bc large_high2");
        asm("incf FSR0L, f, c");
        asm("tstfsz " ___mkstr(BANKMASK(f_write_col@l_byte_count)) ", c");
        asm("goto large_low2");
        asm("bcf T1CON, 0, b");
        asm("goto finish");
        asm("large_low1:");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("large_low2:");
        asm("nop");
        asm("goto small_high_loop");
        asm("large_high1:");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("movlw 0x06");
        asm("goto small_low_loop1");
        asm("large_high2:");
        asm("incf FSR0L, f, c");
        asm("movlw 0x06");
        // .3 us low
        asm("small_low_loop1:");
        asm("movwf " ___mkstr(BANKMASK(CCPR1L)) ", b");
        asm("tstfsz " ___mkstr(BANKMASK(f_write_col@l_byte_count)) ", c");
        asm("goto small_low_loop2");
        asm("bcf T1CON, 0, b");
        asm("finish:");
    }
    *(&RC5PPS - a_col) = 0x00;
}

bool f_can_scan_led()
{
    return (TMR0H << 8 | TMR0L) > SCAN_TIME;
}

void f_scan_led()
{
    TMR0L = 0x00;
    TMR0H = 0x00;
    // Clears previous column to control current consumption
    if(g_current_col == 0)
    {
        f_write_col(COL - 1, true);
    }
    else
    {
        f_write_col(g_current_col - 1, true);
    }
    f_write_col(g_current_col, false);
    g_current_col++;
    if(g_current_col >= COL)
    {
        g_current_col = 0;
    }
}