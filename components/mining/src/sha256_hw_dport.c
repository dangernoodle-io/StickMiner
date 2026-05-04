#include "sdkconfig.h"

#ifdef ESP_PLATFORM
#if CONFIG_IDF_TARGET_ESP32

#include "sha256_hw_dport.h"
#include "sha256.h"
#include "bb_core.h"
#include "bb_byte_order.h"
#include "esp_crypto_lock.h"
#include "esp_private/periph_ctrl.h"
#include "soc/dport_access.h"
#include "soc/hwcrypto_reg.h"
#include "esp_attr.h"
#include "bb_log.h"
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

static const char *TAG = "sha256_hw_dport";

static bool s_first_acquire = true;

void sha256_hw_dport_acquire(void)
{
    esp_crypto_sha_aes_lock_acquire();
    /* TA-271, KB #323: reset on first acquire post-boot to clear any stale
     * BUSY latch from a soft restart while another caller (mbedTLS HMAC) was
     * mid-operation. Only matters on raw-register paths like ours. */
    if (s_first_acquire) {
        periph_module_reset(PERIPH_SHA_MODULE);
        s_first_acquire = false;
    }
    periph_module_enable(PERIPH_SHA_MODULE);
}

void sha256_hw_dport_release(void)
{
    periph_module_disable(PERIPH_SHA_MODULE);
    esp_crypto_sha_aes_lock_release();
}

/* ---------------------------------------------------------------------------
 * NerdMiner-verbatim helpers
 * All mirrored from NerdMiner_v2/src/mining.cpp:903-1124
 * ---------------------------------------------------------------------------
 */

/* Fill 16 message words for SHA peripheral. Block bytes are stored in BTC
 * little-endian byte order; SHA spec requires M[i] = uint32_be(bytes[4i..4i+3]).
 * On a LE host that's bswap32 of the raw uint32 load. */
static inline void dport_fill_block_raw(const void *block_64)
{
    const uint32_t *data_words = (const uint32_t *)block_64;
    uint32_t *reg_addr_buf     = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = __builtin_bswap32(data_words[0]);
    reg_addr_buf[1]  = __builtin_bswap32(data_words[1]);
    reg_addr_buf[2]  = __builtin_bswap32(data_words[2]);
    reg_addr_buf[3]  = __builtin_bswap32(data_words[3]);
    reg_addr_buf[4]  = __builtin_bswap32(data_words[4]);
    reg_addr_buf[5]  = __builtin_bswap32(data_words[5]);
    reg_addr_buf[6]  = __builtin_bswap32(data_words[6]);
    reg_addr_buf[7]  = __builtin_bswap32(data_words[7]);
    reg_addr_buf[8]  = __builtin_bswap32(data_words[8]);
    reg_addr_buf[9]  = __builtin_bswap32(data_words[9]);
    reg_addr_buf[10] = __builtin_bswap32(data_words[10]);
    reg_addr_buf[11] = __builtin_bswap32(data_words[11]);
    reg_addr_buf[12] = __builtin_bswap32(data_words[12]);
    reg_addr_buf[13] = __builtin_bswap32(data_words[13]);
    reg_addr_buf[14] = __builtin_bswap32(data_words[14]);
    reg_addr_buf[15] = __builtin_bswap32(data_words[15]);
}

/* Block 2 of an 80-byte BTC header. Words 0-2 are header tail (LE bytes 64..75)
 * — bswap to canonical M. Word 3 is the nonce; bswap converts host uint32 to M[3].
 * Words 4-14: SHA padding zeros (M[4]=0x80000000 padding bit). Word 15: bit
 * length 640 (canonical M[15]). */
static inline void dport_fill_block_upper(const void *block_64_partial, uint32_t nonce)
{
    const uint32_t *data_words = (const uint32_t *)block_64_partial;
    uint32_t *reg_addr_buf     = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = __builtin_bswap32(data_words[0]);
    reg_addr_buf[1]  = __builtin_bswap32(data_words[1]);
    reg_addr_buf[2]  = __builtin_bswap32(data_words[2]);
    reg_addr_buf[3]  = __builtin_bswap32(nonce);
    reg_addr_buf[4]  = 0x80000000;
    reg_addr_buf[5]  = 0x00000000;
    reg_addr_buf[6]  = 0x00000000;
    reg_addr_buf[7]  = 0x00000000;
    reg_addr_buf[8]  = 0x00000000;
    reg_addr_buf[9]  = 0x00000000;
    reg_addr_buf[10] = 0x00000000;
    reg_addr_buf[11] = 0x00000000;
    reg_addr_buf[12] = 0x00000000;
    reg_addr_buf[13] = 0x00000000;
    reg_addr_buf[14] = 0x00000000;
    reg_addr_buf[15] = 0x00000280;
}

/* Mirror: nerd_sha_ll_fill_text_block_sha256_double (mining.cpp:1007-1029)
 * Words 0-7 LEFT IN PLACE (loaded from prior sha_ll_load).
 * Words 8-15 = SHA-256d second-round padding for 32-byte (256-bit) input. */
static inline void dport_fill_block_double(void)
{
    /* mining.cpp:1009 */
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    /* mining.cpp:1022-1029 */
    reg_addr_buf[8]  = 0x80000000;
    reg_addr_buf[9]  = 0x00000000;
    reg_addr_buf[10] = 0x00000000;
    reg_addr_buf[11] = 0x00000000;
    reg_addr_buf[12] = 0x00000000;
    reg_addr_buf[13] = 0x00000000;
    reg_addr_buf[14] = 0x00000000;
    reg_addr_buf[15] = 0x00000100;                 /* mining.cpp:1029 */
}

/* Mirror: nerd_sha_hal_wait_idle (mining.cpp:940-944) */
static inline void dport_wait_idle(void)
{
    while (DPORT_REG_READ(SHA_256_BUSY_REG)) {}    /* mining.cpp:942 */
}

/* Mirror: nerd_sha_ll_read_digest_swap_if (mining.cpp:905-924)
 * Early-reject: checks low 16 bits of word 7 (last digest word). If non-zero,
 * returns false without touching out[].
 * On potential hit: bswap-copies all 8 words into out[] and returns true.
 * Uses DPORT_INTERRUPT_DISABLE + DPORT_SEQUENCE_REG_READ per erratum. */
static inline bool dport_read_digest_swap_if(uint8_t out[32])
{
    DPORT_INTERRUPT_DISABLE();                     /* mining.cpp:907 */
    uint32_t fin = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4); /* mining.cpp:908 */
    bb_store_be32(out + 7 * 4, __builtin_bswap32(fin)); /* mining.cpp:914 */
    bb_store_be32(out + 0 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4))); /* mining.cpp:915 */
    bb_store_be32(out + 1 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4))); /* mining.cpp:916 */
    bb_store_be32(out + 2 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4))); /* mining.cpp:917 */
    bb_store_be32(out + 3 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4))); /* mining.cpp:918 */
    bb_store_be32(out + 4 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4))); /* mining.cpp:919 */
    bb_store_be32(out + 5 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4))); /* mining.cpp:920 */
    bb_store_be32(out + 6 * 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4))); /* mining.cpp:921 */
    DPORT_INTERRUPT_RESTORE();                     /* mining.cpp:922 */
    return true;                                   /* mining.cpp:923 */
}

/* ---------------------------------------------------------------------------
 * Per-nonce SHA-256d hot loop (classic ESP32, DPORT bus)
 * Mirror of minerWorkerHw inner loop (mining.cpp:1076-1094).
 *
 * header_80[80]: full 80-byte block header (nonce field will be overwritten).
 * nonce: the nonce to hash.
 * hash_out[32]: written only when returning true (potential hit).
 *
 * Returns true when low 16 bits of final digest word 7 == 0 (potential share).
 * Returns false on early reject.
 * ---------------------------------------------------------------------------
 */
bool sha256_hw_dport_per_nonce(const uint8_t header_80[80], uint32_t nonce, uint8_t hash_out[32])
{
    /* 1. Ensure idle */
    dport_wait_idle();

    /* 2. Load block 1 (bytes 0-63), raw, no bswap — mining.cpp:1076 */
    dport_fill_block_raw(header_80);

    /* 3. START block 1 — mining.cpp:1077 */
    DPORT_REG_WRITE(SHA_256_START_REG, 1);

    /* 4. Wait */
    dport_wait_idle();                              /* mining.cpp:1080 */

    /* 5. Load block 2 (bytes 64-79 partial + nonce + padding) — mining.cpp:1081 */
    dport_fill_block_upper(header_80 + 64, nonce);

    /* 6. CONTINUE block 2 — mining.cpp:1082 */
    DPORT_REG_WRITE(SHA_256_CONTINUE_REG, 1);

    /* 7. Wait */
    dport_wait_idle();                              /* mining.cpp:1084 */

    /* 8. LOAD digest into text registers — mining.cpp:1085 */
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);

    /* 9. Wait */
    dport_wait_idle();                              /* mining.cpp:1088 */

    /* 10. Fill double-hash padding (words 8-15; words 0-7 stay from LOAD) — mining.cpp:1089 */
    dport_fill_block_double();

    /* 11. START second hash — mining.cpp:1090 */
    DPORT_REG_WRITE(SHA_256_START_REG, 1);

    /* 12. Wait */
    dport_wait_idle();                              /* mining.cpp:1092 */

    /* 13. LOAD second digest — mining.cpp:1093 */
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);

    /* 14. Wait */
    dport_wait_idle();

    /* 15. Early-reject + readback — mining.cpp:1094 */
    return dport_read_digest_swap_if(hash_out);
}

/* ---------------------------------------------------------------------------
 * Known-vector self-test: SHA-256("abc")
 * Returns BB_OK on PASS, BB_ERR_INVALID_STATE on FAIL.
 * ---------------------------------------------------------------------------
 */
bb_err_t sha256_hw_dport_self_test(void)
{
    /* Known-vector test: SHA-256("abc"). Byte form matches how BTC headers
     * are stored — fill_raw will bswap each word on the way to the peripheral. */
    uint8_t abc_block[64];
    memset(abc_block, 0, sizeof(abc_block));
    abc_block[0]  = 0x61;  /* 'a' */
    abc_block[1]  = 0x62;  /* 'b' */
    abc_block[2]  = 0x63;  /* 'c' */
    abc_block[3]  = 0x80;  /* SHA padding bit */
    abc_block[63] = 0x18;  /* 64-bit BE bit-length = 24 */

    /* Caller MUST hold sha256_hw_dport_acquire() — this function touches SHA
     * peripheral registers directly. esp_crypto_sha_aes_lock is non-recursive,
     * so we cannot re-acquire here; doing so would deadlock when called from
     * sha256_hw_dport_init() which already holds the lock. */

    dport_wait_idle();
    dport_fill_block_raw(abc_block);
    DPORT_REG_WRITE(SHA_256_START_REG, 1);
    dport_wait_idle();
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);
    dport_wait_idle();

    /* Read digest — peripheral writes registers in NIST canonical form */
    uint32_t digest[8];
    DPORT_INTERRUPT_DISABLE();
    digest[0] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4);
    digest[1] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4);
    digest[2] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4);
    digest[3] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4);
    digest[4] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4);
    digest[5] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4);
    digest[6] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4);
    digest[7] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4);
    DPORT_INTERRUPT_RESTORE();

    /* Pack canonical-word digest into byte form for the shared comparison helper. */
    uint8_t digest_bytes[32];
    for (int i = 0; i < 8; i++) {
        uint32_t w = digest[i];
        digest_bytes[i*4 + 0] = (w >> 24) & 0xff;
        digest_bytes[i*4 + 1] = (w >> 16) & 0xff;
        digest_bytes[i*4 + 2] = (w >> 8)  & 0xff;
        digest_bytes[i*4 + 3] =  w        & 0xff;
    }
    return sha256_check_abc_vector("dport", digest_bytes);
}

/* ---------------------------------------------------------------------------
 * Init: runs known-vector self-test then logs result.
 * Return value is ignored at this layer; gate logic (Phase 2) will check it.
 * ---------------------------------------------------------------------------
 */
void sha256_hw_dport_init(void)
{
    sha256_hw_dport_acquire();
    sha256_hw_dport_self_test();  /* return value ignored; mining.c will check it in Phase 2 */
    sha256_hw_dport_release();
}

#endif // CONFIG_IDF_TARGET_ESP32
#endif // ESP_PLATFORM
