# MKR WiFi Provisioning Firmware

This PlatformIO project implements the MKR WiFi provisioning firmware for the NAOchess smart chessboard system. It provides Bluetooth LE communication for WiFi configuration with the Flutter app, following the custom MKR WiFi Provisioning Protocol.

## 🚀 Features

### Communication Protocols
- **Bluetooth LE**: Initial WiFi configuration and setup
- **WiFi TCP Server**: Real-time game communication
- **Web Configuration Interface**: Access point for manual WiFi setup
- **JSON Message Protocol**: Complete implementation of the chessboard communication specification

### Hardware Integration
- **LED Control**: Support for various LED patterns (move highlight, check, checkmate, error)
- **Haptic Feedback**: Vibration patterns for different game events
- **Move Detection**: Simulated sensor system ready for real hardware integration
- **Status Indicators**: Visual feedback for connection and system status

### Advanced Features
- **Comprehensive Error Handling**: Robust error management with detailed logging
- **Memory Management**: Optimized for ESP32 memory constraints
- **Debug System**: Extensive logging and debugging capabilities
- **Modular Architecture**: Clean, maintainable code structure

## 📋 Hardware Requirements

### Required Components
- Arduino MKR WiFi 1010 development board
- Built-in LED (connected to GPIO 2)
- Built-in WiFi module (WiFiNINA)
- Built-in Bluetooth LE module (ArduinoBLE)

### Pin Configuration
```cpp
#define LED_PIN 2           // Built-in LED for status indication
#define HAPTIC_PIN 4        // Future haptic feedback
#define STATUS_LED_PIN 5    // Future status LED
```

## 🛠️ Installation & Setup

### Prerequisites
- [PlatformIO Core](https://platformio.org/install) or PlatformIO IDE
- Arduino MKR WiFi 1010 development board
- USB cable for programming

### 1. Clone and Setup
```bash
# Navigate to project directory
cd MKR_Chessboard_PlatformIO

# Install dependencies (PlatformIO will handle this automatically)
pio lib install
```

### 2. Build and Upload
```bash
# Build the project
pio run

# Upload to MKR WiFi 1010
pio run --target upload

# Monitor serial output
pio device monitor
```

### 3. Initial Configuration

#### Bluetooth LE WiFi Provisioning
1. **Power on the MKR WiFi 1010**
2. **Device will advertise**: "MKR WiFi Provisioning"
3. **Use Flutter app**: Navigate to Profile → Chessboard Connection
4. **Scan and connect**: App will find and connect to MKR device
5. **Configure WiFi**: Enter your WiFi credentials in the app
6. **Automatic connection**: MKR will connect to WiFi and save credentials

## 📡 Communication Protocol

### MKR WiFi Provisioning Protocol

The MKR device implements a custom BLE protocol for WiFi provisioning:

#### 1. Device Advertising
- **Device Name**: "MKR WiFi Provisioning"
- **Service UUID**: `12345678-1234-1234-1234-123456789abc`
- **Data Characteristic**: `12345678-1234-1234-1234-123456789abd` (BLEWrite)
- **Status Characteristic**: `12345678-1234-1234-1234-123456789abe` (BLERead + BLENotify)

#### 2. WiFi Credentials Format
```json
{
  "ssid": "MyWiFiNetwork",
  "pass": "mypassword",
  "timestamp": 1234567890
}
```

#### 3. Status Messages
```json
{
  "status": "ready|ack|connecting|connected|success|error",
  "message": "Status description",
  "timestamp": 1234567890
}
```

#### Move Detection
```json
{
  "type": "MOVE_DETECTED",
  "id": "move_001",
  "data": {
    "fromSquare": "e2",
    "toSquare": "e4",
    "pieceType": "pawn",
    "capturedPiece": null,
    "isPromotion": false,
    "promotionPiece": null
  },
  "timestamp": 1234567890
}
```

#### LED Control
```json
{
  "type": "LED_CONTROL",
  "id": "led_001",
  "data": {
    "pattern": "MOVE_HIGHLIGHT",
    "squares": ["e2", "e4"],
    "color": "blue",
    "duration": 2000,
    "intensity": 100
  },
  "timestamp": 1234567890
}
```

#### Haptic Feedback
```json
{
  "type": "HAPTIC_FEEDBACK",
  "id": "haptic_001",
  "data": {
    "pattern": "MOVE",
    "duration": 100,
    "intensity": 50
  },
  "timestamp": 1234567890
}
```

### API Endpoints

#### WiFi Configuration
- **GET** `/` - Configuration web interface
- **POST** `/configure` - WiFi setup form
- **GET** `/status` - System status

#### Game Communication
- **POST** `/message` - Send JSON messages
- **GET** `/ping` - Connection health check
- **GET** `/game` - Current game state

## 🔧 Flutter App Integration

### Connection Setup
```dart
// Example connection to ESP32
String esp32IP = "192.168.1.100"; // Replace with actual IP
int port = 8080;

// Send LED control message
http.post(
  Uri.parse('http://$esp32IP:$port/message'),
  headers: {'Content-Type': 'application/json'},
  body: jsonEncode({
    'type': 'LED_CONTROL',
    'id': 'led_001',
    'data': {
      'pattern': 'MOVE_HIGHLIGHT',
      'squares': ['e2', 'e4'],
      'color': 'blue',
      'duration': 2000,
      'intensity': 100
    },
    'timestamp': DateTime.now().millisecondsSinceEpoch
  })
);
```

### Bluetooth LE Integration
```dart
// Connect to ESP32 via Bluetooth LE
BluetoothDevice device = await BluetoothDevice.fromId(deviceId);
BluetoothCharacteristic characteristic = await device.connect();
await characteristic.write(utf8.encode(jsonMessage));
```

## 🏗️ Architecture

### File Structure
```
src/
├── main.cpp                 # Main application entry point
├── ChessboardProtocol.h     # Protocol class header
├── ChessboardProtocol.cpp   # Protocol implementation
├── config.h                 # Configuration constants
└── debug.h                  # Debug utilities
```

### Class Hierarchy
```
ChessboardProtocol
├── Bluetooth LE Management
├── WiFi Server Management
├── Message Processing
├── Hardware Control (LED, Haptic)
├── Sensor Management
└── Game State Management
```

## 🐛 Debugging & Development

### Debug Features
- **Comprehensive Logging**: All operations logged with timestamps
- **Memory Monitoring**: Real-time memory usage tracking
- **Connection Status**: WiFi and Bluetooth connection monitoring
- **Performance Metrics**: Timing and throughput measurements

### Debug Commands
```cpp
// Enable debug mode in config.h
#define DEBUG_MODE true

// Use debug macros
DEBUG_LOG("Debug message");
DEBUG_LOG_F("Formatted: %d", value);
DEBUG_LOG_JSON(jsonString);
```

### Serial Monitor Output
Look for these key messages:
- `[DEBUG] NAOchess ESP32 Chessboard Starting...`
- `[BLE] Bluetooth LE advertising started`
- `[WIFI] WiFi server started on port 8080`
- `[MSG] Type: LED_CONTROL Data: {...}`

## 🔧 Customization

### Adding Real Sensors
Replace the sensor simulation in `ChessboardProtocol.cpp`:

```cpp
void ChessboardProtocol::updateSensorStates() {
    // Read from actual magnetic sensors
    for (int i = 0; i < TOTAL_SQUARES; i++) {
        sensorStates[i] = digitalRead(sensorPins[i]);
    }
}
```

### Custom LED Patterns
Add new patterns in `handleLEDControl()`:

```cpp
else if (pattern == "CUSTOM_PATTERN") {
    // Your custom LED pattern implementation
    customLEDPattern();
}
```

### Custom Haptic Patterns
Add new haptic feedback patterns:

```cpp
else if (pattern == "CUSTOM_HAPTIC") {
    // Your custom haptic pattern
    customHapticPattern();
}
```

## 🚨 Troubleshooting

### Common Issues

#### WiFi Connection Failed
- **Check credentials**: Verify SSID and password
- **Network compatibility**: Ensure 2.4GHz network
- **Signal strength**: Check WiFi signal quality
- **Firewall**: Disable firewall temporarily for testing

#### Bluetooth Not Advertising
- **ESP32 compatibility**: Verify Bluetooth LE support
- **Power cycle**: Restart the ESP32
- **Check logs**: Look for BLE initialization errors

#### Flutter App Can't Connect
- **IP verification**: Confirm ESP32 IP address
- **Network connectivity**: Ensure same network
- **Port accessibility**: Check if port 8080 is accessible
- **Firewall settings**: Allow connections on port 8080

### Error Codes
- `BLUETOOTH_DISABLED`: Bluetooth not enabled
- `WIFI_CONNECTION_FAILED`: WiFi connection timeout
- `INVALID_MOVE`: Invalid chess move detected
- `SENSOR_ERROR`: Sensor reading failure
- `LED_ERROR`: LED control failure
- `HAPTIC_ERROR`: Haptic feedback failure

## 📊 Performance Considerations

### Memory Usage
- **Heap**: ~50KB free heap required
- **Stack**: Monitor stack usage in debug mode
- **JSON buffers**: Configurable buffer sizes in config.h

### Timing Requirements
- **Message latency**: <100ms for real-time communication
- **Sensor polling**: 50ms intervals for responsive move detection
- **LED updates**: Immediate response for visual feedback

### Power Optimization
- **Sleep modes**: Implement deep sleep for battery operation
- **WiFi power**: Use WiFi power saving modes
- **BLE advertising**: Adjustable advertising intervals

## 🔮 Future Enhancements

### Planned Features
- **Real sensor integration**: 64 magnetic sensor implementation
- **Multiplayer support**: Multiple device communication
- **Cloud synchronization**: Online game state backup
- **AI integration**: Local chess engine support
- **Battery optimization**: Power management improvements

### Extension Points
- **Custom protocols**: Easy addition of new message types
- **Hardware abstraction**: Support for different sensor types
- **Plugin system**: Modular feature additions
- **Configuration management**: Persistent settings storage

## 📄 License

This project is part of the NAOchess system. See the main project repository for license information.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📞 Support

For issues and questions:
- Check the troubleshooting section
- Review debug logs
- Open an issue in the repository
- Contact the development team

---

**Version**: 1.0.0  
**Last Updated**: 2024  
**Compatibility**: ESP32, PlatformIO, Arduino Framework