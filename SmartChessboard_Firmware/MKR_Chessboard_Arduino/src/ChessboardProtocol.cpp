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
    deviceIP = "";
    
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


void ChessboardProtocol::setupWiFi() {
    DEBUG_LOG("Setting up WiFi...");
    
    // Initialize WiFi module
    if (WiFi.status() == WL_NO_MODULE) {
        DEBUG_LOG_ERROR("WiFi module not found!");
        return;
    }
    
    if (WiFi.status() == WL_NO_SHIELD) {
        DEBUG_LOG_ERROR("WiFi shield not found!");
        return;
    }
    
    DEBUG_LOG_WIFI("WiFi module initialized successfully");
    
    // Connect to WiFi using credentials from wifi_config.h
    DEBUG_LOG_WIFI("Connecting to WiFi: " + String(WIFI_SSID));
    sendSetupStatus(SETUP_STATUS_CONNECTING, "Connecting to " + String(WIFI_SSID));
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_RETRIES) {
        delay(1000);
        attempts++;
        DEBUG_LOG(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        deviceIP = WiFi.localIP().toString();
        DEBUG_LOG_WIFI("Connected! IP: " + deviceIP);
        sendWiFiStatus(WIFI_STATUS_CONNECTED, deviceIP, WiFi.RSSI(), "");
        sendSetupStatus(SETUP_STATUS_COMPLETED, "WiFi connected successfully! IP: " + deviceIP);
    } else {
        wifiConnected = false;
        DEBUG_LOG_WIFI("Connection failed");
        sendWiFiStatus(WIFI_STATUS_ERROR, "", 0, "Connection timeout");
        sendSetupStatus(SETUP_STATUS_FAILED, "WiFi connection failed");
    }
}

void ChessboardProtocol::handleWiFi() {
    // Check WiFi connection status
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
}

void ChessboardProtocol::handleSensors() {
    if (millis() - lastSensorRead > SENSOR_READ_DELAY) {
        updateSensorStates();
        processSensorChanges();
        lastSensorRead = millis();
    }
}

void ChessboardProtocol::updateStatus() {
    // Send periodic ping
    if (millis() - lastPingTime > PING_INTERVAL_MS) {
        DynamicJsonDocument pingData(512);
        pingData["timestamp"] = millis();
        sendMessage(MSG_TYPE_PING, pingData.as<JsonObject>());
        lastPingTime = millis();
    }
    
    // Move simulation removed - now handled by external input
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
    
    if (type == MSG_TYPE_GAME_STATE) {
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
    // Move detection would be implemented with real sensor reading
    // Currently disabled - moves are handled by external input
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
    
    DEBUG_LOG_INFO("Haptic feedback: " + pattern + " (intensity: " + String(intensity) + ")");
    
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
    // Sensor reading would be implemented with real hardware
    // Currently disabled - sensor changes are handled by external input
    for (int i = 0; i < TOTAL_SQUARES; i++) {
        previousSensorStates[i] = sensorStates[i];
        // Keep current sensor states unchanged
    }
}

void ChessboardProtocol::processSensorChanges() {
    // Sensor change processing would be implemented with real hardware
    // Currently disabled - sensor changes are handled by external input
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
    DEBUG_LOG("Device information sent");
}

void ChessboardProtocol::sendSetupStatus(String status, String message) {
    DynamicJsonDocument statusData(512);
    statusData["status"] = status;
    statusData["message"] = message;
    statusData["timestamp"] = millis();
    
    sendMessage(MSG_TYPE_SETUP_STATUS, statusData.as<JsonObject>());
    DEBUG_LOG("Setup status: " + status + " - " + message);
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
