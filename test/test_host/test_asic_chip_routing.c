#include "unity.h"
#include "asic_chip_routing.h"

void test_chip_routing_single_chip_addr_zero(void)
{
    /* Single chip — address 0 → chip_idx 0. Naive 256/1=256 path would
       overflow uint8_t to 0 and divide by zero; helper handles it. */
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0x00, 1));
}

void test_chip_routing_single_chip_any_addr_returns_zero(void)
{
    /* All addresses route to chip 0 when chip_count == 1. */
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0x80, 1));
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0xFF, 1));
}

void test_chip_routing_two_chip_addr_zero(void)
{
    /* Two chips: addr_interval = 128. addr 0 → chip 0. */
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0x00, 2));
}

void test_chip_routing_two_chip_addr_below_boundary(void)
{
    /* Two chips: 0x7F = 127 → chip 0 (127/128 = 0). */
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0x7F, 2));
}

void test_chip_routing_two_chip_addr_at_boundary(void)
{
    /* 0x80 = 128 → chip 1 (128/128 = 1). */
    TEST_ASSERT_EQUAL_INT(1, asic_chip_routing_index(0x80, 2));
}

void test_chip_routing_two_chip_addr_top(void)
{
    /* 0xFF → chip 1 (255/128 = 1). */
    TEST_ASSERT_EQUAL_INT(1, asic_chip_routing_index(0xFF, 2));
}

void test_chip_routing_four_chips(void)
{
    /* addr_interval = 64. */
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0x00, 4));
    TEST_ASSERT_EQUAL_INT(0, asic_chip_routing_index(0x3F, 4));  /* 63/64=0 */
    TEST_ASSERT_EQUAL_INT(1, asic_chip_routing_index(0x40, 4));  /* 64/64=1 */
    TEST_ASSERT_EQUAL_INT(2, asic_chip_routing_index(0x80, 4));  /* 128/64=2 */
    TEST_ASSERT_EQUAL_INT(3, asic_chip_routing_index(0xC0, 4));  /* 192/64=3 */
    TEST_ASSERT_EQUAL_INT(3, asic_chip_routing_index(0xFF, 4));  /* 255/64=3 */
}

void test_chip_routing_invalid_chip_count_zero(void)
{
    TEST_ASSERT_EQUAL_INT(-1, asic_chip_routing_index(0x00, 0));
}

void test_chip_routing_invalid_chip_count_negative(void)
{
    TEST_ASSERT_EQUAL_INT(-1, asic_chip_routing_index(0x00, -1));
}

void test_chip_routing_invalid_chip_count_too_large(void)
{
    TEST_ASSERT_EQUAL_INT(-1, asic_chip_routing_index(0x00, 257));
}
