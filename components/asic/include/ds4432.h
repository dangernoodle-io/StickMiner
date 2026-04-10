#pragma once

#ifdef ASIC_BM1368

#include "esp_err.h"
#include "driver/i2c_master.h"

// Initialize DS4432U+ I2C voltage DAC at given address.
esp_err_t ds4432_init(i2c_master_bus_handle_t bus, uint8_t addr);

// Set output voltage in millivolts (clamped to safe range).
esp_err_t ds4432_set_voltage_mv(uint16_t target_mv);

// Pure conversion function: convert mV to register byte value.
// Can be tested without I2C hardware.
uint8_t ds4432_mv_to_reg(uint16_t mv);

#endif // ASIC_BM1368
