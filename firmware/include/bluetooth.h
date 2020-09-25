#pragma once

void ble_stack_init(void);
void ble_advertising_start(void);

uint32_t ble_serial_write(uint8_t *data, uint16_t len);
uint32_t ble_kbd_status_write(uint16_t data);
