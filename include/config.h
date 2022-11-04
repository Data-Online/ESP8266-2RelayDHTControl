#ifndef config_h
#define config_h
/* General configuration settings */
// Application settings
#pragma once
#define  SW_VERSION "0.4.0"

// #include <Arduino.h>

// // Custom settings
// char baseMQTT[] = "home/garden";  // Base
// // char area[] = "test"; // area in base
// char area[] = "greenhouse"; // area in base - This is also used as the client ID, so needs to be unique
// // char area[] = "bed1"; // area in base
// char state[] = "state"; // Refer comments below
// // --> baseMQTT + area + valve_name + payload (ON, OFF) | MQTT on / off commands
// // --> baseMQTT + area + state + valve_name + payload (ON, OFF)  | Confirmation of valve state sent from client

// // *** SENSORS *** //
// #define DHT_active  // Uncomment to activate DHT sensor on defined port

// Support for 2 valves and sensors
// #define MQTT_CLIENTID "test"

// #define DEBUG_MSG(...) 
#define DEBUG_MSG(...) Serial.printf(__VA_ARGS__)
// #define MOIST_active // Uncomment to actvate moisture sensor on defined analog port

// #define MOIST_BINARY_ACTIVE  // Binary moisture sensor active

// IO Pins definition
#define DHTPIN D2 // DHT - D1/GPIO5
#define MOIST_BINARY D2 // Binary moisture sensor
#define MOIST_SENSOR 0  // Moisture sensor
#define EXTERN_LED1 D5
#define EXTERN_LED2 D6
#define SWITCH1 D1
#define SWITCH2 D3
#define MANUAL_SW D8
#define BLINK_LED D7 //D4    // LED pin

// // WiFi
// #define STATION_SSID  "Atkinson"
// #define STATION_PASSWORD "ferret11"
#define MAXWIFI_CONNECT_RETRYS 10 // How many times to try connecting to WiFi before giving up
#define WIFI_RECONNECT_DELAY 300   // Seconds delay before retrying WiFi connection

// MQTT service
// #define MQTT_USER "atkmqtt"
// #define MQTT_PASSWORD "ferret11"
// #define MQTT_SERVER "192.168.1.150"
#define BINARY_SENSOR_RECHECK_DELAY 2  // Seconds between binary sensor checks  
#define MQTT_RECONNECT_RETRY_DELAY 30000   // MS before retrying connection to Home Assistant MQTT service
#define DHT_MESSAGE_SEND_FREQ 5 // How frequently to update DHT measurements (seconds)
// LED flash speed - signal status of controller
#define CONNECTED_BLINK_PERIOD 10000 // milliseconds until cycle repeat
#define CONNECTED_BLINK_DURATION 100  // milliseconds LED is on for
#define WAITING_BLINK_PERIOD 1000
#define WAITING_BLINK_DURATION 100

// Valve control
#define MAX_ON_TIME_SECS 1800  // Default maximum time to leave any valve on (seconds)

#define MAX_RETRY_BEFORE_REBOOT 5 // Number of connection retries before rebooting the system

#define VALVE1 0
#define VALVE2 1
#define ON HIGH
#define OFF LOW
#define VALVE_STATE_AT_STARTUP OFF  // Whether valves should switch on or off at startup

const uint8_t valveToSwitchIndex[2] = {D1, D3};     // Valve1, Valve2
const uint8_t valveToLEDIndex[2] = {D5, D6};
char *mqttNodeNames[] {"valve1", "valve2"};

#endif
