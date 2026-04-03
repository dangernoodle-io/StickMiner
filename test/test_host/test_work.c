#include "unity.h"
#include "work.h"
#include "sha256.h"
#include <string.h>

// Test: hex_to_bytes conversion
void test_hex_to_bytes(void)
{
    uint8_t out[4];
    const char *hex = "deadbeef";
    const uint8_t expected[4] = {0xde, 0xad, 0xbe, 0xef};

    size_t result = hex_to_bytes(hex, out, 4);
    TEST_ASSERT_EQUAL_INT(4, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, 4);
}

// Test: bytes_to_hex conversion
void test_bytes_to_hex(void)
{
    const uint8_t data[4] = {0xde, 0xad, 0xbe, 0xef};
    char hex[9];  // 4 bytes * 2 chars + null terminator

    bytes_to_hex(data, 4, hex);
    TEST_ASSERT_EQUAL_STRING("deadbeef", hex);
}

// Test: hex_to_bytes and bytes_to_hex roundtrip
void test_hex_roundtrip(void)
{
    const char *original = "0123456789abcdef";
    uint8_t bytes[8];
    char hex[17];

    hex_to_bytes(original, bytes, 8);
    bytes_to_hex(bytes, 8, hex);
    TEST_ASSERT_EQUAL_STRING(original, hex);
}

// Test: serialize_header with genesis block data
void test_serialize_header_genesis(void)
{
    uint8_t header[80];
    uint8_t prevhash[32] = {0};  // all zeros
    uint8_t merkle_root[32] = {
        0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,
        0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
        0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a,
    };
    const uint8_t expected[80] = {
        0x01, 0x00, 0x00, 0x00, // version
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // prevhash
        0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,
        0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
        0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a, // merkle_root
        0x29, 0xab, 0x5f, 0x49, // ntime
        0xff, 0xff, 0x00, 0x1d, // nbits
        0x1d, 0xac, 0x2b, 0x7c, // nonce
    };

    serialize_header(1, prevhash, merkle_root, 0x495fab29, 0x1d00ffff, 0x7c2bac1d, header);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, header, 80);
}

// Test: set_header_nonce modifies bytes 76-79
void test_set_header_nonce(void)
{
    uint8_t header[80];
    uint8_t prevhash[32] = {0};
    uint8_t merkle_root[32] = {0};

    // Create a header with nonce 0x00000001
    serialize_header(1, prevhash, merkle_root, 0x495fab29, 0x1d00ffff, 0x00000001, header);

    // Now change nonce to 0xdeadbeef
    set_header_nonce(header, 0xefbeadde);

    // Verify bytes 76-79 are now 0xefbeadde (little-endian)
    TEST_ASSERT_EQUAL_HEX8(0xde, header[76]);
    TEST_ASSERT_EQUAL_HEX8(0xad, header[77]);
    TEST_ASSERT_EQUAL_HEX8(0xbe, header[78]);
    TEST_ASSERT_EQUAL_HEX8(0xef, header[79]);
}

// Test: nbits_to_target for genesis difficulty
// nbits=0x1d00ffff should produce target with 0x00ffff at the top
void test_nbits_to_target_genesis(void)
{
    uint8_t target[32];
    const uint8_t expected[32] = {
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    nbits_to_target(0x1d00ffff, target);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, target, 32);
}

// Test: nbits_to_target with higher difficulty
void test_nbits_to_target_high_diff(void)
{
    uint8_t target[32];

    // Example: 0x1a00a000 - exponent=0x1a=26, mantissa=0x00a000
    // Position = 32 - 26 = 6
    // Result: 26 zero bytes + 0x00, 0xa0, 0x00 at positions 6-8, then rest zeros
    const uint8_t expected[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    nbits_to_target(0x1a00a000, target);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, target, 32);
}

// Test: meets_target - hash with leading zeros should pass
void test_meets_target_pass(void)
{
    uint8_t hash[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    uint8_t target[32] = {
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    TEST_ASSERT_TRUE(meets_target(hash, target));
}

// Test: meets_target - hash without enough leading zeros should fail
void test_meets_target_fail(void)
{
    uint8_t hash[32] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    uint8_t target[32] = {
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    TEST_ASSERT_FALSE(meets_target(hash, target));
}

// Test: meets_target - hash exactly equal to target should pass
void test_meets_target_equal(void)
{
    uint8_t hash[32] = {
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    uint8_t target[32];
    memcpy(target, hash, 32);

    TEST_ASSERT_TRUE(meets_target(hash, target));
}

// Test: build_coinbase_hash
// Coinbase: coinb1 + extranonce1 + extranonce2 + coinb2, then SHA256d
void test_build_coinbase_hash(void)
{
    // Use simplified test data
    uint8_t coinb1[] = {0x01, 0x00, 0x00, 0x00};
    uint8_t extranonce1[] = {0xaa, 0xbb, 0xcc, 0xdd};
    uint8_t extranonce2[] = {0x11, 0x22, 0x33, 0x44};
    uint8_t coinb2[] = {0x99};
    uint8_t result[32];

    // Manually compute expected: SHA256d(0x01000000 || aabbccdd || 11223344 || 99)
    uint8_t full[13] = {0x01, 0x00, 0x00, 0x00, 0xaa, 0xbb, 0xcc, 0xdd, 0x11, 0x22, 0x33, 0x44, 0x99};
    uint8_t expected[32];
    sha256d(full, 13, expected);

    build_coinbase_hash(coinb1, 4, extranonce1, 4, extranonce2, 4, coinb2, 1, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 32);
}

// Test: build_merkle_root with no branches
// With no branches, root should equal coinbase_hash
void test_build_merkle_root_no_branches(void)
{
    uint8_t coinbase_hash[32] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11};
    uint8_t root[32];

    // Fill rest with pattern
    for (int i = 8; i < 32; i++) {
        coinbase_hash[i] = (uint8_t)(i & 0xff);
    }

    build_merkle_root(coinbase_hash, NULL, 0, root);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(coinbase_hash, root, 32);
}

// Test: build_merkle_root with one branch
// root = SHA256d(coinbase_hash || branch)
void test_build_merkle_root_with_branches(void)
{
    uint8_t coinbase_hash[32];
    uint8_t branch1[32];
    uint8_t branches[1][32];
    uint8_t result[32];

    // Fill test data
    for (int i = 0; i < 32; i++) {
        coinbase_hash[i] = (uint8_t)(i & 0xff);
        branch1[i] = (uint8_t)((i * 2) & 0xff);
    }
    memcpy(branches[0], branch1, 32);

    // Expected: SHA256d(coinbase_hash || branch1)
    uint8_t combined[64];
    memcpy(combined, coinbase_hash, 32);
    memcpy(combined + 32, branch1, 32);
    uint8_t expected[32];
    sha256d(combined, 64, expected);

    build_merkle_root(coinbase_hash, branches, 1, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, result, 32);
}

// Test: decode_stratum_prevhash
// Stratum prevhash format: 8 groups of 4 bytes (hex), each group byte-reversed
void test_decode_stratum_prevhash(void)
{
    // Example: all zeros encoded as stratum format
    // In stratum, each 4-byte group is reversed:
    // prevhash[0:4] = 0x00000000 -> "00000000" in stratum
    // Full prevhash of 32 zero bytes = "00000000" repeated 8 times
    const char *stratum_hex = "00000000000000000000000000000000000000000000000000000000000000000000000000000000";
    uint8_t prevhash[32];
    uint8_t expected[32] = {0};

    decode_stratum_prevhash(stratum_hex, prevhash);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, prevhash, 32);
}
