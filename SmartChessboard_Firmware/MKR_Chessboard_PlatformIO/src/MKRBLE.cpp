#include "MKRBLE.h"

MKRBLE* MKRBLE::instance = nullptr;

MKRBLE::MKRBLE() {
    instance = this;
    messageCallback = nullptr;
    deviceConnected = false;
    oldDeviceConnected = false;
    lastAdvertiseTime = 0;
    chessboardService = nullptr;
    messageCharacteristic = nullptr;
    statusCharacteristic = nullptr;
}

void MKRBLE::begin() {
    Serial.println("Initializing MKR WiFi 1010 BLE...");
    
    // Initialize BLE
    if (!BLE.begin()) {
        Serial.println("Starting BLE failed!");
        return;
    }
    
    // Set device name
    BLE.setLocalName(BLE_DEVICE_NAME);
    BLE.setAdvertisedServiceUuid(CHESSBOARD_SERVICE_UUID);
    
    // Set connection parameters for better compatibility
    BLE.setConnectable(true);
    BLE.setAdvertisingInterval(100); // 100ms advertising interval
    
    // Note: setAdvertisingData is already handled by setLocalName and setAdvertisedServiceUuid
    
    // Create service
    chessboardService = new BLEService(CHESSBOARD_SERVICE_UUID);
    
    // Create data characteristic (for receiving WiFi credentials)
    messageCharacteristic = new BLEStringCharacteristic(
        CHESSBOARD_CHARACTERISTIC_UUID,
        BLEWrite,
        512  // Maximum string length
    );
    
    // Create status characteristic (for sending status updates)
    statusCharacteristic = new BLEStringCharacteristic(
        CHESSBOARD_STATUS_CHARACTERISTIC_UUID,
        BLERead | BLENotify,
        512  // Maximum string length
    );
    
    // Add characteristics to service
    chessboardService->addCharacteristic(*messageCharacteristic);
    chessboardService->addCharacteristic(*statusCharacteristic);
    
    // Add service to BLE
    BLE.addService(*chessboardService);
    
    // Set event handlers - using a static callback function
    messageCharacteristic->setEventHandler(BLEWritten, onCharacteristicWritten);
    
    // Start advertising
    BLE.advertise();
    
    Serial.println("=== MKR WiFi Provisioning BLE Initialized ===");
    Serial.println("Device name: " + String(BLE_DEVICE_NAME));
    Serial.println("Service UUID: " + String(CHESSBOARD_SERVICE_UUID));
    Serial.println("Data Characteristic UUID: " + String(CHESSBOARD_CHARACTERISTIC_UUID));
    Serial.println("Status Characteristic UUID: " + String(CHESSBOARD_STATUS_CHARACTERISTIC_UUID));
    Serial.println("Advertising interval: 100ms");
    Serial.println("BLE advertising started - Device is discoverable!");
    Serial.println("");
    Serial.println("IMPORTANT: This device will appear in:");
    Serial.println("✅ BLE Scanner apps (nRF Connect, LightBlue, etc.)");
    Serial.println("✅ Flutter BLE apps using flutter_blue_plus");
    Serial.println("❌ Phone's built-in Bluetooth settings");
    Serial.println("❌ Classic Bluetooth connections");
    Serial.println("");
    Serial.println("For Flutter apps, scan for: " + String(BLE_DEVICE_NAME));
    Serial.println("=====================================");
}

void MKRBLE::handleEvents() {
    // Poll for BLE events
    BLE.poll();
    
    // Handle connection state changes
    BLEDevice central = BLE.central();
    
    if (central) {
        if (!deviceConnected) {
            deviceConnected = true;
            Serial.println("BLE Client connected: " + central.address());
            Serial.println("Client connected to service: " + String(CHESSBOARD_SERVICE_UUID));
            Serial.println("Characteristic available: " + String(CHESSBOARD_CHARACTERISTIC_UUID));
            
            // Send ready status immediately when connected
            if (statusCharacteristic) {
                String readyStatus = "{\"status\":\"ready\",\"message\":\"Device ready for WiFi provisioning\",\"timestamp\":" + String(millis()) + "}";
                statusCharacteristic->writeValue(readyStatus);
                Serial.println("=== BLE Ready Status Sent ===");
                Serial.println("Status: " + readyStatus);
                Serial.println("=============================");
            }
        }
    } else {
        if (deviceConnected) {
            deviceConnected = false;
            Serial.println("BLE Client disconnected");
        }
    }
    
    // Show periodic advertising status
    if (millis() - lastAdvertiseTime > 30000) {
        Serial.println("BLE advertising active - Device discoverable as: " + String(BLE_DEVICE_NAME));
        Serial.println("Service UUID: " + String(CHESSBOARD_SERVICE_UUID));
        Serial.println("Characteristic UUID: " + String(CHESSBOARD_CHARACTERISTIC_UUID));
        lastAdvertiseTime = millis();
    }
}

void MKRBLE::sendMessage(String message) {
    if (deviceConnected && messageCharacteristic) {
        // Send via characteristic write
        messageCharacteristic->writeValue(message);
        Serial.println("=== BLE Message Sent ===");
        Serial.println("Message: " + message);
        Serial.println("Length: " + String(message.length()) + " bytes");
        Serial.println("========================");
    } else {
        Serial.println("BLE not connected, message not sent: " + message);
        Serial.println("Device connected: " + String(deviceConnected ? "Yes" : "No"));
        Serial.println("Characteristic available: " + String(messageCharacteristic ? "Yes" : "No"));
    }
}

void MKRBLE::sendStatus(String status, String message) {
    if (deviceConnected && statusCharacteristic) {
        // Create JSON status message
        String statusJson = "{\"status\":\"" + status + "\",\"message\":\"" + message + "\",\"timestamp\":" + String(millis()) + "}";
        statusCharacteristic->writeValue(statusJson);
        Serial.println("=== BLE Status Sent ===");
        Serial.println("Status: " + status);
        Serial.println("Message: " + message);
        Serial.println("JSON: " + statusJson);
        Serial.println("=======================");
    } else {
        Serial.println("BLE not connected, status not sent: " + status);
        Serial.println("Device connected: " + String(deviceConnected ? "Yes" : "No"));
        Serial.println("Status characteristic available: " + String(statusCharacteristic ? "Yes" : "No"));
    }
}

bool MKRBLE::isConnected() {
    return deviceConnected;
}

void MKRBLE::setMessageCallback(void (*callback)(String)) {
    messageCallback = callback;
}

void MKRBLE::onCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
    if (instance && instance->messageCharacteristic) {
        String value = instance->messageCharacteristic->value();
        Serial.println("=== BLE Message Received ===");
        Serial.println("From: " + central.address());
        Serial.println("Characteristic: " + String(CHESSBOARD_CHARACTERISTIC_UUID));
        Serial.println("Message: " + value);
        Serial.println("Length: " + String(value.length()) + " bytes");
        Serial.println("=============================");
        
        if (instance->messageCallback) {
            instance->messageCallback(value);
        }
    }
}
