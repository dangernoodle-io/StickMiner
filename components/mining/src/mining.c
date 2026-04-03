#include "mining.h"
#include "sha256.h"
#include "work.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "mining";

// SHA-256 initial hash values
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
};

QueueHandle_t work_queue = NULL;
QueueHandle_t result_queue = NULL;

// Store 32-bit big-endian value
static inline void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

void mining_task(void *arg)
{
    mining_work_t work;
    uint32_t midstate[8];

    // Pre-built SHA-256 blocks (constant padding, only nonce/hash changes)
    // Block 2: tail[16] + 0x80 + zeros + bit_length(640) = 64 bytes
    uint8_t block2[64];
    // Block 3: first_hash[32] + 0x80 + zeros + bit_length(256) = 64 bytes
    uint8_t block3[64];

    // Pre-build block3 padding (constant for all nonces)
    // SHA256(32-byte input): data[32] + 0x80 + 23 zero bytes + 64-bit length (256 bits = 0x100)
    memset(block3, 0, 64);
    block3[32] = 0x80;
    block3[62] = 0x01;  // length = 256 bits = 0x0100 big-endian
    block3[63] = 0x00;

    ESP_LOGI(TAG, "mining task started");

    for (;;) {
        if (xQueueReceive(work_queue, &work, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        ESP_LOGI(TAG, "new job: %s", work.job_id);

        // Compute midstate from first 64 bytes of header
        memcpy(midstate, H0, sizeof(H0));
        sha256_transform(midstate, work.header);

        // Pre-build block2: tail[16] + padding for 80-byte message
        // tail = header[64..79], then 0x80, zeros, bit_length(640 = 80*8)
        memset(block2, 0, 64);
        memcpy(block2, work.header + 64, 16);
        block2[16] = 0x80;
        // bit length of 80 bytes = 640 = 0x0280
        block2[62] = 0x02;
        block2[63] = 0x80;

        int64_t start_us = esp_timer_get_time();
        uint32_t nonce = 0;
        uint32_t hashes = 0;

        for (nonce = 0; ; nonce++) {
            // Set nonce in block2 (bytes 12-15, little-endian)
            block2[12] = (uint8_t)(nonce & 0xff);
            block2[13] = (uint8_t)((nonce >> 8) & 0xff);
            block2[14] = (uint8_t)((nonce >> 16) & 0xff);
            block2[15] = (uint8_t)((nonce >> 24) & 0xff);

            // First SHA-256: clone midstate + transform block2
            uint32_t state[8];
            memcpy(state, midstate, 32);
            sha256_transform(state, block2);

            // Write first hash into block3 (big-endian)
            store_be32(block3,      state[0]);
            store_be32(block3 + 4,  state[1]);
            store_be32(block3 + 8,  state[2]);
            store_be32(block3 + 12, state[3]);
            store_be32(block3 + 16, state[4]);
            store_be32(block3 + 20, state[5]);
            store_be32(block3 + 24, state[6]);
            store_be32(block3 + 28, state[7]);

            // Second SHA-256: H0 + transform block3
            memcpy(state, H0, 32);
            sha256_transform(state, block3);

            // Quick reject: check first 4 bytes of hash (big-endian state[0])
            // For any reasonable difficulty, state[0] must be 0
            if (state[0] == 0) {
                // Full target check — convert state to big-endian bytes
                uint8_t hash[32];
                store_be32(hash,      state[0]);
                store_be32(hash + 4,  state[1]);
                store_be32(hash + 8,  state[2]);
                store_be32(hash + 12, state[3]);
                store_be32(hash + 16, state[4]);
                store_be32(hash + 20, state[5]);
                store_be32(hash + 24, state[6]);
                store_be32(hash + 28, state[7]);

                if (meets_target(hash, work.target)) {
                    mining_result_t result;
                    strncpy(result.job_id, work.job_id, sizeof(result.job_id) - 1);
                    result.job_id[sizeof(result.job_id) - 1] = '\0';
                    strncpy(result.extranonce2_hex, work.extranonce2_hex, sizeof(result.extranonce2_hex) - 1);
                    result.extranonce2_hex[sizeof(result.extranonce2_hex) - 1] = '\0';
                    sprintf(result.ntime_hex, "%08" PRIx32, work.ntime);
                    sprintf(result.nonce_hex, "%08" PRIx32, nonce);

                    ESP_LOGI(TAG, "SHARE FOUND! nonce=%08" PRIx32, nonce);
                    xQueueSend(result_queue, &result, 0);
                }
            }

            hashes++;

            // Every 65536 hashes: check for new work and yield for WDT
            if (((nonce + 1) & 0xFFFF) == 0) {
                if (((nonce + 1) & 0x1FFFFF) == 0) {
                    int64_t elapsed_us = esp_timer_get_time() - start_us;
                    if (elapsed_us > 0) {
                        double hashrate = (double)hashes / ((double)elapsed_us / 1000000.0);
                        ESP_LOGI(TAG, "%.1f H/s (nonce=%08" PRIx32 ")", hashrate, nonce + 1);
                    }
                }

                mining_work_t new_work;
                if (xQueuePeek(work_queue, &new_work, 0) == pdTRUE && new_work.clean) {
                    xQueueReceive(work_queue, &new_work, 0);
                    memcpy(&work, &new_work, sizeof(work));
                    ESP_LOGI(TAG, "new job: %s", work.job_id);
                    memcpy(midstate, H0, sizeof(H0));
                    sha256_transform(midstate, work.header);
                    memset(block2, 0, 64);
                    memcpy(block2, work.header + 64, 16);
                    block2[16] = 0x80;
                    block2[62] = 0x02;
                    block2[63] = 0x80;
                    start_us = esp_timer_get_time();
                    nonce = (uint32_t)-1;
                    hashes = 0;
                    continue;
                }

                vTaskDelay(1);
            }

            if (nonce == UINT32_MAX) break;
        }

        ESP_LOGW(TAG, "exhausted nonce range for job %s", work.job_id);
    }
}
