#include "include/taskMonitor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

//for logging 
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "Task Monitor: ";

void taskMonitor(void * pvParameters){

    ESP_LOGI(TAG, "Taks Monitor thread started");

    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    unsigned long ulTotalRunTime;
    unsigned long ulStatsAsPercentage;

    /* Make sure the write buffer does not contain a string. */
    char writeBuffer[50];

    while (1)
    {      
        //Capture number of running task
        uxArraySize = uxTaskGetNumberOfTasks();

        //Create a TaskStatus struct for each task
        pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

        if( pxTaskStatusArray != NULL )
        {
            /* Generate raw status information about each task. */
            uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                                uxArraySize,
                                                &ulTotalRunTime );


            /* For percentage calculations. */
            ulTotalRunTime /= 100UL;

            /* Avoid divide by zero errors. */
            if( ulTotalRunTime > 0 )
            {
            /* For each populated position in the pxTaskStatusArray array,
            format the raw data as human readable ASCII data. */
                for( x = 0; x < uxArraySize; x++ ) {
                    /* What percentage of the total run time has the task used?
                    This will always be rounded down to the nearest integer.
                    ulTotalRunTimeDiv100 has already been divided by 100. */
                    ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

                    if( ulStatsAsPercentage > 0UL ) {
                        ESP_LOGI(TAG, "%s%s", "Task name: ", pxTaskStatusArray[ x ].pcTaskName);
                        ESP_LOGI(TAG, "%s%u", "Task time counter: ", pxTaskStatusArray[ x ].ulRunTimeCounter);
                        ESP_LOGI(TAG, "%s%lu", "Part of total run time: ", ulStatsAsPercentage);
                        sprintf( writeBuffer, "%stt%utt%lu%%rn",
                                    pxTaskStatusArray[ x ].pcTaskName,
                                    pxTaskStatusArray[ x ].ulRunTimeCounter,
                                    ulStatsAsPercentage );
                        
                    }
                    else
                    {
                        /* If the percentage is zero here then the task has
                        consumed less than 1% of the total run time. */
                        sprintf( writeBuffer, "%stt%utt<1%%rn",
                                    pxTaskStatusArray[ x ].pcTaskName,
                                    pxTaskStatusArray[ x ].ulRunTimeCounter );
                        
                    }

                    *writeBuffer += strlen( ( char * ) writeBuffer );
                }

            }

            /* The array is no longer needed, free the memory it consumes. */
            vPortFree( pxTaskStatusArray );
            ESP_LOGI(TAG, "Monitoring done");
        }

        //TODO send out udp message

        //TODO adjust this
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}  

void init_taskMonitor(void) {
    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    ESP_LOGI(TAG, "in init thread");

    xReturned = xTaskCreatePinnedToCore( taskMonitor,
                            "Task Monitor",
                            TASK_MONITOR_STACK_SIZE,  //TODO UPDATE 
                            NULL, 
                            TASK_MONITOR_PRIORITY,    //TODO UPDATE
                            &xHandle,
                            1);
    
}
 