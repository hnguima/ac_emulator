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

#include "temp_sim.h"
#include "ring_buffer.h"
#include "filesystem.h"

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


void http_server_init();
void http_server_stop(void);

#endif //<-- __HTTPSERVER___H -->