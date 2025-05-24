#ifndef __BLE_H__
#define __BLE_H__

#include <FreeRTOS.h>
#include <task.h>

void ble_init(TaskHandle_t* ws2812_task, TaskHandle_t* created_task);
extern SemaphoreHandle_t initSync;

#endif // __BLE_H__