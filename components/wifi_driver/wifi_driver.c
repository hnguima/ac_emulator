#include "wifi_driver.h"

static const char *TAG = "WIFI_AP";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_driver_init(wifi_data_t *config)
{

    esp_err_t err = ESP_OK;

    esp_netif_t *my_ap = esp_netif_create_default_wifi_ap();

    // ESP_ERROR_CHECK(esp_netif_dhcps_stop(my_ap));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(my_ap));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                                      &wifi_event_handler,
                                                                      NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };

    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 5, 5);
    IP4_ADDR(&ip_info.gw, 192, 168, 5, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_ip_info(my_ap, &ip_info));

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(my_ap));
    // ESP_ERROR_CHECK(esp_netif_dhcps_start(my_ap));

    strcpy((char *)&wifi_config.ap.ssid, config->ssid);
    strcpy((char *)&wifi_config.ap.password, config->password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /*ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);*/
}
