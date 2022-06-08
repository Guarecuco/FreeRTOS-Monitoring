#ifndef __TASK_MONITOR_H__
#define __TASK_MONITOR_H__

// These variables must be set in the config file
// configSUPPORT_DYNAMIC_ALLOCATION = 1
// configUSE_TRACE_FACILITY = 1

#define TASK_MONITOR_STACK_SIZE 4096
#define TASK_MONITOR_PRIORITY 3

void init_taskMonitor(void);
void taskMonitor(void * pvParameters);

#endif