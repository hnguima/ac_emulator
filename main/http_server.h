#ifndef __HTTPSERVER___H
#define __HTTPSERVER___H

#include <esp_http_server.h>
#include "esp_tls_crypto.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "driver/uart.h"

#include "esp_ota_ops.h"
#include "freertos/event_groups.h"

#include "cJSON.h"

#include "telesignals.h"
#include "telecommands.h"
#include "telemeasurements.h"

#include "system.h"
#include "filesystem.h"
#include "system.h"
#include "io_driver.h"
#include "modbus_driver.h"
#include "led_driver.h"
#include "ac_controller.h"
#include "config.h"

#define SERVER_PORT 80

#define LOGIN_DEFAULT_USERNAME "admin"
#define LOGIN_DEFAULT_PASSWORD "ati12345"

#define CONNECTED_BIT BIT0
#define REBOOT_BIT BIT1

#define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */

typedef struct
{

    const uint8_t *start;
    const uint8_t *end;
    const char *type;
    const httpd_uri_t uri;

} http_content;

struct async_send_arg
{
    httpd_handle_t handle;
    int fd;
    char *payload;
};

utr32_data_t *utr32;

httpd_handle_t http_instance;
int ws_fd;
void http_server_init(utr32_data_t *config);
void http_server_stop(void);
esp_err_t websocket_async_send(char *payload);

#endif //<-- __HTTPSERVER___H -->