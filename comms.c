#include "comms.h"

// Global variables for the left and right UART receiver
uint16_t g_rx_buffer[2];
uint8_t g_rx_size[2] = {0, 0};
uint8_t g_rx_index[2] = {0, 0};

// Global variables for the left and right UART transmitter
uint16_t g_tx_buffer[2];
uint8_t g_tx_size[2] = {0, 0};
uint8_t g_tx_index[2] = {0, 0};

// Status variables for both UART busses 
bool g_is_receiving[2] = {false, false};
bool g_is_sending[2] = {false, false};

void f_init_comms()
{
    // Initializing port A UART pins
    PORTA &= ~(_PORTA_RA2_MASK | _PORTA_RA4_MASK | _PORTA_RA5_MASK);
    LATA &= ~(_PORTA_RA2_MASK | _PORTA_RA4_MASK | _PORTA_RA5_MASK);
    ANSELA &= ~(_PORTA_RA2_MASK | _PORTA_RA4_MASK | _PORTA_RA5_MASK);
    TRISA |= _PORTA_RA2_MASK | _PORTA_RA4_MASK;
    TRISA &= ~_PORTA_RA5_MASK;
    // Initializing port C UART pins
    PORTC &= ~_PORTC_RC0_MASK;
    LATC &= ~_PORTC_RC0_MASK;
    ANSELC &= ~_PORTC_RC0_MASK;
    TRISC &= ~_PORTC_RC0_MASK;
    // Setting peripheral selection register for UART1 and UART2
    // UART1 is the left bus, with UART2 being the right
    U1RXPPS = _PORTA_RA4_POSN;
    RA5PPS = 0x10;
    U2RXPPS = _PORTA_RA2_POSN;
    RC0PPS = 0x13;
    // Setting UART1 and UART2 registers
    // Baud rate of 38400 bits/s
    U1BRGH = 0x00;
    U1BRGL = 0x67;
    U1CON0 |= _U1CON0_TXEN_MASK | _U1CON0_RXEN_MASK;
    U1CON1 |= _U1CON1_ON_MASK;
    U1CON2 |= _U1CON2_RUNOVF_MASK;
    U2BRGH = 0x00;
    U2BRGL = 0x67;
    U2CON0 |= _U2CON0_TXEN_MASK | _U2CON0_RXEN_MASK;
    U2CON1 |= _U2CON1_ON_MASK;
    U2CON2 |= _U2CON2_RUNOVF_MASK;
}

bool inline f_can_receive(bool a_dir)
{
    return ~g_is_receiving[a_dir];
}

bool f_receive_bytes(bool a_dir, uint8_t *a_data, uint8_t a_size)
{
    if(f_can_receive(a_dir))
    {
        g_rx_buffer[a_dir] = (uint16_t)a_data;
        g_rx_size[a_dir] = a_size;
        g_rx_index[a_dir] = 0;
        g_is_receiving[a_dir] = true;
        return true;
    }
    else
    {
        return false;
    }
}

void f_proc_receive_bytes(bool a_dir)
{
    if(!f_can_receive(a_dir))
    {
        if((!a_dir) ? PIR4 & _PIR4_U1RXIF_MASK : PIR8 & _PIR8_U2RXIF_MASK)
        {
            *((uint8_t *)g_rx_buffer[a_dir] + g_rx_index[a_dir]) = 
                    (!a_dir) ? U1RXB : U2RXB;
            g_rx_index[a_dir]++;
            if(g_rx_index[a_dir] == g_rx_size[a_dir])
            {
                f_stop_receive(a_dir);
                return;
            } 
        }
    }
}

void inline f_stop_receive(bool a_dir)
{
    g_rx_size[a_dir] = 0;
    g_rx_index[a_dir] = 0;
    g_is_receiving[a_dir] = false;
}

bool inline f_can_send(bool a_dir)
{
    return ~g_is_sending[a_dir];
}

bool f_send_bytes(bool a_dir, uint8_t *a_data, uint8_t a_size)
{
    if(f_can_send(a_dir))
    {
        g_tx_buffer[a_dir] = (uint16_t)a_data;
        g_tx_size[a_dir] = a_size;
        g_tx_index[a_dir] = 0;
        g_is_sending[a_dir] = true;
        return true;
    }
    else
    {
        return false;
    }
}

void f_proc_send_bytes(bool a_dir)
{
    if(!f_can_send(a_dir))
    {
        if((!a_dir) ? PIR4 & _PIR4_U1TXIF_MASK : PIR8 & _PIR8_U2TXIF_MASK)
        {
            if(g_tx_index[a_dir] >= g_tx_size[a_dir])
            {
                f_stop_send(a_dir);
                return;
            }
            (!a_dir) ? (U1TXB = *((uint8_t *)g_tx_buffer[a_dir] + g_tx_index[a_dir])) : 
                (U2TXB = *((uint8_t *)g_tx_buffer[a_dir] + g_tx_index[a_dir]));
            g_tx_index[a_dir]++;
        }
    }
}

void inline f_stop_send(bool a_dir)
{
    g_tx_size[a_dir] = 0;
    g_tx_index[a_dir] = 0;
    g_is_sending[a_dir] = false;
}