#include "wifi_driver.h"

static const char *TAG = "wifi_sta";

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}
// d3sn3tw1f1

void wifi_driver_init(wifi_data_t *config)
{

    nvs_flash_init();

    esp_err_t err = ESP_OK;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());

    esp_netif_t *wifi_netif = esp_netif_create_default_wifi_sta();

    // ESP_ERROR_CHECK(esp_netif_dhcps_stop(my_ap));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(WIFI_EVENT,
                                                                      ESP_EVENT_ANY_ID,
                                                                      &event_handler,
                                                                      NULL,
                                                                      &instance_any_id));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(IP_EVENT,
                                                                      IP_EVENT_STA_GOT_IP,
                                                                      &event_handler,
                                                                      NULL,
                                                                      &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "centaurus",
            .password = "d3sn3tw1f1",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    // disable DHCP
    err = esp_netif_dhcpc_stop(wifi_netif);
    if (err)
        ESP_LOGI(TAG, "Failed to stop DHCPC");

    esp_netif_ip_info_t ip_info;

    ip_info.ip.addr = PP_HTONL(LWIP_MAKEU32(192, 168, 2, 243));
    ip_info.netmask.addr = PP_HTONL(LWIP_MAKEU32(255, 255, 255, 0));
    ip_info.gw.addr = PP_HTONL(LWIP_MAKEU32(192, 168, 2, 254));

    esp_netif_set_ip_info(wifi_netif, &ip_info);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
