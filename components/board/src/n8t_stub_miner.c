#if defined(BOARD_BITDSK_N8T)

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mining.h"

static const char *TAG = "n8t-stub";

static void s_stub_miner_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "N8-T stub miner: parked (BM1397 driver pending TA-47/TA-48)");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

const miner_config_t g_miner_config = {
    .init = NULL,
    .task_fn = s_stub_miner_task,
    .name = "n8t_stub",
    .stack_size = 4096,
    .priority = 20,
    .core = 1,
    .extranonce2_roll = false,
    .roll_interval_ms = 0,
};

#endif
