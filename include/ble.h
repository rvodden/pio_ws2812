// ble.h

void bt_main(void);
struct bt_type{
  uint8_t code;
  uint8_t data;
};

void bt_get_latest(struct bt_type *dst);
void hal_led_on(void);
void hal_led_off(void);
