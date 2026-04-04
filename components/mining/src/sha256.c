#include "sha256.h"
#include <string.h>

// Platform compatibility
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#else
#define IRAM_ATTR
#endif

// SHA-256 K constants (round constants)
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

// Initial hash values (H0 - H7)
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
};

// Rotate right
static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

// SHA-256 logical functions
static inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint32_t Sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static inline uint32_t Sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

// Load 32-bit big-endian value
static inline uint32_t load_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

// Store 32-bit big-endian value
static inline void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

// Store 64-bit big-endian value
static inline void store_be64(uint8_t *p, uint64_t v) {
    store_be32(p, (uint32_t)(v >> 32));
    store_be32(p + 4, (uint32_t)v);
}

// Core SHA-256 compression function (hot path, marked IRAM_ATTR)
IRAM_ATTR void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t T1, T2;
    int i;

    // Load message schedule (first 16 words from block)
    for (i = 0; i < 16; i++) {
        W[i] = load_be32(block + i * 4);
    }

    // Expand message schedule (words 16-63)
    for (i = 16; i < 64; i++) {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }

    // Initialize working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    // Main loop
    for (i = 0; i < 64; i++) {
        T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
        T2 = Sigma0(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // Add compressed chunk to current hash value
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Variant that takes pre-built word array (skips load_be32 for W[0-15])
IRAM_ATTR void sha256_transform_words(uint32_t state[8], const uint32_t words[16]) {
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t T1, T2;
    int i;

    // Words already in SHA-256 big-endian format
    for (i = 0; i < 16; i++) {
        W[i] = words[i];
    }

    // Expand message schedule (words 16-63)
    for (i = 16; i < 64; i++) {
        W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
    }

    // Initialize working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    // Main loop
    for (i = 0; i < 64; i++) {
        T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];
        T2 = Sigma0(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // Add compressed chunk to current hash value
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void sha256_init(sha256_ctx_t *ctx) {
    memcpy(ctx->state, H0, sizeof(H0));
    ctx->count = 0;
}

void sha256_process_block(sha256_ctx_t *ctx, const uint8_t block[64]) {
    sha256_transform(ctx->state, block);
    ctx->count += 64;
}

void sha256_clone(sha256_ctx_t *dst, const sha256_ctx_t *src) {
    memcpy(dst, src, sizeof(sha256_ctx_t));
}

void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len) {
    if (len == 0) return;

    size_t index = ctx->count % 64;
    size_t space = 64 - index;

    ctx->count += len;

    // Fill buffer if we have partial data
    if (len < space) {
        memcpy(ctx->buf + index, data, len);
        return;
    }

    // Process buffered data + start of input
    if (index != 0) {
        memcpy(ctx->buf + index, data, space);
        sha256_transform(ctx->state, ctx->buf);
        data += space;
        len -= space;
    }

    // Process complete 64-byte blocks
    while (len >= 64) {
        sha256_transform(ctx->state, data);
        data += 64;
        len -= 64;
    }

    // Buffer remaining data
    if (len > 0) {
        memcpy(ctx->buf, data, len);
    }
}

void sha256_final(sha256_ctx_t *ctx, uint8_t hash[32]) {
    uint8_t msglen[8];
    size_t index = ctx->count % 64;
    size_t padlen;

    // Store total bit count (big-endian)
    store_be64(msglen, ctx->count * 8);

    // Determine padding length (must reach 56 bytes mod 64)
    padlen = (index < 56) ? (56 - index) : (120 - index);

    // Append 0x80 (first padding byte)
    uint8_t padbuf[128];
    padbuf[0] = 0x80;
    memset(padbuf + 1, 0, padlen - 1);
    memcpy(padbuf + padlen, msglen, 8);

    sha256_update(ctx, padbuf, padlen + 8);

    // Output final hash (big-endian)
    for (int i = 0; i < 8; i++) {
        store_be32(hash + i * 4, ctx->state[i]);
    }
}

void sha256(const uint8_t *data, size_t len, uint8_t hash[32]) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

void sha256d(const uint8_t *data, size_t len, uint8_t hash[32]) {
    uint8_t first_hash[32];
    sha256(data, len, first_hash);
    sha256(first_hash, 32, hash);
}
