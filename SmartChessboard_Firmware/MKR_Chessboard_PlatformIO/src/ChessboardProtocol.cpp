#include "ChessboardProtocol.h"

// Static instance for callbacks
ChessboardProtocol* ChessboardProtocol::instance = nullptr;

// Constructor
ChessboardProtocol::ChessboardProtocol() {
    // Set static instance
    instance = this;
    
    ledPin = LED_PIN;
    hapticPin = HAPTIC_PIN;
    statusLedPin = STATUS_LED_PIN;
    
    wifiConnected = false;
    bluetoothConnected = false;
    deviceIP = "";
    
    // server = nullptr; // Disabled to avoid conflicts
    ble = new MKRBLE();
    
    currentFEN = DEFAULT_FEN;
    currentPlayer = DEFAULT_PLAYER;
    gameIsCheck = false;
    gameIsCheckmate = false;
    gameIsStalemate = false;
    lastMove = "";
    
    lastSensorRead = 0;
    lastPingTime = 0;
    lastMoveTime = 0;
    messageIdCounter = 0;
    
    // Initialize sensor states
    for (int i = 0; i < TOTAL_SQUARES; i++) {
        sensorStates[i] = false;
        previousSensorStates[i] = false;
    }
}

void ChessboardProtocol::begin() {
    DEBUG_LOG("Initializing Chessboard Protocol...");
    
    setupHardware();
    setupBluetooth();
    setupWiFi();
    
    DEBUG_LOG("Chessboard Protocol initialized successfully");
    LOG_CONNECTION_STATUS();
}

void ChessboardProtocol::setupHardware() {
    DEBUG_LOG("Setting up hardware...");
    
    pinMode(ledPin, OUTPUT);
    pinMode(hapticPin, OUTPUT);
    pinMode(statusLedPin, OUTPUT);
    
    // Initialize status LED
    digitalWrite(statusLedPin, HIGH);
    delay(500);
    digitalWrite(statusLedPin, LOW);
    
    initializeSensors();
    
    DEBUG_LOG("Hardware setup complete");
}

void ChessboardProtocol::setupBluetooth() {
    DEBUG_LOG("Setting up Bluetooth LE...");
    
    // Initialize MKR WiFi 1010 BLE
    ble->begin();
    ble->setMessageCallback(handleBLEMessage);
    
    DEBUG_LOG_BLE("Bluetooth LE initialized");
    DEBUG_LOG_BLE("Device name: " + String(BLE_DEVICE_NAME));
    DEBUG_LOG_BLE("Service UUID: " + String(CHESSBOARD_SERVICE_UUID));
    DEBUG_LOG_BLE("Characteristic UUID: " + String(CHESSBOARD_CHARACTERISTIC_UUID));
    
    // Send device information immediately when BLE is ready
    sendDeviceInfo();
    sendSetupStatus(SETUP_STATUS_SCANNING, "Chessboard ready for connection");
}

void ChessboardProtocol::setupWiFi() {
    DEBUG_LOG("Setting up WiFi...");
    
    // Create access point for configuration
    // For MKR WiFi 1010, we'll use station mode instead of AP mode
    // WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    // IPAddress IP = WiFi.softAPIP();
    DEBUG_LOG_WIFI("WiFi setup for MKR WiFi 1010");
    
    // Create web server (disabled to avoid conflicts)
    // server = new WebServer(CHESSBOARD_TCP_PORT);
    
    // Setup routes (disabled to avoid conflicts)
    /*
    server->on("/", [this]() {
        String html = "<!DOCTYPE html><html><head><title>NAOchess Configuration</title>";
        html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0;}";
        html += ".container{background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
        html += "input[type=text],input[type=password]{width:100%;padding:10px;margin:5px 0;border:1px solid #ddd;border-radius:5px;}";
        html += "input[type=submit]{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;}";
        html += "input[type=submit]:hover{background:#0056b3;}</style></head><body>";
        html += "<div class='container'><h1>NAOchess Board Configuration</h1>";
        html += "<form method='POST' action='/configure'>";
        html += "<label>WiFi SSID:</label><br>";
        html += "<input type='text' name='ssid' required><br><br>";
        html += "<label>WiFi Password:</label><br>";
        html += "<input type='password' name='password'><br><br>";
        html += "<input type='submit' value='Configure WiFi'>";
        html += "</form>";
        html += "<p><a href='/status'>Check Status</a></p>";
        html += "</div></body></html>";
        server->send(200, "text/html", html);
    });
    
    server->on("/configure", HTTP_POST, [this]() {
        if (server->hasArg("ssid")) {
            String ssid = server->arg("ssid");
            String password = server->arg("password");
            
            DEBUG_LOG_WIFI("Configuring WiFi: " + ssid);
            
            // Create JSON object for WiFi config
            DynamicJsonDocument doc(JSON_BUFFER_SIZE);
            doc["ssid"] = ssid;
            doc["password"] = password;
            doc["securityType"] = "WPA2";
            
            handleWiFiConfig(doc.as<JsonObject>());
        } else {
            server->send(400, "text/plain", "Missing SSID parameter");
        }
    });
    
    server->on("/status", [this]() {
        String response = "NAOchess ESP32 Chessboard Status\n";
        response += "================================\n";
        response += "WiFi Connected: " + String(wifiConnected ? "Yes" : "No") + "\n";
        if (wifiConnected) {
            response += "IP Address: " + deviceIP + "\n";
            response += "Port: " + String(CHESSBOARD_TCP_PORT) + "\n";
        }
        response += "Bluetooth Connected: " + String(bluetoothConnected ? "Yes" : "No") + "\n";
        response += "Current FEN: " + currentFEN + "\n";
        response += "Current Player: " + currentPlayer + "\n";
        response += "Check: " + String(gameIsCheck ? "Yes" : "No") + "\n";
        response += "Checkmate: " + String(gameIsCheckmate ? "Yes" : "No") + "\n";
        response += "Stalemate: " + String(gameIsStalemate ? "Yes" : "No") + "\n";
        response += "Free Memory: Available\n";
        
        server->send(200, "text/plain", response);
    });
    
    server->on("/message", HTTP_POST, [this]() {
        if (server->hasArg("plain")) {
            String message = server->arg("plain");
            DEBUG_LOG_MESSAGE("WiFi", message);
            processMessage(message);
            server->send(200, "application/json", "{\"status\":\"received\"}");
        } else {
            server->send(400, "application/json", "{\"error\":\"No message body\"}");
        }
    });
    
    server->on("/ping", HTTP_GET, [this]() {
        sendPongResponse(millis());
        server->send(200, "application/json", "{\"status\":\"pong\"}");
    });
    
    server->on("/game", HTTP_GET, [this]() {
        DynamicJsonDocument response(JSON_BUFFER_SIZE);
        response["type"] = MSG_TYPE_GAME_STATE;
        response["id"] = generateMessageId();
        response["data"]["fen"] = currentFEN;
        response["data"]["currentPlayer"] = currentPlayer;
        response["data"]["isCheck"] = gameIsCheck;
        response["data"]["isCheckmate"] = gameIsCheckmate;
        response["data"]["isStalemate"] = gameIsStalemate;
        response["data"]["lastMove"] = lastMove;
        response["timestamp"] = millis();
        
        String responseStr;
        serializeJson(response, responseStr);
        server->send(200, "application/json", responseStr);
    });
    
    server->begin();
    DEBUG_LOG_WIFI("WiFi server started on port " + String(CHESSBOARD_TCP_PORT));
    */
}

void ChessboardProtocol::handleBluetooth() {
    // Handle BLE events
    ble->handleEvents();
}

void ChessboardProtocol::handleWiFi() {
    // WiFi server handling disabled to avoid conflicts
    /*
    if (server) {
        server->handleClient();
    }
    */
}

void ChessboardProtocol::handleSensors() {
    if (millis() - lastSensorRead > SENSOR_READ_DELAY) {
        updateSensorStates();
        processSensorChanges();
        lastSensorRead = millis();
    }
}

void ChessboardProtocol::updateStatus() {
    // Update WiFi connection status
    bool currentWiFiStatus = (WiFi.status() == WL_CONNECTED);
    if (currentWiFiStatus != wifiConnected) {
        wifiConnected = currentWiFiStatus;
        if (wifiConnected) {
            deviceIP = WiFi.localIP().toString();
            DEBUG_LOG_WIFI("Connected! IP: " + deviceIP);
        } else {
            deviceIP = "";
            DEBUG_LOG_WIFI("Disconnected");
        }
    }
    
    // Send periodic ping
    if (millis() - lastPingTime > PING_INTERVAL_MS) {
        DynamicJsonDocument pingData(512);
        pingData["timestamp"] = millis();
        sendMessage(MSG_TYPE_PING, pingData.as<JsonObject>());
        lastPingTime = millis();
    }
    
    // Simulate moves for testing
    if (millis() - lastMoveTime > MOVE_SIMULATION_INTERVAL_MS) {
        simulateMove();
        lastMoveTime = millis();
    }
}

void ChessboardProtocol::processMessage(String message) {
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        DEBUG_LOG_ERROR("JSON parsing failed: " + String(error.c_str()));
        sendErrorMessage(ERROR_INVALID_MESSAGE, "JSON parsing failed");
        return;
    }
    
    if (!validateMessage(doc.as<JsonObject>())) {
        return;
    }
    
    String type = doc["type"];
    String id = doc["id"];
    
    DEBUG_LOG_MESSAGE(type, message);
    
    if (type == MSG_TYPE_WIFI_CONFIG) {
        handleWiFiConfig(doc["data"]);
    } else if (type == MSG_TYPE_GAME_STATE) {
        handleGameState(doc["data"]);
    } else if (type == MSG_TYPE_LED_CONTROL) {
        handleLEDControl(doc["data"]);
    } else if (type == MSG_TYPE_HAPTIC_FEEDBACK) {
        handleHapticFeedback(doc["data"]);
    } else if (type == MSG_TYPE_PING) {
        handlePingMessage(doc["data"]);
    } else if (type == MSG_TYPE_MOVE_DETECTED) {
        handleMoveDetected(doc["data"]);
    } else {
        DEBUG_LOG_ERROR("Unknown message type: " + type);
        sendErrorMessage(ERROR_INVALID_MESSAGE, "Unknown message type: " + type);
    }
}

void ChessboardProtocol::sendMessage(String type, JsonObject data) {
    DynamicJsonDocument message(JSON_BUFFER_SIZE);
    message["type"] = type;
    message["id"] = generateMessageId();
    message["data"] = data;
    message["timestamp"] = millis();
    
    String messageStr;
    serializeJson(message, messageStr);
    
    DEBUG_LOG_JSON(messageStr);
    
    // Send via BLE if connected
    if (ble->isConnected()) {
        ble->sendMessage(messageStr);
    }
    
    // Send via WiFi if connected
    if (wifiConnected) {
        DEBUG_LOG_WIFI("Sent: " + messageStr);
    }
}

void ChessboardProtocol::sendErrorMessage(String errorCode, String errorMessage) {
    DynamicJsonDocument errorData(512);
    errorData["errorCode"] = errorCode;
    errorData["errorMessage"] = errorMessage;
    errorData["details"] = "";
    
    sendMessage(MSG_TYPE_ERROR, errorData.as<JsonObject>());
}

void ChessboardProtocol::handleWiFiConfig(JsonObject data) {
    String ssid = data["ssid"].as<String>();
    String password = data["password"].as<String>();
    String security = data["securityType"].as<String>();
    
    DEBUG_LOG_WIFI("Configuring WiFi: " + ssid);
    sendSetupStatus(SETUP_STATUS_CONFIGURING_WIFI, "Connecting to " + ssid);
    
    // Connect to WiFi
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_RETRIES) {
        delay(500);
        DEBUG_LOG(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        deviceIP = WiFi.localIP().toString();
        DEBUG_LOG_WIFI("Connected! IP: " + deviceIP);
        sendWiFiStatus("CONNECTED", deviceIP, WiFi.RSSI(), "");
        sendSetupStatus(SETUP_STATUS_COMPLETED, "Setup completed successfully! IP: " + deviceIP);
    } else {
        wifiConnected = false;
        DEBUG_LOG_WIFI("Connection failed");
        sendWiFiStatus("FAILED", "", 0, "Connection timeout");
        sendSetupStatus(SETUP_STATUS_FAILED, "WiFi connection failed");
        sendErrorMessage(ERROR_WIFI_CONNECTION_FAILED, "Failed to connect to WiFi network");
    }
}

void ChessboardProtocol::sendWiFiStatus(String status, String ip, int signal, String error) {
    DynamicJsonDocument statusData(512);
    statusData["status"] = status;
    statusData["ipAddress"] = ip;
    statusData["signalStrength"] = signal;
    statusData["errorMessage"] = error;
    
    sendMessage(MSG_TYPE_WIFI_STATUS, statusData.as<JsonObject>());
}

void ChessboardProtocol::handleGameState(JsonObject data) {
    currentFEN = data["fen"].as<String>();
    currentPlayer = data["currentPlayer"].as<String>();
    gameIsCheck = data["isCheck"];
    gameIsCheckmate = data["isCheckmate"];
    gameIsStalemate = data["isStalemate"];
    
    if (data.containsKey("lastMove")) {
        lastMove = data["lastMove"].as<String>();
    }
    
    DEBUG_LOG_INFO("Game state updated: " + currentFEN);
}

void ChessboardProtocol::updateGameState(String fen, String player, bool check, bool checkmate, bool stalemate) {
    currentFEN = fen;
    currentPlayer = player;
    gameIsCheck = check;
    gameIsCheckmate = checkmate;
    gameIsStalemate = stalemate;
}

void ChessboardProtocol::detectMove() {
    // This would be implemented with real sensor reading
    // For now, we'll use the simulation
    simulateMove();
}

void ChessboardProtocol::simulateMove() {
    // Simulate a move for testing purposes
    static int moveCounter = 0;
    String moves[][2] = {
        {"e2", "e4"}, {"e7", "e5"}, {"g1", "f3"}, {"b8", "c6"},
        {"f1", "c4"}, {"f8", "c5"}, {"e1", "g1"}, {"e8", "g8"}
    };
    
    if (moveCounter < 8) {
        String fromSquare = moves[moveCounter][0];
        String toSquare = moves[moveCounter][1];
        
        DynamicJsonDocument moveData(512);
        moveData["fromSquare"] = fromSquare;
        moveData["toSquare"] = toSquare;
        moveData["pieceType"] = "pawn";
        moveData["capturedPiece"] = nullptr;
        moveData["isPromotion"] = false;
        moveData["promotionPiece"] = nullptr;
        
        sendMessage(MSG_TYPE_MOVE_DETECTED, moveData.as<JsonObject>());
        
        // Send haptic feedback
        DynamicJsonDocument hapticData(256);
        hapticData["pattern"] = HAPTIC_PATTERN_MOVE;
        hapticData["duration"] = HAPTIC_DEFAULT_DURATION_MS;
        hapticData["intensity"] = 50;
        handleHapticFeedback(hapticData.as<JsonObject>());
        
        // Send LED control
        DynamicJsonDocument ledData(512);
        ledData["pattern"] = LED_PATTERN_MOVE_HIGHLIGHT;
        JsonArray squares = ledData.createNestedArray("squares");
        squares.add(fromSquare);
        squares.add(toSquare);
        ledData["color"] = "blue";
        ledData["duration"] = 2000;
        ledData["intensity"] = 100;
        handleLEDControl(ledData.as<JsonObject>());
        
        lastMove = fromSquare + toSquare;
        moveCounter++;
    }
}

void ChessboardProtocol::handleLEDControl(JsonObject data) {
    String pattern = data["pattern"].as<String>();
    String color = data["color"].as<String>();
    int duration = data["duration"];
    int intensity = data["intensity"];
    
    DEBUG_LOG_INFO("LED control: " + pattern + " " + color);
    
    if (pattern == LED_PATTERN_MOVE_HIGHLIGHT) {
        blinkLED(duration, intensity);
    } else if (pattern == LED_PATTERN_CHECK) {
        for (int i = 0; i < 3; i++) {
            blinkLED(200, intensity);
            delay(100);
        }
    } else if (pattern == LED_PATTERN_CHECKMATE) {
        for (int i = 0; i < 5; i++) {
            blinkLED(300, intensity);
            delay(150);
        }
    } else if (pattern == LED_PATTERN_ERROR) {
        for (int i = 0; i < 2; i++) {
            blinkLED(500, intensity);
            delay(200);
        }
    }
}

void ChessboardProtocol::controlLEDs(String pattern, JsonArray squares, String color, int duration, int intensity) {
    // This would control individual LEDs for specific squares
    // For now, we'll use the general LED control
    DynamicJsonDocument ledData(512);
    ledData["pattern"] = pattern;
    ledData["squares"] = squares;
    ledData["color"] = color;
    ledData["duration"] = duration;
    ledData["intensity"] = intensity;
    
    handleLEDControl(ledData.as<JsonObject>());
}

void ChessboardProtocol::blinkLED(int duration, int intensity) {
    digitalWrite(ledPin, HIGH);
    delay(duration);
    digitalWrite(ledPin, LOW);
}

void ChessboardProtocol::handleHapticFeedback(JsonObject data) {
    String pattern = data["pattern"].as<String>();
    int duration = data["duration"];
    int intensity = data["intensity"];
    
    DEBUG_LOG_INFO("Haptic feedback: " + pattern);
    
    if (pattern == HAPTIC_PATTERN_MOVE) {
        digitalWrite(hapticPin, HIGH);
        delay(duration);
        digitalWrite(hapticPin, LOW);
    } else if (pattern == HAPTIC_PATTERN_CAPTURE) {
        for (int i = 0; i < 2; i++) {
            digitalWrite(hapticPin, HIGH);
            delay(duration);
            digitalWrite(hapticPin, LOW);
            delay(50);
        }
    } else if (pattern == HAPTIC_PATTERN_CHECK) {
        for (int i = 0; i < 2; i++) {
            digitalWrite(hapticPin, HIGH);
            delay(duration);
            digitalWrite(hapticPin, LOW);
            delay(100);
        }
    } else if (pattern == HAPTIC_PATTERN_ERROR) {
        digitalWrite(hapticPin, HIGH);
        delay(duration * 2);
        digitalWrite(hapticPin, LOW);
    }
}

void ChessboardProtocol::sendHapticFeedback(String pattern, int duration, int intensity) {
    DynamicJsonDocument hapticData(256);
    hapticData["pattern"] = pattern;
    hapticData["duration"] = duration;
    hapticData["intensity"] = intensity;
    
    handleHapticFeedback(hapticData.as<JsonObject>());
}

void ChessboardProtocol::handlePingMessage(JsonObject data) {
    unsigned long originalTimestamp = data["timestamp"];
    sendPongResponse(originalTimestamp);
}

void ChessboardProtocol::sendPongResponse(unsigned long originalTimestamp) {
    DynamicJsonDocument pongData(512);
    pongData["originalTimestamp"] = originalTimestamp;
    pongData["responseTimestamp"] = millis();
    
    sendMessage(MSG_TYPE_PONG, pongData.as<JsonObject>());
}

bool ChessboardProtocol::isWiFiConnected() {
    return wifiConnected;
}

bool ChessboardProtocol::isBluetoothConnected() {
    return bluetoothConnected;
}

String ChessboardProtocol::getDeviceIP() {
    return deviceIP;
}

String ChessboardProtocol::getCurrentFEN() {
    return currentFEN;
}

String ChessboardProtocol::getCurrentPlayer() {
    return currentPlayer;
}

bool ChessboardProtocol::isInCheck() {
    return gameIsCheck;
}

bool ChessboardProtocol::isCheckmate() {
    return gameIsCheckmate;
}

bool ChessboardProtocol::isStalemate() {
    return gameIsStalemate;
}

String ChessboardProtocol::generateMessageId() {
    return String(++messageIdCounter);
}

bool ChessboardProtocol::validateMessage(JsonObject doc) {
    if (doc.isNull()) {
        DEBUG_LOG_ERROR("Invalid JSON message");
        return false;
    }
    
    if (!doc.containsKey("type")) {
        DEBUG_LOG_ERROR("Message missing type field");
        return false;
    }
    
    if (!doc.containsKey("id")) {
        DEBUG_LOG_ERROR("Message missing id field");
        return false;
    }
    
    if (!doc.containsKey("data")) {
        DEBUG_LOG_ERROR("Message missing data field");
        return false;
    }
    
    return true;
}

void ChessboardProtocol::updateSensorStates() {
    // This would read from actual sensors
    // For now, we'll simulate sensor readings
    for (int i = 0; i < TOTAL_SQUARES; i++) {
        previousSensorStates[i] = sensorStates[i];
        // Simulate random sensor changes for testing
        if (random(1000) < 1) { // 0.1% chance per sensor per read
            sensorStates[i] = !sensorStates[i];
        }
    }
}

void ChessboardProtocol::processSensorChanges() {
    // Look for sensor state changes that might indicate a move
    for (int i = 0; i < TOTAL_SQUARES; i++) {
        if (sensorStates[i] != previousSensorStates[i]) {
            String square = squareIndexToNotation(i);
            DEBUG_LOG_INFO("Sensor change detected at square: " + square);
            // Process the sensor change...
        }
    }
}

String ChessboardProtocol::squareIndexToNotation(int index) {
    int row = index / BOARD_SIZE;
    int col = index % BOARD_SIZE;
    char file = 'a' + col;
    int rank = 8 - row;
    return String(file) + String(rank);
}

int ChessboardProtocol::notationToSquareIndex(String notation) {
    if (notation.length() != 2) return -1;
    
    char file = notation.charAt(0);
    char rank = notation.charAt(1);
    
    int col = file - 'a';
    int row = 8 - (rank - '0');
    
    if (col < 0 || col >= BOARD_SIZE || row < 0 || row >= BOARD_SIZE) {
        return -1;
    }
    
    return row * BOARD_SIZE + col;
}

void ChessboardProtocol::initializeSensors() {
    // This would initialize the actual sensor pins
    // For now, we'll just initialize the sensor state arrays
    DEBUG_LOG_INFO("Initializing sensors...");
    
    for (int i = 0; i < TOTAL_SQUARES; i++) {
        sensorStates[i] = false;
        previousSensorStates[i] = false;
    }
    
    DEBUG_LOG_INFO("Sensors initialized");
}

// BLE message handler
void ChessboardProtocol::handleBLEMessage(String message) {
    if (instance != nullptr) {
        DEBUG_LOG_BLE("Received BLE message: " + message);
        instance->processMessage(message);
    }
}

// New methods for enhanced protocol
void ChessboardProtocol::sendDeviceInfo() {
    DynamicJsonDocument deviceData(512);
    deviceData["name"] = DEVICE_NAME;
    deviceData["version"] = DEVICE_VERSION;
    deviceData["status"] = "READY";
    deviceData["capabilities"] = JsonArray();
    deviceData["capabilities"].add("LED_CONTROL");
    deviceData["capabilities"].add("HAPTIC_FEEDBACK");
    deviceData["capabilities"].add("MOVE_DETECTION");
    deviceData["capabilities"].add("GAME_STATE");
    
    sendMessage(MSG_TYPE_DEVICE_INFO, deviceData.as<JsonObject>());
    DEBUG_LOG_BLE("Device information sent");
}

void ChessboardProtocol::sendSetupStatus(String status, String message) {
    DynamicJsonDocument statusData(512);
    statusData["status"] = status;
    statusData["message"] = message;
    statusData["timestamp"] = millis();
    
    sendMessage(MSG_TYPE_SETUP_STATUS, statusData.as<JsonObject>());
    DEBUG_LOG_BLE("Setup status: " + status + " - " + message);
}

void ChessboardProtocol::handleMoveDetected(JsonObject data) {
    String fromSquare = data["fromSquare"].as<String>();
    String toSquare = data["toSquare"].as<String>();
    String pieceType = data["pieceType"].as<String>();
    String moveId = data.containsKey("id") ? data["id"].as<String>() : generateMessageId();
    
    DEBUG_LOG_INFO("Move detected: " + fromSquare + " to " + toSquare + " (" + pieceType + ")");
    
    // Send move confirmation
    sendMoveConfirm(moveId, "MOVE_ACCEPTED");
    
    // Update game state
    lastMove = fromSquare + toSquare;
    
    // Send haptic feedback
    DynamicJsonDocument hapticData(256);
    hapticData["pattern"] = HAPTIC_PATTERN_MOVE;
    hapticData["duration"] = HAPTIC_DEFAULT_DURATION_MS;
    hapticData["intensity"] = 50;
    handleHapticFeedback(hapticData.as<JsonObject>());
    
    // Send LED control
    DynamicJsonDocument ledData(512);
    ledData["pattern"] = LED_PATTERN_MOVE_HIGHLIGHT;
    JsonArray squares = ledData.createNestedArray("squares");
    squares.add(fromSquare);
    squares.add(toSquare);
    ledData["color"] = "blue";
    ledData["duration"] = 2000;
    ledData["intensity"] = 100;
    handleLEDControl(ledData.as<JsonObject>());
}

void ChessboardProtocol::sendMoveConfirm(String moveId, String status, String errorMessage) {
    DynamicJsonDocument confirmData(512);
    confirmData["moveId"] = moveId;
    confirmData["status"] = status;
    confirmData["errorMessage"] = errorMessage;
    
    sendMessage(MSG_TYPE_MOVE_CONFIRM, confirmData.as<JsonObject>());
    DEBUG_LOG_INFO("Move confirmation sent: " + status);
}
