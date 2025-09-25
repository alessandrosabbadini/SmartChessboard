/*
 * SmartChessboard MKR WiFi 1010 - Arduino IDE Version
 * 
 * Questo progetto gestisce:
 * - Connessione WiFi
 * - Protocollo di comunicazione con il Mega
 * - Controllo LED e feedback aptico
 * - Logica di gioco degli scacchi
 * 
 * Librerie richieste:
 * - ArduinoJson (versione 6.x)
 * - WiFiNINA (inclusa con Arduino IDE)
 * 
 * Hardware:
 * - Arduino MKR WiFi 1010
 * - LED strip (WS2812B)
 * - Sensori di pressione
 * - Motore vibrazione
 */

#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "src/config.h"
#include "src/wifi_config.h"
#include "src/ChessboardProtocol.h"

// Istanza del protocollo scacchi
ChessboardProtocol chessboard;

void setup() {
  // Inizializza la comunicazione seriale
  Serial.begin(115200);
  while (!Serial) {
    ; // Attendi che la porta seriale si connetta
  }
  
  // Inizializza la comunicazione con il Mega
  Serial1.begin(9600);
  
  // Stampa informazioni di avvio
  printStartupInfo();
  
  // Inizializza il protocollo scacchi
  chessboard.begin();
  Serial.println("[SUCCESS] Chessboard Protocol initialized");
  
  // Stampa informazioni di connessione
  printConnectionInfo();
  
  Serial.println("========================================");
  Serial.println("SmartChessboard MKR ready!");
  Serial.println("WiFi credentials configured in wifi_config.h");
  Serial.println("========================================");
}

void loop() {
  // Gestisce la connessione WiFi
  chessboard.handleWiFi();
  
  // Gestisce i sensori
  chessboard.handleSensors();
  
  // Gestisce input da monitor seriale
  handleSerialInput();
  
  // Aggiorna lo stato del sistema
  chessboard.updateStatus();
  
  // Piccola pausa per evitare sovraccarico
  delay(10);
}

void printStartupInfo() {
  Serial.println("========================================");
  Serial.println("SmartChessboard MKR WiFi 1010");
  Serial.println("Arduino IDE Version");
  Serial.println("========================================");
  Serial.print("Firmware Version: ");
  Serial.println(DEVICE_VERSION);
  Serial.print("Device Name: ");
  Serial.println(DEVICE_NAME);
  Serial.print("Build Date: ");
  Serial.println(__DATE__ " " __TIME__);
  Serial.println("========================================");
}

void printConnectionInfo() {
  Serial.println("========================================");
  Serial.println("Connection Status:");
  Serial.print("WiFi: ");
  if (chessboard.isWiFiConnected()) {
    Serial.println("Connected");
    Serial.print("IP Address: ");
    Serial.println(chessboard.getDeviceIP());
  } else {
    Serial.println("Disconnected");
  }
  Serial.print("Mega Communication: ");
  Serial.println("Serial1 (9600 baud)");
  Serial.println("========================================");
}

void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      Serial.println("Comando ricevuto: " + input);
      
      // Gestisce comandi di test
      if (input == "help" || input == "?") {
        printHelp();
      }
      else if (input == "status") {
        printSystemStatus();
      }
      else if (input == "wifi") {
        printWiFiStatus();
      }
      else if (input == "ping") {
        sendTestPing();
      }
      else if (input.startsWith("move ")) {
        String move = input.substring(5);
        simulateMove(move);
      }
      else if (input.startsWith("led ")) {
        String ledCmd = input.substring(4);
        controlLED(ledCmd);
      }
      else if (input.startsWith("haptic ")) {
        String hapticCmd = input.substring(7);
        controlHaptic(hapticCmd);
      }
      else if (input == "reset") {
        Serial.println("Resetting system...");
        // Qui potresti aggiungere un reset del sistema
      }
      else {
        Serial.println("Comando non riconosciuto. Digita 'help' per vedere i comandi disponibili.");
      }
    }
  }
}

void printHelp() {
  Serial.println("========================================");
  Serial.println("COMANDI DISPONIBILI:");
  Serial.println("========================================");
  Serial.println("help, ?          - Mostra questo aiuto");
  Serial.println("status           - Stato del sistema");
  Serial.println("wifi             - Stato WiFi");
  Serial.println("ping             - Invia ping di test");
  Serial.println("move e2e4        - Simula mossa (es: e2e4)");
  Serial.println("led on           - Accendi LED");
  Serial.println("led off          - Spegni LED");
  Serial.println("led blink        - Fai lampeggiare LED");
  Serial.println("haptic short     - Feedback aptico breve");
  Serial.println("haptic long      - Feedback aptico lungo");
  Serial.println("reset            - Reset sistema");
  Serial.println("========================================");
}

void printSystemStatus() {
  Serial.println("========================================");
  Serial.println("STATO SISTEMA:");
  Serial.println("========================================");
  Serial.print("WiFi: ");
  Serial.println(chessboard.isWiFiConnected() ? "Connesso" : "Disconnesso");
  if (chessboard.isWiFiConnected()) {
    Serial.print("IP: ");
    Serial.println(chessboard.getDeviceIP());
  }
  Serial.print("FEN corrente: ");
  Serial.println(chessboard.getCurrentFEN());
  Serial.print("Giocatore: ");
  Serial.println(chessboard.getCurrentPlayer());
  Serial.print("Scacco: ");
  Serial.println(chessboard.isInCheck() ? "Sì" : "No");
  Serial.print("Scacco matto: ");
  Serial.println(chessboard.isCheckmate() ? "Sì" : "No");
  Serial.println("========================================");
}

void printWiFiStatus() {
  Serial.println("========================================");
  Serial.println("STATO WIFI:");
  Serial.println("========================================");
  if (chessboard.isWiFiConnected()) {
    Serial.println("Stato: Connesso");
    Serial.print("IP Address: ");
    Serial.println(chessboard.getDeviceIP());
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);
  } else {
    Serial.println("Stato: Disconnesso");
    Serial.print("SSID configurato: ");
    Serial.println(WIFI_SSID);
  }
  Serial.println("========================================");
}

void sendTestPing() {
  Serial.println("Inviando ping di test...");
  // Crea un messaggio ping di test
  DynamicJsonDocument doc(1024);
  doc["type"] = "PING";
  doc["id"] = "test_ping";
  doc["data"]["timestamp"] = millis();
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  Serial.println("Ping inviato: " + message);
}

void simulateMove(String move) {
  Serial.println("Simulando mossa: " + move);
  
  // Invia comando al Mega via Serial1
  Serial1.println(move);
  Serial.println("Comando inviato al Mega: " + move);
  
  // Simula anche la risposta del protocollo
  DynamicJsonDocument doc(1024);
  doc["type"] = "MOVE_DETECTED";
  doc["id"] = "serial_test";
  doc["data"]["fromSquare"] = move.substring(0, 2);
  doc["data"]["toSquare"] = move.substring(2, 4);
  doc["data"]["pieceType"] = "pawn";
  doc["data"]["capturedPiece"] = nullptr;
  doc["data"]["isPromotion"] = false;
  doc["data"]["promotionPiece"] = nullptr;
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  Serial.println("Mossa simulata: " + message);
}

void controlLED(String command) {
  Serial.println("Controllo LED: " + command);
  
  if (command == "on") {
    Serial.println("LED accesi");
    // Qui potresti aggiungere il controllo effettivo dei LED
  }
  else if (command == "off") {
    Serial.println("LED spenti");
    // Qui potresti aggiungere il controllo effettivo dei LED
  }
  else if (command == "blink") {
    Serial.println("LED lampeggianti");
    // Qui potresti aggiungere il controllo effettivo dei LED
  }
  else {
    Serial.println("Comando LED non riconosciuto. Usa: on, off, blink");
  }
}

void controlHaptic(String command) {
  Serial.println("Controllo aptico: " + command);
  
  if (command == "short") {
    Serial.println("Feedback aptico breve");
    // Qui potresti aggiungere il controllo effettivo del motore vibrazione
  }
  else if (command == "long") {
    Serial.println("Feedback aptico lungo");
    // Qui potresti aggiungere il controllo effettivo del motore vibrazione
  }
  else {
    Serial.println("Comando aptico non riconosciuto. Usa: short, long");
  }
}
