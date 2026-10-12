/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "FreeRTOS.h"
#include "semphr.h"

/* EXTI0 kesmesi bu semafora verir, ButtonTask bekler. */
extern SemaphoreHandle_t g_semButton;

/* Kuyrukları, mutex'leri ve görevleri oluşturur. Zamanlayıcı main'de başlar. */
void App_Init(void);

#endif
