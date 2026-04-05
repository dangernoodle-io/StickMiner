#ifdef ASIC_BM1370

#include "asic.h"
#include "bm1370.h"
#include "crc.h"
#include "pll.h"
#include "tps546.h"
#include "emc2101.h"
#include "mining.h"
#include "work.h"
#include "sha256.h"
#include "board.h"

#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include <string.h>
#include <inttypes.h>
#include <math.h>

static const char *TAG = "asic";

// --- Active job table (static to avoid stack allocation ~28 KB) ---
static mining_work_t s_job_table[BM1370_JOB_ID_MOD];
static uint8_t s_next_job_id;
static char s_current_job_id[64];

// --- UART helpers ---
static void asic_uart_write(const uint8_t *data, size_t len)
{
    uart_write_bytes(ASIC_UART_NUM, data, len);
}

static int asic_uart_read(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    return uart_read_bytes(ASIC_UART_NUM, buf, len, pdMS_TO_TICKS(timeout_ms));
}

// --- Command send helper ---
static void send_cmd(uint8_t cmd, uint8_t group, const uint8_t *data, uint8_t data_len)
{
    uint8_t pkt[64];
    size_t n = bm1370_build_cmd(pkt, sizeof(pkt), cmd, group, data, data_len);
    if (n > 0) {
        asic_uart_write(pkt, n);
    }
}

// --- Register write helper (broadcast) ---
static void write_reg(uint8_t reg, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    uint8_t data[6] = {0x00, reg, d0, d1, d2, d3};
    send_cmd(BM1370_CMD_WRITE, BM1370_GROUP_ALL, data, 6);
}

// --- Register write to specific chip ---
static void write_reg_chip(uint8_t chip_addr, uint8_t reg, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    uint8_t data[6] = {chip_addr, reg, d0, d1, d2, d3};
    send_cmd(BM1370_CMD_WRITE, BM1370_GROUP_SINGLE, data, 6);
}

// PLL fb_div range for BM1370
#define BM1370_FB_MIN 160
#define BM1370_FB_MAX 239

// --- Set PLL frequency ---
static void set_pll_freq(float freq_mhz)
{
    pll_params_t pll;
    pll_calc(freq_mhz, BM1370_FB_MIN, BM1370_FB_MAX, &pll);
    uint8_t vdo = pll_vdo_scale(&pll);
    uint8_t postdiv = pll_postdiv_byte(&pll);
    write_reg(BM1370_REG_PLL, vdo, (uint8_t)pll.fb_div, pll.refdiv, postdiv);
    ESP_LOGI(TAG, "PLL: target=%.1f actual=%.1f fb=%u ref=%u p1=%u p2=%u",
             freq_mhz, pll.actual_mhz, pll.fb_div, pll.refdiv, pll.post1, pll.post2);
}

// --- BM1370 chip init sequence ---
static esp_err_t bm1370_chip_init(void)
{
    uint16_t addr_interval = 256 / BM1370_CHIP_COUNT;

    // Step 1: Set version mask x3
    for (int i = 0; i < 3; i++) {
        write_reg(BM1370_REG_VERSION, 0x90, 0x00, 0xFF, 0xFF);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Step 2: Read chip ID (broadcast)
    uint8_t read_data[2] = {0x00, BM1370_REG_CHIP_ID};
    send_cmd(BM1370_CMD_READ, BM1370_GROUP_ALL, read_data, 2);

    // Wait and read chip responses
    int chip_count = 0;
    for (int i = 0; i < BM1370_CHIP_COUNT + 1; i++) {
        uint8_t rx[BM1370_NONCE_LEN];
        int n = asic_uart_read(rx, BM1370_NONCE_LEN, 1000);
        if (n != BM1370_NONCE_LEN) break;
        if (rx[0] != BM1370_PREAMBLE_RX_0 || rx[1] != BM1370_PREAMBLE_RX_1) break;
        // Check chip ID in response bytes 2-3 (big-endian)
        uint16_t chip_id = ((uint16_t)rx[2] << 8) | rx[3];
        ESP_LOGI(TAG, "chip %d: ID=0x%04X", chip_count, chip_id);
        chip_count++;
    }

    if (chip_count == 0) {
        ESP_LOGE(TAG, "no BM1370 chips detected");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "detected %d chip(s)", chip_count);

    // Step 3: Version mask x1 more
    write_reg(BM1370_REG_VERSION, 0x90, 0x00, 0xFF, 0xFF);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Step 4: Reg A8 broadcast
    write_reg(BM1370_REG_A8, 0x00, 0x07, 0x00, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 5: MISC_CTRL broadcast
    write_reg(BM1370_REG_MISC_CTRL, 0xF0, 0x00, 0xC1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 6: Chain inactive
    uint8_t inactive_data[2] = {0x00, 0x00};
    send_cmd(BM1370_CMD_INACTIVE, BM1370_GROUP_ALL, inactive_data, 2);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 7: Address assignment
    for (int i = 0; i < chip_count; i++) {
        uint8_t addr_data[2] = {(uint8_t)(i * addr_interval), 0x00};
        send_cmd(BM1370_CMD_SETADDR, BM1370_GROUP_SINGLE, addr_data, 2);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 8: Core register control (broadcast)
    write_reg(BM1370_REG_CORE_CTRL, 0x80, 0x00, 0x8B, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
    write_reg(BM1370_REG_CORE_CTRL, 0x80, 0x00, 0x80, 0x0C);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 9: Difficulty mask (default diff 512 = 0x1FF → inverted for ASIC)
    // Ticket mask register 0x14: use 0x0000, 0x0003 for diff ~512
    uint8_t diff_data[6] = {0x00, 0x14, 0x00, 0x00, 0x00, 0xFF};
    send_cmd(BM1370_CMD_WRITE, BM1370_GROUP_ALL, diff_data, 6);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 10: IO driver strength
    write_reg(BM1370_REG_IO_DRV, 0x00, 0x01, 0x11, 0x11);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 11: Per-chip register writes
    for (int i = 0; i < chip_count; i++) {
        uint8_t addr = (uint8_t)(i * addr_interval);
        write_reg_chip(addr, BM1370_REG_A8, 0x00, 0x07, 0x01, 0xF0);
        write_reg_chip(addr, BM1370_REG_MISC_CTRL, 0xF0, 0x00, 0xC1, 0x00);
        write_reg_chip(addr, BM1370_REG_CORE_CTRL, 0x80, 0x00, 0x8B, 0x00);
        write_reg_chip(addr, BM1370_REG_CORE_CTRL, 0x80, 0x00, 0x80, 0x0C);
        write_reg_chip(addr, BM1370_REG_CORE_CTRL, 0x80, 0x00, 0x82, 0xAA);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    // Step 12: Misc registers
    write_reg(BM1370_REG_MISC_SET, 0x00, 0x00, 0x44, 0x80);
    vTaskDelay(pdMS_TO_TICKS(5));
    write_reg(BM1370_REG_ANALOG_MUX, 0x00, 0x00, 0x00, 0x02);
    vTaskDelay(pdMS_TO_TICKS(5));
    write_reg(BM1370_REG_MISC_SET, 0x00, 0x00, 0x44, 0x80);
    vTaskDelay(pdMS_TO_TICKS(5));
    write_reg(BM1370_REG_CORE_CTRL, 0x80, 0x00, 0x8D, 0xEE);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Step 13: Baud rate switch to 1 Mbps
    write_reg(BM1370_REG_FAST_UART, 0x11, 0x30, 0x02, 0x00);
    uart_wait_tx_done(ASIC_UART_NUM, pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(uart_set_baudrate(ASIC_UART_NUM, ASIC_BAUD_FAST));
    uart_flush(ASIC_UART_NUM);
    ESP_LOGI(TAG, "baud switched to %d", ASIC_BAUD_FAST);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Step 14: Frequency ramp from 6.25 to target
    float target_freq = (float)BM1370_DEFAULT_FREQ_MHZ;
    ESP_LOGI(TAG, "ramping frequency to %.1f MHz", target_freq);
    for (float freq = 6.25f; freq <= target_freq; freq += 6.25f) {
        set_pll_freq(freq);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // Final set to exact target
    set_pll_freq(target_freq);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Step 15: Hash counting register
    write_reg(BM1370_REG_HASH_COUNT, 0x00, 0x00, 0x1E, 0xB5);
    vTaskDelay(pdMS_TO_TICKS(5));

    ESP_LOGI(TAG, "BM1370 init complete");
    return ESP_OK;
}

// --- asic_init ---
esp_err_t asic_init(void)
{
    ESP_LOGI(TAG, "initializing ASIC subsystem");

    // 1. UART init
    uart_config_t uart_cfg = {
        .baud_rate = ASIC_BAUD_INIT,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
    };
    ESP_RETURN_ON_ERROR(uart_param_config(ASIC_UART_NUM, &uart_cfg), TAG, "uart config");
    ESP_RETURN_ON_ERROR(uart_set_pin(ASIC_UART_NUM, PIN_ASIC_TX, PIN_ASIC_RX,
                                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "uart pins");
    ESP_RETURN_ON_ERROR(uart_driver_install(ASIC_UART_NUM, 2048, 2048, 0, NULL, 0), TAG, "uart install");
    ESP_LOGI(TAG, "UART%d ready at %d baud", ASIC_UART_NUM, ASIC_BAUD_INIT);

    // 2. GPIO: power enable + reset
    gpio_set_direction(PIN_ASIC_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_ASIC_EN, 1);
    ESP_LOGI(TAG, "ASIC EN=1 (power on)");

    gpio_set_direction(PIN_ASIC_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_ASIC_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_ASIC_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "ASIC reset complete");

    // 3. I2C bus
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_BUS_NUM,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t i2c_bus;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &i2c_bus), TAG, "i2c bus");
    ESP_LOGI(TAG, "I2C bus ready (SDA=%d SCL=%d)", PIN_I2C_SDA, PIN_I2C_SCL);

    // 4. TPS546 voltage regulator
    ESP_RETURN_ON_ERROR(tps546_init(i2c_bus, TPS546_I2C_ADDR, BM1370_DEFAULT_MV), TAG, "tps546");
    vTaskDelay(pdMS_TO_TICKS(50));

    // 5. EMC2101 temp sensor
    ESP_RETURN_ON_ERROR(emc2101_init(i2c_bus, EMC2101_I2C_ADDR), TAG, "emc2101");

    // 6. BM1370 chip init sequence
    ESP_RETURN_ON_ERROR(bm1370_chip_init(), TAG, "bm1370 init");

    // Initialize state
    s_next_job_id = 0;
    memset(s_job_table, 0, sizeof(s_job_table));
    memset(s_current_job_id, 0, sizeof(s_current_job_id));

    ESP_LOGI(TAG, "ASIC subsystem ready");
    return ESP_OK;
}

// --- Mining task ---
void asic_mining_task(void *arg)
{
    ESP_LOGI(TAG, "ASIC mining task started");

    mining_work_t work;
    TickType_t last_temp_tick = 0;
    TickType_t last_hashrate_tick = xTaskGetTickCount();
    uint32_t nonces_since_log = 0;

    for (;;) {
        // 1. Peek for work
        if (xQueuePeek(work_queue, &work, pdMS_TO_TICKS(100)) != pdTRUE) {
            continue;
        }

        // 2. Dispatch new job if job_id changed
        if (strcmp(work.job_id, s_current_job_id) != 0) {
            // Clean job: invalidate all active slots
            if (work.clean) {
                memset(s_job_table, 0, sizeof(s_job_table));
            }

            // Cycle job ID
            s_next_job_id = (s_next_job_id + BM1370_JOB_ID_STEP) % BM1370_JOB_ID_MOD;

            // Extract and send job
            bm1370_job_t job;
            bm1370_extract_job(work.header, s_next_job_id, &job);

            uint8_t pkt[BM1370_JOB_PKT_LEN];
            bm1370_build_job(pkt, sizeof(pkt), &job);
            asic_uart_write(pkt, BM1370_JOB_PKT_LEN);

            // Store in job table
            memcpy(&s_job_table[s_next_job_id], &work, sizeof(work));
            strncpy(s_current_job_id, work.job_id, sizeof(s_current_job_id) - 1);

            ESP_LOGI(TAG, "job dispatched (id=%u hw_id=%u)", 0, s_next_job_id);
        }

        // 3. Try to read nonce response
        uint8_t rx[BM1370_NONCE_LEN];
        int n = asic_uart_read(rx, BM1370_NONCE_LEN, 100);
        if (n == BM1370_NONCE_LEN) {
            bm1370_nonce_t nonce;
            if (!bm1370_parse_nonce(rx, n, &nonce)) {
                // Bad preamble — try to re-align
                uart_flush(ASIC_UART_NUM);
                continue;
            }

            // CRC5 check: crc5(buf+2, 9) should equal 0 for valid packet
            if (crc5(rx + 2, 9) != 0) {
                ESP_LOGW(TAG, "nonce CRC5 failed");
                continue;
            }

            // Check if job response (bit 7 of crc_flags)
            if (!(nonce.crc_flags & 0x80)) {
                // Command response, ignore
                continue;
            }

            // Decode job ID and look up
            uint8_t real_job_id = bm1370_decode_job_id(&nonce);
            if (real_job_id >= BM1370_JOB_ID_MOD || s_job_table[real_job_id].job_id[0] == '\0') {
                ESP_LOGW(TAG, "nonce for unknown job_id=%u", real_job_id);
                continue;
            }

            mining_work_t *orig = &s_job_table[real_job_id];
            nonces_since_log++;

            // Reconstruct header for verification
            uint8_t header_copy[80];
            memcpy(header_copy, orig->header, 80);

            // Apply version rolling if present
            uint32_t ver_bits = bm1370_decode_version_bits(&nonce);
            if (ver_bits != 0 && orig->version_mask != 0) {
                uint32_t rolled = (orig->version & ~orig->version_mask) | (ver_bits & orig->version_mask);
                header_copy[0] = (uint8_t)(rolled);
                header_copy[1] = (uint8_t)(rolled >> 8);
                header_copy[2] = (uint8_t)(rolled >> 16);
                header_copy[3] = (uint8_t)(rolled >> 24);
            }

            // Apply nonce (big-endian wire → little-endian header)
            header_copy[76] = nonce.nonce[3];
            header_copy[77] = nonce.nonce[2];
            header_copy[78] = nonce.nonce[1];
            header_copy[79] = nonce.nonce[0];

            // SHA256d verify
            uint8_t hash[32];
            sha256d(header_copy, 80, hash);

            if (!meets_target(hash, orig->target)) {
                // Nonce doesn't meet pool target — normal for ASIC difficulty
                continue;
            }

            ESP_LOGI(TAG, "share found! job_id=%u", real_job_id);

            // Build result for stratum
            mining_result_t result;
            memset(&result, 0, sizeof(result));
            strncpy(result.job_id, orig->job_id, sizeof(result.job_id) - 1);
            strncpy(result.extranonce2_hex, orig->extranonce2_hex, sizeof(result.extranonce2_hex) - 1);

            // ntime from original work
            snprintf(result.ntime_hex, sizeof(result.ntime_hex), "%08" PRIx32, orig->ntime);

            // nonce as LE hex (bytes in wire BE, we need the actual uint32 in LE hex)
            uint32_t nonce_val = ((uint32_t)nonce.nonce[0] << 24) | ((uint32_t)nonce.nonce[1] << 16) |
                                 ((uint32_t)nonce.nonce[2] << 8) | nonce.nonce[3];
            snprintf(result.nonce_hex, sizeof(result.nonce_hex), "%08" PRIx32, nonce_val);

            // Version rolling
            if (ver_bits != 0 && orig->version_mask != 0) {
                uint32_t rolled = (orig->version & ~orig->version_mask) | (ver_bits & orig->version_mask);
                snprintf(result.version_hex, sizeof(result.version_hex), "%08" PRIx32, rolled);
            }

            xQueueSend(result_queue, &result, 0);

            // Update stats
            if (xSemaphoreTake(mining_stats.mutex, 0) == pdTRUE) {
                mining_stats.asic_shares++;
                xSemaphoreGive(mining_stats.mutex);
            }
        }

        // 4. Periodic temp reading (~every 5s)
        TickType_t now = xTaskGetTickCount();
        if (now - last_temp_tick >= pdMS_TO_TICKS(5000)) {
            float temp;
            if (emc2101_read_temp(&temp) == ESP_OK) {
                if (xSemaphoreTake(mining_stats.mutex, 0) == pdTRUE) {
                    mining_stats.asic_temp_c = temp;
                    xSemaphoreGive(mining_stats.mutex);
                }
                ESP_LOGI(TAG, "ASIC temp: %.1f C", temp);
            }
            last_temp_tick = now;
        }

        // 5. Periodic hashrate log (~every 30s)
        if (now - last_hashrate_tick >= pdMS_TO_TICKS(30000)) {
            float elapsed_s = (float)(now - last_hashrate_tick) / (float)configTICK_RATE_HZ;
            if (elapsed_s > 0 && nonces_since_log > 0) {
                ESP_LOGI(TAG, "nonces: %" PRIu32 " in %.0fs", nonces_since_log, elapsed_s);
            }
            nonces_since_log = 0;
            last_hashrate_tick = now;
        }
    }
}

#endif // ASIC_BM1370
