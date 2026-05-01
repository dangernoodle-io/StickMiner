#pragma once

// PID algorithm adapted from Brett Beauregard's Arduino PID Library (MIT). Time source abstracted for host testability.

#include <stdbool.h>
#include <stdint.h>

#define AUTOMATIC 1
#define MANUAL    0

typedef enum {
    PID_DIRECT  = 0,
    PID_REVERSE = 1,
} PIDDirection;

typedef enum {
    PID_P_ON_M = 0,
    PID_P_ON_E = 1,
} PIDProportionalMode;

/* Clock function pointer: returns current time in milliseconds.
 * Defaults to a weak fallback returning 0; inject a real clock via
 * pid_set_clock() before calling pid_compute() on the target. */
typedef unsigned long (*pid_now_ms_fn)(void);

typedef struct {
    float dispKp;
    float dispKi;
    float dispKd;

    float kp;
    float ki;
    float kd;

    PIDDirection        controllerDirection;
    PIDProportionalMode pOn;
    bool                pOnE;

    float *input;
    float *output;
    float *setpoint;

    unsigned long lastTime;
    unsigned long sampleTime;
    float         outMin;
    float         outMax;
    bool          inAuto;

    float outputSum;
    float lastInput;

    pid_now_ms_fn now_ms;
} PIDController;

void pid_init(PIDController *pid, float *input, float *output, float *setpoint,
              float Kp, float Ki, float Kd,
              PIDProportionalMode POn, PIDDirection ControllerDirection);

/* Replace the clock source. Call before pid_compute() on non-host targets. */
void pid_set_clock(PIDController *pid, pid_now_ms_fn fn);

void pid_set_mode(PIDController *pid, int mode);
bool pid_compute(PIDController *pid);
void pid_set_output_limits(PIDController *pid, float min, float max);
void pid_set_tunings(PIDController *pid, float Kp, float Ki, float Kd);
void pid_set_tunings_adv(PIDController *pid, float Kp, float Ki, float Kd, PIDProportionalMode POn);
void pid_set_sample_time(PIDController *pid, int newSampleTime);
void pid_set_controller_direction(PIDController *pid, PIDDirection direction);
void pid_initialize(PIDController *pid);

float        pid_get_kp(PIDController *pid);
float        pid_get_ki(PIDController *pid);
float        pid_get_kd(PIDController *pid);
float        pid_get_ti(PIDController *pid);
float        pid_get_td(PIDController *pid);
int          pid_get_mode(PIDController *pid);
PIDDirection pid_get_direction(PIDController *pid);
