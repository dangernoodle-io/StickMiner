#pragma once
#ifdef ASIC_CHIP

#include <stdint.h>
#include <stddef.h>

// Telemetry register map — shared by BM1368 and BM1370 (identical offsets per AxeOS).
// Counters increment by 1 per 2^32 hashes (HASH_CNT_LSB).
#define ASIC_REG_ERROR_COUNT     0x4C
#define ASIC_REG_DOMAIN_0_COUNT  0x88
#define ASIC_REG_DOMAIN_1_COUNT  0x89
#define ASIC_REG_DOMAIN_2_COUNT  0x8A
#define ASIC_REG_DOMAIN_3_COUNT  0x8B
#define ASIC_REG_TOTAL_COUNT     0x8C

// UART helpers (defined in asic_task.c)
int  asic_uart_read(uint8_t *buf, size_t len, uint32_t timeout_ms);
void asic_uart_write(const uint8_t *buf, size_t len);

// Command helpers (defined in asic_task.c)
void send_cmd(uint8_t cmd, uint8_t group, const uint8_t *data, uint8_t data_len);
void write_reg(uint8_t reg, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
void write_reg_chip(uint8_t chip_addr, uint8_t reg, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
void set_ticket_mask(double difficulty);

#endif
