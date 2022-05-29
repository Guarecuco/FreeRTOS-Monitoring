#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "sdkconfig.h"

#include "cses_hw_monitor.h"
#include "cses_udp.h"

static const char *TAG = "cses_hw_monitor";

static esp_err_t chip_info(void){
    char syslog_msg[256];
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    //Send syslog message
    sprintf(syslog_msg, "<%d>1 - %s %s - - - This is a %s chip with %d CPU core(s), WiFi%s%s, Silicon revision %d",
     FACILITY_CODE*8 + Info, CSES_HOSTNAME, TAG, CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", chip_info.revision);
    udp_send_msg(syslog_msg);
    //Print same message to console
    ESP_LOGI(TAG,"This is a %s chip with %d CPU core(s), WiFi%s%s, Silicon revision %d ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", chip_info.revision);

    int flash_size = spi_flash_get_chip_size() / (1024 * 1024);
    //Send syslog message
    sprintf(syslog_msg, "<%d>1 - %s %s - - - Flash size: %d MB %s",
    FACILITY_CODE*8 + Info, CSES_HOSTNAME, TAG, flash_size,
     (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    udp_send_msg(syslog_msg);
    //Print same message to console
    ESP_LOGI(TAG, "Flash size: %d MB %s", flash_size, (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    
    int free_heap = xPortGetFreeHeapSize();
    int free_heap_ever = xPortGetMinimumEverFreeHeapSize();
    //Send syslog
    sprintf(syslog_msg, "<%d>1 - %s %s - - - Free heap size: %d bytes, Minimun heap size ever: %d bytes",
    FACILITY_CODE*8 + Info, CSES_HOSTNAME, TAG, free_heap, free_heap_ever);
    udp_send_msg(syslog_msg);
    //Print same message to console
    ESP_LOGI(TAG, "Free heap size: %d bytes", free_heap);
    ESP_LOGI(TAG, "Minimun heap size ever: %d bytes", free_heap_ever);

    return ESP_OK;
}



static esp_err_t cpu_stats(TickType_t xTicksToWait)
{
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t numberTasks, start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;
    esp_err_t ret;
    char syslog_msg[256];
    int priority;

    //Gets the number of tasks
    numberTasks = uxTaskGetNumberOfTasks();
    sprintf(syslog_msg, "<%d>1 - %s %s - - - Running tasks: %d", FACILITY_CODE*8 + Info, CSES_HOSTNAME, TAG, numberTasks);
    udp_send_msg(syslog_msg);
    ESP_LOGI(TAG,"Running tasks: %d", numberTasks);
        
    //Allocate array to store current task states
    start_array_size = numberTasks + CSES_ARRAY_SIZE_OFFSET;
    start_array = malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + CSES_ARRAY_SIZE_OFFSET;
    end_array = malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    //Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

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

            //Send warning to syslog server if CPU usage > CPU_THRESHOLD
            if ( percentage_time > CPU_THRESHOLD){
                priority = FACILITY_CODE*8 + Warning;
                sprintf(syslog_msg, "<%d>1 - %s %s - - - Task: %s used %d%% over the last %d ms", priority, CSES_HOSTNAME, TAG, start_array[i].pcTaskName, percentage_time, CSES_STATS_MS_SECONDS);
                udp_send_msg(syslog_msg);
                ESP_LOGW(TAG, "Task: %s used %d%% over the last %d ms", start_array[i].pcTaskName, percentage_time, CSES_STATS_MS_SECONDS);
            }
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            priority = FACILITY_CODE*8 + Warning;
            sprintf(syslog_msg, "<%d>1 - %s %s - - - Task: %s was deleted", priority, CSES_HOSTNAME, TAG, start_array[i].pcTaskName);
            udp_send_msg(syslog_msg);
            ESP_LOGW(TAG, "Task: %s was deleted", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            priority = FACILITY_CODE*8 + Warning;
            sprintf(syslog_msg, "<%d>1 - %s %s - - - Task: %s was created", priority, CSES_HOSTNAME, TAG, end_array[i].pcTaskName);
            udp_send_msg(syslog_msg);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
    free(start_array);
    free(end_array);
    return ret;
}


//Entry function
static void stats_task(void *arg)
{
    //Print real time stats periodically
    while (1) {
        if (cpu_stats(CSES_STATS_TICKS) == ESP_OK) {
            ESP_LOGI(TAG, "Real time stats obtained");
        } else {
            ESP_LOGE(TAG, "Error getting real time stats");
        }
        chip_info();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


//Schedule the task to the FreeRTOS scheduler
void setup_cpu_monitoring(void)
{
    xTaskCreatePinnedToCore(stats_task, "hw_monitor", 4096, NULL, CSES_STATS_TASK_PRIO, NULL, 1);

}