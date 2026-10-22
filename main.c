#include <xc.h>
#include <string.h>
#include "grid.h"
#include "comms.h"
#include "millis.h"

#pragma config FEXTOSC = OFF, RSTOSC = HFINTOSC_64MHZ 
#pragma config CSWEN = OFF, FCMEN = OFF, FCMENP = OFF, FCMENS = OFF
#pragma config PWRTS = PWRT_64
#pragma config BORV = VBOR_2P85
#pragma config WDTE = OFF
#pragma config DEBUG = ON

// Status macros
#define STAT_IDLE 0
#define STAT_PEND 1
#define STAT_PROC 2

// Message macros
#define MSG_IDLE 0
#define MSG_RDY 1
#define MSG_ID 2
#define MSG_ERR 3

// Max transmission size
#define TMSN_SIZE 8

// Timeout durations in clock cycles
#define TIMEOUT_L 20000
#define TIMEOUT_R 10000

// Global variables used for communications
uint8_t g_id = 0;
bool g_new_data = false;
uint8_t g_stat[2] = {STAT_IDLE, STAT_IDLE};
uint8_t g_data[2][TMSN_SIZE];
uint8_t g_txmsg[2] = {MSG_IDLE, MSG_IDLE};
uint8_t g_rxmsg[2] = {MSG_IDLE, MSG_IDLE};
uint16_t g_timeout[2] = {0, 0};

// Gets module ID when powered on 
void f_get_id(void)
{
    // Send message to the module to the left 
    g_txmsg[LEFT] = MSG_ID;
    f_send_bytes(LEFT, &g_txmsg[LEFT], 1);
    while(!f_can_send(LEFT))
    {f_proc_send_bytes(LEFT);}
    // Get ID from the module to the left
    while(g_rxmsg[LEFT] != MSG_ID)
    {
        f_receive_bytes(LEFT, &g_rxmsg[LEFT], 1);
        f_proc_receive_bytes(LEFT);
    }
    f_receive_bytes(LEFT, &g_rxmsg[LEFT], 1);
    while(!f_can_receive(LEFT))
    {f_proc_receive_bytes(LEFT);}
    g_id = g_rxmsg[LEFT] + 1;
    g_rxmsg[LEFT] = MSG_IDLE;
}

// Handles communications on the left side
void f_left_comms(void)
{
    switch(g_stat[LEFT])
    {
        case STAT_IDLE:
            // Executed when the left module is ready to send data
            if(g_rxmsg[LEFT] == MSG_RDY)
            {
                // Send confirmation message and receive data 
                g_rxmsg[LEFT] = MSG_IDLE;
                g_txmsg[LEFT] = MSG_RDY;
                while(!f_send_bytes(LEFT, &g_txmsg[LEFT], 1))
                {f_proc_send_bytes(LEFT);}
                while(!f_receive_bytes(LEFT, g_data[LEFT], TMSN_SIZE))
                {f_proc_receive_bytes(LEFT);}
                g_stat[LEFT] = STAT_PEND;
            }
            // Idle and waiting for data 
            else
            {
                f_receive_bytes(LEFT, &g_rxmsg[LEFT], 1);
                g_txmsg[LEFT] = MSG_IDLE;
                f_send_bytes(LEFT, &g_txmsg[LEFT], 1);
            }
            break;
        case STAT_PEND:
            // Data has been successfully received
            if(f_can_receive(LEFT))
            {
                g_timeout[LEFT] = 0;
                g_stat[LEFT] = STAT_PROC;
            }
            // Data has not been received and timed out 
            if(g_timeout[LEFT] > TIMEOUT_L)
            {
                // Sending an error message and resetting communications
                f_stop_receive(LEFT);
                g_txmsg[LEFT] = MSG_ERR;
                f_send_bytes(LEFT, &g_txmsg[LEFT], 1);
                g_timeout[LEFT] = 0;
                g_stat[LEFT] = STAT_IDLE;
            }
            g_timeout[LEFT]++;
            break;
        case STAT_PROC:
            // The data belongs to this grid
            if(g_data[LEFT][0] == g_id)
            {
                // Getting grid data and setting grid
                char l_char = g_data[LEFT][1];
                s_rgb l_char_rgb = {g_data[LEFT][2], g_data[LEFT][3],
                        g_data[LEFT][4]};
                s_rgb l_bg_rgb = {g_data[LEFT][5], g_data[LEFT][6],
                        g_data[LEFT][7]};
                f_set_grid(l_char, l_char_rgb, l_bg_rgb);
                // Resetting left communications
                g_stat[LEFT] = STAT_IDLE;
            }
            // The data belongs to the next grid
            else if(g_data[LEFT][0] > g_id)
            {
                // Only pass grid data on if transmission not active
                if(!g_new_data)
                {
                    // Copy data and start transmission to right grid
                    memcpy(g_data[RIGHT], g_data[LEFT], TMSN_SIZE);
                    g_new_data = true;
                    // Resetting left communications
                    g_stat[LEFT] = STAT_IDLE;
                }
            }
            else 
            {
                g_stat[LEFT] = STAT_IDLE;
            }
            break;
    }
    f_proc_receive_bytes(LEFT);
    f_proc_send_bytes(LEFT);
}

// Handles communications on the right side
void f_right_comms(void)
{
    // Constantly checks for messages from the right module
    f_receive_bytes(RIGHT, &g_rxmsg[RIGHT], 1);
    // Handles request for module ID
    if(g_rxmsg[RIGHT] == MSG_ID)
    {
        g_rxmsg[RIGHT] = MSG_IDLE;
        f_stop_receive(RIGHT);
        f_stop_send(RIGHT);
        while(!f_send_bytes(RIGHT, &g_id, 1))
        {f_proc_send_bytes(RIGHT);}
        g_stat[RIGHT] = STAT_IDLE;
    }
    // Handles error message
    else if(g_rxmsg[RIGHT] == MSG_ERR)
    {
        g_rxmsg[RIGHT] = MSG_IDLE;
        f_stop_receive(LEFT);
        f_stop_send(LEFT);
        f_stop_receive(RIGHT);
        f_stop_send(RIGHT);
        // Error message will eventually reach the controller module
        g_txmsg[LEFT] = MSG_ERR;
        while(!f_send_bytes(LEFT, &g_txmsg[LEFT], 1))
        {f_proc_send_bytes(LEFT);}
        g_stat[RIGHT] = STAT_IDLE;
    }
    switch(g_stat[RIGHT])
    {
        case STAT_IDLE:
            // Sends a ready message if new data is present 
            if(g_rxmsg[RIGHT] == MSG_IDLE)
            {
                if(g_new_data)
                {
                    g_txmsg[RIGHT] = MSG_RDY;
                    while(!f_send_bytes(RIGHT, &g_txmsg[RIGHT], 1))
                    {f_proc_send_bytes(RIGHT);}
                    g_stat[RIGHT] = STAT_PEND;
                }
            }
            break;
        case STAT_PEND:
            // Confirmation message has been received
            if(g_rxmsg[RIGHT] == MSG_RDY)
            {
                // Sends data to the right module 
                while(!f_send_bytes(RIGHT, g_data[RIGHT], TMSN_SIZE))
                {f_proc_send_bytes(RIGHT);}
                g_timeout[RIGHT] = 0;
                g_stat[RIGHT] = STAT_PROC;
            }
            // Confirmation message has not been received and timed out
            else
            {
                if(g_timeout[RIGHT] > TIMEOUT_R)
                {
                    g_timeout[RIGHT] = 0;
                    g_stat[RIGHT] = STAT_IDLE;
                }
                g_timeout[RIGHT]++;
            }
            break;
        case STAT_PROC:
            // Resets communication when all data has been sent
            if(f_can_send(RIGHT))
            {
                if(g_rxmsg[RIGHT] == MSG_IDLE)
                {
                    g_new_data = false;
                    g_stat[RIGHT] = STAT_IDLE;
                }
            }
            break;
    }
    f_proc_receive_bytes(RIGHT);
    f_proc_send_bytes(RIGHT);
}

// Main function that include the setup and loop
void main(void) {
    // Calling initialization functions for the communications and grid
    f_init_comms();
    f_init_grid();
    // Getting device ID
    f_get_id();
    //f_set_grid('$', (s_rgb){0xFF, 0xFF, 0xFF}, (s_rgb){0x00, 0x00, 0x00});
    // Main loop
    while(1 == 1)
    {
        // Refresh rate is set at 120 Hz
        // In between refresh cycles the PIC is allowed to communicate
        if(f_can_scan_led())
        {
            if(g_stat[LEFT] == STAT_IDLE && g_stat[RIGHT] == STAT_IDLE)
            {
                f_scan_led();
            }
            else
            {
                f_left_comms();
                f_right_comms();
            }
        }
        else
        {
            f_left_comms();
            f_right_comms();
        }
    }
    return;
}
