#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ASIC_DROP_KIND_TOTAL = 0,
    ASIC_DROP_KIND_ERROR = 1,
    ASIC_DROP_KIND_DOMAIN = 2,
} asic_drop_kind_t;

typedef struct {
    uint64_t ts_us;
    uint8_t  chip_idx;
    uint8_t  kind;
    uint8_t  domain_idx;
    uint8_t  asic_addr;
    float    ghs;
    uint32_t delta;
    float    elapsed_s;
} asic_drop_event_t;

#define ASIC_DROP_LOG_CAP 16

typedef struct {
    asic_drop_event_t entries[ASIC_DROP_LOG_CAP];
    uint32_t total_written;
    uint8_t  head;
} asic_drop_log_t;

void   asic_drop_log_reset(asic_drop_log_t *log);
void   asic_drop_log_push(asic_drop_log_t *log, const asic_drop_event_t *ev);
size_t asic_drop_log_snapshot(const asic_drop_log_t *log, asic_drop_event_t *out, size_t max_out);

/* Live snapshot of the asic task's internal ring (firmware-only consumer). */
size_t asic_task_get_drop_log(asic_drop_event_t *out, size_t max_out);

#ifdef __cplusplus
}
#endif
