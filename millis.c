#include "millis.h"

uint16_t g_millis = 0;

void f_init_millis()
{
    T0CON1 |= _T0CON1_CS1_MASK | _T0CON1_CKPS2_MASK | _T0CON1_CKPS1_MASK;
    TMR0H = 0xFA;
    INTCON0 |= _INTCON0_GIE_MASK;
}

void f_start_millis()
{
    g_millis = 0; 
    TMR0L = 0x00;
    PIE3 |= _PIE3_TMR0IE_MASK;
    T0CON0 |= _T0CON0_EN_MASK;
}

void f_stop_millis()
{
    PIE3 &= ~_PIE3_TMR0IE_MASK;
    T0CON0 &= ~_T0CON0_EN_MASK;
}

uint16_t f_get_millis()
{
    return g_millis;
}

bool f_has_elapsed(uint16_t a_start, uint16_t a_limit)
{
    if(g_millis > a_start)
    {
        return (g_millis - a_start) >= a_limit;
    }
    else
    {
        return false;
    }
}

void __interrupt(irq(IRQ_TMR0)) f_increment_millis(void)
{
    g_millis++;
    PIR3 &= ~_PIR3_TMR0IF_MASK;
}
