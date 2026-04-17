#pragma once

// BITDSK N8-T: Fixed-voltage board (no VRM), BM1397 ASIC with I2C display support
#define BOARD_NAME "bitdsk-n8t"

// BM1397 ASIC (UART1) — pins verified from BLACKBOX firmware disassembly
#define PIN_ASIC_TX        4
#define PIN_ASIC_RX        5
#define ASIC_UART_NUM      1       // UART_NUM_1
#define ASIC_BAUD_INIT     115200
#define ASIC_BAUD_FAST     3125000

// ASIC control
#define PIN_ASIC_RST       1       // active-low reset
#define PIN_ASIC_EN        10      // power enable

// I2C bus — pins verified from BLACKBOX firmware disassembly
#define PIN_I2C_SDA        6
#define PIN_I2C_SCL        7
#define I2C_BUS_SPEED_HZ   400000
#define I2C_BUS_NUM         0

// EMC2101 fan/temperature controller (reserved for future use)
#define EMC2101_I2C_ADDR   0x4C

// SSD1306 OLED display (reserved for future use)
#define SSD1306_I2C_ADDR   0x3C

// ADC power monitoring
#define PIN_ADC_VMON       2       // ADC1_CH1

// BOOT button
#define PIN_BOOT_BTN       0

// BM1397 operating parameters
#define BM1397_DEFAULT_FREQ_MHZ 330    // initial PLL target frequency (MHz)
#define BM1397_CHIP_COUNT       1      // number of BM1397 chips in chain
#define BM1397_JOB_INTERVAL_MS  500    // job dispatch interval (ms)
