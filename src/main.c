#include <stdio.h>                                    
                                                                    
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
                                                         
#include <FreeRTOS.h>
#include <task.h>

void ble_init();
void ws2812_init();

void vBlinkTask() {                                             
    while (1) {                                                            
        printf("blink\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(100);                                            
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);        
        vTaskDelay(100);                                      
    }                     
}

int main() {
    stdio_init_all();

    printf("Starting Jem's Disco!\n");
    
    int result = cyw43_arch_init();
    if (result) {
        printf("Failed to initialize CYW43_ARCH: %d\n", result);
        while (true);
    }
    
    ble_init();
    ws2812_init();
    vTaskStartScheduler();
    
    return 0;
}