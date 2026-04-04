#ifdef ESP_PLATFORM

#include "sha256_hw.h"
#include "soc/hwcrypto_reg.h"
#include "soc/soc.h"
#include "soc/periph_defs.h"
#include "esp_private/periph_ctrl.h"
#include "esp_attr.h"

// Volatile pointers for direct register access
#define SHA_H_REG   ((volatile uint32_t *)SHA_H_BASE)
#define SHA_TEXT_REG ((volatile uint32_t *)SHA_TEXT_BASE)

// ESP32-S3 SHA hardware stores registers as raw bytes in memory-mapped IO.
// On the LE Xtensa core, reading/writing uint32_t gives the native LE
// representation.  SHA_TEXT receives message words as raw LE casts of the
// byte stream (the hardware handles BE conversion internally).  SHA_H
// stores the hash state the same way — memcpy of SHA_H to a byte buffer
// yields correct BE hash bytes (this is how ESP-IDF's mbedTLS port works).
//
// Our software SHA and mining code use *standard* SHA-256 word values
// (H0 = 0x6a09e667, etc.).  To convert between standard and HW format
// we bswap32 on every SHA_H read/write.  SHA_TEXT needs no swapping —
// we cast byte buffers to uint32_t* and write directly, matching ESP-IDF.

void sha256_hw_init(void)
{
    periph_module_enable(PERIPH_SHA_MODULE);
    REG_WRITE(SHA_MODE_REG, 2);  // SHA-256
}

IRAM_ATTR void sha256_hw_transform(uint32_t state[8], const uint8_t block[64])
{
    // Write state to SHA_H (bswap: standard → HW format)
    for (int i = 0; i < 8; i++) {
        SHA_H_REG[i] = __builtin_bswap32(state[i]);
    }

    // Write block to SHA_TEXT (no bswap — raw LE cast matches HW expectation)
    const uint32_t *w = (const uint32_t *)block;
    for (int i = 0; i < 16; i++) {
        SHA_TEXT_REG[i] = w[i];
    }

    // Continue from existing state
    REG_WRITE(SHA_CONTINUE_REG, 1);
    while (REG_READ(SHA_BUSY_REG)) {}

    // Read result (bswap: HW format → standard)
    for (int i = 0; i < 8; i++) {
        state[i] = __builtin_bswap32(SHA_H_REG[i]);
    }
}

IRAM_ATTR void sha256_hw_transform_start(uint32_t state[8], const uint8_t block[64])
{
    // Write block to SHA_TEXT (no bswap)
    const uint32_t *w = (const uint32_t *)block;
    for (int i = 0; i < 16; i++) {
        SHA_TEXT_REG[i] = w[i];
    }

    // Start fresh (H0 auto-seeded by hardware)
    REG_WRITE(SHA_START_REG, 1);
    while (REG_READ(SHA_BUSY_REG)) {}

    // Read result (bswap: HW format → standard)
    for (int i = 0; i < 8; i++) {
        state[i] = __builtin_bswap32(SHA_H_REG[i]);
    }
}

// --- Phase 2: Optimized mining functions ---

void sha256_hw_init_job(const uint8_t block2[64])
{
    // Prime SHA_TEXT with block2 (no bswap — raw LE cast)
    const uint32_t *w = (const uint32_t *)block2;
    for (int i = 0; i < 16; i++) {
        SHA_TEXT_REG[i] = w[i];
    }
}

IRAM_ATTR void sha256_hw_mine_first(uint32_t state[8],
                                     const uint32_t midstate[8],
                                     const uint8_t block2[64],
                                     uint32_t nonce)
{
    const uint32_t *w = (const uint32_t *)block2;

    // Write midstate to SHA_H (bswap: standard → HW format)
    for (int i = 0; i < 8; i++) {
        SHA_H_REG[i] = __builtin_bswap32(midstate[i]);
    }

    // Rewrite all 16 SHA_TEXT words (hardware overwrites during schedule expansion).
    // Words 0-2: header tail constants (no bswap — raw LE cast from block2)
    SHA_TEXT_REG[0] = w[0];
    SHA_TEXT_REG[1] = w[1];
    SHA_TEXT_REG[2] = w[2];

    // Word 3: nonce — block2 stores nonce LE at bytes 12-15, so the raw
    // LE uint32 cast of those bytes equals the nonce value.
    SHA_TEXT_REG[3] = nonce;

    // Word 4: 0x80 padding byte at position 16 → LE uint32 = 0x00000080
    SHA_TEXT_REG[4] = 0x00000080;

    // Words 5-14: zeros
    SHA_TEXT_REG[5] = 0;
    SHA_TEXT_REG[6] = 0;
    SHA_TEXT_REG[7] = 0;
    SHA_TEXT_REG[8] = 0;
    SHA_TEXT_REG[9] = 0;
    SHA_TEXT_REG[10] = 0;
    SHA_TEXT_REG[11] = 0;
    SHA_TEXT_REG[12] = 0;
    SHA_TEXT_REG[13] = 0;
    SHA_TEXT_REG[14] = 0;

    // Word 15: bit length 640 = 0x280 → bytes [0x00,0x00,0x02,0x80] → LE uint32
    SHA_TEXT_REG[15] = 0x80020000;

    // Continue from midstate
    REG_WRITE(SHA_CONTINUE_REG, 1);
    while (REG_READ(SHA_BUSY_REG)) {}

    // Read first-hash result (bswap: HW format → standard)
    for (int i = 0; i < 8; i++) {
        state[i] = __builtin_bswap32(SHA_H_REG[i]);
    }
}

IRAM_ATTR uint32_t sha256_hw_mine_second(uint32_t state[8], uint32_t target_word0)
{
    // Write first-hash state to SHA_TEXT as the 32-byte message for the second hash.
    // state[] is in standard format; bswap to get the raw LE byte representation
    // that SHA_TEXT expects (hash byte 0 in low byte of word 0, etc.).
    for (int i = 0; i < 8; i++) {
        SHA_TEXT_REG[i] = __builtin_bswap32(state[i]);
    }

    // Word 8: 0x80 padding byte at position 32 → LE uint32 = 0x00000080
    SHA_TEXT_REG[8] = 0x00000080;

    // Words 9-14: zeros
    SHA_TEXT_REG[9] = 0;
    SHA_TEXT_REG[10] = 0;
    SHA_TEXT_REG[11] = 0;
    SHA_TEXT_REG[12] = 0;
    SHA_TEXT_REG[13] = 0;
    SHA_TEXT_REG[14] = 0;

    // Word 15: bit length 256 = 0x100 → bytes [0x00,0x00,0x01,0x00] → LE uint32
    SHA_TEXT_REG[15] = 0x00010000;

    // Start fresh (H0 auto-seeded)
    REG_WRITE(SHA_START_REG, 1);
    while (REG_READ(SHA_BUSY_REG)) {}

    // Early reject: read only SHA_H[7] (MSB word in LE convention) for target comparison
    uint32_t h7 = __builtin_bswap32(SHA_H_REG[7]);
    if (h7 > target_word0) {
        return h7;
    }

    // Potential hit — read full digest (bswap to standard)
    for (int i = 0; i < 7; i++) {
        state[i] = __builtin_bswap32(SHA_H_REG[i]);
    }
    state[7] = h7;
    return h7;
}

#endif // ESP_PLATFORM
