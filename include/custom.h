// Custom settings
char baseMQTT[] = "home/garden";  // Base
// char baseMQTT[] = "school/garden";  // Base
// char area[] = "test"; // area in base
char area[] = "greenhouse"; // area in base - This is also used as the client ID, so needs to be unique
// char area[] = "bed1"; // area in base
// char area[] = "bed2"; // area in base
char state[] = "state"; // Refer comments below
// --> baseMQTT + area + valve_name + payload (ON, OFF) | MQTT on / off commands
// --> baseMQTT + area + state + valve_name + payload (ON, OFF)  | Confirmation of valve state sent from client

// *** SENSORS *** //
#define DHT_active  // Uncomment to activate DHT sensor on defined port

// WiFi
#define WIFI_CHANNEL 1   // 1, 5, 9, 13 etc

// #define STATION_SSID  "Broadbay"
#define STATION_SSID  "Atkinson"
// #define STATION_SSID  "Atkinson-Boost"
// #define STATION_PASSWORD "yellowh34d"
#define STATION_PASSWORD "ferret11"

// MQTT service
#define MQTT_USER "atkmqtt"
// #define MQTT_USER "bbsmqtt"
#define MQTT_PASSWORD "ferret11"
#define MQTT_SERVER "192.168.1.150"
// #define MQTT_SERVER "192.168.22.136"