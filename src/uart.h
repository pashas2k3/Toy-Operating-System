/**
 * uart.h
 * UART routines for polled serial I/O
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-251 Fall 2012, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef _UART_H
#define _UART_H

#include "derivative.h"

/* IRQ for UART2 status sources is 49 */
#define UART2_STATUS_IRQ_NUMBER 49
/* Interrupt priority for UART2 status sources is 7 (range is 0 to 15) */
#define UART2_STATUS_INTERRUPT_PRIORITY 7

/* IRQ for UART2 error sources is 50 */
#define UART2_ERROR_IRQ_NUMBER 50
/* Interrupt priority for UART2 error sources is 7 (range is 0 to 15) */
#define UART2_ERROR_INTERRUPT_PRIORITY 7

void uartInit(UART_MemMapPtr uartChannel, int clockInKHz, int baud);
char uartGetchar(UART_MemMapPtr uartChannel);
void uartPutchar(UART_MemMapPtr uartChannel, char ch);
int uartGetcharPresent(UART_MemMapPtr uartChannel);
void uartPuts(UART_MemMapPtr uartChannel, char *p);

#endif /* ifndef _UART_H */
