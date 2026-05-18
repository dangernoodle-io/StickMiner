#include "unity.h"
#include "bb_json.h"
#include "bb_manifest.h"
#include "config.h"
#include <string.h>

void test_register_manifest_returns_ok(void)
{
    bb_manifest_clear();
    TEST_ASSERT_EQUAL(0, config_register_manifest());
}

void test_register_manifest_registers_tm_keys(void)
{
    bb_manifest_clear();
    TEST_ASSERT_EQUAL(0, config_register_manifest());
    bb_json_t doc = bb_manifest_emit();
    TEST_ASSERT_NOT_NULL(doc);
    char *json = bb_json_serialize(doc);
    TEST_ASSERT_NOT_NULL(json);
    /* TM-owned keys must be present */
    const char *expected[] = {"pool_host", "pool_port", "wallet_addr", "worker", "pool_pass"};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        TEST_ASSERT_NOT_NULL_MESSAGE(strstr(json, expected[i]), expected[i]);
    }
    /* hostname is BB-owned — must NOT appear in TM's manifest registration */
    TEST_ASSERT_NULL_MESSAGE(strstr(json, "\"hostname\""), "hostname must not be in TM manifest");
    bb_json_free_str(json);
    bb_json_free(doc);
}

void test_register_manifest_hostname_absent(void)
{
    bb_manifest_clear();
    TEST_ASSERT_EQUAL(0, config_register_manifest());
    bb_json_t doc = bb_manifest_emit();
    TEST_ASSERT_NOT_NULL(doc);
    char *json = bb_json_serialize(doc);
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NULL_MESSAGE(strstr(json, "\"hostname\""), "hostname must not be in TM manifest");
    bb_json_free_str(json);
    bb_json_free(doc);
}

void test_register_manifest_idempotent_via_clear(void)
{
    bb_manifest_clear();
    TEST_ASSERT_EQUAL(0, config_register_manifest());
    bb_manifest_clear();
    TEST_ASSERT_EQUAL(0, config_register_manifest());
}
