#include "include/taskMonitor.h"


#include <string.h>

//for logging 
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#define THRESHOLD 5
#define TASK_BUFFER_SIZE 500

static const char *TAG = "Task Monitor";

void taskMonitor(void * pvParameters){

    ESP_LOGI(TAG, "Task Monitor thread started");

    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    unsigned long ulTotalRunTime;
    unsigned long ulStatsAsPercentage;

    /* Make sure the write buffer does not contain a string. */
    //char writeBuffer = (( char ) pvParameters);
    char writeBuffer[TASK_BUFFER_SIZE];
    int length = 0;


    while (1)
    { 
        sprintf(writeBuffer, "%u", 0);
        length = 0;      
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

            DoS_Monitoring(pxTaskStatusArray);

            ESP_LOGI(TAG, "done filtering");

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
                        length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length, "%s %u %lu%% \r\n",
                                            pxTaskStatusArray[ x ].pcTaskName,
                                            pxTaskStatusArray[ x ].ulRunTimeCounter,
                                            ulStatsAsPercentage );
                    }
                    else
                    {
                        /* If the percentage is zero here then the task has
                        consumed less than 1% of the total run time. */
                        length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length, "%s %u \r\n",
                                            pxTaskStatusArray[ x ].pcTaskName,
                                            pxTaskStatusArray[ x ].ulRunTimeCounter );
                        
                    }
                }

            }

            /* The array is no longer needed, free the memory it consumes. */
            vPortFree( pxTaskStatusArray );
            
        }

        ESP_LOGI(TAG, "%s \n %s", "content of write buffer: ", writeBuffer);

        vTaskDelay(pdMS_TO_TICKS(4000));
    }

}  

void init_taskMonitor(void) {
    ESP_LOGI(TAG, "in init function");

    xTaskCreatePinnedToCore( taskMonitor,
                            "Task Monitor",
                            TASK_MONITOR_STACK_SIZE,  
                            NULL, 
                            TASK_MONITOR_PRIORITY,    
                            NULL,
                            1);
    
}

//------------------------------FILTERING FUNCTIONS-----------------------------------------------

// Denial of Service
void DoS_Monitoring(TaskStatus_t *pxTaskStatusArray) {
    static UBaseType_t lastArraySize;
    volatile UBaseType_t currentArraySize, x;

    char pcWriteBuffer[TASK_BUFFER_SIZE];
    int length = 0;

    ESP_LOGI(TAG, "in filter function");

    currentArraySize = uxTaskGetNumberOfTasks();
    int dxduArraySize = currentArraySize - lastArraySize;
    if (dxduArraySize >= THRESHOLD ) {
        length += snprintf(pcWriteBuffer + length, TASK_BUFFER_SIZE - length, "%s %u\nTasks:", "Delta tasks:", dxduArraySize);
        for( x = 0; x < currentArraySize; x++ ) {
        
            length += snprintf( pcWriteBuffer + length, TASK_BUFFER_SIZE - length, "%s %u \n",
                    pxTaskStatusArray[ x ].pcTaskName,
                    pxTaskStatusArray[ x ].uxCurrentPriority);               
        }
        ESP_LOGI(TAG, "%s %s","Filter: ", pcWriteBuffer);
    }

    lastArraySize = currentArraySize;
}

// Task starvation
