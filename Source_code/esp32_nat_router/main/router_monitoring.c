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

#include "includes/cmd_decl.h"

#include "includes/router_monitoring.h"
#include "includes/sockets.h"
#include "includes/udp_send.h"
#include "includes/global_config.h"

static const char *TAG = "(ROUTER_MONITORING)";
static int connect_count_m = 0;
ip_event_got_ip_t* ip_event;
ip_event_ap_staipassigned_t* mac_event;
char msg_data[256];


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
    if(i != EXPECTED_STATIONS || j != 6){
        sprintf(msg_data, "%s: WARNING: Station MAC address not in list of expected values! ("MACSTR")",TAG, MAC2STR(mac_addr));
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, msg_data);  //Sending data to remote server
    }
}

void filter_network_params()
{
    ESP_LOGI(TAG,"Number of connected stations: %d",connect_count_m);
    //Check that the number of stations is within the expected range
    if( connect_count_m < MINIMUM_STATION_THRESHOLD){
        sprintf(msg_data, "%s: WARNING: Number of connected stations below minimum threshold! Connected = %d, Minimum expected = %d",TAG, connect_count_m, MINIMUM_STATION_THRESHOLD);
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, msg_data);
    }
    
    if( connect_count_m > MAXIMUM_STATION_THRESHOLD){
        sprintf(msg_data, "%s: WARNING: Number of connected stations above maximum threshold! Connected = %d, Maximum expected = %d",TAG, connect_count_m, MAXIMUM_STATION_THRESHOLD);
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, msg_data);  //Sending data to remote server
    }
        

}


//Handler that is posted to the event loop and manages the wifi events
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
    case WIFI_EVENT_AP_STACONNECTED:
        connect_count_m++;
        ESP_LOGI(TAG,"%d. station connected", connect_count_m);
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        validate_station_mac_addr(event->mac);
        filter_network_params();
        break;
    case WIFI_EVENT_AP_STADISCONNECTED:
        connect_count_m--;
        ESP_LOGI(TAG,"station disconnected - %d remain", connect_count_m);
        filter_network_params();
        break;
    default:
        break;
  }
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
        vTaskDelay( xDelay );
    }
}


//Create the monitoring task and pin it to the Core 1
void setup_router_monitoring(void)
{
    int CSES_STATS_TASK_PRIO = 1;

    
    xTaskCreatePinnedToCore(init_router_monitoring, "router_monitor", 8192, NULL, 1, NULL, 1);
}
