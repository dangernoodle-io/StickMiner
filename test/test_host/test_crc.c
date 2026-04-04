#include "unity.h"
#include "crc.h"
#include <string.h>

void test_crc5_chain_inactive(void) {
    uint8_t data[] = {0x53, 0x05, 0x00, 0x00};
    TEST_ASSERT_EQUAL_HEX8(0x03, crc5(data, sizeof(data)));
}

void test_crc5_reg_write_a8(void) {
    uint8_t data[] = {0x51, 0x09, 0x00, 0xA8, 0x00, 0x07, 0x00, 0x00};
    TEST_ASSERT_EQUAL_HEX8(0x03, crc5(data, sizeof(data)));
}

void test_crc5_reg_write_3c(void) {
    uint8_t data[] = {0x51, 0x09, 0x00, 0x3C, 0x80, 0x00, 0x8B, 0x00};
    TEST_ASSERT_EQUAL_HEX8(0x12, crc5(data, sizeof(data)));
}

void test_crc5_set_address(void) {
    uint8_t data[] = {0x40, 0x05, 0x00, 0x00};
    TEST_ASSERT_EQUAL_HEX8(0x1C, crc5(data, sizeof(data)));
}

void test_crc16_false_standard(void) {
    // "123456789" is the canonical CRC-16/CCITT-FALSE test vector
    const uint8_t data[] = "123456789";
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_false(data, 9));
}

void test_crc16_false_empty(void) {
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_false(NULL, 0));
}
