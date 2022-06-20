#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "esp32_nat_router.h"
#include "router_monitoring.h"

void app_main(void)
{
    //Starting monitoring thread on Core 1
    setup_router_monitoring();

    //Starting router
    init_nat_router();

    
}