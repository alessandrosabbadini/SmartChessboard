#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <Arduino.h>
#include <WiFiNINA.h>
#include <FlashStorage.h>
#include <ArduinoJson.h>
#include "config.h"
#include "MKRBLE.h"

// JSON-like WiFi credential storage using FlashStorage
// This approach stores JSON data in flash memory for true persistence

// WiFi Credentials Structure
struct WiFiCredentials {
    char ssid[32];
    char password[64];
    bool isValid;
    
    // Constructor to initialize
    WiFiCredentials() {
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
        isValid = false;
    }
};

// JSON data structure for WiFi credentials storage
struct WiFiConfigData {
    char jsonData[512];  // Store JSON as string in flash
    bool isValid;
    unsigned long timestamp;
    
    WiFiConfigData() {
        memset(jsonData, 0, sizeof(jsonData));
        isValid = false;
        timestamp = 0;
    }
};

// FlashStorage instance will be declared in the .cpp file

class WiFiProvisioning {
public:
    WiFiProvisioning();
    void begin();
    void handleEvents();
    void processWiFiCredentials(String jsonData);
    bool isWiFiConnected();
    String getWiFiStatus();
    void sendStatusUpdate(String status, String message = "");
    
private:
    MKRBLE* ble;
    WiFiCredentials wifiCreds;
    bool isProvisioning;
    bool isConnected;
    unsigned long connectionStartTime;
    unsigned long lastStatusUpdate;
    
    void startBLEProvisioning();
    void attemptWiFiConnection();
    void saveWiFiCredentials();
    void loadWiFiCredentials();
    void sendReadyStatus();
    void sendAckStatus();
    void sendConnectingStatus();
    void sendSuccessStatus();
    void sendErrorStatus(String error);
    void restartProvisioning();
    void handleBLEMessage(String message);
    static void onBLEMessage(String message);
    static WiFiProvisioning* instance;
};

#endif // WIFI_PROVISIONING_H
