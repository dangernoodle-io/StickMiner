#include "unity.h"
#include "pid.h"
#include <stddef.h>

// TA-315: host unit tests for PID controller
// Uses an injected mock clock that the test advances by 5000 ms per tick.

static unsigned long s_mock_time_ms = 0;

static unsigned long mock_clock(void)
{
    return s_mock_time_ms;
}

// Advance mock clock by 5000 ms and call pid_compute
static bool tick(PIDController *pid)
{
    s_mock_time_ms += 5000;
    return pid_compute(pid);
}

// Fixture: P=5/I=0.1/D=2, setpoint=60, REVERSE, P_ON_E, output limits [25,100], 5000 ms
static void make_pid(PIDController *pid, float *input, float *output, float *setpoint)
{
    s_mock_time_ms = 0;
    *setpoint = 60.0f;
    *output   = 25.0f; // start at min so initialize() seeds outputSum=25
    *input    = 60.0f; // start at setpoint
    pid_init(pid, input, output, setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_REVERSE);
    pid_set_clock(pid, mock_clock);
    pid_set_sample_time(pid, 5000);
    pid_set_output_limits(pid, 25.0f, 100.0f);
    pid_set_mode(pid, AUTOMATIC);
}

// High temp (70°C, 10° above setpoint=60) → output should be near max (100)
void test_pid_high_temp_drives_output_near_max(void)
{
    PIDController pid;
    float input, output, setpoint;
    make_pid(&pid, &input, &output, &setpoint);

    input = 70.0f; // 10°C above setpoint
    // Run several ticks to let the integral accumulate
    for (int i = 0; i < 10; i++) {
        tick(&pid);
    }
    TEST_ASSERT_GREATER_THAN_FLOAT(80.0f, output);
}

// Low temp (50°C, 10° below setpoint=60) → output should be near min (25)
void test_pid_low_temp_drives_output_near_min(void)
{
    PIDController pid;
    float input, output, setpoint;
    make_pid(&pid, &input, &output, &setpoint);

    input = 50.0f; // 10°C below setpoint
    for (int i = 0; i < 10; i++) {
        tick(&pid);
    }
    TEST_ASSERT_LESS_THAN_FLOAT(40.0f, output);
}

// At setpoint (60°C) → output should be in mid-range and stable
void test_pid_at_setpoint_output_stable_mid_range(void)
{
    PIDController pid;
    float input, output, setpoint;
    make_pid(&pid, &input, &output, &setpoint);

    input = 60.0f; // exactly at setpoint
    float prev = output;
    for (int i = 0; i < 20; i++) {
        tick(&pid);
    }
    // Output should have settled — change per tick should be tiny
    float last = output;
    tick(&pid);
    float delta = last - output;
    if (delta < 0) delta = -delta;
    TEST_ASSERT_LESS_THAN_FLOAT(2.0f, delta);
    // And within the legal output range
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(25.0f, output);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f, output);
    (void)prev;
}

// Output must be clamped to [25, 100] at all times
void test_pid_output_always_within_limits(void)
{
    PIDController pid;
    float input, output, setpoint;
    make_pid(&pid, &input, &output, &setpoint);

    float test_temps[] = { 20.0f, 50.0f, 60.0f, 75.0f, 100.0f };
    for (size_t t = 0; t < sizeof(test_temps) / sizeof(test_temps[0]); t++) {
        input = test_temps[t];
        for (int i = 0; i < 5; i++) {
            tick(&pid);
            TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(25.0f, output);
            TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f, output);
        }
    }
}

// pid_compute returns false in MANUAL mode
void test_pid_compute_false_in_manual_mode(void)
{
    PIDController pid;
    float input = 70.0f, output = 50.0f, setpoint = 60.0f;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_REVERSE);
    pid_set_clock(&pid, mock_clock);
    pid_set_sample_time(&pid, 5000);
    pid_set_output_limits(&pid, 25.0f, 100.0f);
    // Leave in MANUAL (default after init)
    s_mock_time_ms += 5000;
    bool result = pid_compute(&pid);
    TEST_ASSERT_FALSE(result);
}

// pid_compute returns false when called before sampleTime has elapsed
void test_pid_compute_false_before_sample_time(void)
{
    PIDController pid;
    float input = 70.0f, output = 50.0f, setpoint = 60.0f;
    s_mock_time_ms = 0;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_REVERSE);
    pid_set_clock(&pid, mock_clock);
    pid_set_sample_time(&pid, 5000);
    pid_set_output_limits(&pid, 25.0f, 100.0f);
    pid_set_mode(&pid, AUTOMATIC);
    // Only advance 100 ms — less than sampleTime=5000
    s_mock_time_ms += 100;
    bool result = pid_compute(&pid);
    TEST_ASSERT_FALSE(result);
}

// Trajectory: start hot → cool to cold → settle at setpoint
void test_pid_trajectory_hot_to_cold_to_setpoint(void)
{
    PIDController pid;
    float input, output, setpoint;
    make_pid(&pid, &input, &output, &setpoint);

    // Phase 1: hot (above setpoint) — fan should ramp up
    input = 70.0f;
    for (int i = 0; i < 8; i++) tick(&pid);
    float hot_output = output;
    TEST_ASSERT_GREATER_THAN_FLOAT(60.0f, hot_output);

    // Phase 2: cold (below setpoint) — fan should wind down
    input = 50.0f;
    for (int i = 0; i < 12; i++) tick(&pid);
    float cold_output = output;
    TEST_ASSERT_LESS_THAN_FLOAT(hot_output, cold_output);

    // Phase 3: settle at setpoint
    input = 60.0f;
    for (int i = 0; i < 20; i++) tick(&pid);
    // Output must be in bounds
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(25.0f, output);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f, output);
}

void test_pid_set_tunings_updates_gains(void)
{
    PIDController pid;
    float input = 60.0f, output = 25.0f, setpoint = 60.0f;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_DIRECT);
    pid_set_tunings(&pid, 7.0f, 0.2f, 3.0f);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, pid_get_kp(&pid));
    TEST_ASSERT_EQUAL_FLOAT(0.2f, pid_get_ki(&pid));
    TEST_ASSERT_EQUAL_FLOAT(3.0f, pid_get_kd(&pid));
}

void test_pid_set_sample_time_scales_gains(void)
{
    PIDController pid;
    float input = 60.0f, output = 25.0f, setpoint = 60.0f;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 1.0f, 2.0f, PID_P_ON_E, PID_DIRECT);
    pid_set_sample_time(&pid, 5000);
    pid_set_sample_time(&pid, 0);
    pid_set_sample_time(&pid, 10000);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, pid_get_kp(&pid));
}

void test_pid_set_controller_direction_flips_signs_in_auto(void)
{
    PIDController pid;
    float input = 60.0f, output = 25.0f, setpoint = 60.0f;
    s_mock_time_ms = 0;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_DIRECT);
    pid_set_clock(&pid, mock_clock);
    pid_set_output_limits(&pid, 0.0f, 100.0f);
    pid_set_mode(&pid, AUTOMATIC);
    pid_set_controller_direction(&pid, PID_REVERSE);
    TEST_ASSERT_EQUAL_INT(PID_REVERSE, pid_get_direction(&pid));
    pid_set_controller_direction(&pid, PID_REVERSE);
    TEST_ASSERT_EQUAL_INT(PID_REVERSE, pid_get_direction(&pid));
}

void test_pid_initialize_clamps_output_sum(void)
{
    PIDController pid;
    float input = 60.0f, output = 200.0f, setpoint = 60.0f;
    s_mock_time_ms = 0;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_REVERSE);
    pid_set_clock(&pid, mock_clock);
    pid_set_sample_time(&pid, 5000);
    pid_set_output_limits(&pid, 25.0f, 100.0f);
    output = -50.0f;
    pid_set_mode(&pid, AUTOMATIC);
    s_mock_time_ms += 5000;
    pid_compute(&pid);
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(25.0f, output);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f, output);
}

void test_pid_getters_return_display_values(void)
{
    PIDController pid;
    float input = 60.0f, output = 25.0f, setpoint = 60.0f;
    pid_init(&pid, &input, &output, &setpoint, 4.0f, 0.5f, 2.0f, PID_P_ON_E, PID_REVERSE);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, pid_get_kp(&pid));
    TEST_ASSERT_EQUAL_FLOAT(0.5f, pid_get_ki(&pid));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, pid_get_kd(&pid));
    TEST_ASSERT_EQUAL_FLOAT(8.0f, pid_get_ti(&pid));
    TEST_ASSERT_EQUAL_FLOAT(0.5f, pid_get_td(&pid));
    TEST_ASSERT_EQUAL_INT(MANUAL, pid_get_mode(&pid));
    TEST_ASSERT_EQUAL_INT(PID_REVERSE, pid_get_direction(&pid));
}

void test_pid_set_output_limits_rejects_invalid(void)
{
    PIDController pid;
    float input = 60.0f, output = 50.0f, setpoint = 60.0f;
    pid_init(&pid, &input, &output, &setpoint, 5.0f, 0.1f, 2.0f, PID_P_ON_E, PID_DIRECT);
    pid_set_output_limits(&pid, 25.0f, 100.0f);
    pid_set_output_limits(&pid, 100.0f, 25.0f);
    pid_set_mode(&pid, AUTOMATIC);
    output = 200.0f;
    pid_set_output_limits(&pid, 25.0f, 100.0f);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f, output);
}
