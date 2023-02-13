#include <Arduino.h>

// Custom values in this file
#include "custom.h"
#include "config.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// #include <Wire.h>
#include <TaskScheduler.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <ezButton.h>
#include "DHT.h"

#ifdef DHT_active
  // #define DHTTYPE DHT22
  DHT dht(DHTPIN, DHT22);
#endif

WiFiClient nodeClient;
PubSubClient mqttClient(nodeClient);
unsigned long lastMsg = 0;   
unsigned long dhtLastMsg = 0;   

// Scheduler
Scheduler userScheduler; // to control your personal task
//
unsigned long lastReconnectMillis = 0;
bool allowManualControl = false;      // Whether switch operation will allow manual switch without MQTT command
// Tasks
// - Signal status of node
Task blinkStatusTask;
bool onFlag = true; // Status LED
int BLINK_PERIOD = CONNECTED_BLINK_PERIOD;
int BLINK_DURATION = CONNECTED_BLINK_DURATION;
int maxValveOpenTimeSecs = MAX_ON_TIME_SECS;

void ledOnCallback();
void valveTriggerCallBack();
void ledOffCallback();
bool setup_wifi(); 
bool setupWiFiandMQTT();
bool onOffTest(String state);
String onOffTest(bool state);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void getDHT();
void setSwitch(uint8_t valveNo, bool state);
void allValves(bool state);
void checkValveOpenTimes();
void publishData(float p_temperature, float p_humidity, int p_moisture);
void toggleSwitch(int valveNo);
void notifyValveStates(uint8_t valveNo, bool state);
void returnMqttAddress(uint8_t valve, bool returnResp, char address[40]);
void defineMQTTnodes();
void notifyMoistureState(char state[3]);

bool autSwitchValves = false;

bool wifiConnected = false;
unsigned long retryWiFiConnectDelay = 0;
unsigned long recheckBinarySensorDelay = 0;
unsigned long switchOnLimitsMS[2] = {millis(), millis()};
const int LONG_PRESS_TIME  = 500; // 1000 milliseconds
ezButton button(MANUAL_SW);  // create ezButton object that attach to pin D8;
#ifdef MOIST_BINARY_ACTIVE
  ezButton binary_sensor(MOIST_BINARY);  // create ezButton object that attach to pin D2;
#endif
char binary_sensor_state[3];

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
int connectionRetryCount = 0;       // How many times has the system tried to reconnect

char nodeLocation[40];
char sensors[40];
bool startupInitial = true;   // Used to flag startup to trigger initial actions following system start

unsigned long waitUntil=0;

  
void setup()
{
  Serial.begin(115200);

  DEBUG_MSG("System startup for area %s\n", area);
  
  //pinMode(LED_BUILTIN, OUTPUT); 

  wifiConnected = setupWiFiandMQTT();
  // Relay switch control 
  pinMode(SWITCH1, OUTPUT);
  pinMode(SWITCH2, OUTPUT);
  pinMode(EXTERN_LED1, OUTPUT);
  pinMode(EXTERN_LED2, OUTPUT);
  pinMode(MANUAL_SW, INPUT);
  // Manual switch
  button.setDebounceTime(50); // set debounce time to 50 milliseconds

  #ifdef MOIST_BINARY_ACTIVE
  // Using binary moisture sensor
     pinMode(MOIST_BINARY, INPUT);
     strcat(binary_sensor_state,"off"); // off == dry
     // Set initial state
     notifyMoistureState(binary_sensor_state);
  #endif

  // Additional - optional - functions
  #ifdef DHT_active
    //pinMode(DHTPIN, INPUT);
    dht.begin();
    //digitalWrite(DHTPIN, LOW);  // Off initially 
  #endif 

  // Signal status of sensor
  pinMode(BLINK_LED, OUTPUT);
  digitalWrite(BLINK_LED, LOW);  // Off initially 

  blinkStatusTask.set(BLINK_PERIOD, TASK_FOREVER, &ledOnCallback);
  userScheduler.addTask(blinkStatusTask);
  blinkStatusTask.enable();  

  #ifdef AUTONOMOUS_DELAY_ACTIVE
    Task autonomousDelayTask;
    int AUT_SWITCH_PERIOD = AUTONOMOUS_DELAY_MINS * 60 * 1000;
    autonomousDelayTask.set(AUT_SWITCH_PERIOD, TASK_FOREVER, &valveTriggerCallBack);
    autonomousDelayTask.enable();
  #endif

  defineMQTTnodes();
  // Ensure all switched off at startup
  allValves(VALVE_STATE_AT_STARTUP);

  waitUntil += DELAY_MS;
  
}

void loop()
{
  userScheduler.execute();
  //
  button.loop();
  #ifdef MOIST_BINARY_ACTIVE
  // Using binary moisture sensor
    binary_sensor.loop();
  #endif
  unsigned long dbgMsgTime = millis();
  if (dbgMsgTime - lastMsg > 1000 * 30) {
    lastMsg = dbgMsgTime;
    // DEBUG_MSG("\nStatus: WiFi - %i (WiFiconnect flag = %i).\nMQTT connected - %i. MQTT state - %i.\nManual control setting - %i\nMQTT reconnect count - %i\n", 
    //                                 WiFi.status(), wifiConnected, mqttClient.connected(), mqttClient.state(), allowManualControl, connectionRetryCount);
  }

  if (connectionRetryCount > MAX_RETRY_BEFORE_REBOOT) {
    DEBUG_MSG("Max WiFi connect retry exceeded. Restarting ...\n");
    /// Unable to reconnect MQTT so resort to this.
    ESP.restart();
  }
  if (!mqttClient.connected()) {
    allowManualControl = true;
    if ((millis() - lastReconnectMillis > MQTT_RECONNECT_RETRY_DELAY))
    {
      if (wifiConnected) { 
        connectionRetryCount++; 
        reconnect();
      }
      lastReconnectMillis = millis();
    }
    BLINK_PERIOD = WAITING_BLINK_PERIOD;
    BLINK_DURATION = WAITING_BLINK_DURATION;
  }
  else {
    allowManualControl = false;
    BLINK_PERIOD = CONNECTED_BLINK_PERIOD;
    BLINK_DURATION = CONNECTED_BLINK_DURATION;
    if (startupInitial) { 
      allValves(VALVE_STATE_AT_STARTUP);
      startupInitial = false;
    }
  }

  if (wifiConnected) {
    mqttClient.loop();
  }
  else if (millis() - retryWiFiConnectDelay > (WIFI_RECONNECT_DELAY * 1000)) {
    wifiConnected = setupWiFiandMQTT();
    retryWiFiConnectDelay = millis();
    allowManualControl = true;
    connectionRetryCount++; 
  }
  
  #ifdef DHT_active
  unsigned long dhtMsgTime = millis();
  // Send a message every minute
  if (dhtMsgTime - dhtLastMsg > 1000 * DHT_MESSAGE_SEND_FREQ) {
    dhtLastMsg = dhtMsgTime;
    getDHT();
  }
  #endif

  if(button.isReleased()) {
    DEBUG_MSG("Pressed");
    pressedTime = millis();
  }

  if(button.isPressed()) {
    releasedTime = millis();
    DEBUG_MSG("Released");

    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < LONG_PRESS_TIME ) {
      toggleSwitch(VALVE1);
    }
    if( pressDuration >= LONG_PRESS_TIME ) {
      toggleSwitch(VALVE2);
    }
  }

  digitalWrite(BLINK_LED, onFlag);  
  checkValveOpenTimes();

#ifdef MOIST_BINARY_ACTIVE
  if (binary_sensor.isPressed()) {
      DEBUG_MSG("Binary sensor trigger! %s\n", "on");
      strcpy(binary_sensor_state,"on");
      notifyMoistureState(binary_sensor_state);

  }
  else if (binary_sensor.isReleased()){
    DEBUG_MSG("Binary sensor trigger! %s\n", "off");

    strcpy(binary_sensor_state,"off");
    notifyMoistureState(binary_sensor_state);
  }
  // if (millis() - recheckBinarySensorDelay > (BINARY_SENSOR_RECHECK_DELAY * 1000)) {
  //   recheckBinarySensorDelay = millis();
  //   notifyMoistureState(binary_sensor_state);
  // }
#endif

#ifdef AUTONOMOUS_DELAY_ACTIVE
  if (autSwitchValves) {
    toggleSwitch(VALVE1);
    toggleSwitch(VALVE2);
    autSwitchValves = false;
  }
#endif

}

void defineMQTTnodes() {
  // Node location
  strcpy(nodeLocation,baseMQTT);
  strcat(nodeLocation,"/");
  strcat(nodeLocation,area);
  strcpy(sensors,nodeLocation);
  strcat(sensors,"/sensors");
}

void toggleSwitch(int valveNo) {
  setSwitch(valveNo, !digitalRead(valveToSwitchIndex[valveNo]));
}

bool setupWiFiandMQTT() {
  DEBUG_MSG("WiFi and MQTT setup. ");
  if (setup_wifi()) { 
    DEBUG_MSG("WiFi setup. Now MQTT...\n");
    mqttClient.setServer(MQTT_SERVER, 1883);
    mqttClient.setCallback(callback);
    return true;
  }
  DEBUG_MSG("WiFi setup failed\n");
  return false;
}

void checkValveOpenTimes() {
  // After approx. 50 days millis will reset. Need to account for
  // Includes manual switch of valve in case loss of WiFi connection
  for (int i=0; i < sizeof(valveToSwitchIndex)/sizeof(valveToSwitchIndex[0]); i++) {
    if (millis() < switchOnLimitsMS[i]) { switchOnLimitsMS[i] = millis(); }
    if (digitalRead(valveToSwitchIndex[i]) && (millis() - switchOnLimitsMS[i] > (1000 * maxValveOpenTimeSecs))) {
      DEBUG_MSG("Switch timeout (%i secs)\n", maxValveOpenTimeSecs);
      setSwitch(i, false);
    }
  }
}

void allValves(bool state) {
  DEBUG_MSG("Switching all valves to state %i\n", state);
  for (uint8_t i=0; i < sizeof(valveToSwitchIndex)/sizeof(valveToSwitchIndex[0]); i++) {
    setSwitch(i, state);
  }
}

bool setup_wifi() {
  int _retryCount = 0;

  // We start by connecting to a WiFi network
  // sprintf(dispMsg, "\n\nConnecting to %s\n",STATION_SSID);  
  DEBUG_MSG("Connecting to %s\n",STATION_SSID);
  // int8_t _channel = WIFI_CHANNEL;
  WiFi.mode(WIFI_STA);
  // WiFi.channel(_channel);
  DEBUG_MSG("UID %s PWD %s", STATION_SSID,STATION_PASSWORD);
  WiFi.begin(STATION_SSID, STATION_PASSWORD);

  while ((WiFi.status() != WL_CONNECTED) && _retryCount < MAXWIFI_CONNECT_RETRYS) {
    delay(500);
    DEBUG_MSG(".");
    _retryCount++;
  }
  wifi_set_channel(WIFI_CHANNEL);
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_MSG("WiFi connected failed!\n");
    return false;
  }
  else {
    DEBUG_MSG("\nWiFi connected\nIP address %s\nChannel %i\n",WiFi.localIP().toString().c_str(), WiFi.channel());
    return true;
  }
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  String strTopic;
  
  payload[length] = '\0';
  strTopic = String((char*)topic);
  DEBUG_MSG("Callback: Payload = %s topic =  %s\n", payload, topic);
  if (strTopic.substring(String(nodeLocation).length()) == "/runtime") {
    maxValveOpenTimeSecs = 60 * String((char*)payload).toInt();
    DEBUG_MSG("Runtime set to = %i\n", maxValveOpenTimeSecs);
  }
  else {
    // Check topic against available valve switches and toggle if required
    char _testString[40];
    for (int i=0; i < sizeof(valveToSwitchIndex)/sizeof(valveToSwitchIndex[0]); i++) {
      returnMqttAddress(i, false, _testString);
      if (strTopic == _testString) {
        // Match
        DEBUG_MSG("Set switch call for # %i. State : %i\n", i,onOffTest(String((char*)payload)));
        setSwitch(i, (onOffTest(String((char*)payload))));
      }
    }
  }
}

void returnMqttAddress(uint8_t valveNo, bool returnResp, char address[40]) {
  strcpy(address,nodeLocation);
  if (returnResp) {
    strcat(address,"/");
    strcat(address, state);
  }
  strcat(address,"/");
  strcat(address, mqttNodeNames[valveNo]);
}

void setSwitch(uint8_t valveNo, bool state) { 
  // Set state of switch and indicator LED
  digitalWrite(valveToSwitchIndex[valveNo], state);
  digitalWrite(valveToLEDIndex[valveNo], state);

  DEBUG_MSG("Switch %d. Set state %d\n", valveNo, state);

  if (state) {
    switchOnLimitsMS[(int)valveNo] = millis();
  }

  // Send out status notification
  if (mqttClient.connected()) notifyValveStates(valveNo, state);
}

void notifyValveStates(uint8_t valveNo, bool state) {
  char _respString[40]; 
  returnMqttAddress(valveNo, true, _respString);
  DEBUG_MSG("Sending notification to %s, state = %s\n", _respString,onOffTest(state).c_str());
  mqttClient.publish(_respString, onOffTest(state).c_str()); 
  delay(100);
  // returnMqttAddress(valveNo, false, _respString);
  // DEBUG_MSG("Sending notification to %s, state = %s\n", _respString,onOffTest(state).c_str());
  // mqttClient.publish(_respString, onOffTest(state).c_str()); 
  // DEBUG_MSG("wait ...\n");
  // delay(1000);
}
// NEW end

void notifyMoistureState(char state[3]) {
  mqttClient.publish("school/garden/greenhouse/moisture",state);
}

void reconnect() {
  char _subscribe[40];
  strcpy(_subscribe, nodeLocation);
  strcat(_subscribe,"/#");
  DEBUG_MSG("Attempting MQTT connection...\n");
  if (mqttClient.connect(area, MQTT_USER, MQTT_PASSWORD)) {
    DEBUG_MSG("connected\n");
    // Once connected, subscribe 
    mqttClient.subscribe(_subscribe);
    // and publish an announcement...??
  } 
}

#ifdef DHT_active
void getDHT()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read moisture
  int m = analogRead(MOIST_SENSOR);
  // Check if any reads failed and exit early (to try again).
  //  if (isnan(h) || isnan(t) || isnan(f)) {
  if (isnan(h) || isnan(t)) {
    DEBUG_MSG("Failed to read from DHT sensor!\n");
    //return;
  }

  // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  //float hic = dht.computeHeatIndex(t, h, false);

  DEBUG_MSG("Temperature : %f; Humidity : %f; Moisture : %i\n", t, h, m);
  publishData(t,h,m);
}
#endif

bool onOffTest(String state) {
  if (state == "ON") {
    return true;
  }
  return false;
}

String onOffTest(bool state) {
  if (state) {
    return "ON";
  }
  return "OFF";
}

void  ledOnCallback() {
 onFlag = false;
 blinkStatusTask.setCallback(&ledOffCallback);
 blinkStatusTask.setInterval(BLINK_PERIOD);
}

void valveTriggerCallBack() {
  autSwitchValves = true;
}

void ledOffCallback() {
  onFlag = true;
  blinkStatusTask.setCallback(&ledOnCallback);
  blinkStatusTask.setInterval(BLINK_DURATION);
}


void publishData(float p_temperature, float p_humidity, int p_moisture) {  //, float p_airquality) {
    // create a JSON object
    StaticJsonDocument<200> jsonBuffer;
    jsonBuffer["temperature"] = (String)p_temperature;
    jsonBuffer["humidity"] = (String)p_humidity;
    jsonBuffer["moisture"] = (String)p_moisture;
    /*
    {
    "temperature": "23.20" ,
    "humidity": "43.70"
    }
    */
    char data[200];
    serializeJson(jsonBuffer,data);
    //DEBUG_MSG("Runtime set to = %i\n", maxValveOpenTimeSecs);
    DEBUG_MSG("Publish data:\n");
    DEBUG_MSG(data);
    if (wifiConnected) { 
      mqttClient.publish(sensors, data, true);
      yield();
    }
}