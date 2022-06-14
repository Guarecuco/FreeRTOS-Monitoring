#include "include/taskMonitor.h"


#include <string.h>

//for logging 
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

#define THRESHOLD 5
#define TASK_BUFFER_SIZE 500
#define DELAY_TIME 4000
#define ARRAY_SIZE_OFFSET 5

static const char *TAG = "Task Monitor";

void taskMonitor2(void * pvParameters){

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
                        length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length, "%s %u %lu%%\r\n",
                                            pxTaskStatusArray[ x ].pcTaskName,
                                            pxTaskStatusArray[ x ].ulRunTimeCounter,
                                            ulStatsAsPercentage );
                    }
                    else
                    {
                        /* If the percentage is zero here then the task has
                        consumed less than 1% of the total run time. */
                        length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length, "%s %u\r\n",
                                            pxTaskStatusArray[ x ].pcTaskName,
                                            pxTaskStatusArray[ x ].ulRunTimeCounter );
                        
                    }
                }

            }

            /* The array is no longer needed, free the memory it consumes. */
            vPortFree( pxTaskStatusArray );
            
        }

        ESP_LOGI(TAG, "\n%s\n%s", "content of write buffer: ", writeBuffer);

        vTaskDelay(pdMS_TO_TICKS(4000));
    }

}  

void taskMonitor(void * pvParameters){

    ESP_LOGI(TAG, "Task Monitor thread started");
    static char writeBuffer[TASK_BUFFER_SIZE];
    static char DoSwriteBuffer[TASK_BUFFER_SIZE];
    static int length = 0;
    static int DoSlength = 0;


    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;

    while(1) {
        
        sprintf(writeBuffer, "%u", 0);
        length = 0;  

        //Allocate array to store current task states
        start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
        start_array = malloc(sizeof(TaskStatus_t) * start_array_size);

        //Get current task states
        start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);


        vTaskDelay(DELAY_TIME);

        //Allocate array to store tasks states post delay
        end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
        end_array = malloc(sizeof(TaskStatus_t) * end_array_size);

        //Get post delay task states
        end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);


        //Calculate total_elapsed_time in units of run time stats clock period.
        uint32_t total_elapsed_time = (end_run_time - start_run_time);


        length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length, "%s\n", "| Task | Run Time | Percentage");
        //Match each task in start_array to those in the end_array
        for (int i = 0; i < start_array_size; i++) {
            int k = -1;
            for (int j = 0; j < end_array_size; j++) {
                if (start_array[i].xHandle == end_array[j].xHandle) {
                    k = j;
                    //Mark that task have been matched by overwriting their handles
                    start_array[i].xHandle = NULL;
                    end_array[j].xHandle = NULL;
                    break;
                }
            }
            //Check if matching task found
            if (k >= 0) {
                uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
                uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
                length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length, "| %s | %d | %d%%\n", 
                                    start_array[i].pcTaskName, 
                                    task_elapsed_time, 
                                    percentage_time);
            }
        }

        //Print unmatched tasks
        for (int i = 0; i < start_array_size; i++) {
            if (start_array[i].xHandle != NULL) {
                length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length,"| %s | Deleted\n", start_array[i].pcTaskName);
            }
        }


        int created_count = 0;
        for (int i = 0; i < end_array_size; i++) {
            if (end_array[i].xHandle != NULL) {
                length += snprintf( writeBuffer + length, TASK_BUFFER_SIZE - length,"| %s | Created\n", end_array[i].pcTaskName);
                created_count += 1;
            }
        }

        // DoS attack handling
        if (created_count > THRESHOLD) {
            DoS_Monitoring2(end_array, end_array_size, DoSlength, DoSwriteBuffer, created_count);
        }

        ESP_LOGI(TAG, "\n%s\n%s", "content of write buffer: ", writeBuffer);

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

void DoS_Monitoring2(TaskStatus_t *pxTaskStatusArray, UBaseType_t array_size, int buffer_length, char *writeBuffer, int count) {
    buffer_length += snprintf( writeBuffer + buffer_length, TASK_BUFFER_SIZE - buffer_length,"| DoS suspected, number of created tasks: %u\n", count);
    buffer_length += snprintf( writeBuffer + buffer_length, TASK_BUFFER_SIZE - buffer_length,"| Task name | Task base priority \n");
    for (int i = 0; i < array_size; i++) {
        if (pxTaskStatusArray[i].xHandle != NULL) {
            buffer_length += snprintf( writeBuffer + buffer_length, TASK_BUFFER_SIZE - buffer_length,"| %s | %u \n", 
                                    pxTaskStatusArray[i].pcTaskName, 
                                    pxTaskStatusArray[i].uxBasePriority);
        }
    }

    ESP_LOGI(TAG, "%s %s","Filter: ", writeBuffer);

    //TODO send to udp

}

// Task starvation
void starvation_Monitoring() {
    // Take tasks before they get matched
    // Check if they are suspended in both samples
    // Check that they run on core 0
    // It a task is suspended for a threshold number of periods
        // Suspect starvation, send msg on which task
}
