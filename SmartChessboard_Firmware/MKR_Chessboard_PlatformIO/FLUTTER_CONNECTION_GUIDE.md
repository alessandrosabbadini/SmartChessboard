# Guida Connessione Flutter App con NAOchess Board

## Informazioni BLE del Dispositivo

### Identificatori BLE
- **Nome Dispositivo**: `NAOchess Board`
- **Service UUID**: `12345678-1234-1234-1234-123456789ABC`
- **Characteristic UUID**: `87654321-4321-4321-4321-CBA987654321`
- **Capacità**: Read, Write, Notify

### Configurazione Flutter BLE

```dart
// UUIDs del dispositivo
const String SERVICE_UUID = "12345678-1234-1234-1234-123456789ABC";
const String CHARACTERISTIC_UUID = "87654321-4321-4321-4321-CBA987654321";
const String DEVICE_NAME = "NAOchess Board";

// Configurazione BLE
BluetoothDevice? device;
BluetoothCharacteristic? characteristic;
bool isConnected = false;
```

## Flusso di Connessione

### 1. Scansione e Connessione
```dart
// Scansiona dispositivi BLE
await FlutterBluePlus.startScan(timeout: Duration(seconds: 10));

// Filtra per nome dispositivo
var devices = FlutterBluePlus.scanResults
    .where((result) => result.device.platformName == DEVICE_NAME)
    .map((result) => result.device);

// Connetti al dispositivo
device = devices.first;
await device.connect();

// Trova il servizio e la caratteristica
List<BluetoothService> services = await device.discoverServices();
BluetoothService service = services.firstWhere(
    (s) => s.uuid.toString().toUpperCase() == SERVICE_UUID.toUpperCase()
);
characteristic = service.characteristics.firstWhere(
    (c) => c.uuid.toString().toUpperCase() == CHARACTERISTIC_UUID.toUpperCase()
);
```

### 2. Ascolta i Messaggi dal Dispositivo
```dart
// Ascolta notifiche
await characteristic.setNotifyValue(true);
characteristic.lastValueStream.listen((value) {
    String message = String.fromCharCodes(value);
    print("Ricevuto: $message");
    handleDeviceMessage(message);
});
```

### 3. Invia Messaggi al Dispositivo
```dart
// Invia messaggio JSON
String message = '{"type":"PING","id":"1","timestamp":${DateTime.now().millisecondsSinceEpoch},"data":{"message":"Hello"}}';
await characteristic.write(utf8.encode(message));
```

## Messaggi del Protocollo

### Messaggi da Inviare al Dispositivo

#### 1. Test di Connessione (PING)
```json
{
  "type": "PING",
  "id": "1",
  "timestamp": 1234567890,
  "data": {
    "message": "Hello from Flutter"
  }
}
```

#### 2. Configurazione WiFi
```json
{
  "type": "WIFI_CONFIG",
  "id": "2",
  "timestamp": 1234567890,
  "data": {
    "ssid": "YourWiFiNetwork",
    "password": "YourWiFiPassword"
  }
}
```

#### 3. Controllo LED
```json
{
  "type": "LED_CONTROL",
  "id": "3",
  "timestamp": 1234567890,
  "data": {
    "pattern": "blink",
    "squares": ["e4", "d4"],
    "color": "red",
    "duration": 1000,
    "intensity": 255
  }
}
```

### Messaggi Ricevuti dal Dispositivo

#### 1. Informazioni Dispositivo (automatico alla connessione)
```json
{
  "type": "DEVICE_INFO",
  "id": "1",
  "timestamp": 1234567890,
  "data": {
    "name": "NAOchess Board",
    "version": "1.0.0",
    "capabilities": ["BLE", "WiFi", "LED", "Haptic"]
  }
}
```

#### 2. Risposta PONG
```json
{
  "type": "PONG",
  "id": "1",
  "timestamp": 1234567890,
  "data": {
    "originalId": "1",
    "originalTimestamp": 1234567890,
    "message": "Hello from Flutter"
  }
}
```

#### 3. Stato WiFi
```json
{
  "type": "WIFI_STATUS",
  "id": "2",
  "timestamp": 1234567890,
  "data": {
    "status": "connecting",
    "ip": "",
    "signal": 0,
    "error": ""
  }
}
```

## Debug e Risoluzione Problemi

### 1. Verifica Connessione
- Controlla che il dispositivo sia visibile come "NAOchess Board"
- Verifica che i UUID siano corretti
- Assicurati che la caratteristica supporti Read/Write/Notify

### 2. Log del Dispositivo
Il dispositivo stampa informazioni dettagliate sulla porta seriale:
- Connessioni/disconnessioni BLE
- Messaggi ricevuti e inviati
- Errori di comunicazione

### 3. Test di Base
1. Connetti al dispositivo
2. Invia un messaggio PING
3. Aspetta la risposta PONG
4. Se funziona, prova la configurazione WiFi

### 4. Problemi Comuni
- **Dispositivo non trovato**: Verifica che sia in modalità advertising
- **Connessione fallisce**: Controlla i permessi BLE dell'app
- **Messaggi non ricevuti**: Verifica che la caratteristica supporti le notifiche
- **UUID non trovati**: Controlla che i UUID siano esatti (case-sensitive)

## Esempio Completo Flutter

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:convert';

class ChessboardBLE {
  static const String SERVICE_UUID = "12345678-1234-1234-1234-123456789ABC";
  static const String CHARACTERISTIC_UUID = "87654321-4321-4321-4321-CBA987654321";
  static const String DEVICE_NAME = "NAOchess Board";
  
  BluetoothDevice? device;
  BluetoothCharacteristic? characteristic;
  
  Future<bool> connect() async {
    try {
      // Scansiona dispositivi
      await FlutterBluePlus.startScan(timeout: Duration(seconds: 10));
      
      // Trova il dispositivo
      var devices = FlutterBluePlus.scanResults
          .where((result) => result.device.platformName == DEVICE_NAME)
          .map((result) => result.device);
      
      if (devices.isEmpty) return false;
      
      device = devices.first;
      await device!.connect();
      
      // Trova servizio e caratteristica
      List<BluetoothService> services = await device!.discoverServices();
      BluetoothService service = services.firstWhere(
          (s) => s.uuid.toString().toUpperCase() == SERVICE_UUID.toUpperCase());
      characteristic = service.characteristics.firstWhere(
          (c) => c.uuid.toString().toUpperCase() == CHARACTERISTIC_UUID.toUpperCase());
      
      // Abilita notifiche
      await characteristic!.setNotifyValue(true);
      
      return true;
    } catch (e) {
      print("Errore connessione: $e");
      return false;
    }
  }
  
  void sendPing() {
    String message = jsonEncode({
      "type": "PING",
      "id": "1",
      "timestamp": DateTime.now().millisecondsSinceEpoch,
      "data": {"message": "Hello from Flutter"}
    });
    characteristic?.write(utf8.encode(message));
  }
  
  void sendWiFiConfig(String ssid, String password) {
    String message = jsonEncode({
      "type": "WIFI_CONFIG",
      "id": "2",
      "timestamp": DateTime.now().millisecondsSinceEpoch,
      "data": {"ssid": ssid, "password": password}
    });
    characteristic?.write(utf8.encode(message));
  }
}
```
