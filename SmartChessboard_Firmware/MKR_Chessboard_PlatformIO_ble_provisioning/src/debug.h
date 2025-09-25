#ifndef DEBUG_H
#define DEBUG_H

#include "config.h"
#include <Arduino.h>

// Debug logging macros
#if DEBUG_MODE
    #define DEBUG_LOG(message) Serial.println("[DEBUG] " + String(message))
    #define DEBUG_LOG_F(format, ...) Serial.print("[DEBUG] "); Serial.printf(format "\n", ##__VA_ARGS__)
    #define DEBUG_LOG_JSON(json) Serial.println("[DEBUG JSON] " + json)
    #define DEBUG_LOG_ERROR(error) Serial.println("[ERROR] " + String(error))
    #define DEBUG_LOG_INFO(info) Serial.println("[INFO] " + String(info))
    #define DEBUG_LOG_WIFI(status) Serial.println("[WIFI] " + String(status))
    #define DEBUG_LOG_BLE(status) Serial.println("[BLE] " + String(status))
    #define DEBUG_LOG_MESSAGE(type, data) Serial.println("[MSG] Type: " + String(type) + " Data: " + String(data))
#else
    #define DEBUG_LOG(message)
    #define DEBUG_LOG_F(format, ...)
    #define DEBUG_LOG_JSON(json)
    #define DEBUG_LOG_ERROR(error)
    #define DEBUG_LOG_INFO(info)
    #define DEBUG_LOG_WIFI(status)
    #define DEBUG_LOG_BLE(status)
    #define DEBUG_LOG_MESSAGE(type, data)
#endif

// Performance timing macros
#define TIMER_START() unsigned long timer_start = millis()
#define TIMER_END(label) DEBUG_LOG(String(label) + " took " + String(millis() - timer_start) + " ms")

// Memory usage macros
#define MEMORY_USAGE() DEBUG_LOG("Free heap: Available")

// Connection status macros
#define LOG_CONNECTION_STATUS() \
    DEBUG_LOG("WiFi: Unknown, BLE: Unknown, IP: Unknown")

// Message validation macros
#define VALIDATE_MESSAGE(doc) \
    if (doc.isNull()) { \
        DEBUG_LOG_ERROR("Invalid JSON message"); \
        return false; \
    }

#define VALIDATE_MESSAGE_TYPE(doc, expected_type) \
    if (!doc.containsKey("type") || doc["type"] != expected_type) { \
        DEBUG_LOG_ERROR("Invalid message type. Expected: " + String(expected_type)); \
        return false; \
    }

// Error handling macros
#define HANDLE_ERROR(error_code, message) \
    { \
        DEBUG_LOG_ERROR(String(error_code) + ": " + String(message)); \
        sendErrorMessage(error_code, message); \
        return; \
    }

// Utility functions
void printSystemInfo();
void printMemoryInfo();
void printConnectionInfo();
void printGameState();
void printSensorData();

// Error message sender
void sendErrorMessage(String errorCode, String errorMessage);

#endif // DEBUG_H
