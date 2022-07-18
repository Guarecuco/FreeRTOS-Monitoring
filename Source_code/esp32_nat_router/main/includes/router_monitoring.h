#define EXPECTED_STATIONS 3

#define MINIMUM_STATION_THRESHOLD 2
#define MAXIMUM_STATION_THRESHOLD 3

#define TARGET_IP       "192.168.4.2"
#define TARGET_PORT     443

int VALID_MAC_ADDRS[EXPECTED_STATIONS][6] = { //6 is MAC len in bytes
    {0x11, 0x00, 0x00, 0x00, 0x4c, 0x03} //Insert your whitelist MAC addresses here
};

void setup_router_monitoring(void);
