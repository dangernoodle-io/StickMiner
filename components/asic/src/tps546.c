#include "asic_chip.h"
#ifdef ASIC_CHIP

#include "tps546.h"
#include "tps546_decode.h"
#include "esp_log.h"
#include "bb_log.h"
#include "esp_check.h"
#include <string.h>

static const char *TAG = "tps546";

// PMBus registers
#define PMBUS_CLEAR_FAULTS         0x03
#define PMBUS_OPERATION            0x01
#define PMBUS_VOUT_MODE            0x20
#define PMBUS_VOUT_COMMAND         0x21
#define PMBUS_FREQUENCY_SWITCH     0x33
#define PMBUS_IOUT_OC_FAULT_LIMIT  0x46
#define PMBUS_IOUT_OC_FAULT_RESPONSE 0x47
#define PMBUS_READ_VIN             0x88
#define PMBUS_READ_VOUT            0x8B
#define PMBUS_READ_IOUT            0x8C
#define PMBUS_READ_TEMPERATURE_1   0x8D

// OPERATION values
#define OPERATION_ON  0x80
#define OPERATION_OFF 0x00

// TPS546 design values for Gamma-family (matches AxeOS main/power/TPS546.c).
// NVM defaults from TI datasheet: FREQUENCY_SWITCH=400kHz, OC limit higher
// and retry-based. Programming explicitly gives predictable ripple + fault
// behavior independent of unit-to-unit OTP variation.
#define TPS546_INIT_FREQUENCY_KHZ  650   // kHz
#define TPS546_INIT_IOUT_OC_LIMIT  30    // A
#define TPS546_INIT_OC_RESPONSE    0xC0  // shutdown, no retry

static i2c_master_dev_handle_t s_dev;
static int8_t s_vout_n;  // VOUT_MODE exponent (negative, e.g. -9)

// Read one byte from a PMBus register
static esp_err_t pmbus_read_byte(uint8_t reg, uint8_t *val)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, val, 1, 100);
}

// Write one byte to a PMBus register
static esp_err_t pmbus_write_byte(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev, buf, 2, 100);
}

// Write two bytes (word) to a PMBus register (little-endian)
static esp_err_t pmbus_write_word(uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = {reg, val & 0xFF, (val >> 8) & 0xFF};
    return i2c_master_transmit(s_dev, buf, 3, 100);
}

// PMBus send-byte (command only, no data payload). Used for CLEAR_FAULTS.
static esp_err_t pmbus_send_byte(uint8_t reg)
{
    return i2c_master_transmit(s_dev, &reg, 1, 100);
}

// Encode a small positive integer as SLINEAR11 with exponent 0.
// Mantissa is 11 bits signed; valid range 0..1023 (we use 30 and 650).
static uint16_t slinear11_encode_int(uint16_t value)
{
    return value & 0x07FFu;  // top 5 exponent bits = 0
}

// Encode millivolts to ULINEAR16 using the VOUT_MODE exponent
static uint16_t mv_to_ulinear16(uint16_t mv)
{
    // ULINEAR16: value = voltage / 2^N where N is negative
    // So: code = voltage_V * 2^(-N) = (mv / 1000) * 2^(-N)
    // Using integer math: code = mv * 2^(-N) / 1000
    int shift = -s_vout_n;
    return (uint16_t)((uint32_t)mv * (1U << shift) / 1000);
}

bb_err_t tps546_init(i2c_master_bus_handle_t bus, uint8_t addr, uint16_t target_mv)
{
    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_dev), TAG, "add device");

    // Read VOUT_MODE to get exponent
    uint8_t vout_mode;
    ESP_RETURN_ON_ERROR(pmbus_read_byte(PMBUS_VOUT_MODE, &vout_mode), TAG, "read VOUT_MODE");

    // Extract 5-bit signed exponent from bits[4:0]
    s_vout_n = (int8_t)(vout_mode & 0x1F);
    if (s_vout_n & 0x10) {
        s_vout_n |= 0xE0;  // sign-extend from bit 4
    }
    bb_log_i(TAG, "VOUT_MODE=0x%02X exponent=%d", vout_mode, s_vout_n);

    // Set output voltage
    uint16_t code = mv_to_ulinear16(target_mv);
    ESP_RETURN_ON_ERROR(pmbus_write_word(PMBUS_VOUT_COMMAND, code), TAG, "set VOUT");
    bb_log_i(TAG, "VOUT_COMMAND=0x%04X (%u mV)", code, target_mv);

    // Program switching frequency explicitly (NVM default is 400 kHz; AxeOS
    // sets 650 kHz on every boot). At 400 kHz inductor ripple is ~62% larger,
    // which pumps voltage noise into the BM1370 supply and raises the silicon
    // hash error rate.
    ESP_RETURN_ON_ERROR(pmbus_write_word(PMBUS_FREQUENCY_SWITCH,
                                          slinear11_encode_int(TPS546_INIT_FREQUENCY_KHZ)),
                        TAG, "freq switch");
    bb_log_i(TAG, "switching freq %u kHz", TPS546_INIT_FREQUENCY_KHZ);

    // Over-current fault: 30 A hard limit, shutdown-no-retry response. Without
    // these the TPS546 uses OTP defaults which vary unit-to-unit and may
    // trigger silent fault-and-recover cycles under transient load.
    ESP_RETURN_ON_ERROR(pmbus_write_word(PMBUS_IOUT_OC_FAULT_LIMIT,
                                          slinear11_encode_int(TPS546_INIT_IOUT_OC_LIMIT)),
                        TAG, "oc limit");
    ESP_RETURN_ON_ERROR(pmbus_write_byte(PMBUS_IOUT_OC_FAULT_RESPONSE,
                                          TPS546_INIT_OC_RESPONSE),
                        TAG, "oc response");

    // Clear any latched faults from prior boots before enabling output. On
    // ESP32 soft-reset the TPS546 is not power-cycled, so stale faults can
    // re-assert at OPERATION=ON and glitch VDD as the ASIC is coming up.
    ESP_RETURN_ON_ERROR(pmbus_send_byte(PMBUS_CLEAR_FAULTS), TAG, "clear faults");

    // Power on
    ESP_RETURN_ON_ERROR(pmbus_write_byte(PMBUS_OPERATION, OPERATION_ON), TAG, "power on");
    bb_log_i(TAG, "powered on at %u mV", target_mv);

    return BB_OK;
}

bb_err_t tps546_set_voltage_mv(uint16_t target_mv)
{
    uint16_t code = mv_to_ulinear16(target_mv);
    ESP_RETURN_ON_ERROR(pmbus_write_word(PMBUS_VOUT_COMMAND, code), TAG, "set VOUT");
    bb_log_i(TAG, "VOUT_COMMAND=0x%04X (%u mV)", code, target_mv);
    return BB_OK;
}

static esp_err_t pmbus_read_word(uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];
    esp_err_t err = i2c_master_transmit_receive(s_dev, &reg, 1, buf, 2, 100);
    if (err == ESP_OK) {
        *val = (uint16_t)(buf[0] | ((uint16_t)buf[1] << 8));
    }
    return err;
}

int tps546_read_vout_mv(void)
{
    uint16_t raw;
    if (pmbus_read_word(PMBUS_READ_VOUT, &raw) != ESP_OK) {
        return -1;
    }
    return tps546_ulinear16_to_mv(raw, s_vout_n);
}

int tps546_read_iout_ma(void)
{
    uint16_t raw;
    if (pmbus_read_word(PMBUS_READ_IOUT, &raw) != ESP_OK) {
        return -1;
    }
    return tps546_slinear11_to_ma(raw);
}

int tps546_read_vin_mv(void)
{
    uint16_t raw;
    if (pmbus_read_word(PMBUS_READ_VIN, &raw) != ESP_OK) {
        return -1;
    }
    return tps546_slinear11_to_mv(raw);
}

int tps546_read_temp_c(void)
{
    uint16_t raw;
    if (pmbus_read_word(PMBUS_READ_TEMPERATURE_1, &raw) != ESP_OK) {
        return -1;
    }
    return tps546_slinear11_to_c_int(raw);
}

#endif // ASIC_BM1370 || ASIC_BM1368
