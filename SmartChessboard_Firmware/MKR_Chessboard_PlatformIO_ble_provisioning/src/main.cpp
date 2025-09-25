#include <Arduino.h>
#include "WiFiProvisioning.h"
#include "config.h"
#include "debug.h"

// Global WiFi provisioning instance
WiFiProvisioning wifiProvisioning;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000); // Give time for serial to initialize
    
    DEBUG_LOG("========================================");
    DEBUG_LOG("MKR WiFi Provisioning Starting...");
    DEBUG_LOG("Version: " + String(DEVICE_VERSION));
    DEBUG_LOG("========================================");
    
    // Print system information
    printSystemInfo();
    
    // Initialize the WiFi provisioning
    wifiProvisioning.begin();
    
    DEBUG_LOG("========================================");
    DEBUG_LOG("WiFi Provisioning ready!");
    DEBUG_LOG("Connect via Bluetooth LE: " + String(BLE_DEVICE_NAME));
    DEBUG_LOG("========================================");
}

void loop() {
    // Handle WiFi provisioning events
    wifiProvisioning.handleEvents();
    
    // Small delay to prevent overwhelming the system
    delay(10);
}

// Debug utility functions
void printSystemInfo() {
    DEBUG_LOG("System Information:");
    DEBUG_LOG("  Board: Arduino MKR WiFi 1010");
    DEBUG_LOG("  CPU: SAMD21G18A 48MHz");
    DEBUG_LOG("  Flash: 256KB");
    DEBUG_LOG("  RAM: 32KB");
    DEBUG_LOG("  Arduino Core Version: 1.8.14");
}

void printMemoryInfo() {
    DEBUG_LOG("Memory Information:");
    DEBUG_LOG("  Free Heap: Available");
    DEBUG_LOG("  RAM: 32KB total");
    DEBUG_LOG("  Flash: 256KB total");
}

void printConnectionInfo() {
    DEBUG_LOG("Connection Information:");
    DEBUG_LOG("  WiFi Status: " + wifiProvisioning.getWiFiStatus());
}

void printProvisioningInfo() {
    DEBUG_LOG("Provisioning Information:");
    DEBUG_LOG("  Status: " + wifiProvisioning.getWiFiStatus());
    DEBUG_LOG("  BLE Device: " + String(BLE_DEVICE_NAME));
    DEBUG_LOG("  Service UUID: " + String(CHESSBOARD_SERVICE_UUID));
}
