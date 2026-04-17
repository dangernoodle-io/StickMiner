// ASIC stubs for boards without full asic_task.c compilation (e.g., BM1397 pending TA-48)
// Provides asic_get/set_i2c_bus for display.c I2C initialization.
// Only compiled if asic_task.c is NOT present (i.e., when neither BM1370 nor BM1368 is defined).

#if !defined(ASIC_BM1370) && !defined(ASIC_BM1368)

#include "driver/i2c_master.h"

static i2c_master_bus_handle_t s_i2c_bus = NULL;

i2c_master_bus_handle_t asic_get_i2c_bus(void)
{
    return s_i2c_bus;
}

void asic_set_i2c_bus(i2c_master_bus_handle_t bus)
{
    s_i2c_bus = bus;
}

#endif
