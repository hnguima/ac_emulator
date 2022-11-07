#include <protobuf-c/protobuf-c.h>
#include "http_server.h"

static esp_err_t content_handler(httpd_req_t *req);
static esp_err_t config_handler(httpd_req_t *req);
static esp_err_t load_handler(httpd_req_t *req);
static esp_err_t reset_handler(httpd_req_t *req);
static esp_err_t OTA_status_handler(httpd_req_t *req);
static esp_err_t OTA_upload_handler(httpd_req_t *req);
static esp_err_t ac_state_handler(httpd_req_t *req);

#define HTTP_URI(__uri, __method, __handler, __user_ctx) \
    {                                                    \
        .uri = __uri,                                    \
        .method = __method,                              \
        .handler = __handler,                            \
        .user_ctx = __user_ctx,                          \
    }

httpd_uri_t l_http_data[] = {
    HTTP_URI("/", HTTP_GET, content_handler, NULL),
    HTTP_URI("/img/*", HTTP_GET, content_handler, NULL),
    HTTP_URI("/js/*", HTTP_GET, content_handler, NULL),
    HTTP_URI("/style/*", HTTP_GET, content_handler, NULL),
    HTTP_URI("/save_config", HTTP_PUT, config_handler, NULL),
    HTTP_URI("/load_config", HTTP_GET, load_handler, NULL),
};

#define HTTP_DATA_MAX_INDEX (sizeof(l_http_data) / sizeof(l_http_data[0]))

httpd_handle_t http_instance = NULL;

static const char *TAG = "http_server";

static esp_err_t content_handler(httpd_req_t *req)
{

    esp_err_t err = ESP_OK;
    char *buf = NULL;
    size_t buf_len = 0;

    char filepath[32] = "/server";

    if (strcmp(req->uri, "/") == 0)
    {
        strcat(filepath, "/index.html");
    }
    else if (strcmp(req->uri, "/config") == 0)
    {
        strcat(filepath, "/config.html");
    }
    else
    {
        strcat(filepath, req->uri);
    }

    fs_mount();

    uint32_t size = 0;
    char *filebuffer = fs_read_file(filepath, &size);

    if (filebuffer)
    {
        if (strstr(filepath, ".html"))
        {
            httpd_resp_set_type(req, "text/html");
        }
        if (strstr(filepath, ".css"))
        {
            httpd_resp_set_type(req, "text/css");
        }
        else if (strstr(filepath, ".js"))
        {
            httpd_resp_set_type(req, "text/javascript");
        }
        else if (strstr(filepath, ".svg"))
        {
            httpd_resp_set_type(req, "image/svg+xml");
        }

        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        httpd_resp_send(req, filebuffer, size);

        free(filebuffer);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open file for reading %s", filepath);
        httpd_resp_set_status(req, HTTPD_404);
        httpd_resp_send(req, NULL, 0);
        err = ESP_FAIL;
    }

    return err;
}



static esp_err_t config_handler(httpd_req_t *req)
{

    char *req_content = calloc(req->content_len + 1, sizeof(char));

    uint16_t bytes_read = 0;

    while (bytes_read < req->content_len)
    {
        uint16_t ret = httpd_req_recv(req, req_content + bytes_read, req->content_len);
        if (ret <= 0)
        {
            ESP_LOGE(TAG, "Error receiving data");
            free(req_content);
            return ESP_FAIL;
        }
        bytes_read += ret;
    }

    cJSON *body = cJSON_Parse(req_content);
    free(req_content);

    if (!body)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_send(req, NULL, 0);
        return ESP_FAIL;
    }

    // saving old config
    cJSON *old_config_json = config_parse(utr32, CONFIG_ALL);
    char *old_config_str = cJSON_Print(old_config_json);
    cJSON_Delete(old_config_json);

    fs_write_file("/data/old_config", old_config_str);
    free(old_config_str);

    config_load_json(utr32, body);
    cJSON_Delete(body);

    const char resp[] = "Success";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static esp_err_t load_handler(httpd_req_t *req)
{
    char *body_str = NULL;

    char config_type[10];
    httpd_req_get_hdr_value_str(req, "Config-type", config_type, 10);

    if (strcmp(config_type, "all") == 0)
    {
        cJSON *data_json = config_parse(utr32, CONFIG_ALL);
        body_str = cJSON_Print(data_json);
        cJSON_Delete(data_json);
    }
    else if (strcmp(config_type, "old") == 0)
    {
        body_str = fs_read_file("/data/old_config", NULL);
    }
    else if (strcmp(config_type, "factory") == 0)
    {
        body_str = fs_read_file("/data/default", NULL);
    }
    else
    {
        cJSON *data_json = config_parse(utr32, CONFIG_TO);
        body_str = cJSON_Print(data_json);
        cJSON_Delete(data_json);
    }

    if (!is_root)
    {
        cJSON *body_json = cJSON_Parse(body_str);
        free(body_str);

        cJSON_DeleteItemFromObject(body_json, "admin");

        body_str = cJSON_Print(body_json);
        cJSON_Delete(body_json);
    }

    httpd_resp_set_type(req, "application/json");

    if (!body_str)
    {
        body_str = malloc(sizeof(char));
    }
    httpd_resp_send(req, body_str, HTTPD_RESP_USE_STRLEN);

    free(body_str);

    return ESP_OK;
}

void http_server_init(utr32_data_t *config)
{
    utr32 = config;

    esp_err_t err = ESP_OK;

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.uri_match_fn = httpd_uri_match_wildcard;
    http_config.max_uri_handlers = HTTP_DATA_MAX_INDEX;
    http_config.backlog_conn = 10;
    http_config.max_open_sockets = 3;
    http_config.lru_purge_enable = true;
    http_config.stack_size = 8192;

    if (httpd_start(&http_instance, &http_config) == ESP_OK)
    {

        for (uint8_t i = 0; i < HTTP_DATA_MAX_INDEX; i++)
        {
            httpd_uri_t content_uri = l_http_data[i];
            SYSTEM_ERROR_CHECK(httpd_register_uri_handler(http_instance, &content_uri), err, TAG, "Erro ao registrar uri");
        }

        ESP_LOGI(TAG, "Server Started");
    }
    else
    {
        ESP_LOGI(TAG, "Error starting http server...");
    }
}

void http_server_stop(void)
{
    esp_err_t err = ESP_OK;

    if (http_instance != NULL)
    {
        SYSTEM_ERROR_CHECK(httpd_stop(http_instance), err, TAG, "Erro ao parar o servidor");
        ESP_LOGI(TAG, "Server Stopped");
    }
}
