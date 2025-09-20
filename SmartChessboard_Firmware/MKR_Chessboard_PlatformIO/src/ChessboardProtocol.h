#ifndef CHESSBOARD_PROTOCOL_H
#define CHESSBOARD_PROTOCOL_H

#include <Arduino.h>
#include <WiFiNINA.h>
// #include <WebServer.h> // Removed to avoid conflicts
#include <ArduinoJson.h>
#include "MKRBLE.h"
#include "config.h"
#include "debug.h"

class ChessboardProtocol {
public:
    // Constructor
    ChessboardProtocol();
    
    // Static instance for callbacks
    static ChessboardProtocol* instance;
    
    // Initialization
    void begin();
    void setupBluetooth();
    void setupWiFi();
    void setupHardware();
    
    // Main loop functions
    void handleBluetooth();
    void handleWiFi();
    void handleSensors();
    void updateStatus();
    
    // Message handling
    void processMessage(String message);
    void sendMessage(String type, JsonObject data);
    void sendErrorMessage(String errorCode, String errorMessage);
    void sendDeviceInfo();
    void sendSetupStatus(String status, String message = "");
    
    // WiFi configuration
    void handleWiFiConfig(JsonObject data);
    void sendWiFiStatus(String status, String ip, int signal, String error);
    
    // Game state management
    void handleGameState(JsonObject data);
    void updateGameState(String fen, String player, bool check, bool checkmate, bool stalemate);
    
    // Move detection
    void detectMove();
    void simulateMove(); // For testing
    void handleMoveDetected(JsonObject data);
    void sendMoveConfirm(String moveId, String status, String errorMessage = "");
    
    // LED control
    void handleLEDControl(JsonObject data);
    void controlLEDs(String pattern, JsonArray squares, String color, int duration, int intensity);
    void blinkLED(int duration, int intensity);
    
    // Haptic feedback
    void handleHapticFeedback(JsonObject data);
    void sendHapticFeedback(String pattern, int duration, int intensity);
    
    // Ping/Pong
    void handlePingMessage(JsonObject data);
    void sendPongResponse(unsigned long originalTimestamp);
    
    // Connection management
    bool isWiFiConnected();
    bool isBluetoothConnected();
    String getDeviceIP();
    
    // Status getters
    String getCurrentFEN();
    String getCurrentPlayer();
    bool isInCheck();
    bool isCheckmate();
    bool isStalemate();

private:
    // Hardware pins
    int ledPin;
    int hapticPin;
    int statusLedPin;
    
    // Connection status
    bool wifiConnected;
    bool bluetoothConnected;
    String deviceIP;
    
    // WiFi server (disabled to avoid conflicts)
    // WebServer* server;
    
    // BLE components
    MKRBLE* ble;
    
    // Game state
    String currentFEN;
    String currentPlayer;
    bool gameIsCheck;
    bool gameIsCheckmate;
    bool gameIsStalemate;
    String lastMove;
    
    // Sensor data
    bool sensorStates[TOTAL_SQUARES];
    bool previousSensorStates[TOTAL_SQUARES];
    unsigned long lastSensorRead;
    
    // Timing
    unsigned long lastPingTime;
    unsigned long lastMoveTime;
    
    // Message ID counter
    unsigned long messageIdCounter;
    
    // Helper functions
    String generateMessageId();
    bool validateMessage(JsonObject doc);
    void updateSensorStates();
    void processSensorChanges();
    String squareIndexToNotation(int index);
    int notationToSquareIndex(String notation);
    void initializeSensors();
    
    // BLE message handler
    static void handleBLEMessage(String message);
};

#endif // CHESSBOARD_PROTOCOL_H
