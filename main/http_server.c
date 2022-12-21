#include <protobuf-c/protobuf-c.h>
#include "http_server.h"

static esp_err_t content_handler(httpd_req_t *req);
static esp_err_t config_get_handler(httpd_req_t *req);
static esp_err_t config_put_handler(httpd_req_t *req);
static esp_err_t graph_get_handler(httpd_req_t *req);

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
    HTTP_URI("/config", HTTP_GET, config_get_handler, NULL),
    HTTP_URI("/config", HTTP_PUT, config_put_handler, NULL),
    HTTP_URI("/graph", HTTP_GET, graph_get_handler, NULL),
};

#define HTTP_DATA_MAX_INDEX (sizeof(l_http_data) / sizeof(l_http_data[0]))

httpd_handle_t http_instance = NULL;

static const char *TAG = "http_server";
// extern temp_simulator_t temp_sim;

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

extern temp_simulator_t temp_sim;

static esp_err_t config_get_handler(httpd_req_t *req)
{

    cJSON *config = cJSON_CreateObject();

    cJSON_AddNumberToObject(config, "ac_heat", temp_sim.ac_heat);
    cJSON_AddNumberToObject(config, "eq_heat", temp_sim.eq_heat);
    cJSON_AddNumberToObject(config, "amb_temp", temp_sim.amb_temp);
    cJSON_AddNumberToObject(config, "curr_temp", temp_sim.curr_temp);

    char *body = cJSON_Print(config);
    cJSON_Delete(config);

    httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
    free(body);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static esp_err_t config_put_handler(httpd_req_t *req)
{
    char *req_content = calloc(req->content_len + 1, sizeof(char));
    uint16_t ret = httpd_req_recv(req, req_content, req->content_len);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Error receiving data");
        free(req_content);
        return ESP_FAIL;
    }

    cJSON *config = cJSON_Parse(req_content);

    temp_sim.ac_heat = cJSON_GetObjectItem(config, "ac_heat")->valuedouble;
    temp_sim.eq_heat = cJSON_GetObjectItem(config, "eq_heat")->valuedouble;
    temp_sim.amb_temp = cJSON_GetObjectItem(config, "amb_temp")->valuedouble;
    temp_sim.curr_temp = cJSON_GetObjectItem(config, "curr_temp")->valuedouble;

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static esp_err_t graph_get_handler(httpd_req_t *req)
{

    uint8_t *bytes = ring_buffer_peek(temp_sim.buffer);

    httpd_resp_send(req, (char *) bytes, temp_sim.buffer->count * temp_sim.buffer->item_size);
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}


void http_server_init()
{

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
            httpd_register_uri_handler(http_instance, &content_uri);
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
        httpd_stop(http_instance);
        ESP_LOGI(TAG, "Server Stopped");
    }
}
