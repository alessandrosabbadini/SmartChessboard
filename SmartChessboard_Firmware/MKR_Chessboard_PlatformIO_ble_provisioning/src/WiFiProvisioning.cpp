#include "WiFiProvisioning.h"

// FlashStorage instance for JSON-like credential storage
FlashStorage(wifiConfigStorage, WiFiConfigData);

WiFiProvisioning* WiFiProvisioning::instance = nullptr;

WiFiProvisioning::WiFiProvisioning() {
    instance = this;
    ble = new MKRBLE();
    isProvisioning = false;
    isConnected = false;
    connectionStartTime = 0;
    lastStatusUpdate = 0;
    
    // Initialize WiFi credentials
    wifiCreds.isValid = false;
    memset(wifiCreds.ssid, 0, sizeof(wifiCreds.ssid));
    memset(wifiCreds.password, 0, sizeof(wifiCreds.password));
}

void WiFiProvisioning::begin() {
    Serial.println("");
    Serial.println("╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║              🚀 MKR WiFi Provisioning Starting 🚀           ║");
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    Serial.println("║  🔧 Method: Simple WiFiNINA Approach                       ║");
    Serial.println("║  📱 Board: Arduino MKR WiFi 1010                          ║");
    Serial.println("║  💾 Storage: FlashStorage (Persistent)                     ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println("");
    
    // Initialize WiFi module
    Serial.println("🔧 Initializing WiFi module...");
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("ERROR: WiFi module not found!");
        Serial.println("Please check your MKR WiFi 1010 board");
        return;
    }
    
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("ERROR: WiFi shield not found!");
        return;
    }
    
    Serial.println("WiFi module initialized successfully");
    
    // Load saved WiFi credentials
    loadWiFiCredentials();
    
    // If we have valid credentials, try to connect
    if (wifiCreds.isValid) {
        Serial.println("Found saved WiFi credentials, attempting connection...");
        Serial.println("SSID: " + String(wifiCreds.ssid));
        Serial.println("Password: " + String(wifiCreds.password));
        
        attemptWiFiConnection();
    } else {
        Serial.println("");
        Serial.println("╔══════════════════════════════════════════════════════════════╗");
        Serial.println("║              📡 BLE PROVISIONING MODE 📡                    ║");
        Serial.println("╠══════════════════════════════════════════════════════════════╣");
        Serial.println("║  🔍 No saved WiFi credentials found                        ║");
        Serial.println("║  📱 Starting BLE provisioning mode                         ║");
        Serial.println("║  🔧 Initializing MKR WiFi 1010 BLE...                      ║");
        Serial.println("╚══════════════════════════════════════════════════════════════╝");
        Serial.println("");
        startBLEProvisioning();
    }
}

void WiFiProvisioning::handleEvents() {
    // Handle BLE events if provisioning
    if (isProvisioning && ble) {
        ble->handleEvents();
    }
    
    // Check WiFi connection status
    if (isConnected && WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, attempting to reconnect...");
        isConnected = false;
        
        // Try to reconnect with saved credentials
        if (wifiCreds.isValid) {
            attemptWiFiConnection();
        } else {
            Serial.println("No saved credentials, starting BLE provisioning...");
            startBLEProvisioning();
        }
    }
}

void WiFiProvisioning::processWiFiCredentials(String jsonData) {
    Serial.println("Processing WiFi credentials from BLE...");
    Serial.println("Received JSON: " + jsonData);
    
    // Parse JSON
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        Serial.println("ERROR: Failed to parse JSON: " + String(error.c_str()));
        sendErrorStatus("Invalid JSON format");
        return;
    }
    
    // Extract credentials - handle both "password" and "pass" fields
    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();
    
    // Check if password is empty or "null", then try "pass" field
    if (password.length() == 0 || password == "null") {
        password = doc["pass"].as<String>();
        Serial.println("Using 'pass' field: " + password);
    } else {
        Serial.println("Using 'password' field: " + password);
    }
    
    Serial.println("Extracted SSID: " + ssid);
    Serial.println("Extracted Password: " + password);
    Serial.println("SSID length: " + String(ssid.length()));
    Serial.println("Password length: " + String(password.length()));
    
    if (ssid.length() == 0 || password.length() == 0 || password == "null") {
        Serial.println("ERROR: Invalid credentials (empty SSID or password)");
        Serial.println("SSID length: " + String(ssid.length()));
        Serial.println("Password length: " + String(password.length()));
        Serial.println("Password value: '" + password + "'");
        sendErrorStatus("Invalid credentials");
        return;
    }
    
    // Store credentials
    strncpy(wifiCreds.ssid, ssid.c_str(), sizeof(wifiCreds.ssid) - 1);
    strncpy(wifiCreds.password, password.c_str(), sizeof(wifiCreds.password) - 1);
    wifiCreds.isValid = true;
    
    Serial.println("========================================");
    Serial.println("=== CREDENTIALS STORED IN MEMORY ===");
    Serial.println("SSID: " + String(wifiCreds.ssid));
    Serial.println("Password: " + String(wifiCreds.password));
    Serial.println("isValid: " + String(wifiCreds.isValid ? "true" : "false"));
    Serial.println("========================================");
    
    Serial.println("Credentials stored successfully");
    
    // Save credentials to persistent storage
    Serial.println("Saving credentials to persistent storage...");
    saveWiFiCredentials();
    
    // Send acknowledgment
    sendAckStatus();
    
    // Attempt connection
    Serial.println("Attempting WiFi connection...");
    sendConnectingStatus();
    
    attemptWiFiConnection();
    
    // If connection is successful, the device will auto-restart
    // If connection fails, we'll stay in BLE mode for retry
}

void WiFiProvisioning::attemptWiFiConnection() {
    if (!wifiCreds.isValid) {
        Serial.println("ERROR: No valid WiFi credentials to connect with");
        sendErrorStatus("No credentials available");
        return;
    }
    
        Serial.println("");
        Serial.println("╔══════════════════════════════════════════════════════════════╗");
        Serial.println("║              🔗 ATTEMPTING WIFI CONNECTION 🔗                ║");
        Serial.println("╠══════════════════════════════════════════════════════════════╣");
        Serial.println("║  📶 Network: " + String(wifiCreds.ssid) + String(" ").substring(0, max(0, 30 - String(wifiCreds.ssid).length())) + "║");
        Serial.println("║  🔑 Password: " + String(wifiCreds.password) + String(" ").substring(0, max(0, 28 - String(wifiCreds.password).length())) + "║");
        Serial.println("╚══════════════════════════════════════════════════════════════╝");
        Serial.println("");
    
    connectionStartTime = millis();
    
    // Attempt connection with timeout
    int attempts = 0;
    const int maxAttempts = 20; // 20 seconds timeout
    
    Serial.print("Connecting");
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        if (attempts == 0) {
            // First attempt - start connection
            WiFi.begin(wifiCreds.ssid, wifiCreds.password);
        }
        
        delay(1000);
        attempts++;
        Serial.print(".");
        
        // Check connection status
        int status = WiFi.status();
        if (status == WL_CONNECT_FAILED) {
            Serial.println("\nERROR: Connection failed - invalid credentials");
            sendErrorStatus("Connection failed - check credentials");
            return;
        } else if (status == WL_NO_MODULE) {
            Serial.println("\nERROR: WiFi module not found");
            sendErrorStatus("WiFi module error");
            return;
        } else if (status == WL_NO_SHIELD) {
            Serial.println("\nERROR: WiFi shield not found");
            sendErrorStatus("WiFi shield error");
            return;
        }
    }
    
    Serial.println(); // New line after dots
    
    if (WiFi.status() == WL_CONNECTED) {
        // Connection successful
        unsigned long connectionTime = millis() - connectionStartTime;
        
        // Get MAC address
        uint8_t mac[6];
        WiFi.macAddress(mac);
        String macStr = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + 
                       String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + 
                       String(mac[4], HEX) + ":" + String(mac[5], HEX);
        
        // Clear, organized connection success log
        Serial.println("");
        Serial.println("╔══════════════════════════════════════════════════════════════╗");
        Serial.println("║                    🌐 WIFI CONNECTED! 🌐                    ║");
        Serial.println("╠══════════════════════════════════════════════════════════════╣");
        Serial.println("║  📶 Network: " + String(WiFi.SSID()) + String(" ").substring(0, max(0, 30 - String(WiFi.SSID()).length())) + "║");
        Serial.println("║  📱 IP Address: " + WiFi.localIP().toString() + String(" ").substring(0, max(0, 25 - WiFi.localIP().toString().length())) + "║");
        Serial.println("║  🔗 Gateway: " + WiFi.gatewayIP().toString() + String(" ").substring(0, max(0, 28 - WiFi.gatewayIP().toString().length())) + "║");
        Serial.println("║  📶 Signal: " + String(WiFi.RSSI()) + " dBm" + String(" ").substring(0, max(0, 30 - String(WiFi.RSSI()).length())) + "║");
        Serial.println("║  ⏱️  Connection Time: " + String(connectionTime) + "ms" + String(" ").substring(0, max(0, 25 - String(connectionTime).length())) + "║");
        Serial.println("║  🕐 Connected at: " + String(millis()) + "ms" + String(" ").substring(0, max(0, 28 - String(millis()).length())) + "║");
        Serial.println("║  🔧 Method: WiFiNINA (Persistent)" + String(" ").substring(0, 20) + "║");
        Serial.println("╚══════════════════════════════════════════════════════════════╝");
        Serial.println("");
        
        isConnected = true;
        
        // Save credentials for persistence
        saveWiFiCredentials();
        
        // Stop BLE provisioning
        if (ble && isProvisioning) {
            // Note: MKRBLE doesn't have a stop() method, so we just set the flag
            isProvisioning = false;
        }
        
        // Only auto-restart if this was a BLE provisioning connection (not auto-reconnect)
        if (isProvisioning) {
            Serial.println("🔄 Auto-restarting device in 3 seconds after BLE provisioning...");
            Serial.println("💾 Credentials saved successfully - will reconnect automatically on restart");
            delay(3000);
            Serial.println("🔄 Restarting now...");
            NVIC_SystemReset();  // Software reset
        } else {
            Serial.println("✅ WiFi connected successfully - staying online!");
            Serial.println("💾 Credentials already saved - no restart needed");
        }
        
        // Send success status
        sendSuccessStatus();
        
    } else {
        Serial.println("ERROR: Failed to connect to WiFi after " + String(attempts) + " attempts");
        Serial.println("WiFi Status: " + String(WiFi.status()));
        sendErrorStatus("Connection timeout");
    }
}

bool WiFiProvisioning::isWiFiConnected() {
    return isConnected && (WiFi.status() == WL_CONNECTED);
}

String WiFiProvisioning::getWiFiStatus() {
    if (!isConnected || WiFi.status() != WL_CONNECTED) {
        return "Disconnected";
    }
    
    String status = "Connected";
    status += " | SSID: " + String(WiFi.SSID());
    status += " | IP: " + WiFi.localIP().toString();
    status += " | Gateway: " + WiFi.gatewayIP().toString();
    status += " | RSSI: " + String(WiFi.RSSI()) + "dBm";
    
    return status;
}

void WiFiProvisioning::sendStatusUpdate(String status, String message) {
    if (!ble || !isProvisioning) {
        return;
    }
    
    String statusMessage = "{\"status\":\"" + status + "\"";
    if (message.length() > 0) {
        statusMessage += ",\"message\":\"" + message + "\"";
    }
    statusMessage += "}";
    
    ble->sendMessage(statusMessage);
}

void WiFiProvisioning::startBLEProvisioning() {
    Serial.println("Starting BLE provisioning...");
    
    if (!ble) {
        Serial.println("ERROR: BLE not initialized");
        return;
    }
    
    // Initialize BLE
    ble->begin();
    ble->setMessageCallback(onBLEMessage);
    
    isProvisioning = true;
    isConnected = false;
    
    // Send ready status
    sendReadyStatus();
    
    Serial.println("BLE provisioning started");
    Serial.println("Device ready for WiFi credentials via BLE");
}

void WiFiProvisioning::saveWiFiCredentials() {
    Serial.println("Saving WiFi credentials as JSON to FlashStorage...");
    Serial.println("SSID to save: " + String(wifiCreds.ssid));
    Serial.println("Password to save: " + String(wifiCreds.password));
    Serial.println("isValid flag: " + String(wifiCreds.isValid ? "true" : "false"));
    
    // JSON-LIKE APPROACH: Store JSON data in FlashStorage
    // This creates a JSON structure that can be modified and read like a file
    
    if (wifiCreds.isValid && strlen(wifiCreds.ssid) > 0) {
        // Create JSON document
        DynamicJsonDocument doc(512);
        doc["ssid"] = wifiCreds.ssid;
        doc["password"] = wifiCreds.password;  // Save as "password" for consistency
        doc["pass"] = wifiCreds.password;      // Also save as "pass" for compatibility
        doc["isValid"] = wifiCreds.isValid;
        doc["timestamp"] = millis();
        doc["version"] = "1.0";
        doc["device"] = "MKR-WiFi-1010";
        doc["lastModified"] = millis();
        
        // Serialize to string
        String jsonString;
        serializeJson(doc, jsonString);
        
        Serial.println("JSON to save: " + jsonString);
        Serial.println("JSON length: " + String(jsonString.length()) + " bytes");
        
        // Check if JSON fits in our storage structure
        if (jsonString.length() >= sizeof(WiFiConfigData::jsonData)) {
            Serial.println("ERROR: JSON too large for storage (" + String(jsonString.length()) + " > " + String(sizeof(WiFiConfigData::jsonData)) + ")");
            return;
        }
        
        // Create storage structure
        WiFiConfigData configData;
        strncpy(configData.jsonData, jsonString.c_str(), sizeof(configData.jsonData) - 1);
        configData.isValid = true;
        configData.timestamp = millis();
        
        // Write to flash storage
        wifiConfigStorage.write(configData);
        
        Serial.println("");
        Serial.println("╔══════════════════════════════════════════════════════════════╗");
        Serial.println("║              💾 CREDENTIALS SAVED! 💾                        ║");
        Serial.println("╠══════════════════════════════════════════════════════════════╣");
        Serial.println("║  📶 Network: " + String(wifiCreds.ssid) + String(" ").substring(0, max(0, 30 - String(wifiCreds.ssid).length())) + "║");
        Serial.println("║  🔑 Password: " + String(wifiCreds.password) + String(" ").substring(0, max(0, 28 - String(wifiCreds.password).length())) + "║");
        Serial.println("║  💾 Method: FlashStorage JSON data" + String(" ").substring(0, 20) + "║");
        Serial.println("║  📏 Size: " + String(jsonString.length()) + " bytes" + String(" ").substring(0, max(0, 30 - String(jsonString.length()).length())) + "║");
        Serial.println("║  🕐 Timestamp: " + String(configData.timestamp) + "ms" + String(" ").substring(0, max(0, 25 - String(configData.timestamp).length())) + "║");
        Serial.println("║  ✅ Will persist across reboots and power cycles            ║");
        Serial.println("╚══════════════════════════════════════════════════════════════╝");
        Serial.println("");
        
        // Verify the write by reading it back
        WiFiConfigData verifyData = wifiConfigStorage.read();
        if (verifyData.isValid && strcmp(verifyData.jsonData, configData.jsonData) == 0) {
            Serial.println("VERIFICATION: JSON data saved and verified successfully!");
            Serial.println("VERIFICATION: " + String(verifyData.jsonData));
        } else {
            Serial.println("WARNING: Verification failed - data may not be saved correctly");
        }
    } else {
        Serial.println("ERROR: Invalid credentials - cannot save to storage");
    }
}

void WiFiProvisioning::loadWiFiCredentials() {
    Serial.println("Loading WiFi credentials from JSON data in FlashStorage...");
    
    // JSON-LIKE APPROACH: Load from FlashStorage
    // This will retrieve JSON data saved across reboots and power cycles
    
    WiFiConfigData storedData = wifiConfigStorage.read();
    
    if (storedData.isValid && strlen(storedData.jsonData) > 0) {
        Serial.println("JSON data found in flash storage");
        Serial.println("JSON content: " + String(storedData.jsonData));
        Serial.println("JSON size: " + String(strlen(storedData.jsonData)) + " bytes");
        Serial.println("Timestamp: " + String(storedData.timestamp));
        
        // Parse JSON
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, storedData.jsonData);
        
        if (error) {
            Serial.println("ERROR: Failed to parse JSON: " + String(error.c_str()));
            wifiCreds.isValid = false;
            memset(wifiCreds.ssid, 0, sizeof(wifiCreds.ssid));
            memset(wifiCreds.password, 0, sizeof(wifiCreds.password));
            Serial.println("Device will start in BLE provisioning mode");
            return;
        }
        
        // Extract credentials from JSON - handle both "password" and "pass" fields
        String ssid = doc["ssid"].as<String>();
        String password = doc["password"].as<String>();
        
        // Check if password is empty or "null", then try "pass" field
        if (password.length() == 0 || password == "null") {
            password = doc["pass"].as<String>();
            Serial.println("Using 'pass' field: " + password);
        } else {
            Serial.println("Using 'password' field: " + password);
        }
        bool isValid = doc["isValid"].as<bool>();
        unsigned long timestamp = doc["timestamp"].as<unsigned long>();
        String version = doc["version"].as<String>();
        String device = doc["device"].as<String>();
        unsigned long lastModified = doc["lastModified"].as<unsigned long>();
        
        Serial.println("Extracted SSID: " + ssid);
        Serial.println("Extracted Password: " + password);
        Serial.println("Extracted isValid: " + String(isValid ? "true" : "false"));
        Serial.println("Extracted timestamp: " + String(timestamp));
        Serial.println("Extracted version: " + version);
        Serial.println("Extracted device: " + device);
        Serial.println("Extracted lastModified: " + String(lastModified));
        
        // Validate credentials
        if (ssid.length() > 0 && password.length() > 0 && isValid) {
            // Copy to working structure
            strncpy(wifiCreds.ssid, ssid.c_str(), sizeof(wifiCreds.ssid) - 1);
            strncpy(wifiCreds.password, password.c_str(), sizeof(wifiCreds.password) - 1);
            wifiCreds.isValid = true;
            
            Serial.println("");
            Serial.println("╔══════════════════════════════════════════════════════════════╗");
            Serial.println("║              📂 LOADED SAVED CREDENTIALS 📂                  ║");
            Serial.println("╠══════════════════════════════════════════════════════════════╣");
            Serial.println("║  📶 Network: " + String(wifiCreds.ssid) + String(" ").substring(0, max(0, 30 - String(wifiCreds.ssid).length())) + "║");
            Serial.println("║  🔑 Password: " + String(wifiCreds.password) + String(" ").substring(0, max(0, 28 - String(wifiCreds.password).length())) + "║");
            Serial.println("║  💾 Method: FlashStorage JSON data" + String(" ").substring(0, 20) + "║");
            Serial.println("║  🕐 Saved at: " + String(timestamp) + "ms" + String(" ").substring(0, max(0, 25 - String(timestamp).length())) + "║");
            Serial.println("║  📝 Version: " + version + String(" ").substring(0, max(0, 30 - version.length())) + "║");
            Serial.println("║  📱 Device: " + device + String(" ").substring(0, max(0, 30 - device.length())) + "║");
            Serial.println("╚══════════════════════════════════════════════════════════════╝");
            Serial.println("");
        } else {
            Serial.println("ERROR: Invalid credentials in JSON data");
            wifiCreds.isValid = false;
            memset(wifiCreds.ssid, 0, sizeof(wifiCreds.ssid));
            memset(wifiCreds.password, 0, sizeof(wifiCreds.password));
            Serial.println("Device will start in BLE provisioning mode");
        }
    } else {
        Serial.println("No valid JSON data found in flash storage");
        wifiCreds.isValid = false;
        memset(wifiCreds.ssid, 0, sizeof(wifiCreds.ssid));
        memset(wifiCreds.password, 0, sizeof(wifiCreds.password));
        Serial.println("Device will start in BLE provisioning mode");
    }
}

void WiFiProvisioning::sendReadyStatus() {
    sendStatusUpdate("ready", "Device ready for WiFi credentials");
}

void WiFiProvisioning::sendAckStatus() {
    sendStatusUpdate("ack", "Credentials received");
}

void WiFiProvisioning::sendConnectingStatus() {
    sendStatusUpdate("connecting", "Attempting WiFi connection");
}

void WiFiProvisioning::sendSuccessStatus() {
    String message = "Connected to " + String(WiFi.SSID()) + " | IP: " + WiFi.localIP().toString() + " | RSSI: " + String(WiFi.RSSI()) + "dBm";
    sendStatusUpdate("success", message);
}

void WiFiProvisioning::sendErrorStatus(String error) {
    sendStatusUpdate("error", error);
}

void WiFiProvisioning::restartProvisioning() {
    Serial.println("Restarting WiFi provisioning...");
    
    // Disconnect WiFi
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(1000);
    }
    
    // Reset state
    isConnected = false;
    isProvisioning = false;
    
    // Clear credentials
    wifiCreds.isValid = false;
    memset(wifiCreds.ssid, 0, sizeof(wifiCreds.ssid));
    memset(wifiCreds.password, 0, sizeof(wifiCreds.password));
    
    // Restart BLE provisioning
    startBLEProvisioning();
}

void WiFiProvisioning::handleBLEMessage(String message) {
    Serial.println("Received BLE message: " + message);
    
    // Check if it's a WiFi credentials message - handle both "password" and "pass" fields
    if (message.startsWith("{\"ssid\":") && 
        (message.indexOf("\"password\":") > 0 || message.indexOf("\"pass\":") > 0)) {
        Serial.println("Recognized as WiFi credentials message");
        processWiFiCredentials(message);
    } else if (message == "restart") {
        restartProvisioning();
    } else {
        Serial.println("Unknown BLE message: " + message);
    }
}

void WiFiProvisioning::onBLEMessage(String message) {
    if (instance) {
        instance->handleBLEMessage(message);
    }
}