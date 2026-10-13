/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
#ifndef UART_LOG_H
#define UART_LOG_H

#include "FreeRTOS.h"
#include "queue.h"

/* Every received byte goes to this queue (filled from the interrupt, read by CommandTask). */
extern QueueHandle_t g_qUartRx;

void UartLog_Init(void);           /* creates the mutex and the queue */
void UartLog_StartRx(void);        /* starts interrupt-driven reception */

/* printf that is safe across tasks: all output is serialized by one mutex. */
void Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
