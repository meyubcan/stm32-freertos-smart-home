#ifndef UART_LOG_H
#define UART_LOG_H

#include "FreeRTOS.h"
#include "queue.h"

/* Alınan her bayt bu kuyruğa gider (kesmeden doldurulur, CommandTask okur). */
extern QueueHandle_t g_qUartRx;

void UartLog_Init(void);           /* mutex ve kuyruğu oluşturur */
void UartLog_StartRx(void);        /* kesmeyle bayt almayı başlatır */

/* Görevler arası güvenli printf: tüm yazdırmalar tek mutex ile sıralanır. */
void Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
