#ifndef __MAIN__H
#define __MAIN__H

/*
 *Includes - BLibliotecas do sistema e do usuario.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <math.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h" 
#include "freertos/ringbuf.h"

#include "esp_log.h"
#include "esp_system.h"

// Callbacks
#include "esp_event.h"

// Watchdog
#include "soc/rtc_wdt.h"
#include "esp_task_wdt.h"

// Drivers
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "esp32/rom/gpio.h"

// User Libs
#include "wifi_driver.h"

typedef struct
{

  float curr_temp;
  float ac_heat;
  float eq_heat;
  float amb_temp;

  bool ac1_on;
  bool ac2_on;

  RingbufHandle_t buffer;

} temp_simulator_t;

#endif //<-- __MAIN__H -->