#ifndef __WS2812_H__
#define __WS2812_H__

#include <FreeRTOS.h>
#include <task.h>

#include "patterns.h"

void ws2812_init(TaskHandle_t* created_task);
extern QueueHandle_t command_queue;

typedef struct {
    pattern_t *pattern;
} state_t;

#endif // __WS2812_H__