# SmartChessboard MKR WiFi 1010 - Arduino IDE Version

## Descrizione
Progetto Arduino per la gestione della scacchiera intelligente usando Arduino MKR WiFi 1010.

## Hardware Richiesto
- Arduino MKR WiFi 1010
- LED strip WS2812B (opzionale)
- Sensori di pressione (opzionale)
- Motore vibrazione (opzionale)

## Librerie Richieste
Installa queste librerie tramite Arduino IDE Library Manager:

1. **ArduinoJson** (versione 6.x)
   - Vai su Tools → Manage Libraries
   - Cerca "ArduinoJson"
   - Installa la versione 6.x (NON la 7.x)

2. **WiFiNINA** (inclusa con Arduino IDE)
   - Già inclusa con l'IDE Arduino

## Configurazione WiFi
Modifica il file `src/wifi_config.h` con le tue credenziali WiFi:

```cpp
#define WIFI_SSID "Il_Tuo_SSID"
#define WIFI_PASSWORD "La_Tua_Password"
```

## Installazione
1. Apri Arduino IDE
2. Vai su File → Open
3. Seleziona il file `MKR_Chessboard_Arduino.ino`
4. Seleziona la scheda: Tools → Board → Arduino MKR WiFi 1010
5. Seleziona la porta: Tools → Port → [La tua porta MKR]
6. Compila e carica il codice

## Struttura del Progetto
```
MKR_Chessboard_Arduino/
├── MKR_Chessboard_Arduino.ino    # File principale
├── src/
│   ├── config.h                  # Configurazioni generali
│   ├── wifi_config.h             # Credenziali WiFi
│   ├── ChessboardProtocol.h      # Header del protocollo
│   └── ChessboardProtocol.cpp    # Implementazione del protocollo
└── README.md                     # Questo file
```

## Funzionalità
- Connessione WiFi automatica
- Comunicazione seriale con Arduino Mega
- Gestione protocollo scacchi
- Controllo LED e feedback aptico
- Logica di gioco degli scacchi

## Monitor Seriale
Apri il monitor seriale a 115200 baud per vedere:
- Stato di connessione WiFi
- Messaggi di debug
- Comunicazione con il Mega
- Eventi del gioco

## Risoluzione Problemi
- **Errore di compilazione**: Verifica che ArduinoJson 6.x sia installato
- **WiFi non si connette**: Controlla le credenziali in `wifi_config.h`
- **Porta non trovata**: Verifica che l'MKR sia collegato e riconosciuto