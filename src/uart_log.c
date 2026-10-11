#include "uart_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "semphr.h"

QueueHandle_t g_qUartRx;

static SemaphoreHandle_t s_txMutex;
static uint8_t s_rxByte;
static char s_txBuf[160];   /* mutex altında kullanılır: tek tampon yeter */

void UartLog_Init(void)
{
    s_txMutex = xSemaphoreCreateMutex();
    g_qUartRx = xQueueCreate(64, sizeof(uint8_t));
    configASSERT(s_txMutex != NULL && g_qUartRx != NULL);
}

void UartLog_StartRx(void)
{
    HAL_UART_Receive_IT(&huart2, &s_rxByte, 1);
}

void Log(const char *fmt, ...)
{
    va_list ap;
    int n;

    if (s_txMutex == NULL) {
        return;
    }
    if (xSemaphoreTake(s_txMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }
    va_start(ap, fmt);
    n = vsnprintf(s_txBuf, sizeof s_txBuf, fmt, ap);
    va_end(ap);
    if (n > 0) {
        if ((size_t)n >= sizeof s_txBuf) {
            n = (int)sizeof s_txBuf - 1;
        }
        HAL_UART_Transmit(&huart2, (uint8_t *)s_txBuf, (uint16_t)n, 200);
    }
    xSemaphoreGive(s_txMutex);
}

/* ---- Kesme tarafı ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h)
{
    if (h->Instance == USART2 && g_qUartRx != NULL) {
        BaseType_t woken = pdFALSE;
        /* Kuyruk doluysa bayt düşer, kesme asla bloklanmaz. */
        xQueueSendFromISR(g_qUartRx, &s_rxByte, &woken);
        HAL_UART_Receive_IT(&huart2, &s_rxByte, 1);
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *h)
{
    if (h->Instance == USART2) {
        /* taşma, çerçeve hatası vb.: alımı yeniden başlat */
        HAL_UART_Receive_IT(&huart2, &s_rxByte, 1);
    }
}
