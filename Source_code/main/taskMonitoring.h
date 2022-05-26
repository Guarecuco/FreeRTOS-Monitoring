#ifndef __T_MONITORING_H__
#define __T_MONITORING_H__

#define STATS_TASK_PRIO     3
#define STATS_MS_SECONDS    1000   //1000 ms = 1 s
#define STATS_TICKS         pdMS_TO_TICKS(STATS_MS_SECONDS)
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

void startMonitoring(void);

#endif