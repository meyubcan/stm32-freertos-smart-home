#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

extern uint32_t SystemCoreClock;
void vAssertCalled(const char *file, int line);

#define configUSE_PREEMPTION                      1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION   1
#define configUSE_IDLE_HOOK                       0
#define configUSE_TICK_HOOK                       0
#define configCPU_CLOCK_HZ                        (SystemCoreClock)
#define configTICK_RATE_HZ                        ((TickType_t)1000)
#define configMAX_PRIORITIES                      7
#define configMINIMAL_STACK_SIZE                  ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                     ((size_t)(40 * 1024))
#define configMAX_TASK_NAME_LEN                   12
#define configUSE_16_BIT_TICKS                    0
#define configIDLE_SHOULD_YIELD                   1
#define configUSE_MUTEXES                         1
#define configUSE_RECURSIVE_MUTEXES               0
#define configUSE_COUNTING_SEMAPHORES             0
#define configQUEUE_REGISTRY_SIZE                 0
#define configUSE_TIMERS                          0
#define configUSE_TRACE_FACILITY                  0
#define configSUPPORT_STATIC_ALLOCATION           0
#define configSUPPORT_DYNAMIC_ALLOCATION          1
#define configENABLE_BACKWARD_COMPATIBILITY       0

/* Hata yakalama: stack taşması ve bellek yetersizliği kancaları hooks.c içinde. */
#define configCHECK_FOR_STACK_OVERFLOW            2
#define configUSE_MALLOC_FAILED_HOOK              1
#define configASSERT(x)                           do { if ((x) == 0) { vAssertCalled(__FILE__, __LINE__); } } while (0)

/* Kullanılan API'ler */
#define INCLUDE_vTaskDelay                        1
#define INCLUDE_vTaskDelayUntil                   1
#define INCLUDE_vTaskDelete                       0
#define INCLUDE_vTaskSuspend                      1
#define INCLUDE_xTaskGetSchedulerState            1
#define INCLUDE_uxTaskGetStackHighWaterMark       1

/* Cortex-M4: 4 öncelik biti. Bu değerin altındaki (sayısal olarak küçük) kesmeler
   FreeRTOS API'si çağıramaz. */
#define configPRIO_BITS                           4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY           (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY      (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* FreeRTOS kesme işleyicileri CMSIS'in vektör adlarına bağlanır. */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif
