#include "unity.h"
#include <stdbool.h>
#include <math.h>

// TA-141: Autofan thermal aggregation tests
// Tests the max(die_ema, vr_ema) logic and source tracking

// Helper: compute EMA with alpha=0.2
static float apply_ema(float current_ema, float new_sample, float alpha)
{
    if (current_ema < 0.0f) {
        return new_sample;  // Initialize on first call
    }
    return alpha * new_sample + (1.0f - alpha) * current_ema;
}

// Test: die > vr → PID input should be die, src = "die"
void test_autofan_die_greater_than_vr(void)
{
    float die_ema = -1.0f;
    float vr_ema = -1.0f;
    float alpha = 0.2f;

    // First sample: die=60, vr=50
    die_ema = apply_ema(die_ema, 60.0f, alpha);
    vr_ema = apply_ema(vr_ema, 50.0f, alpha);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, die_ema);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, vr_ema);

    float pid_input = (vr_ema > die_ema) ? vr_ema : die_ema;
    const char *src = (vr_ema > die_ema) ? "vr" : "die";
    TEST_ASSERT_EQUAL_FLOAT(60.0f, pid_input);
    TEST_ASSERT_EQUAL_STRING("die", src);
}

// Test: vr > die → PID input should be vr, src = "vr"
void test_autofan_vr_greater_than_die(void)
{
    float die_ema = -1.0f;
    float vr_ema = -1.0f;
    float alpha = 0.2f;

    // First sample: die=50, vr=60
    die_ema = apply_ema(die_ema, 50.0f, alpha);
    vr_ema = apply_ema(vr_ema, 60.0f, alpha);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, die_ema);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, vr_ema);

    float pid_input = (vr_ema > die_ema) ? vr_ema : die_ema;
    const char *src = (vr_ema > die_ema) ? "vr" : "die";
    TEST_ASSERT_EQUAL_FLOAT(60.0f, pid_input);
    TEST_ASSERT_EQUAL_STRING("vr", src);
}

// Test: VR read invalid (-1 sentinel) → fall back to die, src = "die"
void test_autofan_vr_invalid_fallback_to_die(void)
{
    float die_ema = -1.0f;
    float vr_ema = -1.0f;
    float alpha = 0.2f;

    // First sample: die=60, vr=-1 (invalid)
    die_ema = apply_ema(die_ema, 60.0f, alpha);
    // vr_ema remains -1 (uninitialized/invalid)
    TEST_ASSERT_EQUAL_FLOAT(60.0f, die_ema);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, vr_ema);

    // Logic: if vr_ema >= 0 and vr_ema > die_ema, use vr; else use die
    bool vr_valid = (vr_ema >= 0.0f);
    float pid_input = (vr_valid && vr_ema > die_ema) ? vr_ema : die_ema;
    const char *src = (vr_valid && vr_ema > die_ema) ? "vr" : "die";
    TEST_ASSERT_EQUAL_FLOAT(60.0f, pid_input);
    TEST_ASSERT_EQUAL_STRING("die", src);
}

// Test: Both EMAs update independently across sequential samples
void test_autofan_independent_ema_updates(void)
{
    float die_ema = -1.0f;
    float vr_ema = -1.0f;
    float alpha = 0.2f;

    // Sample 1: die=50, vr=60
    die_ema = apply_ema(die_ema, 50.0f, alpha);
    vr_ema = apply_ema(vr_ema, 60.0f, alpha);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, die_ema);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, vr_ema);

    // Sample 2: die=55, vr=58
    float die_ema_prev = die_ema;
    float vr_ema_prev = vr_ema;
    die_ema = apply_ema(die_ema, 55.0f, alpha);
    vr_ema = apply_ema(vr_ema, 58.0f, alpha);
    // die_ema should be 0.2*55 + 0.8*50 = 11 + 40 = 51
    // vr_ema should be 0.2*58 + 0.8*60 = 11.6 + 48 = 59.6
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 51.0f, die_ema);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 59.6f, vr_ema);

    // Sample 3: die=70, vr=55
    die_ema = apply_ema(die_ema, 70.0f, alpha);
    vr_ema = apply_ema(vr_ema, 55.0f, alpha);
    // die_ema should be 0.2*70 + 0.8*51 = 14 + 40.8 = 54.8
    // vr_ema should be 0.2*55 + 0.8*59.6 = 11 + 47.68 = 58.68
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 54.8f, die_ema);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 58.68f, vr_ema);

    // After sample 3, die is now approaching vr but still lower
    float pid_input = (vr_ema > die_ema) ? vr_ema : die_ema;
    const char *src = (vr_ema > die_ema) ? "vr" : "die";
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 58.68f, pid_input);
    TEST_ASSERT_EQUAL_STRING("vr", src);
}

// Test: Die can catch up and overtake VR
void test_autofan_source_switch_die_overtakes_vr(void)
{
    float die_ema = -1.0f;
    float vr_ema = -1.0f;
    float alpha = 0.2f;

    // Start: vr > die
    die_ema = apply_ema(die_ema, 50.0f, alpha);
    vr_ema = apply_ema(vr_ema, 70.0f, alpha);
    TEST_ASSERT_TRUE(vr_ema > die_ema);

    // Feed high die temps to catch up
    for (int i = 0; i < 10; i++) {
        die_ema = apply_ema(die_ema, 80.0f, alpha);
        vr_ema = apply_ema(vr_ema, 50.0f, alpha);
    }

    // After convergence, die should be > vr
    TEST_ASSERT_TRUE(die_ema > vr_ema);
    float pid_input = (vr_ema > die_ema) ? vr_ema : die_ema;
    const char *src = (vr_ema > die_ema) ? "vr" : "die";
    TEST_ASSERT_EQUAL_STRING("die", src);
}

// Test: EMA initialization on first valid read
void test_autofan_ema_initialization(void)
{
    float die_ema = -1.0f;
    float vr_ema = -1.0f;

    // Both should initialize to first sample
    die_ema = (die_ema < 0.0f) ? 60.0f : (0.2f * 60.0f + 0.8f * die_ema);
    vr_ema = (vr_ema < 0.0f) ? 55.0f : (0.2f * 55.0f + 0.8f * vr_ema);

    TEST_ASSERT_EQUAL_FLOAT(60.0f, die_ema);
    TEST_ASSERT_EQUAL_FLOAT(55.0f, vr_ema);

    // Second sample should apply EMA
    die_ema = (die_ema < 0.0f) ? 62.0f : (0.2f * 62.0f + 0.8f * die_ema);
    vr_ema = (vr_ema < 0.0f) ? 54.0f : (0.2f * 54.0f + 0.8f * vr_ema);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.4f, die_ema);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 54.8f, vr_ema);
}
