#ifndef __WIFI_AP__H
#define __WIFI_AP__H

/**
 * Lib C
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/**
 * Logs;
 */
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

/**
 * Callbacks
 */
//#include "esp_event_loop.h"
#include "esp_event.h"

/**
 * Drivers;
 */
#include "driver/periph_ctrl.h"
#include "esp_wifi.h"

#include "lwip/err.h"
#include "lwip/sys.h"

typedef struct
{

    uint8_t enabled;

    char *ssid;
    char *password;

} wifi_data_t;

void wifi_driver_init(wifi_data_t *config);
// void wifi_driver_init(void);

#endif //<-- __WIFI_AP__H -->