#ifdef ASIC_BM1368

#include "asic.h"
#include "bm137x_regs.h"
#include "bm1368.h"
#include "tps546.h"
#include "board.h"
#include "bb_system.h"

static bb_err_t chip_init(void)
{
    return bm137x_chip_init(&bm1368_regs,
                            (float)BM1368_DEFAULT_FREQ_MHZ,
                            BM1368_CHIP_COUNT);
}

static bb_err_t chip_quiesce(void)
{
    return bm137x_chip_quiesce(&bm1368_regs);
}

static bb_err_t chip_resume(void)
{
    return chip_init();
}

static esp_err_t tps546_vreg_init(i2c_master_bus_handle_t bus, uint16_t target_mv)
{
    return tps546_init(bus, TPS546_I2C_ADDR, target_mv);
}

static const asic_chip_ops_t s_bm1368_ops = {
    .chip_init        = chip_init,
    .chip_quiesce     = chip_quiesce,
    .chip_resume      = chip_resume,
    .vreg_init        = tps546_vreg_init,
    .fb_min           = BM1368_FB_MIN,
    .fb_max           = BM1368_FB_MAX,
    .default_mv       = BM1368_DEFAULT_MV,
    .default_freq_mhz = BM1368_DEFAULT_FREQ_MHZ,
    .chip_count       = BM1368_CHIP_COUNT,
    .chip_id          = BM1368_CHIP_ID,
    .job_interval_ms  = BM1368_JOB_INTERVAL_MS,
};

const asic_chip_ops_t *g_chip_ops = &s_bm1368_ops;

#endif // ASIC_BM1368
