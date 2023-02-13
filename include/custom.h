#define atkinson
//#define bbs
//#define bbstst

// *** SENSORS *** //
#define DHT_active  // Uncomment to activate DHT sensor on defined port
//#define MOIST_BINARY_ACTIVE  // Binary moisture sensor active
//#define AUTONOMOUS_DELAY_ACTIVE // Will activate autonomus activity (no MQTT server)
#define AUTONOMOUS_DELAY_MINS 2 //1440  // How long between triggers

// WiFi
#define WIFI_CHANNEL 1   // 1, 5, 9, 13 etc
#ifdef bbstst
    char baseMQTT[] = "school/garden";  // Base
    char area[] = "greenhouse"; // area in base - This is also used as the client ID, so needs to be unique

    #define STATION_SSID  "Atkinson"
    #define STATION_PASSWORD "ferret11"
    #define MQTT_USER "bbsmqtt"
    #define MQTT_PASSWORD "ferret11"
    #define MQTT_SERVER "broadbayschool.local"
#endif
#ifdef bbs
    char baseMQTT[] = "school/garden";  // Base
    char area[] = "greenhouse"; // area in base - This is also used as the client ID, so needs to be unique

    #define STATION_SSID  "Broadbay"
    #define STATION_PASSWORD "yellowh34d"
    #define MQTT_USER "bbsmqtt"
    #define MQTT_PASSWORD "ferret11"
    #define MQTT_SERVER "broadbayschool.local"
#endif

#ifdef atkinson
    char baseMQTT[] = "home/garden";  // Base
    //char area[] = "greenhouse"; // area in base - This is also used as the client ID, so needs to be unique
    //char area[] = "front"; // area in base
    char area[] = "test"; // area in base
    //char area[] = "veg2"; // area in base

    #define STATION_SSID  "Atkinson"
    // #define STATION_SSID  "Atkinson-Boost"
    #define STATION_PASSWORD "ferret11"
    #define MQTT_PASSWORD "ferret11"
    #define MQTT_USER "atkmqtt"
    #define MQTT_SERVER "haatkinson.local"
#endif
char state[] = "state"; // Refer comments below
// --> baseMQTT + area + valve_name + payload (ON, OFF) | MQTT on / off commands
// --> baseMQTT + area + state + valve_name + payload (ON, OFF)  | Confirmation of valve state sent from client

// MQTT service