#ifndef __T_MONITORING_H__
#define __T_MONITORING_H__

#define CSES_STATS_TASK_PRIO     3
#define CSES_STATS_MS_SECONDS    1000   //1000 ms = 1 s
#define CSES_STATS_TICKS         pdMS_TO_TICKS(CSES_STATS_MS_SECONDS)
#define CSES_ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE
#define CPU_THRESHOLD            40

#define CSES_HOSTNAME "ESP32_1"

void setup_cpu_monitoring(void);

#endif