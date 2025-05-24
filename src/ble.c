#include <stdio.h>

#include <pico/cyw43_arch.h>
#include <pico/btstack_cyw43.h>
#include <btstack.h>
#include <ble/gatt-service/nordic_spp_service_server.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <queue.h>

#include "mygatt.h"

#include "ble.h"
#include "ws2812.h"

static btstack_timer_source_t heartbeat;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static btstack_context_callback_registration_t send_request;
static btstack_packet_callback_registration_t hci_event_callback_registration;

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06, 
    // Name
    6, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'D', 'I', 'S','C', 'O',
    // UUID ...
    17, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x9e, 0xca, 0xdc, 0x24, 0xe, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x1, 0x0, 0x40, 0x6e,
};
const uint8_t adv_data_len = sizeof(adv_data);

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;
    switch(hci_event_packet_get_type(packet)){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            break;
        default:
            break;
    }
}

static void nordic_spp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    switch (packet_type){
        case HCI_EVENT_PACKET:
            if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) break;
            switch (hci_event_gattservice_meta_get_subevent_code(packet)){
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
                    con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                    printf("Nordic SPP connected, con handle 0x%04x\n", con_handle);
                    break;
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
                    printf("Nordic SPP disconnected, con handle 0x%04x\n", con_handle);
                    con_handle = HCI_CON_HANDLE_INVALID;
                    break;
                default:
                    break;
            }
            break;
        case RFCOMM_DATA_PACKET:
            printf("RECV: ");
            printf_hexdump(packet, size);
            switch(packet[0]) {
                case 1:

                    if (xQueueSend(command_queue, packet+1, 0) != pdTRUE) {
                        printf("Failed to send command to queue - discarding.\n");
                        break;
                    }
                    switch(packet[1]) {
                        case 1: {
                            printf("Red\n");
                            break;
                        }
                        case 2: {
                            printf("Green\n");
                            break;
                        }
                        case 3: {
                            printf("Blue\n");
                            break;
                        }
                    }
                    break;
                default:
                    printf("Unknown Command\n");
                    break;
            }
            break;
        default:
            break;
    };
}

static void configure_advertising() {
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);
}

static void setup_ble()
{
    int result = cyw43_arch_init();
    if (result) {
        printf("Failed to initialize CYW43_ARCH: %d\n", result);
        while (true);
    }
    printf("CYW43_ARCH initialized\n");

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();
    sm_init();
    att_server_init(profile_data, NULL, NULL);
    nordic_spp_service_server_init(&nordic_spp_packet_handler);

    configure_advertising();

    result = hci_power_control(HCI_POWER_ON);
    if(result) {
        printf("Error enabling Bluetooth: %d\n", result);
    }
}

static void server_task(void *ws2812_task)
{
    printf("BLE Task has started\n");
    setup_ble();
    printf("Resuming WS2812 Task: %p\n", ws2812_task);
    xSemaphoreGive(initSync);
    vTaskSuspend( NULL );
}

void ble_init(TaskHandle_t* ws2812_task, TaskHandle_t* created_task)
{
    int result = xTaskCreate(server_task, "BLEserver", 2048, ws2812_task, tskIDLE_PRIORITY + 2, created_task);
}