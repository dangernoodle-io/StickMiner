#pragma once

#include "asic_chip.h"  // defines ASIC_CHIP when ASIC_BM1370 or ASIC_BM1368 is set
#ifdef ASIC_CHIP

#include <stdint.h>
#include <stdbool.h>

// Per-chip register data table for BM137x-family ASICs.
// All register addresses are shared; only init byte values and sequencing flags differ.
typedef struct {
    const char *tag;

    // PLL fb_div range
    uint16_t fb_min;
    uint16_t fb_max;

    // Step 5: MISC_CTRL broadcast bytes
    uint8_t misc_ctrl_b[4];

    // Step 8: second CORE_CTRL broadcast write (b3 differs: 0x0C vs 0x18)
    uint8_t core_ctrl2_b3;

    // Step 10: IO_DRV broadcast bytes
    uint8_t io_drv_b[4];

    // Step 11: per-chip second CORE_CTRL write (b3 mirrors broadcast)
    uint8_t core_ctrl2_per_chip_b3;

    // Step 12: ANALOG_MUX byte (b3 differs: 0x02 vs 0x03)
    uint8_t analog_mux_b3;

    // Sequencing: true = freq ramp before baud switch (BM1370),
    //             false = baud switch before freq ramp (BM1368)
    bool freq_before_baud;

    // Address interval: true = compute from detected chip count (BM1370),
    //                   false = compute from nominal chip count constant (BM1368)
    bool addr_interval_from_detected;
    uint8_t nominal_chip_count;  // used only when addr_interval_from_detected=false
} bm137x_regs_t;

extern const bm137x_regs_t bm1370_regs;
extern const bm137x_regs_t bm1368_regs;

#include "bb_system.h"

bb_err_t bm137x_chip_init(const bm137x_regs_t *r, float target_freq_mhz, int max_chips);
bb_err_t bm137x_chip_quiesce(const bm137x_regs_t *r);

#endif // ASIC_CHIP
