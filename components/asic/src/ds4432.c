#ifdef ASIC_BM1368

#include "ds4432.h"
#include "esp_log.h"
#include "esp_check.h"
#include <math.h>

static const char *TAG = "ds4432";

// I2C registers
#define DS4432_REG_OUT0 0xF8
#define DS4432_REG_OUT1 0xF9

// DS4432U+ voltage control calibration
// Hardware: TPS40305 buck converter with voltage divider
// VFB = 0.6V (internal reference)
// RA = 4750 ohms (R14)
// RB = 3320 ohms (R15)
// Full-scale current IFS = 98.921 µA
// VRFS = 0.997
// Nominal voltage (DAC code 0) = VFB * (RA + RB) / RB ≈ 1.459V
// FIXME: verify calibration on hardware

static i2c_master_dev_handle_t s_dev;

// Read one byte from a register
static esp_err_t ds4432_read_byte(uint8_t reg, uint8_t *val)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, val, 1, 100);
}

// Write one byte to a register
static esp_err_t ds4432_write_byte(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev, buf, 2, 100);
}

// Convert millivolts to register byte value
// Voltage divider: vout = VFB * (RA + RB) / RB + (IOUT - IREF) * RA
// where IOUT = (code / 127) * IFS, IREF = VFB / RB
// Rearranged: change = |((VFB/RB) - ((vout - VFB)/RA))| / IFS * 127
// bit7 = (vout < nominal) ? 0x80 : 0x00
// reg = bit7 | (uint8_t)ceil(change)
uint8_t ds4432_mv_to_reg(uint16_t mv)
{
    const float VFB = 0.6f;       // Reference voltage (V)
    const float RA = 4750.0f;     // Feedback divider resistor (ohms)
    const float RB = 3320.0f;     // Ground divider resistor (ohms)
    const float IFS = 0.000098921f; // Full-scale current (A)
    const float VRFS = 0.997f;    // Reference voltage scaling

    // Nominal voltage when DAC code = 0
    float nominal_v = (VFB * (RA + RB) / RB) * VRFS;

    // Convert input to volts
    float vout_v = (float)mv / 1000.0f;

    // Clamp to safe range (1000-1400 mV)
    if (vout_v < 1.0f) vout_v = 1.0f;
    if (vout_v > 1.4f) vout_v = 1.4f;

    // Calculate DAC change needed
    float fb_ref = VFB / RB;
    float output_term = (vout_v - VFB) / RA;
    float current_a = (fb_ref - output_term) / IFS;
    float change = fabs(current_a) * 127.0f;

    // Determine sign bit: 0x80 if vout < nominal, 0x00 otherwise
    uint8_t sign_bit = (vout_v < nominal_v) ? 0x80 : 0x00;

    // Combine sign and magnitude
    uint8_t magnitude = (uint8_t)ceilf(change);
    if (magnitude > 127) magnitude = 127;

    return sign_bit | magnitude;
}

esp_err_t ds4432_init(i2c_master_bus_handle_t bus, uint8_t addr)
{
    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev), TAG, "add device");

    ESP_LOGI(TAG, "DS4432U+ initialized at 0x%02X", addr);
    return ESP_OK;
}

esp_err_t ds4432_set_voltage_mv(uint16_t target_mv)
{
    uint8_t reg_val = ds4432_mv_to_reg(target_mv);
    ESP_RETURN_ON_ERROR(ds4432_write_byte(DS4432_REG_OUT0, reg_val), TAG, "set OUT0");
    ESP_LOGI(TAG, "voltage set to %u mV (reg=0x%02X)", target_mv, reg_val);
    return ESP_OK;
}

#endif // ASIC_BM1368
