# Comandi di Test - Monitor Seriale

## Come Testare il Sistema

### 1. **Apri il Monitor Seriale**
- Arduino IDE: Tools → Serial Monitor (115200 baud)
- Oppure usa PlatformIO: `pio device monitor --port /dev/cu.usbmodem12101 --baud 115200`

### 2. **Comandi Disponibili**

#### **Comandi di Informazione**
```
help, ?          - Mostra tutti i comandi disponibili
status           - Stato completo del sistema
wifi             - Stato della connessione WiFi
```

#### **Comandi di Test**
```
ping             - Invia un ping di test
move e2e4        - Simula una mossa (esempio: e2e4)
led on           - Accendi i LED
led off          - Spegni i LED
led blink        - Fai lampeggiare i LED
haptic short     - Feedback aptico breve
haptic long      - Feedback aptico lungo
reset            - Reset del sistema
```

## Esempi di Test

### **Test di Connessione WiFi**
```
wifi
```
**Output atteso:**
```
========================================
STATO WIFI:
========================================
Stato: Connesso
IP Address: 192.168.1.100
SSID: iPhone di Alessandro
========================================
```

### **Test di Stato Sistema**
```
status
```
**Output atteso:**
```
========================================
STATO SISTEMA:
========================================
WiFi: Connesso
IP: 192.168.1.100
FEN corrente: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
Giocatore: White
Scacco: No
Scacco matto: No
========================================
```

### **Test di Mossa**
```
move e2e4
```
**Output atteso:**
```
Comando ricevuto: move e2e4
Simulando mossa: e2e4
Comando inviato al Mega: e2e4
Mossa simulata: {"type":"MOVE_DETECTED","id":"serial_test","data":{"fromSquare":"e2","toSquare":"e4","pieceType":"pawn","capturedPiece":null,"isPromotion":false,"promotionPiece":null},"timestamp":12345}
```

### **Test di Ping**
```
ping
```
**Output atteso:**
```
Comando ricevuto: ping
Inviando ping di test...
Ping inviato: {"type":"PING","id":"test_ping","data":{"timestamp":12345},"timestamp":12345}
```

### **Test LED**
```
led blink
```
**Output atteso:**
```
Comando ricevuto: led blink
Controllo LED: blink
LED lampeggianti
```

### **Test Feedback Aptico**
```
haptic short
```
**Output atteso:**
```
Comando ricevuto: haptic short
Controllo aptico: short
Feedback aptico breve
```

## Test di Comunicazione con Mega

### **Test di Mossa al Mega**
1. Digita: `move e2e4`
2. Il comando viene inviato al Mega via Serial1
3. Il Mega dovrebbe ricevere: `e2e4`
4. Il Mega dovrebbe eseguire il movimento del Dobot

### **Test di Comunicazione Bidirezionale**
1. Il Mega può inviare risposte via Serial1
2. L'MKR le riceve e le processa
3. I messaggi vengono mostrati nel monitor seriale

## Risoluzione Problemi

### **Comando non riconosciuto**
```
Comando non riconosciuto. Digita 'help' per vedere i comandi disponibili.
```
**Soluzione:** Usa `help` per vedere tutti i comandi disponibili

### **WiFi non connesso**
```
Stato: Disconnesso
SSID configurato: iPhone di Alessandro
```
**Soluzione:** 
1. Verifica le credenziali in `src/wifi_config.h`
2. Controlla che la rete WiFi sia disponibile
3. Riavvia l'MKR

### **Mega non risponde**
**Soluzione:**
1. Verifica che il Mega sia collegato
2. Controlla la connessione Serial1
3. Verifica che il Mega sia programmato correttamente

## Comandi Avanzati

### **Sequenza di Test Completa**
```
help
status
wifi
ping
move e2e4
led blink
haptic short
status
```

### **Test di Movimenti Multipli**
```
move e2e4
move e7e5
move g1f3
move b8c6
```

Questo ti permette di testare completamente il sistema senza bisogno dell'app! 🚀
