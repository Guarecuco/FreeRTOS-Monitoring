#ifndef __T_CSES_UDP_H__
#define __T_CSES_UDP_H__

#define HOST_IP_ADDR "169.46.82.175"
#define PORT 35208

#define FACILITY_CODE 16

enum SysLogLevel{
    Alert = 1,
    Critical = 2,
    Error = 3,
    Warning = 4,
    Notice = 5,
    Info = 6,
    Debug = 7
} ;

void udp_send_msg(char *msg);

#endif