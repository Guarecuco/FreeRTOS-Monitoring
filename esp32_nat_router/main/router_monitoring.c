#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "sdkconfig.h"

#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "cmd_decl.h"
#include <esp_http_server.h>

#include "router_monitoring.h"

static const char *TAG = "Router monitoring";
static int connect_count_m = 0;
ip_event_got_ip_t* ip_event;

static void monitoring_event_handler(void *handler_args, esp_event_base_t event, int32_t id, void* event_data)
{
  esp_netif_dns_info_t dns;
  //ESP_LOGI(TAG, "Event recieved! %d", id);
  switch(id) {
    case SYSTEM_EVENT_STA_START:
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ip_event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&ip_event->ip_info.ip));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG,"disconnected - retry to connect to the AP");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        connect_count_m++;
        ESP_LOGI(TAG,"%d. station connected", connect_count_m);
        ESP_LOGI(TAG,"STA MAC@: %s\n", ip4addr_ntoa(&event->event_info.sta_connected.mac));
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        connect_count_m--;
        ESP_LOGI(TAG,"station disconnected - %d remain", connect_count_m);
        break;
    default:
        break;
  }
}

void init_router_monitoring(void *arg)
{
    ESP_LOGI(TAG,"Initializing esp32_nat_router monitoring...");
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
    vTaskDelay( xDelay );

    int ret = esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, monitoring_event_handler, NULL);
    //ESP_LOGI(TAG,"Handler registered with result %d", ret);

    while(1)
    {
        vTaskDelay( xDelay );
    }
}

void setup_router_monitoring(void)
{
    int CSES_STATS_TASK_PRIO = 1;
    xTaskCreatePinnedToCore(init_router_monitoring, "router_monitor", 4096, NULL, CSES_STATS_TASK_PRIO, NULL, 1);
    //udp_logging_init(TARGET_IP, TARGET_PORT, udp_logging_vprintf );
}
