/* FreeRTOS Real Time Stats Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "taskMonitoring.c"
#include "cses_wifi.c"
#include "cses_udp.h"

void app_main(void)
{
    //Starting Wifi
    setup_wifi();

    //Anouncing to server that node is connected
    char syslog_msg[256];
    int priority = FACILITY_CODE*8 + Warning;
    sprintf(syslog_msg, "<%d>1 - %s %s - - - This node is now connected", priority, HOSTNAME, "Main");
    udp_send_msg(syslog_msg);

    //Starting monitoring thread on Core 1
    setup_cpu_monitoring();



}
