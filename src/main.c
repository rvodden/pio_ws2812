#include <stdio.h>                                    
                                                                    
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
                                                         
#include <FreeRTOS.h>
#include <task.h>

void ble_init();
void ws2812_init();

int main() {
    stdio_init_all();

    printf("Starting Jem's Disco!\n");
    
    
    ble_init();
    ws2812_init();
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