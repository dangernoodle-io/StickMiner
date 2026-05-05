#ifdef ASIC_BM1370

#include "bm137x_regs.h"

const bm137x_regs_t bm1370_regs = {
    .tag                        = "bm1370",
    .fb_min                     = 160,
    .fb_max                     = 239,
    .misc_ctrl_b                = {0xF0, 0x00, 0xC1, 0x00},
    .core_ctrl2_b3              = 0x0C,
    .io_drv_b                   = {0x00, 0x01, 0x11, 0x11},
    .core_ctrl2_per_chip_b3     = 0x0C,
    .analog_mux_b3              = 0x02,
    .freq_before_baud           = true,
    .addr_interval_from_detected = true,
    .nominal_chip_count         = 0,
};

#endif // ASIC_BM1370
