#pragma once

#ifdef ESP_PLATFORM

#include <stdint.h>
#include "esp_attr.h"

// Enable SHA-256 hardware peripheral clock. Call once at startup.
void sha256_hw_init(void);

// Drop-in hardware replacement for sha256_transform.
// Writes state to SHA_H, bswaps block to SHA_TEXT, SHA_CONTINUE, polls, reads back.
IRAM_ATTR void sha256_hw_transform(uint32_t state[8], const uint8_t block[64]);

// Like sha256_hw_transform but starts fresh (SHA_START auto-seeds H0).
// Ignores incoming state, writes result to state.
IRAM_ATTR void sha256_hw_transform_start(uint32_t state[8], const uint8_t block[64]);

// --- Phase 2: Optimized mining functions ---

// Once per job: prime SHA_TEXT with block2 constants and persistent zeros.
void sha256_hw_init_job(const uint8_t block2[64]);

// Per nonce first hash: write midstate to SHA_H, repair damaged SHA_TEXT words,
// write nonce, SHA_CONTINUE, poll, read result to state.
IRAM_ATTR void sha256_hw_mine_first(uint32_t state[8],
                                     const uint32_t midstate[8],
                                     const uint8_t block2[64],
                                     uint32_t nonce);

// Per nonce second hash: write state[0-7] to SHA_TEXT[0-7], set word 8+15,
// SHA_START, poll. Returns SHA_H[0] for early reject.
// If SHA_H[0] <= target_word0, reads full digest into state[0-7].
IRAM_ATTR uint32_t sha256_hw_mine_second(uint32_t state[8], uint32_t target_word0);

#endif // ESP_PLATFORM
