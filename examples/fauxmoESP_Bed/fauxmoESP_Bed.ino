#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include "fauxmoESP.h"

// Rename the credentials.sample.h file to credentials.h and 
// edit it according to your router configuration
#include "credentials.h"

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200

#define PWM_BED_RIGHT       5  // D1 - IO, SCL
#define PWM_BED_LEFT        4  // D2 - IO, SDA
#define PWM_HEADBOARD       14 // D5 - IO, SCK
#define PIR_RIGHT           0  // D3 - IO, 10k Pull-up
#define PIR_LEFT            2  // D4 - IO, 10k Pull-up, BUILTIN_LED

#define ID_BED_RIGHT        "bed right"
#define ID_BED_LEFT         "bed left"
#define ID_HEADBOARD        "headboard"
#define ID_MOTION           "motion"    // Enable motion activation

// Maybe add minimum / maximum values


bool stateBedRight = false;
unsigned char valueBedRight = 0;
bool stateBedLeft = false;
unsigned char valueBedLeft = 0;
bool stateHeadboard = false;
unsigned char valueHeadboard = 0;
bool stateMotion = false;
unsigned char valueMotion = 0; // Maybe use as timeout

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

void setup() {

    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();

    // LED strips via MOSFET
    pinMode(PWM_BED_RIGHT, OUTPUT);
    pinMode(PWM_BED_LEFT,  OUTPUT);
    pinMode(PWM_HEADBOARD, OUTPUT);

    // PIR motion sensors
    pinMode(PIR_RIGHT, OUTPUT);
    pinMode(PIR_LEFT,  OUTPUT);

    analogWrite(PWM_BED_RIGHT, 0);
    analogWrite(PWM_BED_LEFT,  0);
    analogWrite(PWM_HEADBOARD, 0);

    // Wifi
    wifiSetup();

    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // You can use different ways to invoke alexa to modify the devices state:
    // "Alexa, turn yellow lamp on"
    // "Alexa, turn on yellow lamp
    // "Alexa, set yellow lamp to fifty" (50 means 50% of brightness, note, this example does not use this functionality)

    // Add virtual devices
    fauxmo.addDevice(ID_BED_RIGHT);
    fauxmo.addDevice(ID_BED_LEFT);
    fauxmo.addDevice(ID_HEADBOARD);
    fauxmo.addDevice(ID_MOTION);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        if (strcmp(device_name, ID_BED_RIGHT)==0) {
            stateBedRight = state;
            valueBedRight = value;
        } else if (strcmp(device_name, ID_BED_LEFT)==0) {
            stateBedLeft = state;
            valueBedLeft = value;
        } else if (strcmp(device_name, ID_HEADBOARD)==0) {
            stateHeadboard = state;
            valueHeadboard = value;
        } else if (strcmp(device_name, ID_MOTION)==0) {
            stateMotion = state;
            valueMotion = value;
        }

    });

}

void loop() {

    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();

    // This is a sample code to output free heap every 5 seconds
    // This is a cheap way to detect memory leaks
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);

}
