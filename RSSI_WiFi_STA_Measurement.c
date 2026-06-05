// Wi-Fi STA network on ESP32 using the ESP-IDF 5.0 framework.
// RSSI will be measured and printed to the console every 5 seconds.

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "my_data.h"

volatile bool wifi_ready = false;

// Periodically measure RSSI for the connected router (STA mode) and print it to the console.
void rssi_task(void *pv) {
    while (1) {
        if (!wifi_ready) {
            printf("WiFi not ready - cannot read RSSI\n");
        } else {
            wifi_ap_record_t ap_info;
            esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
            if (err == ESP_OK) {
                printf("Connected with RSSI: %d dBm\n", ap_info.rssi);
            } else {
                printf("esp_wifi_sta_get_ap_info failed: %d\n", err);
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void wifi_event_handler(void *event_handler_arg,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void *event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            printf("WiFi connecting WIFI_EVENT_STA_START ... \n");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            printf("WiFi connected WIFI_EVENT_STA_CONNECTED ... \n");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            printf("WiFi lost connection WIFI_EVENT_STA_DISCONNECTED. Retrying connection...");
            esp_wifi_connect();
            break;
        case IP_EVENT_STA_GOT_IP:
            printf("WiFi got IP ... \n\n");
            wifi_ready = true;
            break;
        default:
            break;
    }
}

void wifi_connection(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID_STA,
            .password = PASS_STA,
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_start();
    esp_wifi_connect();

    while (!wifi_ready) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) { 
    wifi_connection();

    // start RSSI measurement task
    xTaskCreate(rssi_task, "rssi_task", 4096, NULL, 5, NULL);
    while (1) {
        printf("WiFi is connected and ready to use ... \n");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
