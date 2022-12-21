#ifndef __FILESYSTEM__H
#define __FILESYSTEM__H

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <sys/dirent.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <esp_err.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_spiffs.h>

#include <cJSON.h>

#define JSON_TYPE_INT valueint
#define JSON_TYPE_DOUBLE valuedouble
#define JSON_TYPE_STRING valuestring

#define JSON_GET_VALUE(_json, _item, _type) \
    cJSON_GetObjectItem(_json, _item) ? cJSON_GetObjectItem(_json, _item)->_type : 0

esp_err_t fs_mount();
esp_err_t fs_unmount();
esp_err_t fs_is_mounted();

char *fs_read_file(const char *file_path, uint32_t *size);
esp_err_t fs_write_file(const char *file_path, const char *file_text);

#endif /*__FILESYSTEM__H*/