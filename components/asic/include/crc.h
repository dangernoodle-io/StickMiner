#pragma once
#include <stdint.h>

uint8_t crc5(const uint8_t *data, uint8_t len);
uint16_t crc16_false(const uint8_t *data, uint8_t len);
