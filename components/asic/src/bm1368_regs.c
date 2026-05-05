#ifdef ASIC_BM1368

#include "bm137x_regs.h"
#include "bm1368.h"
#include "board.h"

const bm137x_regs_t bm1368_regs = {
    .tag                        = "bm1368",
    .fb_min                     = 144,
    .fb_max                     = 235,
    .misc_ctrl_b                = {0xFF, 0x0F, 0xC1, 0x00},
    .core_ctrl2_b3              = 0x18,
    .io_drv_b                   = {0x02, 0x11, 0x11, 0x11},
    .core_ctrl2_per_chip_b3     = 0x18,
    .analog_mux_b3              = 0x03,
    .freq_before_baud           = false,
    .addr_interval_from_detected = false,
    .nominal_chip_count         = BM1368_CHIP_COUNT,
};

#endif // ASIC_BM1368
