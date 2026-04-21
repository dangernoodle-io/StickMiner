#include "taipan_web.h"
#include "esp_http_server.h"
#include "bb_http.h"
#include "bb_info.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs.h"
#include "board.h"
#include "mining.h"
#include "bb_nv.h"
#include "taipan_config.h"
#include "bb_wifi.h"
#include "bb_prov.h"
#include "bb_mdns.h"
#include "stratum.h"
#include "cJSON.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <inttypes.h>
#include "bb_ota_pull.h"
#include "bb_ota_push.h"
#include "ota_validator.h"
#include "bb_log.h"
#include "bb_board.h"

static const char *TAG = "web";

extern const unsigned char prov_form_html_gz[];
extern const unsigned int prov_form_html_gz_len;
extern const unsigned char theme_css_gz[];
extern const unsigned int theme_css_gz_len;
extern const unsigned char logo_svg_gz[];
extern const unsigned int logo_svg_gz_len;
extern const unsigned char mining_html_gz[];
extern const unsigned int mining_html_gz_len;
extern const unsigned char mining_js_gz[];
extern const unsigned int mining_js_gz_len;
extern const unsigned char prov_save_html_gz[];
extern const unsigned int prov_save_html_gz_len;
extern const uint8_t favicon_svg_gz[];
extern const unsigned int favicon_svg_gz_len;

static uint32_t s_wdt_resets = 0;

static void set_common_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Private-Network", "true");
}

static esp_err_t preflight_handler(httpd_req_t *req)
{
    set_common_headers(req);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

bb_err_t taipan_web_register_preflight(bb_http_handle_t server)
{
    httpd_handle_t h = (httpd_handle_t)server;
    httpd_uri_t preflight = { .uri = "/*", .method = HTTP_OPTIONS, .handler = preflight_handler };
    return (bb_err_t)httpd_register_uri_handler(h, &preflight);
}

static esp_err_t prov_form_handler(httpd_req_t *req)
{
    set_common_headers(req);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)prov_form_html_gz, prov_form_html_gz_len);
    return ESP_OK;
}

static esp_err_t taipan_prov_save_cb(httpd_req_t *req, const char *body, int len)
{
    (void)len;
    char pool_host[64] = "", wallet[64] = "", worker[32] = "";
    char pool_pass[64] = "";
    char port_str[8] = "";

    bb_url_decode_field(body, "pool_host", pool_host, sizeof(pool_host));
    bb_url_decode_field(body, "pool_port", port_str, sizeof(port_str));
    bb_url_decode_field(body, "wallet", wallet, sizeof(wallet));
    bb_url_decode_field(body, "worker", worker, sizeof(worker));
    bb_url_decode_field(body, "pool_pass", pool_pass, sizeof(pool_pass));

    if (pool_host[0] == '\0' || wallet[0] == '\0' || worker[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "All fields required");
        return ESP_FAIL;
    }
    uint16_t port = (uint16_t)strtoul(port_str, NULL, 10);
    if (port == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Valid port required");
        return ESP_FAIL;
    }
    if (taipan_config_set_pool(pool_host, port, wallet, worker, pool_pass) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save config");
        return ESP_FAIL;
    }

    set_common_headers(req);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)prov_save_html_gz, prov_save_html_gz_len);
    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    set_common_headers(req);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)mining_html_gz, mining_html_gz_len);
    return ESP_OK;
}

static esp_err_t stats_handler(httpd_req_t *req)
{
    set_common_headers(req);
    double hw_rate = 0, hw_ema = 0;
    double pool_diff = 0;
    double best_diff = 0;
    uint32_t hw_shares = 0;
    float temp = 0;
    uint32_t session_shares = 0, session_rejected = 0;
    int64_t last_share_us = 0, session_start_us = 0;
    mining_lifetime_t lifetime = {0};
#if defined(ASIC_BM1370) || defined(ASIC_BM1368)
    double asic_rate = 0, asic_ema = 0;
    uint32_t asic_shares = 0;
    float asic_temp = 0;
    float asic_freq_cfg = -1.0f, asic_freq_eff = -1.0f;
#endif

    if (xSemaphoreTake(mining_stats.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        hw_rate = mining_stats.hw_hashrate;
        hw_ema = mining_stats.hw_ema.value;
        hw_shares = mining_stats.hw_shares;
        pool_diff = mining_stats.pool_difficulty;
        temp = mining_stats.temp_c;
        session_shares = mining_stats.session.shares;
        session_rejected = mining_stats.session.rejected;
        last_share_us = mining_stats.session.last_share_us;
        session_start_us = mining_stats.session.start_us;
        best_diff = mining_stats.session.best_diff;
        lifetime = mining_stats.lifetime;
#if defined(ASIC_BM1370) || defined(ASIC_BM1368)
        asic_rate = mining_stats.asic_hashrate;
        asic_ema = mining_stats.asic_ema.value;
        asic_shares = mining_stats.asic_shares;
        asic_temp = mining_stats.asic_temp_c;
        asic_freq_cfg = mining_stats.asic_freq_configured_mhz;
        asic_freq_eff = mining_stats.asic_freq_effective_mhz;
#endif
        xSemaphoreGive(mining_stats.mutex);
    }

    const esp_app_desc_t *app = esp_app_get_description();
    int64_t now_us = esp_timer_get_time();
    int64_t uptime_s = (session_start_us > 0) ? (now_us - session_start_us) / 1000000 : 0;
    int64_t last_share_ago_s = (last_share_us > 0) ? (now_us - last_share_us) / 1000000 : -1;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "hashrate", hw_rate);
    cJSON_AddNumberToObject(root, "hashrate_avg", hw_ema);
    cJSON_AddNumberToObject(root, "temp_c", (double)temp);
    cJSON_AddNumberToObject(root, "shares", hw_shares);
    cJSON_AddNumberToObject(root, "pool_difficulty", pool_diff);
    cJSON_AddNumberToObject(root, "session_shares", session_shares);
    cJSON_AddNumberToObject(root, "session_rejected", session_rejected);
    cJSON_AddNumberToObject(root, "last_share_ago_s", (double)last_share_ago_s);
    cJSON_AddNumberToObject(root, "lifetime_shares", lifetime.total_shares);
    cJSON_AddNumberToObject(root, "best_diff", best_diff);
    cJSON_AddStringToObject(root, "pool_host", taipan_config_pool_host());
    cJSON_AddNumberToObject(root, "pool_port", taipan_config_pool_port());
    cJSON_AddStringToObject(root, "worker", taipan_config_worker_name());
    cJSON_AddStringToObject(root, "wallet", taipan_config_wallet_addr());
    cJSON_AddNumberToObject(root, "uptime_s", (double)uptime_s);
    cJSON_AddNumberToObject(root, "free_heap", (double)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    cJSON_AddNumberToObject(root, "total_heap", (double)heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        cJSON_AddNumberToObject(root, "rssi_dbm", ap_info.rssi);
    } else {
        cJSON_AddNullToObject(root, "rssi_dbm");
    }
    cJSON_AddStringToObject(root, "version", app->version);
    cJSON_AddStringToObject(root, "build_date", app->date);
    cJSON_AddStringToObject(root, "build_time", app->time);
    cJSON_AddStringToObject(root, "board", BOARD_NAME);
    cJSON_AddBoolToObject(root, "display_en", bb_nv_config_display_enabled());
#if defined(ASIC_BM1370) || defined(ASIC_BM1368)
    cJSON_AddNumberToObject(root, "asic_hashrate", asic_rate);
    cJSON_AddNumberToObject(root, "asic_hashrate_avg", asic_ema);
    cJSON_AddNumberToObject(root, "asic_shares", asic_shares);
    cJSON_AddNumberToObject(root, "asic_temp_c", (double)asic_temp);
    if (asic_freq_cfg >= 0) {
        cJSON_AddNumberToObject(root, "asic_freq_configured_mhz", (double)asic_freq_cfg);
    } else {
        cJSON_AddNullToObject(root, "asic_freq_configured_mhz");
    }
    if (asic_freq_eff >= 0) {
        cJSON_AddNumberToObject(root, "asic_freq_effective_mhz", (double)asic_freq_eff);
    } else {
        cJSON_AddNullToObject(root, "asic_freq_effective_mhz");
    }
    cJSON_AddNumberToObject(root, "asic_small_cores", BOARD_SMALL_CORES);
    cJSON_AddNumberToObject(root, "asic_count", BOARD_ASIC_COUNT);
#endif

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

#if defined(ASIC_BM1370) || defined(ASIC_BM1368)
static esp_err_t power_handler(httpd_req_t *req)
{
    set_common_headers(req);
    int vcore_mv = -1, icore_ma = -1, pcore_mw = -1;
    int vin_mv = -1;
    float board_temp_c = -1.0f, vr_temp_c = -1.0f;
    double asic_hashrate = 0;

    if (xSemaphoreTake(mining_stats.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        vcore_mv = mining_stats.vcore_mv;
        icore_ma = mining_stats.icore_ma;
        pcore_mw = mining_stats.pcore_mw;
        vin_mv = mining_stats.vin_mv;
        asic_hashrate = mining_stats.asic_hashrate;
        board_temp_c = mining_stats.board_temp_c;
        vr_temp_c = mining_stats.vr_temp_c;
        xSemaphoreGive(mining_stats.mutex);
    }

    cJSON *root = cJSON_CreateObject();
    if (vcore_mv >= 0) {
        cJSON_AddNumberToObject(root, "vcore_mv", vcore_mv);
    } else {
        cJSON_AddNullToObject(root, "vcore_mv");
    }
    if (icore_ma >= 0) {
        cJSON_AddNumberToObject(root, "icore_ma", icore_ma);
    } else {
        cJSON_AddNullToObject(root, "icore_ma");
    }
    if (pcore_mw >= 0) {
        cJSON_AddNumberToObject(root, "pcore_mw", pcore_mw);
    } else {
        cJSON_AddNullToObject(root, "pcore_mw");
    }
    if (pcore_mw > 0 && asic_hashrate > 0) {
        cJSON_AddNumberToObject(root, "efficiency_jth", (pcore_mw / 1000.0) / (asic_hashrate / 1e12));
    } else {
        cJSON_AddNullToObject(root, "efficiency_jth");
    }
    if (vin_mv >= 0) {
        cJSON_AddNumberToObject(root, "vin_mv", vin_mv);
    } else {
        cJSON_AddNullToObject(root, "vin_mv");
    }
    if (vin_mv >= 0) {
        bool vin_low = (vin_mv < (BOARD_NOMINAL_VIN_MV + 500) * 87 / 100);
        cJSON_AddBoolToObject(root, "vin_low", vin_low);
    } else {
        cJSON_AddNullToObject(root, "vin_low");
    }
    if (board_temp_c >= 0.0f) {
        cJSON_AddNumberToObject(root, "board_temp_c", (double)board_temp_c);
    } else {
        cJSON_AddNullToObject(root, "board_temp_c");
    }
    if (vr_temp_c >= 0.0f) {
        cJSON_AddNumberToObject(root, "vr_temp_c", (double)vr_temp_c);
    } else {
        cJSON_AddNullToObject(root, "vr_temp_c");
    }

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t fan_handler(httpd_req_t *req)
{
    set_common_headers(req);
    int fan_rpm = -1;
    int fan_duty_pct = -1;

    if (xSemaphoreTake(mining_stats.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        fan_rpm = mining_stats.fan_rpm;
        fan_duty_pct = mining_stats.fan_duty_pct;
        xSemaphoreGive(mining_stats.mutex);
    }

    cJSON *root = cJSON_CreateObject();
    if (fan_rpm >= 0) {
        cJSON_AddNumberToObject(root, "rpm", fan_rpm);
    } else {
        cJSON_AddNullToObject(root, "rpm");
    }
    if (fan_duty_pct >= 0) {
        cJSON_AddNumberToObject(root, "duty_pct", fan_duty_pct);
    } else {
        cJSON_AddNullToObject(root, "duty_pct");
    }

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}
#endif // ASIC_BM1370 || ASIC_BM1368


static esp_err_t ota_mark_valid_handler(httpd_req_t *req)
{
    set_common_headers(req);
    esp_err_t err = ota_validator_mark_valid_manual();

    cJSON *root = cJSON_CreateObject();

    if (err == ESP_OK) {
        httpd_resp_set_status(req, "200 OK");
        cJSON_AddBoolToObject(root, "validated", true);
    } else if (err == ESP_ERR_INVALID_STATE) {
        httpd_resp_set_status(req, "409 Conflict");
        cJSON_AddStringToObject(root, "error", "not pending");
    } else {
        httpd_resp_set_status(req, "500 Internal Server Error");
        cJSON_AddStringToObject(root, "error", "mark_valid failed");
    }

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t logo_handler(httpd_req_t *req)
{
    set_common_headers(req);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_send(req, (const char *)logo_svg_gz, logo_svg_gz_len);
    return ESP_OK;
}

static esp_err_t theme_handler(httpd_req_t *req)
{
    set_common_headers(req);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)theme_css_gz, theme_css_gz_len);
    return ESP_OK;
}

static esp_err_t mining_js_handler(httpd_req_t *req)
{
    set_common_headers(req);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)mining_js_gz, mining_js_gz_len);
    return ESP_OK;
}

static esp_err_t favicon_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    set_common_headers(req);
    httpd_resp_send(req, (const char *)favicon_svg_gz, favicon_svg_gz_len);
    return ESP_OK;
}

static void taipan_info_extender(cJSON *root)
{
    cJSON_AddStringToObject(root, "worker_name", taipan_config_worker_name());

    const char *ssid = bb_nv_config_wifi_ssid();
    if (ssid) cJSON_AddStringToObject(root, "ssid", ssid);

    cJSON_AddBoolToObject(root, "validated", !ota_validator_is_pending());
    cJSON_AddNumberToObject(root, "wdt_resets", s_wdt_resets);

    time_t now = time(NULL);
    if (now > 1700000000) {
        int64_t uptime_s = esp_timer_get_time() / 1000000LL;
        cJSON_AddNumberToObject(root, "boot_time", (double)(now - uptime_s));
    }

    cJSON *network = cJSON_GetObjectItem(root, "network");
    if (network) {
        cJSON_AddBoolToObject(network, "mdns", bb_mdns_started());
        cJSON_AddBoolToObject(network, "stratum", stratum_is_connected());
        cJSON_AddNumberToObject(network, "stratum_reconnect_ms", stratum_get_reconnect_delay_ms());
        cJSON_AddNumberToObject(network, "stratum_fail_count", stratum_get_connect_fail_count());
    }
}

bb_err_t taipan_web_register_info_extender(void)
{
    return bb_info_register_extender(taipan_info_extender);
}

static esp_err_t settings_get_handler(httpd_req_t *req)
{
    set_common_headers(req);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "pool_host", taipan_config_pool_host());
    cJSON_AddNumberToObject(root, "pool_port", taipan_config_pool_port());
    cJSON_AddStringToObject(root, "wallet", taipan_config_wallet_addr());
    cJSON_AddStringToObject(root, "worker", taipan_config_worker_name());
    cJSON_AddStringToObject(root, "pool_pass", taipan_config_pool_pass());
    cJSON_AddBoolToObject(root, "display_en", bb_nv_config_display_enabled());
    cJSON_AddBoolToObject(root, "ota_skip_check", bb_nv_config_ota_skip_check());

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

// Shared helper for POST (full) and PATCH (partial) settings
static esp_err_t apply_settings(httpd_req_t *req, bool partial)
{
    set_common_headers(req);
    char body[512];

    if (req->content_len > sizeof(body) - 1) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body too large");
        return ESP_FAIL;
    }
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }
    body[len] = '\0';

    cJSON *root = cJSON_Parse(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extract fields — use current values as defaults for PATCH
    const char *pool_host = taipan_config_pool_host();
    uint16_t pool_port = taipan_config_pool_port();
    const char *wallet = taipan_config_wallet_addr();
    const char *worker = taipan_config_worker_name();
    const char *pool_pass = taipan_config_pool_pass();
    bool reboot_required = false;

    cJSON *j;

    j = cJSON_GetObjectItem(root, "pool_host");
    if (j && cJSON_IsString(j)) { pool_host = j->valuestring; }
    else if (!partial) { if (!j) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pool_host required"); return ESP_FAIL; } }

    j = cJSON_GetObjectItem(root, "pool_port");
    if (j && cJSON_IsNumber(j)) { pool_port = (uint16_t)j->valuedouble; }
    else if (!partial) { if (!j) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pool_port required"); return ESP_FAIL; } }

    j = cJSON_GetObjectItem(root, "wallet");
    if (j && cJSON_IsString(j)) { wallet = j->valuestring; }
    else if (!partial) { if (!j) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "wallet required"); return ESP_FAIL; } }

    j = cJSON_GetObjectItem(root, "worker");
    if (j && cJSON_IsString(j)) { worker = j->valuestring; }
    else if (!partial) { if (!j) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "worker required"); return ESP_FAIL; } }

    j = cJSON_GetObjectItem(root, "pool_pass");
    if (j && cJSON_IsString(j)) { pool_pass = j->valuestring; }
    // pool_pass is optional even for POST

    // Compare against current values to determine if reboot is needed
    if (strcmp(pool_host, taipan_config_pool_host()) != 0 ||
        pool_port != taipan_config_pool_port() ||
        strcmp(wallet, taipan_config_wallet_addr()) != 0 ||
        strcmp(worker, taipan_config_worker_name()) != 0 ||
        strcmp(pool_pass, taipan_config_pool_pass()) != 0) {
        reboot_required = true;
    }

    // Validate
    if (pool_host[0] == '\0' || wallet[0] == '\0' || worker[0] == '\0') {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pool_host, wallet, worker must not be empty");
        return ESP_FAIL;
    }
    if (pool_port == 0) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pool_port must be > 0");
        return ESP_FAIL;
    }

    // Save mining config if any mining field was provided
    if (reboot_required) {
        esp_err_t err = taipan_config_set_pool(pool_host, pool_port, wallet, worker, pool_pass);
        if (err != ESP_OK) {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save config");
            return ESP_FAIL;
        }
    }

    // Handle display_en separately (takes effect immediately, no reboot needed)
    j = cJSON_GetObjectItem(root, "display_en");
    if (j && cJSON_IsBool(j)) {
        esp_err_t err = bb_nv_config_set_display_enabled(cJSON_IsTrue(j));
        if (err != ESP_OK) {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save display setting");
            return ESP_FAIL;
        }
    }

    j = cJSON_GetObjectItem(root, "ota_skip_check");
    if (j && cJSON_IsBool(j)) {
        esp_err_t err = bb_nv_config_set_ota_skip_check(cJSON_IsTrue(j));
        if (err != ESP_OK) {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Failed to save ota_skip_check");
            return ESP_FAIL;
        }
    }

    cJSON_Delete(root);

    // Response
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"status\":\"saved\",\"reboot_required\":%s}",
             reboot_required ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t settings_post_handler(httpd_req_t *req)
{
    return apply_settings(req, false);
}

static esp_err_t settings_patch_handler(httpd_req_t *req)
{
    return apply_settings(req, true);
}


bb_err_t taipan_web_register_prov_routes(bb_http_handle_t server)
{
    httpd_handle_t h = (httpd_handle_t)server;
    httpd_uri_t prov_form = { .uri = "/", .method = HTTP_GET, .handler = prov_form_handler };
    httpd_uri_t theme_uri = { .uri = "/theme.css", .method = HTTP_GET, .handler = theme_handler };
    httpd_uri_t logo_uri = { .uri = "/logo.svg", .method = HTTP_GET, .handler = logo_handler };
    httpd_uri_t favicon_uri = { .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler };

    bb_prov_set_save_callback(taipan_prov_save_cb);
    httpd_register_uri_handler(h, &prov_form);
    httpd_register_uri_handler(h, &theme_uri);
    httpd_register_uri_handler(h, &logo_uri);
    httpd_register_uri_handler(h, &favicon_uri);

    bb_http_register_common_routes(server);
    bb_info_register_routes(server);

    ESP_LOGI(TAG, "provisioning routes registered");
    return BB_OK;
}

bb_err_t taipan_web_register_mining_routes(bb_http_handle_t server)
{
    httpd_handle_t h = (httpd_handle_t)server;

    // Cache WDT reset count — only changes on boot
    {
        nvs_handle_t nvs_h;
        if (nvs_open("taipanminer", NVS_READONLY, &nvs_h) == ESP_OK) {
            nvs_get_u32(nvs_h, "wdt_resets", &s_wdt_resets);
            nvs_close(nvs_h);
        }
    }

    httpd_uri_t status_uri = { .uri = "/", .method = HTTP_GET, .handler = status_handler };
    httpd_uri_t stats_uri = { .uri = "/api/stats", .method = HTTP_GET, .handler = stats_handler };
    httpd_uri_t mining_js_uri = { .uri = "/mining.js", .method = HTTP_GET, .handler = mining_js_handler };
    httpd_uri_t theme_uri = { .uri = "/theme.css", .method = HTTP_GET, .handler = theme_handler };
    httpd_uri_t logo_uri = { .uri = "/logo.svg", .method = HTTP_GET, .handler = logo_handler };
    httpd_uri_t favicon_uri = { .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler };

    httpd_register_uri_handler(h, &status_uri);
    httpd_register_uri_handler(h, &stats_uri);
    httpd_register_uri_handler(h, &mining_js_uri);
    httpd_register_uri_handler(h, &theme_uri);
    httpd_register_uri_handler(h, &logo_uri);
    httpd_register_uri_handler(h, &favicon_uri);

    httpd_uri_t ota_mark_valid_uri = { .uri = "/api/ota/mark-valid", .method = HTTP_POST, .handler = ota_mark_valid_handler };
    httpd_register_uri_handler(h, &ota_mark_valid_uri);

    httpd_uri_t settings_get_uri = { .uri = "/api/settings", .method = HTTP_GET, .handler = settings_get_handler };
    httpd_uri_t settings_post_uri = { .uri = "/api/settings", .method = HTTP_POST, .handler = settings_post_handler };
    httpd_uri_t settings_patch_uri = { .uri = "/api/settings", .method = HTTP_PATCH, .handler = settings_patch_handler };
    httpd_register_uri_handler(h, &settings_get_uri);
    httpd_register_uri_handler(h, &settings_post_uri);
    httpd_register_uri_handler(h, &settings_patch_uri);

#if defined(ASIC_BM1370) || defined(ASIC_BM1368)
    httpd_uri_t power_uri = { .uri = "/api/power", .method = HTTP_GET, .handler = power_handler };
    httpd_register_uri_handler(h, &power_uri);
    httpd_uri_t fan_uri = { .uri = "/api/fan", .method = HTTP_GET, .handler = fan_handler };
    httpd_register_uri_handler(h, &fan_uri);
#endif

    bb_ota_pull_register_handler(server);
    bb_ota_push_register_handler(server);

    bb_http_register_common_routes(server);
    bb_info_register_routes(server);
    bb_wifi_register_routes(server);
    bb_board_register_routes(server);
    bb_log_stream_register_routes(server);

    ESP_LOGI(TAG, "mining routes registered");
    return BB_OK;
}
