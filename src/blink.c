#include <stdio.h>                                    
                                                                    
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
                                                         
#include <FreeRTOS.h>
#include <task.h>

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
    cyw43_arch_init();
    
    // while (1) {                                                            
    //     printf("blink\n");
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    //     sleep_ms(100);                                            
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);        
    //     sleep_ms(100);                                      
    // }                     
    
    xTaskCreate(vBlinkTask, "Blink Task", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
    
    return 0;
}