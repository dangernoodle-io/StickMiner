#include "unity.h"
#include "asic_drop_log.h"
#include <string.h>

static asic_drop_event_t mk(uint64_t ts, uint8_t chip)
{
    asic_drop_event_t ev = {0};
    ev.ts_us = ts;
    ev.chip_idx = chip;
    ev.kind = ASIC_DROP_KIND_TOTAL;
    ev.ghs = (float)ts;
    return ev;
}

void test_drop_log_empty_snapshot(void)
{
    asic_drop_log_t log;
    asic_drop_log_reset(&log);
    asic_drop_event_t out[4];
    TEST_ASSERT_EQUAL_UINT(0, asic_drop_log_snapshot(&log, out, 4));
    TEST_ASSERT_EQUAL_UINT32(0, log.total_written);
}

void test_drop_log_single_push_readback(void)
{
    asic_drop_log_t log;
    asic_drop_log_reset(&log);
    asic_drop_event_t ev = mk(100, 2);
    asic_drop_log_push(&log, &ev);
    asic_drop_event_t out[4];
    TEST_ASSERT_EQUAL_UINT(1, asic_drop_log_snapshot(&log, out, 4));
    TEST_ASSERT_EQUAL_UINT64(100, out[0].ts_us);
    TEST_ASSERT_EQUAL_UINT8(2, out[0].chip_idx);
    TEST_ASSERT_EQUAL_UINT32(1, log.total_written);
}

void test_drop_log_fill_to_cap_newest_first(void)
{
    asic_drop_log_t log;
    asic_drop_log_reset(&log);
    for (uint64_t i = 1; i <= ASIC_DROP_LOG_CAP; i++) {
        asic_drop_event_t ev = mk(i, 0);
        asic_drop_log_push(&log, &ev);
    }
    asic_drop_event_t out[ASIC_DROP_LOG_CAP];
    size_t n = asic_drop_log_snapshot(&log, out, ASIC_DROP_LOG_CAP);
    TEST_ASSERT_EQUAL_UINT(ASIC_DROP_LOG_CAP, n);
    /* newest first: ts CAP, CAP-1, ... 1 */
    TEST_ASSERT_EQUAL_UINT64(ASIC_DROP_LOG_CAP, out[0].ts_us);
    TEST_ASSERT_EQUAL_UINT64(1, out[ASIC_DROP_LOG_CAP - 1].ts_us);
}

void test_drop_log_wrap_drops_oldest(void)
{
    asic_drop_log_t log;
    asic_drop_log_reset(&log);
    /* Push CAP+1 entries. ts=1 should be evicted. */
    for (uint64_t i = 1; i <= ASIC_DROP_LOG_CAP + 1; i++) {
        asic_drop_event_t ev = mk(i, 0);
        asic_drop_log_push(&log, &ev);
    }
    asic_drop_event_t out[ASIC_DROP_LOG_CAP];
    size_t n = asic_drop_log_snapshot(&log, out, ASIC_DROP_LOG_CAP);
    TEST_ASSERT_EQUAL_UINT(ASIC_DROP_LOG_CAP, n);
    TEST_ASSERT_EQUAL_UINT64(ASIC_DROP_LOG_CAP + 1, out[0].ts_us);  /* newest */
    TEST_ASSERT_EQUAL_UINT64(2, out[ASIC_DROP_LOG_CAP - 1].ts_us);  /* oldest surviving */
    TEST_ASSERT_EQUAL_UINT32(ASIC_DROP_LOG_CAP + 1, log.total_written);
}

void test_drop_log_total_written_monotonic(void)
{
    asic_drop_log_t log;
    asic_drop_log_reset(&log);
    for (uint64_t i = 1; i <= 100; i++) {
        asic_drop_event_t ev = mk(i, 0);
        asic_drop_log_push(&log, &ev);
    }
    TEST_ASSERT_EQUAL_UINT32(100, log.total_written);
}

void test_drop_log_max_out_smaller_than_cap(void)
{
    asic_drop_log_t log;
    asic_drop_log_reset(&log);
    for (uint64_t i = 1; i <= 10; i++) {
        asic_drop_event_t ev = mk(i, 0);
        asic_drop_log_push(&log, &ev);
    }
    asic_drop_event_t out[3];
    size_t n = asic_drop_log_snapshot(&log, out, 3);
    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_UINT64(10, out[0].ts_us);
    TEST_ASSERT_EQUAL_UINT64(9, out[1].ts_us);
    TEST_ASSERT_EQUAL_UINT64(8, out[2].ts_us);
}
