#pragma once

// Initialize WiFi in station mode and connect.
// Blocks until connected or max retries exhausted.
// Returns ESP_OK on success.
#include "esp_err.h"
esp_err_t wifi_init(void);
