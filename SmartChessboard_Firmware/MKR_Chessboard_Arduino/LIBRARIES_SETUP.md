# Setup Librerie per Arduino IDE

## Librerie Richieste

### 1. ArduinoJson (versione 6.x)
**IMPORTANTE**: Installa la versione 6.x, NON la 7.x!

**Come installare:**
1. Apri Arduino IDE
2. Vai su **Tools → Manage Libraries...**
3. Cerca "**ArduinoJson**"
4. Seleziona la versione **6.21.5** (o l'ultima 6.x disponibile)
5. Clicca **Install**

**Verifica installazione:**
- Vai su **Sketch → Include Library**
- Dovresti vedere "ArduinoJson" nella lista

### 2. WiFiNINA
Questa libreria è già inclusa con Arduino IDE per le schede MKR.

**Verifica:**
- Vai su **Sketch → Include Library**
- Dovresti vedere "WiFiNINA" nella lista

## Configurazione Arduino IDE

### 1. Seleziona la Scheda
1. Vai su **Tools → Board → Arduino SAMD (32-bits ARM Cortex-M0+) → Arduino MKR WiFi 1010**

### 2. Seleziona la Porta
1. Collega l'MKR al computer via USB
2. Vai su **Tools → Port**
3. Seleziona la porta che appare (es. `/dev/cu.usbmodem12101` su Mac)

### 3. Configurazione WiFi
Modifica il file `src/wifi_config.h` con le tue credenziali:

```cpp
#define WIFI_SSID "Il_Tuo_SSID"
#define WIFI_PASSWORD "La_Tua_Password"
```

## Compilazione e Upload

1. **Compila**: Clicca il pulsante ✓ (Verify)
2. **Upload**: Clicca il pulsante → (Upload)
3. **Monitor Seriale**: Vai su **Tools → Serial Monitor** (115200 baud)

## Risoluzione Problemi

### Errore: "ArduinoJson.h: No such file or directory"
- **Soluzione**: Installa ArduinoJson 6.x dalla Library Manager

### Errore: "WiFiNINA.h: No such file or directory"
- **Soluzione**: Assicurati di aver selezionato la scheda MKR WiFi 1010

### Errore di compilazione con ArduinoJson 7.x
- **Soluzione**: Disinstalla ArduinoJson 7.x e installa la 6.x

### Porta non trovata
- **Soluzione**: 
  1. Ricollega l'MKR
  2. Controlla che sia riconosciuto dal sistema
  3. Seleziona la porta corretta in Tools → Port

## Test del Sistema

Dopo l'upload, apri il monitor seriale (115200 baud) e dovresti vedere:

```
========================================
SmartChessboard MKR WiFi 1010
Arduino IDE Version
========================================
Firmware Version: 1.0.0
Device Name: SmartChessboard MKR
Build Date: [data di compilazione]
========================================
[SUCCESS] Chessboard Protocol initialized
========================================
Connection Status:
WiFi: Connected/Disconnected
IP Address: [se connesso]
Mega Communication: Serial1 (9600 baud)
========================================
SmartChessboard MKR ready!
WiFi credentials configured in wifi_config.h
========================================
```
