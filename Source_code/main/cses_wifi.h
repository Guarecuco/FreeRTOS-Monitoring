#ifndef __T_CSES_WIFI_H__
#define __T_CSES_WIFI_H__

#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_SSID      "FRITZ!Box 7530 ZE"
#define WIFI_PASS      "86500691595796239417"
#define MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void setup_wifi(void);

#endif