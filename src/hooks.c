/* M. Eyub Can, TECH-OHM (https://tech-ohm.com) */
/* FreeRTOS error hooks: on a fault the red LED blinks fast and the system halts. */
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

static void fatal(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
        Board_LedToggle(LED_RED);
        for (volatile uint32_t i = 0; i < 400000U; i++) { }
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    fatal();
}

void vApplicationMallocFailedHook(void)
{
    fatal();
}

void vAssertCalled(const char *file, int line)
{
    (void)file;
    (void)line;
    fatal();
}
