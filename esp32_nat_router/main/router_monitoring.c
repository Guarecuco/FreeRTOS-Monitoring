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

#include "router_monitoring.h"
#include "sockets.h"
#include "udp_send.h"
#include "global_config.h"

static const char *TAG = "Router monitoring";
static int connect_count_m = 0;
ip_event_got_ip_t* ip_event;
ip_event_ap_staipassigned_t* mac_event;

int VALID_MAC_ADDRS[EXPECTED_STATIONS][6] = { //6 is MAC len in bytes
    //{0x11, 0x00, 0x00, 0x00, 0x4c, 0x02} Juan's phone MAC addr
    {0x11, 0x00, 0x00, 0x00, 0x4c, 0x03}
};

void validate_station_mac_addr(uint8_t* mac_addr)
{
    int i;
    int j;
    for(i = 0; i < EXPECTED_STATIONS; i = i+1){
        for(j = 0; j < 6; j = j + 1){
            if(mac_addr[j] != VALID_MAC_ADDRS[i][j])
                break;
        }
        if(j == 6)
            break;
    }
    ESP_LOGI(TAG,"Values after MAC validation: (i, j) : (%d, %d)", i, j);
    if(i != EXPECTED_STATIONS || j != 6)
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, "WARNING: Station MAC address not in list of expected values!");  //Sending data to remote server
}

static void monitoring_event_handler(void *handler_args, esp_event_base_t event, int32_t id, void* event_data)
{
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
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        validate_station_mac_addr(event->mac);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        connect_count_m--;
        ESP_LOGI(TAG,"station disconnected - %d remain", connect_count_m);
        break;
    default:
        break;
  }
}

void filter_network_params()
{
    ESP_LOGI(TAG,"Number of connected stations: %d",connect_count_m);

    //Check that the number of stations is within the expected range
    if( (connect_count_m/EXPECTED_STATIONS) < MINIMUM_STATION_THRESHOLD)
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, "WARNING: Number of connected stations below minimum threshold!");  //Sending data to remote server
    
    if( (connect_count_m/EXPECTED_STATIONS) > MAXIMUM_STATION_THRESHOLD)
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, "WARNING: Number of connected stations above maximum threshold!");  //Sending data to remote server

}


void init_router_monitoring(void *arg)
{
    ESP_LOGI(TAG,"Initializing esp32_nat_router monitoring...");

    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    vTaskDelay( xDelay );

    //Register the event handler
    esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, monitoring_event_handler, NULL);

    while(1)
    {
        filter_network_params();
        vTaskDelay( xDelay );
    }
}



void setup_router_monitoring(void)
{
    int CSES_STATS_TASK_PRIO = 1;
    xTaskCreatePinnedToCore(init_router_monitoring, "router_monitor", 4096, NULL, CSES_STATS_TASK_PRIO, NULL, 1);
}
