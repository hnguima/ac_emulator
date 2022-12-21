#include "filesystem.h"

// todo: implement a mutex lock to secure files
// todo: write function should save a temporary copy to secure writing
// todo: configurable periodic/event-driven backups
// todo: test fatfs and littlefs against spiffs
// todo: read function should have a max size of output in case file being read is larger than available memory 

static const char *TAG = "fs";

esp_vfs_spiffs_conf_t fs_server = {
    .base_path = "/server",
    .partition_label = "storage0",
    .max_files = 20,
    .format_if_mount_failed = false,
};

esp_vfs_spiffs_conf_t fs_data = {
    .base_path = "/data",
    .partition_label = "storage1",
    .max_files = 1,
    .format_if_mount_failed = false,
};

esp_err_t fs_mount()
{
    if (!fs_is_mounted())
    {
        esp_err_t err = esp_vfs_spiffs_register(&fs_server);
        err = esp_vfs_spiffs_register(&fs_data);

        if (err != ESP_OK)
        {
            if (err == ESP_FAIL)
            {
                ESP_LOGE(TAG, "Failed to mount or format filesystem");
            }
            else if (err == ESP_ERR_NOT_FOUND)
            {
                ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", err);
            }
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t fs_unmount()
{
    esp_err_t err = ESP_OK;

    err = esp_vfs_spiffs_unregister(fs_server.partition_label);
    err = esp_vfs_spiffs_unregister(fs_data.partition_label);

    return err;
}

esp_err_t fs_is_mounted()
{

    return esp_spiffs_mounted(fs_data.partition_label);
}

char *fs_read_file(const char *file_path, uint32_t *size)
{
    uint8_t count = 10;

    for (uint8_t i = 0; i < count; i++)
    {
        char *file_buffer = NULL;
        long file_length;

        FILE *f = fopen(file_path, "r");
        if (f != NULL)
        {
            fseek(f, 0, SEEK_END);
            file_length = ftell(f);

            fseek(f, 0, SEEK_SET);
            file_buffer = malloc(sizeof(char) * (file_length + 1));

            if (file_buffer)
            {
                fread(file_buffer, 1, file_length, f);
                file_buffer[file_length] = '\0';
            }
            fclose(f);

            if (file_buffer)
            {
                if (size)
                {
                    *size = file_length;
                }
                return file_buffer;
            }
        }
        else
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }

    return NULL;
}

esp_err_t fs_write_file(const char *file_path, const char *file_text)
{
    uint8_t count = 10;

    esp_err_t err = ESP_OK;

    for (uint8_t i = 0; i < count; i++)
    {
        err = ESP_OK;

        if (file_text == NULL)
        {
            err = ESP_ERR_INVALID_ARG;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        if (strlen(file_text) == 0)
        {
            err = ESP_ERR_INVALID_SIZE;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        FILE *f = fopen(file_path, "w");
        if (f == NULL)
        {
            ESP_LOGW(TAG, "Failed to open file for writing");
            err = ESP_ERR_NOT_FOUND;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        fprintf(f, "%s", file_text);
        fclose(f);

        if (err == ESP_OK)
        {
            return err;
        }
    }

    return err;
}