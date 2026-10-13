/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "FreeRTOS.h"
#include "semphr.h"

/* The EXTI0 interrupt gives this semaphore and ButtonTask waits on it. */
extern SemaphoreHandle_t g_semButton;

/* Creates the queues, mutexes and tasks. The scheduler is started in main. */
void App_Init(void);

#endif
