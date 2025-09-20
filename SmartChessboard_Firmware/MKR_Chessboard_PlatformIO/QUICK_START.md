# MKR WiFi Provisioning - Quick Start

## 🚀 Carica il Firmware sul MKR WiFi 1010

### Prerequisiti
- Arduino MKR WiFi 1010
- PlatformIO installato
- Cavo USB

### Passi Rapidi

1. **Collega il MKR** al computer via USB

2. **Compila e carica**:
   ```bash
   cd MKR_Chessboard_PlatformIO
   pio run --target upload
   ```

3. **Monitora i log**:
   ```bash
   pio device monitor
   ```

4. **Verifica il funzionamento**:
   - Il MKR dovrebbe apparire come "MKR WiFi Provisioning" in BLE
   - I log dovrebbero mostrare: "BLE advertising started"

### Test con l'App Flutter

1. **Apri l'app NAOchess**
2. **Vai a Profile → Chessboard Connection**
3. **Premi "Start Provisioning"**
4. **L'app dovrebbe trovare il dispositivo MKR**
5. **Inserisci le credenziali WiFi**
6. **Il MKR si connetterà automaticamente**

### Risoluzione Problemi

**Se il dispositivo non appare**:
- Verifica che il MKR sia alimentato
- Controlla i log seriali per errori
- Riavvia il MKR

**Se l'app non si connette**:
- Verifica che il Bluetooth sia abilitato
- Controlla che il dispositivo appaia come "MKR WiFi Provisioning"
- Riavvia l'app

**Se il WiFi non si connette**:
- Verifica le credenziali WiFi
- Controlla che la rete sia 2.4GHz
- Verifica la potenza del segnale

### Log di Esempio

```
========================================
MKR WiFi Provisioning Starting...
Version: 1.0.0
========================================
System Information:
  Board: Arduino MKR WiFi 1010
  CPU: SAMD21G18A 48MHz
  Flash: 256KB
  RAM: 32KB
========================================
WiFi Provisioning ready!
Connect via Bluetooth LE: MKR WiFi Provisioning
========================================
=== MKR WiFi Provisioning BLE Initialized ===
Device name: MKR WiFi Provisioning
Service UUID: 12345678-1234-1234-1234-123456789abc
Data Characteristic UUID: 12345678-1234-1234-1234-123456789abd
Status Characteristic UUID: 12345678-1234-1234-1234-123456789abe
BLE advertising started - Device is discoverable!
```

### Successo! 🎉

Se vedi questi log, il firmware è caricato correttamente e pronto per la configurazione WiFi tramite l'app Flutter.
