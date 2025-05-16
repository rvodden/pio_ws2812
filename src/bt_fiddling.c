#include <btstack_run_loop.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

void btstack_init();

int main() {
    stdio_init_all();

    printf("Started the bluetooth app of joy!\n");

    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }
    
    btstack_init();
    btstack_run_loop_execute();
}