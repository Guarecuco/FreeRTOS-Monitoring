#ifndef __TASK_MONITOR_H__
#define __TASK_MONITOR_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// These variables must be set in the config file
// configSUPPORT_DYNAMIC_ALLOCATION = 1
// configUSE_TRACE_FACILITY = 1

#define TASK_MONITOR_STACK_SIZE 4096
#define TASK_MONITOR_PRIORITY 4
#define THRESHOLD 5

#define TASK_BUFFER_SIZE 500
#define DELAY_TIME 4000
#define ARRAY_SIZE_OFFSET 5

void init_taskMonitor(void);
void taskMonitor_filtering(void * pvParameters);
void taskMonitor_rawData(void * pvParameters);
void DoS_Monitoring(TaskStatus_t *pxTaskStatusArray, UBaseType_t array_size, int buffer_length, char *writeBuffer, int count);
void DoSTask(void * param);
void DoSTest(void);

#endif