#pragma once

// BM1368 chip identity (Antminer S21)
#define BM1368_CHIP_ID        0x1368
#define BM1368_CORES          64
#define BM1368_SMALL_CORES    16

// PLL fb_div range
#define BM1368_FB_MIN         144
#define BM1368_FB_MAX         235

// Register addresses — same as BM1370 but with different init data
#define BM1368_REG_CHIP_ID    0x00
#define BM1368_REG_PLL        0x08
#define BM1368_REG_HASH_COUNT 0x10
#define BM1368_REG_MISC_CTRL  0x18
#define BM1368_REG_TICKET_MASK 0x14
#define BM1368_REG_FAST_UART  0x28
#define BM1368_REG_CORE_CTRL  0x3C
#define BM1368_REG_ANALOG_MUX 0x54
#define BM1368_REG_IO_DRV     0x58
#define BM1368_REG_VERSION    0xA4
#define BM1368_REG_A8         0xA8
#define BM1368_REG_MISC_SET   0xB9
