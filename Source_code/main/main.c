#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "cses_hw_monitor.h"
#include "cses_wifi.h"

static const char *TAG = "main";

void app_main(void)
{
    //Starting Wifi
    setup_wifi();

    //Starting monitoring thread on Core 1
    setup_cpu_monitoring();
}
