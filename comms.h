#pragma once

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#define LEFT 0
#define RIGHT 1

// Initializes UART control registers
void f_init_comms(void);

// Checks if receiver is open
bool inline f_can_receive(bool a_dir);

// Initializes the UART receiver
bool f_receive_bytes(bool a_dir, uint8_t *a_data, uint8_t a_size);

// Called in a loop to process received bytes
void f_proc_receive_bytes(bool a_dir);

// Terminates current receiver activity
void inline f_stop_receive(bool a_dir);

// Checks if transmitter is open
bool inline f_can_send(bool a_dir);

// Initializers the UART transmitter
bool f_send_bytes(bool a_dir, uint8_t *a_data, uint8_t a_size);

// Called in a loop to process transmitted bytes
void f_proc_send_bytes(bool a_dir);

// Terminates current transmitter activity
void inline f_stop_send(bool a_dir);

