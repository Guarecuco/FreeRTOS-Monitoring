#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "cses_hw_monitor.h"
#include "cses_wifi.h"
#include "cses_udp.h"

static const char *TAG = "main";

void app_main(void)
{
    //Starting Wifi
    setup_wifi();

    //Anouncing to server that node is connected
    char syslog_msg[256];
    int priority = FACILITY_CODE*8 + Warning;
    sprintf(syslog_msg, "<%d>1 - %s %s - - - This node is now connected", priority, CSES_HOSTNAME, TAG);
    udp_send_msg(syslog_msg);

    //Starting monitoring thread on Core 1
    setup_cpu_monitoring();



}
