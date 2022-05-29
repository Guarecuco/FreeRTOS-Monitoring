#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#include "cses_udp.h"
#include "cses_wifi.h"

extern EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "cses_udp";

/*
This function provides a easy way to send a string to a Syslog server
HOST_IP_ADD and PORT and defined in cses_udp.h
*/
void udp_send_msg(char *msg)
{
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_CONNECTED_BIT){
        int addr_family = AF_INET;
        int ip_protocol = IPPROTO_IP;

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            return;
        } 
        int err = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending syslog: errno %d", errno);
            return;
        }
        ESP_LOGI(TAG, "Syslog Message sent to%s:%d", HOST_IP_ADDR, PORT);
        shutdown(sock, 0);
        close(sock);
    } else {
        ESP_LOGE(TAG, "Wi-Fi not connected, cannot send message");
    }     
}