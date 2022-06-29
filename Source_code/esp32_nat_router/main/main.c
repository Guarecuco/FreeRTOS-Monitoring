#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "includes/esp32_nat_router.h"
#include "includes/router_monitoring.h"
#include "includes/udp_send.h"
#include "includes/taskMonitor.h"

void app_main(void)
{
    // Starting monitoring thread on Core 1
    setup_router_monitoring();

    // Starting router
    init_nat_router();

    // Starting task monitor
    init_taskMonitor();
}