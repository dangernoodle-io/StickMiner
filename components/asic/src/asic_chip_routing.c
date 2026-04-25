#include "asic_chip_routing.h"

int asic_chip_routing_index(uint8_t asic_addr, int chip_count)
{
    if (chip_count <= 0) return -1;
    if (chip_count > 256) return -1;

    /* uint16_t arithmetic — 256/1 overflows uint8_t to 0 and divides by zero. */
    uint16_t addr_interval = (uint16_t)(256 / chip_count);
    if (addr_interval == 0) return -1;

    int chip_idx = (int)(asic_addr / addr_interval);
    if (chip_idx < 0 || chip_idx >= chip_count) return -1;
    return chip_idx;
}
