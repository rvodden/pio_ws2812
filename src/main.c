#define CYW43_DEBUG(...) printf(__VA_ARGS__)

#define CYW43_TASK_PRIORITY 5

#include <stdio.h>                                    
                                                                    
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
                                                         
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "ws2812.h"
#include "ble.h"

QueueHandle_t command_queue;
SemaphoreHandle_t initSync;

int main() {
    stdio_init_all();

    printf("Starting Jem's Disco!\n");

    initSync = xSemaphoreCreateBinary();
    if(!initSync) {
        printf("Failed to create semaphore\n");
        while(true);
    }

    command_queue = xQueueCreate(10, sizeof(uint8_t));
    
    TaskHandle_t ws2812_task = NULL;
    TaskHandle_t ble_task = NULL;

    ws2812_init(&ws2812_task);
    ble_init(&ws2812_task, &ble_task);
    vTaskStartScheduler();
    
    return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName ) {
    printf("Stack overflow!\n");
    while (true);
}

void vApplicationMallocFailedHook( void ) {
    printf("Malloc failed!\n");
    while (true);
}

void configure_timer_for_run_time_stats(void){
    // this can remain empty since the SDK already sets up a free timer for us !! 
}

/** \brief This macro should just return the current 'time' 
    \details Current time as defined by timer that has been configured above. 
*/
static configRUN_TIME_COUNTER_TYPE thistime = 0UL;
configRUN_TIME_COUNTER_TYPE get_run_time_counter_value(void){
    thistime = timer_hw->timelr;
    return thistime;
}