/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <ws2812.pio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>

#include "patterns.h"
#include "ws2812.h"

extern SemaphoreHandle_t initSync;

#define FRAC_BITS 4
#define NUM_PIXELS 102
#define WS2812_PIN_BASE 28

// Check the pin is compatible with the platform
#if WS2812_PIN_BASE >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

static uint8_t DMA_CHANNEL;
static uint8_t DMA_CB_CHANNEL;
static uint8_t DMA_CHANNEL_MASK;
static uint8_t DMA_CB_CHANNEL_MASK;
static uint8_t DMA_CB_CHANNELS_MASK;


#define VALUE_PLANE_COUNT (8 + FRAC_BITS)
// we store value (8 bits + fractional bits of a single color (R/G/B/W) value) for multiple
// strips of pixels, in bit planes. bit plane N has the Nth bit of each strip of pixels.
typedef struct {
    // stored MSB first
    uint32_t planes[VALUE_PLANE_COUNT];
} value_bits_t;

// Add FRAC_BITS planes of e to s and store in d
void add_error(value_bits_t *d, const value_bits_t *s, const value_bits_t *e) {
    uint32_t carry_plane = 0;
    
    // add the FRAC_BITS low planes
    for (int p = VALUE_PLANE_COUNT - 1; p >= 8; p--) {
        uint32_t e_plane = e->planes[p];
        uint32_t s_plane = s->planes[p];
        d->planes[p] = (e_plane ^ s_plane) ^ carry_plane;
        carry_plane = (e_plane & s_plane) | (carry_plane & (s_plane ^ e_plane));
    }

    // then just ripple carry through the non fractional bits
    for (int p = 7; p >= 0; p--) {
        uint32_t s_plane = s->planes[p];
        d->planes[p] = s_plane ^ carry_plane;
        carry_plane &= s_plane;
    }
}

typedef struct {
    uint8_t *data;
    uint data_len;
    uint frac_brightness; // 256 = *1.0;
} strip_t;

// takes 8 bit color values, multiply by brightness and store in bit planes
void transform_strips(strip_t **strips, uint num_strips, value_bits_t *values, uint value_length,
                       uint frac_brightness) {
    for (uint v = 0; v < value_length; v++) {
        memset(&values[v], 0, sizeof(values[v]));
        for (uint i = 0; i < num_strips; i++) {
            if (v < strips[i]->data_len) {
                // todo clamp?
                uint32_t value = (strips[i]->data[v] * strips[i]->frac_brightness) >> 8u;
                value = (value * frac_brightness) >> 8u;
                for (int j = 0; j < VALUE_PLANE_COUNT && value; j++, value >>= 1u) {
                    if (value & 1u) values[v].planes[VALUE_PLANE_COUNT - 1 - j] |= 1u << i;
                }
            }
        }
    }
}

void dither_values(const value_bits_t *colors, value_bits_t *state, const value_bits_t *old_state, uint value_length) {
    for (uint i = 0; i < value_length; i++) {
        add_error(state + i, colors + i, old_state + i);
    }
}

// requested colors * 4 to allow for RGBW
static value_bits_t colors[NUM_PIXELS * 4];
// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version
static value_bits_t states[2][NUM_PIXELS * 4];

// example - strip 0 is RGB only
static uint8_t strip0_data[NUM_PIXELS * 3];

strip_t strip0 = {
        .data = strip0_data,
        .data_len = sizeof(strip0_data),
        .frac_brightness = 0x40,
};

strip_t *strips[] = {
        &strip0,
};

// start of each value fragment (+1 for NULL terminator)
static uintptr_t fragment_start[NUM_PIXELS * 4 + 1];

// posted when it is safe to output a new set of values
static SemaphoreHandle_t reset_delay_complete_sem;

// alarm handle for handling delay
TimerHandle_t reset_delay_alarm_id;

void reset_delay_complete(__unused TimerHandle_t xTimer) {
    reset_delay_alarm_id = 0;
    xSemaphoreGive(reset_delay_complete_sem);
}

void __isr dma_complete_handler() {
    if (dma_hw->ints0 & DMA_CHANNEL_MASK) {
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;
        // when the dma is complete we start the reset delay timer
        xTimerStartFromISR(reset_delay_alarm_id, 0);
    }
}

void dma_init(PIO pio, uint sm) {
    DMA_CHANNEL = dma_claim_unused_channel(true);
    DMA_CB_CHANNEL = dma_claim_unused_channel(true);
    // DMA_CHANNEL = 2;
    // DMA_CB_CHANNEL = 3;
    printf("DMA initialized, channel %u\n", DMA_CHANNEL);
    printf("DMA CB initialized, channel %u\n", DMA_CB_CHANNEL);
    
    DMA_CHANNEL_MASK = 1 << DMA_CHANNEL;
    DMA_CB_CHANNEL_MASK = 1 << DMA_CB_CHANNEL;
    DMA_CB_CHANNELS_MASK = DMA_CHANNEL_MASK | DMA_CB_CHANNEL_MASK;

    // main DMA channel outputs 8 word fragment;
    dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_dreq(&channel_config, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL);
    channel_config_set_irq_quiet(&channel_config, true);
    dma_channel_configure(DMA_CHANNEL,
                          &channel_config,
                          &pio->txf[sm],
                          NULL, // set by chain
                          8, // 8 words for 8 bit planes
                          false);

    // chain channel sends single word pointer to start of fragment each time
    dma_channel_config chain_config = dma_channel_get_default_config(DMA_CB_CHANNEL);
    dma_channel_configure(DMA_CB_CHANNEL,
                          &chain_config,
                          &dma_channel_hw_addr(
                                  DMA_CHANNEL)->al3_read_addr_trig,  // ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
                          NULL, // set later
                          1,
                          false);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);

}

void output_strips_dma(value_bits_t *bits, uint value_length, uint DMA_CB_CHANNEL) {
    for (uint i = 0; i < value_length; i++) {
        fragment_start[i] = (uintptr_t) bits[i].planes; // MSB first
    }
    fragment_start[value_length] = 0;
    dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = (uintptr_t) fragment_start;
}


void ws2812_run_loop_execute() {
    int t = 0;
    uint8_t *current_strip_out;
    bool current_strip_4color;
    
    uint8_t pat = 0;
    uint8_t new_pat;
    BaseType_t ret;
    while (1) {
        // int pat = rand() % pattern_count;
        
        int dir = (rand() >> 30) & 1 ? 1 : -1;
        // if (rand() & 1) dir = 0;
        puts(pattern_table[pat].name);
        puts(dir == 1 ? "(forward)" : dir ? "(backward)" : "(still)");
        int brightness = 0xFF;
        uint current = 0;
        for (int i = 0; i < 1000; ++i) {
            current_strip_out = strip0.data;
            current_strip_4color = false;
            pattern_table[pat].pat(NUM_PIXELS, t, current_strip_out, current_strip_4color);

            transform_strips(strips, count_of(strips), colors, NUM_PIXELS * 4, brightness);
            dither_values(colors, states[current], states[current ^ 1], NUM_PIXELS * 4);
            xSemaphoreTake(reset_delay_complete_sem, portMAX_DELAY);
            output_strips_dma(states[current], NUM_PIXELS * 4, DMA_CB_CHANNEL);

            current ^= 1;
            t += dir;
            brightness++;
            if (brightness == (0x20 << FRAC_BITS)) brightness = 0;
            if (xQueueReceive(command_queue, &new_pat, 0) == pdTRUE) {
                if(new_pat >= pattern_count || pat < 0) {
                    printf("Invalid pattern received: %d. Ignoring.\n", new_pat);
                    continue;
                } else {
                    pat = new_pat + 3;
                    printf("Received new pattern: %s\n", pattern_table[pat].name);
                    i = 0;
                }
            }
        }
        memset(&states, 0, sizeof(states)); // clear out errors
    }
}

void ws2812_task() {
    PIO pio;
    uint sm;
    uint offset;
    
    
    // There appears to be some kind of race condition between btstack initialization and setting up
    // DMAs. We suspend here, and then the btstack task will resume this task after initialization is done.
    xSemaphoreTake(initSync, portMAX_DELAY);
    printf("WS2812 Task has started\n");
    
    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_parallel_program, &pio, &sm, &offset, WS2812_PIN_BASE, count_of(strips), true);
    hard_assert(success);

    ws2812_parallel_program_init(pio, sm, offset, WS2812_PIN_BASE, count_of(strips), 800000);

    reset_delay_complete_sem = xSemaphoreCreateBinary(); 
    xSemaphoreGive(reset_delay_complete_sem); // initially posted so we don't block first time
    if(!reset_delay_complete_sem) {
        printf("Failed to create semaphore\n");
        while(true);
    }

    reset_delay_alarm_id = xTimerCreate("Reset delay", 10, pdTRUE, ( void * ) 0, reset_delay_complete);

    dma_init(pio, sm);
   
    ws2812_run_loop_execute(); // does not return
    
    pio_remove_program_and_unclaim_sm(&ws2812_parallel_program, pio, sm, offset);
}

void ws2812_init(TaskHandle_t* created_task) {
    int result = xTaskCreate(ws2812_task, "WS2812 task", 2048, NULL, tskIDLE_PRIORITY + 2, created_task);
}