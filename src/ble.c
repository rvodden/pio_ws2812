#include <stdio.h>

#include <pico/cyw43_arch.h>
#include <pico/btstack_cyw43.h>
#include <btstack.h>
#include <ble/gatt-service/nordic_spp_service_server.h>

#include <FreeRTOS.h>
#include <task.h>

#include "mygatt.h"

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
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(packet);
    UNUSED(size);
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

static void server_task(void *pvParameters)
{
    printf("BLE Task has started\n");
    setup_ble();
    // btstack_run_loop_execute(); /* does not return */
    vTaskDelete( NULL );
}

void ble_init(void)
{
    int result = xTaskCreate(server_task, "BLEserver", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
}