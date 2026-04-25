#include "asic_drop_log.h"
#include <string.h>

void asic_drop_log_reset(asic_drop_log_t *log)
{
    if (!log) return;
    memset(log, 0, sizeof(*log));
}

void asic_drop_log_push(asic_drop_log_t *log, const asic_drop_event_t *ev)
{
    if (!log || !ev) return;
    log->entries[log->head] = *ev;
    log->head = (uint8_t)((log->head + 1) % ASIC_DROP_LOG_CAP);
    log->total_written++;
}

size_t asic_drop_log_snapshot(const asic_drop_log_t *log, asic_drop_event_t *out, size_t max_out)
{
    if (!log || !out || max_out == 0) return 0;
    size_t available = (log->total_written < ASIC_DROP_LOG_CAP)
        ? log->total_written
        : ASIC_DROP_LOG_CAP;
    size_t n = (max_out < available) ? max_out : available;
    /* head points at next-write slot; newest is head-1 (mod CAP). */
    for (size_t i = 0; i < n; i++) {
        size_t idx = (log->head + ASIC_DROP_LOG_CAP - 1 - i) % ASIC_DROP_LOG_CAP;
        out[i] = log->entries[idx];
    }
    return n;
}
