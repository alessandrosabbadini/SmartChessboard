# Comandi di Test - Mega (Dobot Controller)

## Come Testare il Sistema Mega

### 1. **Apri il Monitor Seriale**
- Arduino IDE: Tools → Serial Monitor (115200 baud)
- Oppure usa PlatformIO: `pio device monitor --port /dev/cu.usbmodem[PORTA] --baud 115200`

### 2. **Comandi Disponibili**

#### **Comandi di Informazione**
```
help, ?          - Mostra tutti i comandi disponibili
status           - Stato completo del sistema
calib            - Stato della calibrazione
```

#### **Comandi di Controllo**
```
calibrate        - Avvia calibrazione scacchiera
start            - Avvia partita
stop             - Ferma partita
emergency        - Stop di emergenza
reset            - Reset stop di emergenza
home             - Vai a posizione home
```

#### **Comandi di Test**
```
test             - Test movimento Dobot
gripper          - Test gripper (apertura/chiusura)
move e2e4        - Simula mossa (esempio: e2e4)
```

## Esempi di Test

### **Test di Avvio Sistema**
```
help
status
calib
```
**Output atteso:**
```
========================================
SmartChessboard MEGA - Dobot Controller
========================================
Firmware Version: 1.0.0 | Build Date: [data]
Calibration Status: NOT CALIBRATED
Game Status: STOPPED
Emergency Stop: INACTIVE
========================================
```

### **Test di Calibrazione**
```
calibrate
```
**Output atteso:**
```
=== STARTING CALIBRATION ===
Calibrazione in corso...
Calibration completed and saved to EEPROM
```

### **Test di Movimento Dobot**
```
test
```
**Output atteso:**
```
Test movimento Dobot...
1. Movimento a posizione sicura...
2. Test movimento a coordinate scacchiera...
Movimento a: X=150.0, Y=150.0, Z=20.0
3. Ritorno a posizione home...
Test movimento completato!
```

### **Test Gripper**
```
gripper
```
**Output atteso:**
```
Test gripper...
1. Apertura gripper...
2. Chiusura gripper...
3. Apertura gripper...
4. Disattivazione gripper...
Test gripper completato!
```

### **Test di Mossa**
```
start
move e2e4
```
**Output atteso:**
```
Game started
Comando ricevuto: move e2e4
Simulando mossa: e2e4
=== EXECUTING MOVE ===
Move from column: e to column: e - Using travel height: 37.0
1. Moving to safe height above source (Z=22.0)
2. Moving down to pickup position (Z=37.0)
3. Moving to safe height with piece (Z=22.0)
4. Moving above destination (Z=22.0)
5. Moving down to place position (Z=37.0)
6. Moving to final safe height (Z=22.0)
```

## Test di Comunicazione con MKR

### **Test di Comunicazione Bidirezionale**
1. **Dal MKR:** Invia comando `move e2e4`
2. **Al Mega:** Il comando viene ricevuto via Serial1
3. **Dal Mega:** Esegue il movimento del Dobot
4. **Al MKR:** Invia conferma del movimento

### **Test di Sequenza Completa**
```
calibrate
start
test
gripper
move e2e4
move e7e5
stop
```

## Risoluzione Problemi

### **Errore: "Sistema non calibrato"**
```
ERROR: Sistema non calibrato!
```
**Soluzione:** Esegui `calibrate` prima di usare il sistema

### **Errore: "Partita non in corso"**
```
ERROR: Partita non in corso!
```
**Soluzione:** Esegui `start` per avviare la partita

### **Errore: "Stop di emergenza attivo"**
```
EMERGENCY STOP ACTIVATED!
```
**Soluzione:** Esegui `reset` per disattivare lo stop di emergenza

### **Dobot non si muove**
**Soluzione:**
1. Verifica che il Dobot sia collegato e alimentato
2. Controlla che la libreria Dobot sia installata
3. Esegui `test` per verificare il movimento

### **Gripper non funziona**
**Soluzione:**
1. Verifica che il gripper sia collegato
2. Esegui `gripper` per testare il funzionamento
3. Controlla le connessioni elettriche

## Comandi Avanzati

### **Sequenza di Test Completa**
```
help
status
calib
calibrate
start
test
gripper
move e2e4
move e7e5
move g1f3
stop
```

### **Test di Movimenti Multipli**
```
start
move e2e4
move e7e5
move g1f3
move b8c6
move f1c4
move f8b4
```

### **Test di Sicurezza**
```
emergency
reset
home
```

## Comunicazione con MKR

### **Comandi Ricevuti dal MKR**
- `CALIBRATE` - Avvia calibrazione
- `STARTGAME` - Avvia partita
- `ENDGAME` - Ferma partita
- `e2e4` - Esegue mossa (formato notazione scacchi)

### **Messaggi Inviati al MKR**
- `CALIB_MSG:Calibration loaded from EEPROM`
- `CALIB_MSG:ERROR: Cannot start game without calibration!`
- `CALIB_MSG:EMERGENCY STOP ACTIVATED!`

Questo ti permette di testare completamente il sistema Mega senza bisogno dell'app! 🚀
