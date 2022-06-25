/* Monitoring leaf node example

*/

#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"

#include "sockets.h"
#include "taskMonitor.h"
#include "udp_send.h"
#include "global_config.h"

#define HOST_IP_ADDR "192.168.1.1"
#define PORT         35500

/*Generates a random number
*/
int randomHeartRate(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
    
}

static void heartRateTask(void * pvParameters ){

    // Use current time as seed for random generator
    srand(time(0));

    while (1){
        // Get heartRate (sensor abstracted by random function)
        int heartRate = randomHeartRate(60, 200);
        ESP_LOGI("HR", "%d",heartRate);
        
        //Send heartRate to server   
        int addr_family = 0;
        int ip_protocol = 0;
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        char payload[4];

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        sprintf(payload,"%d", heartRate);
        sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        shutdown(sock, 0);
        close(sock);

        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     */
    ESP_ERROR_CHECK(example_connect());

    /* This functions configures the monitoring on Freertos
    */
    init_taskMonitor();




    //Use Case
    
    /* This create a periodic task for heart rate monitoring
    */
    xTaskCreatePinnedToCore(heartRateTask,
                            "Heart Rate",
                            4096,  
                            NULL, 
                            3,    
                            NULL,
                            0);

    vTaskDelay(pdMS_TO_TICKS(4000));

    DoSTest();
}
