#include "includes/taskMonitor.h"


#include <string.h>

//for logging 
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "includes/udp_send.h"
#include "includes/global_config.h"


static const char *TAG = "Task Monitor";

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


        vTaskDelay(pdMS_TO_TICKS(12000));


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

        ESP_LOGI(TAG, "\n%s %d", "number of tasks created: ", created_count);

        // DoS attack handling
        if (created_count > THRESHOLD) {
            DoS_Monitoring(end_array, end_array_size, DoSlength, DoSwriteBuffer, created_count);
        }

        ESP_LOGI(TAG, "\n%s\n%s", "content of write buffer: ", writeBuffer);
        udp_send_msg(UDP_SERVER_IP, UDP_SERVER_PORT, writeBuffer);  //Sending data to remote server

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

//--------------------------------------- FILTERING FUNCTIONS --------------------------------------------------

// Denial of Service
void DoS_Monitoring(TaskStatus_t *pxTaskStatusArray, UBaseType_t array_size, int buffer_length, char *writeBuffer, int count) {
    buffer_length += snprintf( writeBuffer + buffer_length, TASK_BUFFER_SIZE - buffer_length,"| DoS suspected, number of created tasks: %u\n", count);
    buffer_length += snprintf( writeBuffer + buffer_length, TASK_BUFFER_SIZE - buffer_length,"| Task name | Task base priority \n");
    for (int i = 0; i < array_size; i++) {
        if (pxTaskStatusArray[i].xHandle != NULL) {
            buffer_length += snprintf( writeBuffer + buffer_length, TASK_BUFFER_SIZE - buffer_length,"| %s | %u \n", 
                                    pxTaskStatusArray[i].pcTaskName, 
                                    pxTaskStatusArray[i].uxBasePriority);
        }
    }

    ESP_LOGI("DoS detection: ", "%s", writeBuffer);

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


//----------------------------------------- TEST FUNCTIONS -----------------------------------------------------

void DoSTask(void * param) {
    int *num1 = param;
    int num = num1;

    ESP_LOGI("Task number starting: ", "%d", num);

    while(1) {
        ESP_LOGI("Task number running: ", "%d", num);
        vTaskDelay(pdMS_TO_TICKS(5600));
    }
}

void DoSTest(void) {

    ESP_LOGI("Starting DoS test", "%d", 1);

    for(int i = 0; i < THRESHOLD +1; i++) {
        int y = i;
        xTaskCreatePinnedToCore(DoSTask,
                        "DoS test",
                        4096,  
                        y, 
                        3,    
                        NULL,
                        0);

        vTaskDelay(pdMS_TO_TICKS(500));
    }

}