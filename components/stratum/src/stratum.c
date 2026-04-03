#include "stratum.h"
#include "config.h"
#include "mining.h"
#include "work.h"
#include "sha256.h"

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "stratum";

// Stratum state
static int s_sock = -1;
static int s_msg_id = 1;
static char s_extranonce1_hex[32];
static uint8_t s_extranonce1[MAX_EXTRANONCE1_SIZE];
static size_t s_extranonce1_len = 0;
static int s_extranonce2_size = 4;  // bytes
static double s_difficulty = 1.0;
static stratum_job_t s_job;

// Line buffer for reading from socket
static char s_linebuf[4096];
static int s_linebuf_len = 0;

// Send a string to the socket
static int stratum_send(const char *msg)
{
    int len = strlen(msg);
    int sent = send(s_sock, msg, len, 0);
    if (sent < 0) {
        ESP_LOGE(TAG, "send error: %d", errno);
        return -1;
    }
    ESP_LOGD(TAG, ">> %s", msg);
    return 0;
}

// Send a JSON-RPC request
static int stratum_request(const char *method, const char *params_json)
{
    char buf[512];
    int id = s_msg_id++;
    snprintf(buf, sizeof(buf),
             "{\"id\":%d,\"method\":\"%s\",\"params\":%s}\n",
             id, method, params_json);
    return stratum_send(buf);
}

// Read one newline-terminated line from socket. Returns line length, 0 for no data yet, -1 on error.
static int stratum_readline(char *out, int max_len, int timeout_ms)
{
    // Check if we already have a complete line in buffer
    for (int i = 0; i < s_linebuf_len; i++) {
        if (s_linebuf[i] == '\n') {
            int line_len = i;  // exclude newline
            if (line_len >= max_len) line_len = max_len - 1;
            memcpy(out, s_linebuf, line_len);
            out[line_len] = '\0';
            // Shift buffer
            int remaining = s_linebuf_len - (i + 1);
            if (remaining > 0) {
                memmove(s_linebuf, s_linebuf + i + 1, remaining);
            }
            s_linebuf_len = remaining;
            return line_len;
        }
    }

    // Read more data from socket
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(s_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int space = sizeof(s_linebuf) - s_linebuf_len - 1;
    if (space <= 0) {
        ESP_LOGE(TAG, "line buffer overflow");
        s_linebuf_len = 0;
        return -1;
    }

    int n = recv(s_sock, s_linebuf + s_linebuf_len, space, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  // timeout, no data
        }
        ESP_LOGE(TAG, "recv error: %d", errno);
        return -1;
    }
    if (n == 0) {
        ESP_LOGW(TAG, "connection closed");
        return -1;
    }

    s_linebuf_len += n;

    // Try again to find a line
    for (int i = 0; i < s_linebuf_len; i++) {
        if (s_linebuf[i] == '\n') {
            int line_len = i;
            if (line_len >= max_len) line_len = max_len - 1;
            memcpy(out, s_linebuf, line_len);
            out[line_len] = '\0';
            int remaining = s_linebuf_len - (i + 1);
            if (remaining > 0) {
                memmove(s_linebuf, s_linebuf + i + 1, remaining);
            }
            s_linebuf_len = remaining;
            return line_len;
        }
    }

    return 0;  // no complete line yet
}

// Connect TCP socket to pool
static int stratum_connect(void)
{
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", CONFIG_POOL_PORT);

    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res = NULL;

    int err = getaddrinfo(CONFIG_POOL_HOST, port_str, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed: %d", err);
        if (res) freeaddrinfo(res);
        return -1;
    }

    s_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s_sock < 0) {
        ESP_LOGE(TAG, "socket failed: %d", errno);
        freeaddrinfo(res);
        return -1;
    }

    if (connect(s_sock, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "connect failed: %d", errno);
        close(s_sock);
        s_sock = -1;
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    s_linebuf_len = 0;
    s_msg_id = 1;

    ESP_LOGI(TAG, "connected to %s:%d", CONFIG_POOL_HOST, CONFIG_POOL_PORT);
    return 0;
}

// Build share target from difficulty
static void difficulty_to_target(double diff, uint8_t target[32])
{
    memset(target, 0, 32);
    if (diff <= 0) {
        memset(target, 0xff, 32);
        return;
    }
    // target = 0x00000000FFFF0000... / diff
    // Place result in bytes 4-11
    uint64_t t = (uint64_t)((double)0x0000FFFF00000000ULL / diff);
    target[4]  = (uint8_t)(t >> 56);
    target[5]  = (uint8_t)(t >> 48);
    target[6]  = (uint8_t)(t >> 40);
    target[7]  = (uint8_t)(t >> 32);
    target[8]  = (uint8_t)(t >> 24);
    target[9]  = (uint8_t)(t >> 16);
    target[10] = (uint8_t)(t >> 8);
    target[11] = (uint8_t)t;
}

// Build mining work from current job
static void build_work(mining_work_t *work)
{
    // Build extranonce2 (all zeros)
    uint8_t extranonce2[MAX_EXTRANONCE2_SIZE];
    memset(extranonce2, 0, s_extranonce2_size);

    // Store extranonce2 hex in work
    char en2_hex[17];
    bytes_to_hex(extranonce2, s_extranonce2_size, en2_hex);
    strncpy(work->extranonce2_hex, en2_hex, sizeof(work->extranonce2_hex) - 1);
    work->extranonce2_hex[sizeof(work->extranonce2_hex) - 1] = '\0';

    // Build coinbase hash
    uint8_t coinbase_hash[32];
    build_coinbase_hash(s_job.coinb1, s_job.coinb1_len,
                        s_extranonce1, s_extranonce1_len,
                        extranonce2, s_extranonce2_size,
                        s_job.coinb2, s_job.coinb2_len,
                        coinbase_hash);

    // Build merkle root
    uint8_t merkle_root[32];
    build_merkle_root(coinbase_hash, s_job.merkle_branches, s_job.merkle_count, merkle_root);

    // Serialize header
    serialize_header(s_job.version, s_job.prevhash, merkle_root,
                     s_job.ntime, s_job.nbits, 0, work->header);

    // Set target from current difficulty
    difficulty_to_target(s_difficulty, work->target);

    work->version = s_job.version;
    work->ntime = s_job.ntime;
    strncpy(work->job_id, s_job.job_id, sizeof(work->job_id) - 1);
    work->job_id[sizeof(work->job_id) - 1] = '\0';
}

// Handle mining.notify
static void handle_notify(cJSON *params)
{
    cJSON *arr = params;
    if (!cJSON_IsArray(arr) || cJSON_GetArraySize(arr) < 9) {
        ESP_LOGW(TAG, "invalid notify params");
        return;
    }

    // Parse job fields
    cJSON *job_id_j = cJSON_GetArrayItem(arr, 0);
    cJSON *prevhash_j = cJSON_GetArrayItem(arr, 1);
    cJSON *coinb1_j = cJSON_GetArrayItem(arr, 2);
    cJSON *coinb2_j = cJSON_GetArrayItem(arr, 3);
    cJSON *merkle_j = cJSON_GetArrayItem(arr, 4);
    cJSON *version_j = cJSON_GetArrayItem(arr, 5);
    cJSON *nbits_j = cJSON_GetArrayItem(arr, 6);
    cJSON *ntime_j = cJSON_GetArrayItem(arr, 7);
    cJSON *clean_j = cJSON_GetArrayItem(arr, 8);

    if (!job_id_j || !prevhash_j || !coinb1_j || !coinb2_j ||
        !merkle_j || !version_j || !nbits_j || !ntime_j) {
        ESP_LOGW(TAG, "missing notify fields");
        return;
    }

    // Copy job_id
    strncpy(s_job.job_id, job_id_j->valuestring, sizeof(s_job.job_id) - 1);
    s_job.job_id[sizeof(s_job.job_id) - 1] = '\0';

    // Decode prevhash (stratum format: 8 groups of 4 bytes, each reversed)
    decode_stratum_prevhash(prevhash_j->valuestring, s_job.prevhash);

    // Decode coinb1
    s_job.coinb1_len = hex_to_bytes(coinb1_j->valuestring, s_job.coinb1, MAX_COINB1_SIZE);

    // Decode coinb2
    s_job.coinb2_len = hex_to_bytes(coinb2_j->valuestring, s_job.coinb2, MAX_COINB2_SIZE);

    // Decode merkle branches
    s_job.merkle_count = 0;
    int branch_count = cJSON_GetArraySize(merkle_j);
    for (int i = 0; i < branch_count && i < MAX_MERKLE_BRANCHES; i++) {
        cJSON *branch = cJSON_GetArrayItem(merkle_j, i);
        if (branch && branch->valuestring) {
            hex_to_bytes(branch->valuestring, s_job.merkle_branches[i], 32);
            s_job.merkle_count++;
        }
    }

    // Parse version, nbits, ntime (hex strings → uint32)
    s_job.version = (uint32_t)strtoul(version_j->valuestring, NULL, 16);
    s_job.nbits = (uint32_t)strtoul(nbits_j->valuestring, NULL, 16);
    s_job.ntime = (uint32_t)strtoul(ntime_j->valuestring, NULL, 16);
    s_job.clean_jobs = clean_j ? cJSON_IsTrue(clean_j) : false;

    ESP_LOGI(TAG, "notify: job=%s clean=%d", s_job.job_id, s_job.clean_jobs);

    mining_work_t work;
    build_work(&work);
    work.clean = s_job.clean_jobs;

    if (s_job.clean_jobs) {
        xQueueReset(work_queue);
    }
    xQueueOverwrite(work_queue, &work);
}

// Handle mining.set_difficulty
static void handle_set_difficulty(cJSON *params)
{
    if (!cJSON_IsArray(params) || cJSON_GetArraySize(params) < 1) {
        return;
    }
    cJSON *diff = cJSON_GetArrayItem(params, 0);
    if (cJSON_IsNumber(diff)) {
        s_difficulty = diff->valuedouble;
        ESP_LOGI(TAG, "difficulty set to %.4f", s_difficulty);
    }
}

// Handle subscribe response
static int handle_subscribe_result(cJSON *result)
{
    if (!cJSON_IsArray(result) || cJSON_GetArraySize(result) < 3) {
        ESP_LOGE(TAG, "invalid subscribe result");
        return -1;
    }

    // result[1] = extranonce1 (hex string)
    cJSON *en1 = cJSON_GetArrayItem(result, 1);
    if (!en1 || !en1->valuestring) {
        ESP_LOGE(TAG, "no extranonce1");
        return -1;
    }
    strncpy(s_extranonce1_hex, en1->valuestring, sizeof(s_extranonce1_hex) - 1);
    s_extranonce1_hex[sizeof(s_extranonce1_hex) - 1] = '\0';
    s_extranonce1_len = hex_to_bytes(s_extranonce1_hex, s_extranonce1, MAX_EXTRANONCE1_SIZE);

    // result[2] = extranonce2_size
    cJSON *en2sz = cJSON_GetArrayItem(result, 2);
    if (cJSON_IsNumber(en2sz)) {
        s_extranonce2_size = en2sz->valueint;
    }

    ESP_LOGI(TAG, "subscribed: en1=%s en2_size=%d", s_extranonce1_hex, s_extranonce2_size);
    return 0;
}

// Submit a share
static int submit_share(mining_result_t *result)
{
    char params[256];
    snprintf(params, sizeof(params),
             "[\"%s.%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",
             CONFIG_WALLET_ADDR, CONFIG_WORKER_NAME,
             result->job_id,
             result->extranonce2_hex,
             result->ntime_hex,
             result->nonce_hex);

    ESP_LOGI(TAG, "submit: job=%s nonce=%s", result->job_id, result->nonce_hex);
    return stratum_request("mining.submit", params);
}

// Process one JSON message from pool
static void process_message(const char *line)
{
    cJSON *json = cJSON_Parse(line);
    if (!json) {
        ESP_LOGW(TAG, "invalid JSON");
        return;
    }

    ESP_LOGD(TAG, "<< %s", line);

    cJSON *method = cJSON_GetObjectItem(json, "method");
    cJSON *id_item = cJSON_GetObjectItem(json, "id");
    cJSON *result_item = cJSON_GetObjectItem(json, "result");
    cJSON *params = cJSON_GetObjectItem(json, "params");
    cJSON *error_item = cJSON_GetObjectItem(json, "error");

    if (method && method->valuestring) {
        // Server notification
        if (strcmp(method->valuestring, "mining.notify") == 0) {
            handle_notify(params);
        } else if (strcmp(method->valuestring, "mining.set_difficulty") == 0) {
            handle_set_difficulty(params);
        } else {
            ESP_LOGD(TAG, "unhandled method: %s", method->valuestring);
        }
    } else if (id_item && cJSON_IsNumber(id_item)) {
        // Response to our request
        int id = id_item->valueint;
        if (id == 1) {
            // Subscribe response
            if (result_item) {
                handle_subscribe_result(result_item);
            }
        } else if (id == 2) {
            // Authorize response
            if (result_item && cJSON_IsTrue(result_item)) {
                ESP_LOGI(TAG, "authorized");
            } else {
                ESP_LOGE(TAG, "authorization failed");
                if (error_item && !cJSON_IsNull(error_item)) {
                    char *err_str = cJSON_PrintUnformatted(error_item);
                    if (err_str) {
                        ESP_LOGE(TAG, "error: %s", err_str);
                        free(err_str);
                    }
                }
            }
        } else {
            // Submit response or other
            if (error_item && !cJSON_IsNull(error_item)) {
                char *err_str = cJSON_PrintUnformatted(error_item);
                if (err_str) {
                    ESP_LOGE(TAG, "share rejected: %s", err_str);
                    free(err_str);
                }
            } else if (result_item && cJSON_IsTrue(result_item)) {
                ESP_LOGI(TAG, "share accepted");
            }
        }
    }

    cJSON_Delete(json);
}

void stratum_task(void *arg)
{
    static char line[2048];

    ESP_LOGI(TAG, "stratum task started");

    for (;;) {
        // Connect
        if (stratum_connect() != 0) {
            ESP_LOGW(TAG, "reconnecting in 5s");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Subscribe (id=1)
        if (stratum_request("mining.subscribe", "[\"StickMiner/0.1\"]") != 0) {
            goto reconnect;
        }

        // Wait for subscribe response
        for (int i = 0; i < 50; i++) {  // 5s timeout
            int n = stratum_readline(line, sizeof(line), 100);
            if (n > 0) {
                process_message(line);
                if (s_extranonce1_len > 0) break;
            } else if (n < 0) {
                goto reconnect;
            }
        }

        if (s_extranonce1_len == 0) {
            ESP_LOGE(TAG, "subscribe timeout");
            goto reconnect;
        }

        // Authorize (id=2)
        {
            char auth_params[128];
            snprintf(auth_params, sizeof(auth_params),
                     "[\"%s.%s\",\"x\"]",
                     CONFIG_WALLET_ADDR, CONFIG_WORKER_NAME);
            if (stratum_request("mining.authorize", auth_params) != 0) {
                goto reconnect;
            }
        }

        // Main loop: read messages and submit shares
        for (;;) {
            // Try to read a message (100ms timeout to check results)
            int n = stratum_readline(line, sizeof(line), 100);
            if (n > 0) {
                process_message(line);
            } else if (n < 0) {
                break;  // connection error
            }

            // Check for mining results to submit
            mining_result_t result;
            while (xQueueReceive(result_queue, &result, 0) == pdTRUE) {
                if (submit_share(&result) != 0) {
                    goto reconnect;
                }
            }
        }

reconnect:
        if (s_sock >= 0) {
            close(s_sock);
            s_sock = -1;
        }
        s_extranonce1_len = 0;
        ESP_LOGW(TAG, "reconnecting in 5s");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
