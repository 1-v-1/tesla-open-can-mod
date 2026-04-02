// include/web_server.h
#pragma once

#if defined(DRIVER_TWAI) && !defined(NATIVE_BUILD)

#include <WiFi.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <lwip/sockets.h>
#include <cJSON.h>
#include <driver/twai.h>

#include "shared_types.h"
#include "can_helpers.h"
#include "log_buffer.h"
#include "web/web_ui.h"

static const char *AP_SSID = "TeslaCAN";
static const char *AP_PASS = "canmod12"; // WPA2, min 8 chars

// --- NVS helpers ---

static bool nvsInit()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

static bool nvsReadForceFSD()
{
    nvs_handle_t handle;
    if (nvs_open("canmod", NVS_READONLY, &handle) != ESP_OK)
        return false;
    uint8_t val = 0;
    nvs_get_u8(handle, "force_fsd", &val);
    nvs_close(handle);
    return val != 0;
}

static void nvsWriteForceFSD(bool enabled)
{
    nvs_handle_t handle;
    if (nvs_open("canmod", NVS_READWRITE, &handle) != ESP_OK)
        return;
    nvs_set_u8(handle, "force_fsd", enabled ? 1 : 0);
    nvs_commit(handle);
    nvs_close(handle);
}

// --- Rate limiter ---

static unsigned long lastToggleMs = 0;
static const unsigned long kToggleMinIntervalMs = 500;

static bool rateLimitOk()
{
    unsigned long now = millis();
    if (now - lastToggleMs < kToggleMinIntervalMs)
        return false;
    lastToggleMs = now;
    return true;
}

// --- HTTP handlers ---

static esp_err_t rootHandler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WEB_UI_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t statusHandler(httpd_req_t *req)
{
    // Parse log_since from query string
    uint32_t logSince = 0;
    char queryBuf[32];
    if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK)
    {
        char val[16];
        if (httpd_query_key_value(queryBuf, "log_since", val, sizeof(val)) == ESP_OK)
        {
            logSince = strtoul(val, nullptr, 10);
        }
    }

    // Read handler state (atomic reads -- no lock needed)
    bool fsdEnabled = appHandler ? (bool)appHandler->FSDEnabled : false;
    bool forceFsd = (bool)forceFSDRuntime;
    int speedProfile = appHandler ? (int)appHandler->speedProfile : 0;
    int speedOffset = appHandler ? (int)appHandler->speedOffset : 0;
    bool enablePrint = appHandler ? (bool)appHandler->enablePrint : true;

    // Build JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "fsd_enabled", fsdEnabled);
    cJSON_AddBoolToObject(root, "force_fsd", forceFsd);
    cJSON_AddNumberToObject(root, "speed_profile", speedProfile);
    cJSON_AddNumberToObject(root, "speed_offset", speedOffset);
    cJSON_AddBoolToObject(root, "enable_print", enablePrint);
    cJSON_AddNumberToObject(root, "uptime_s", millis() / 1000);
    cJSON_AddNumberToObject(root, "log_head", logRing.currentHead());

    // Add log entries since last poll
    LogRingBuffer::Entry logEntries[LogRingBuffer::kCapacity];
    int logCount = logRing.readSince(logSince, logEntries, LogRingBuffer::kCapacity);
    cJSON *logs = cJSON_AddArrayToObject(root, "logs");
    for (int i = 0; i < logCount; i++)
    {
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddStringToObject(entry, "msg", logEntries[i].msg);
        cJSON_AddNumberToObject(entry, "ts", logEntries[i].timestamp_ms);
        cJSON_AddItemToArray(logs, entry);
    }

    // CAN bus diagnostics
    twai_status_info_t twaiStatus;
    cJSON *can = cJSON_AddObjectToObject(root, "can");
    if (twai_get_status_info(&twaiStatus) == ESP_OK)
    {
        const char *stateStr = "UNKNOWN";
        switch (twaiStatus.state)
        {
        case TWAI_STATE_STOPPED:
            stateStr = "STOPPED";
            break;
        case TWAI_STATE_RUNNING:
            stateStr = "RUNNING";
            break;
        case TWAI_STATE_BUS_OFF:
            stateStr = "BUS_OFF";
            break;
        case TWAI_STATE_RECOVERING:
            stateStr = "RECOVERING";
            break;
        }
        cJSON_AddStringToObject(can, "state", stateStr);
        cJSON_AddNumberToObject(can, "rx_errors", twaiStatus.rx_error_counter);
        cJSON_AddNumberToObject(can, "tx_errors", twaiStatus.tx_error_counter);
        cJSON_AddNumberToObject(can, "bus_errors", twaiStatus.bus_error_count);
        cJSON_AddNumberToObject(can, "rx_missed", twaiStatus.rx_missed_count);
        cJSON_AddNumberToObject(can, "rx_queued", twaiStatus.msgs_to_rx);
    }
    else
    {
        cJSON_AddStringToObject(can, "state", "NO_DRIVER");
    }
    cJSON_AddNumberToObject(can, "frames_received",
                            appHandler ? (uint32_t)appHandler->frameCount : 0);
    cJSON_AddNumberToObject(can, "frames_sent",
                            appHandler ? (uint32_t)appHandler->framesSent : 0);

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    free(json);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t forceFsdHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (cJSON_IsBool(enabled))
    {
        bool val = cJSON_IsTrue(enabled);
        forceFSDRuntime = val;
        nvsWriteForceFSD(val);
        Serial.printf("Web: FORCE_FSD set to %d\n", val);
    }
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t enablePrintHandler(httpd_req_t *req)
{
    if (!rateLimitOk())
    {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_send(req, "Rate limited", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    char body[64];
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No body");
        return ESP_FAIL;
    }
    body[len] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    if (cJSON_IsBool(enabled) && appHandler)
    {
        appHandler->enablePrint = cJSON_IsTrue(enabled);
    }
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Captive portal: redirect connectivity checks to root
static esp_err_t captiveRedirectHandler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// --- DNS captive portal task ---

static void dnsTask(void *param)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        Serial.println("DNS: socket failed");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        Serial.println("DNS: bind failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[512];
    struct sockaddr_in client;
    socklen_t clientLen;

    while (true)
    {
        clientLen = sizeof(client);
        int len = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr *)&client, &clientLen);
        if (len < 12)
            continue;

        // Build DNS response: copy query, set response flags, append answer
        buf[2] = 0x81; // QR=1, Opcode=0, AA=1, TC=0, RD=1
        buf[3] = 0x80; // RA=1, RCODE=0
        buf[6] = 0x00; // ANCOUNT = 1
        buf[7] = 0x01;

        // Append answer after the query section
        int pos = len;
        buf[pos++] = 0xC0; // Name pointer to offset 12 (query name)
        buf[pos++] = 0x0C;
        buf[pos++] = 0x00;
        buf[pos++] = 0x01; // Type A
        buf[pos++] = 0x00;
        buf[pos++] = 0x01; // Class IN
        buf[pos++] = 0x00;
        buf[pos++] = 0x00;
        buf[pos++] = 0x00;
        buf[pos++] = 0x3C; // TTL 60s
        buf[pos++] = 0x00;
        buf[pos++] = 0x04; // RDLENGTH 4
        buf[pos++] = 192;
        buf[pos++] = 168; // 192.168.4.1
        buf[pos++] = 4;
        buf[pos++] = 1;

        sendto(sock, buf, pos, 0,
               (struct sockaddr *)&client, clientLen);
    }
}

// --- Public init function ---

static httpd_handle_t webServer = NULL;

static void webServerInit()
{
    // NVS: load persisted FORCE_FSD
    if (nvsInit())
    {
        forceFSDRuntime = nvsReadForceFSD();
        Serial.printf("NVS: FORCE_FSD = %d\n", (bool)forceFSDRuntime);
    }
    else
    {
        Serial.println("NVS: init failed, using defaults");
    }

    // WiFi AP
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(100); // let AP stabilize
    Serial.printf("WiFi AP \"%s\" started (pass: %s): ", AP_SSID, AP_PASS);
    Serial.println(WiFi.softAPIP());

    // DNS captive portal on Core 0
    xTaskCreatePinnedToCore(dnsTask, "dns", 4096, NULL, 2, NULL, 0);

    // HTTP server on Core 0
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;
    config.stack_size = 8192;

    if (httpd_start(&webServer, &config) != ESP_OK)
    {
        Serial.println("HTTP: server start failed");
        return;
    }

    // Routes
    httpd_uri_t uriRoot = {
        .uri = "/", .method = HTTP_GET, .handler = rootHandler, .user_ctx = NULL};
    httpd_uri_t uriStatus = {
        .uri = "/api/status", .method = HTTP_GET, .handler = statusHandler, .user_ctx = NULL};
    httpd_uri_t uriForceFsd = {
        .uri = "/api/force-fsd", .method = HTTP_POST, .handler = forceFsdHandler, .user_ctx = NULL};
    httpd_uri_t uriEnablePrint = {
        .uri = "/api/enable-print", .method = HTTP_POST, .handler = enablePrintHandler, .user_ctx = NULL};
    httpd_uri_t uriGenerate204 = {
        .uri = "/generate_204", .method = HTTP_GET, .handler = captiveRedirectHandler, .user_ctx = NULL};
    httpd_uri_t uriHotspot = {
        .uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = captiveRedirectHandler, .user_ctx = NULL};

    httpd_register_uri_handler(webServer, &uriRoot);
    httpd_register_uri_handler(webServer, &uriStatus);
    httpd_register_uri_handler(webServer, &uriForceFsd);
    httpd_register_uri_handler(webServer, &uriEnablePrint);
    httpd_register_uri_handler(webServer, &uriGenerate204);
    httpd_register_uri_handler(webServer, &uriHotspot);

    Serial.println("Web dashboard ready at http://192.168.4.1/");
}

#endif // DRIVER_TWAI && !NATIVE_BUILD
