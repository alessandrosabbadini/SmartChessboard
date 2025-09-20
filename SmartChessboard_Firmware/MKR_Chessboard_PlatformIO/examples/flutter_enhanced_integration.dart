// Enhanced Flutter Integration Example for NAOchess ESP32 Chessboard
// This file demonstrates the new automatic setup flow and enhanced protocol

import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:connectivity_plus/connectivity_plus.dart';

class EnhancedChessboardController {
  String? esp32IP;
  int port = 8080;
  BluetoothConnection? bluetoothConnection;
  bool isConnected = false;
  String setupStatus = "IDLE";
  String? deviceInfo;

  // Enhanced setup flow with automatic WiFi detection
  Future<bool> performCompleteSetup() async {
    try {
      // Step 1: Enable Bluetooth and scan for devices
      await requestBluetoothPermissions();
      await initializeBluetooth();
      
      // Step 2: Scan for NAOchess Board
      updateSetupStatus("SCANNING", "Scanning for chessboard...");
      final devices = await scanForDevices();
      
      if (devices.isEmpty) {
        updateSetupStatus("FAILED", "No chessboard found. Please ensure it's powered on.");
        return false;
      }
      
      // Step 3: Connect to the first NAOchess Board found
      updateSetupStatus("CONNECTING", "Connecting to chessboard...");
      final connected = await connectBluetooth(devices.first.id);
      
      if (!connected) {
        updateSetupStatus("FAILED", "Failed to connect to chessboard");
        return false;
      }
      
      // Step 4: Get current WiFi information
      updateSetupStatus("CONFIGURING_WIFI", "Detecting WiFi network...");
      final wifiInfo = await getCurrentWiFiInfo();
      
      if (wifiInfo == null) {
        updateSetupStatus("FAILED", "Could not detect WiFi network");
        return false;
      }
      
      // Step 5: Configure WiFi on chessboard
      updateSetupStatus("CONFIGURING_WIFI", "Configuring WiFi connection...");
      final wifiConfigured = await configureWiFi(
        wifiInfo['ssid'], 
        wifiInfo['password'], 
        securityType: wifiInfo['securityType']
      );
      
      if (!wifiConfigured) {
        updateSetupStatus("FAILED", "WiFi configuration failed");
        return false;
      }
      
      // Step 6: Wait for WiFi connection and get IP
      updateSetupStatus("CONFIGURING_WIFI", "Waiting for WiFi connection...");
      final ipReceived = await waitForWiFiConnection();
      
      if (ipReceived == null) {
        updateSetupStatus("FAILED", "WiFi connection timeout");
        return false;
      }
      
      // Step 7: Switch to WiFi TCP connection
      updateSetupStatus("COMPLETED", "Setup completed successfully!");
      final tcpConnected = await connectWiFi(ipReceived);
      
      return tcpConnected;
      
    } catch (e) {
      updateSetupStatus("FAILED", "Setup failed: $e");
      return false;
    }
  }

  // Request Bluetooth permissions
  Future<void> requestBluetoothPermissions() async {
    // Implementation depends on your permission handling
    // This is a placeholder for the actual permission request
    print("Requesting Bluetooth permissions...");
  }

  // Initialize Bluetooth
  Future<void> initializeBluetooth() async {
    // Initialize Bluetooth adapter
    print("Initializing Bluetooth...");
  }

  // Scan for NAOchess Board devices
  Future<List<BluetoothDevice>> scanForDevices() async {
    try {
      List<BluetoothDevice> devices = await FlutterBluetoothSerial.instance.getBondedDevices();
      List<BluetoothDevice> chessboardDevices = [];
      
      for (BluetoothDevice device in devices) {
        if (device.name == 'NAOchess Board') {
          chessboardDevices.add(device);
        }
      }
      
      return chessboardDevices;
    } catch (e) {
      print('Error scanning for devices: $e');
      return [];
    }
  }

  // Connect to chessboard via Bluetooth
  Future<bool> connectBluetooth(String deviceId) async {
    try {
      bluetoothConnection = await BluetoothConnection.toAddress(deviceId);
      isConnected = true;
      print('Connected to chessboard via Bluetooth');
      return true;
    } catch (e) {
      print('Bluetooth connection failed: $e');
      return false;
    }
  }

  // Get current WiFi information from the phone
  Future<Map<String, String>?> getCurrentWiFiInfo() async {
    try {
      // This would use platform-specific code to get WiFi info
      // For now, return a mock implementation
      return {
        'ssid': 'MyWiFiNetwork',
        'password': 'mypassword',
        'securityType': 'WPA2'
      };
    } catch (e) {
      print('Error getting WiFi info: $e');
      return null;
    }
  }

  // Configure WiFi on the chessboard
  Future<bool> configureWiFi(String ssid, String password, {String securityType = 'WPA2'}) async {
    Map<String, dynamic> configMessage = {
      'type': 'WIFI_CONFIG',
      'id': 'config_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'ssid': ssid,
        'password': password,
        'securityType': securityType
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendBluetoothMessage(configMessage);
  }

  // Wait for WiFi connection confirmation
  Future<String?> waitForWiFiConnection() async {
    // Listen for WiFi status messages for up to 30 seconds
    int attempts = 0;
    while (attempts < 60) { // 30 seconds with 500ms intervals
      await Future.delayed(Duration(milliseconds: 500));
      
      // Check if we received a WiFi status message with IP
      if (esp32IP != null) {
        return esp32IP;
      }
      
      attempts++;
    }
    
    return null;
  }

  // Connect to ESP32 via WiFi
  Future<bool> connectWiFi(String ip) async {
    try {
      esp32IP = ip;
      // Test connection with a ping
      final response = await http.get(
        Uri.parse('http://$ip:$port/ping'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(Duration(seconds: 5));
      
      if (response.statusCode == 200) {
        isConnected = true;
        print('Connected to ESP32 at $ip');
        return true;
      }
    } catch (e) {
      print('WiFi connection failed: $e');
    }
    return false;
  }

  // Send message via Bluetooth
  Future<bool> sendBluetoothMessage(Map<String, dynamic> message) async {
    if (bluetoothConnection == null) {
      print('No Bluetooth connection');
      return false;
    }

    try {
      String jsonMessage = jsonEncode(message);
      bluetoothConnection!.output.add(utf8.encode(jsonMessage + '\n'));
      await bluetoothConnection!.output.allSent;
      return true;
    } catch (e) {
      print('Failed to send Bluetooth message: $e');
      return false;
    }
  }

  // Send a message to the ESP32 (WiFi or Bluetooth)
  Future<bool> sendMessage(Map<String, dynamic> message) async {
    if (!isConnected) {
      print('Not connected to ESP32');
      return false;
    }

    try {
      String jsonMessage = jsonEncode(message);
      
      if (esp32IP != null) {
        // Send via WiFi
        final response = await http.post(
          Uri.parse('http://$esp32IP:$port/message'),
          headers: {'Content-Type': 'application/json'},
          body: jsonMessage,
        ).timeout(Duration(seconds: 5));
        
        return response.statusCode == 200;
      } else if (bluetoothConnection != null) {
        // Send via Bluetooth
        return await sendBluetoothMessage(message);
      }
    } catch (e) {
      print('Failed to send message: $e');
    }
    return false;
  }

  // Send LED control command
  Future<bool> controlLEDs({
    required String pattern,
    required List<String> squares,
    required String color,
    required int duration,
    required int intensity,
  }) async {
    Map<String, dynamic> ledMessage = {
      'type': 'LED_CONTROL',
      'id': 'led_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'pattern': pattern,
        'squares': squares,
        'color': color,
        'duration': duration,
        'intensity': intensity
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendMessage(ledMessage);
  }

  // Send haptic feedback
  Future<bool> sendHapticFeedback({
    required String pattern,
    required int duration,
    required int intensity,
  }) async {
    Map<String, dynamic> hapticMessage = {
      'type': 'HAPTIC_FEEDBACK',
      'id': 'haptic_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'pattern': pattern,
        'duration': duration,
        'intensity': intensity
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendMessage(hapticMessage);
  }

  // Update game state
  Future<bool> updateGameState({
    required String fen,
    required String currentPlayer,
    required bool isCheck,
    required bool isCheckmate,
    required bool isStalemate,
    String? lastMove,
  }) async {
    Map<String, dynamic> gameStateMessage = {
      'type': 'GAME_STATE',
      'id': 'state_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'fen': fen,
        'currentPlayer': currentPlayer,
        'isCheck': isCheck,
        'isCheckmate': isCheckmate,
        'isStalemate': isStalemate,
        if (lastMove != null) 'lastMove': lastMove,
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendMessage(gameStateMessage);
  }

  // Send move detection
  Future<bool> sendMoveDetected({
    required String fromSquare,
    required String toSquare,
    required String pieceType,
    String? capturedPiece,
    bool isPromotion = false,
    String? promotionPiece,
  }) async {
    Map<String, dynamic> moveMessage = {
      'type': 'MOVE_DETECTED',
      'id': 'move_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'fromSquare': fromSquare,
        'toSquare': toSquare,
        'pieceType': pieceType,
        'capturedPiece': capturedPiece,
        'isPromotion': isPromotion,
        'promotionPiece': promotionPiece,
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendMessage(moveMessage);
  }

  // Update setup status
  void updateSetupStatus(String status, String message) {
    setupStatus = status;
    print('Setup Status: $status - $message');
    // Here you would notify the UI about the status change
  }

  // Disconnect from ESP32
  Future<void> disconnect() async {
    if (bluetoothConnection != null) {
      await bluetoothConnection!.close();
      bluetoothConnection = null;
    }
    esp32IP = null;
    isConnected = false;
    setupStatus = "IDLE";
    print('Disconnected from ESP32');
  }
}

// Enhanced Flutter widget with automatic setup
class EnhancedChessboardWidget extends StatefulWidget {
  @override
  _EnhancedChessboardWidgetState createState() => _EnhancedChessboardWidgetState();
}

class _EnhancedChessboardWidgetState extends State<EnhancedChessboardWidget> {
  final EnhancedChessboardController _controller = EnhancedChessboardController();
  String _connectionStatus = 'Disconnected';
  String _setupStatus = 'IDLE';
  String _setupMessage = '';
  String _gameState = '';

  @override
  void initState() {
    super.initState();
    _startAutomaticSetup();
  }

  Future<void> _startAutomaticSetup() async {
    setState(() {
      _setupStatus = 'STARTING';
      _setupMessage = 'Starting automatic setup...';
    });

    bool success = await _controller.performCompleteSetup();
    
    setState(() {
      _connectionStatus = success ? 'Connected' : 'Disconnected';
      _setupStatus = success ? 'COMPLETED' : 'FAILED';
      _setupMessage = success ? 'Setup completed successfully!' : 'Setup failed';
    });
  }

  Future<void> _highlightMove(String from, String to) async {
    await _controller.controlLEDs(
      pattern: 'MOVE_HIGHLIGHT',
      squares: [from, to],
      color: 'blue',
      duration: 2000,
      intensity: 100,
    );
  }

  Future<void> _sendHapticFeedback(String pattern) async {
    await _controller.sendHapticFeedback(
      pattern: pattern,
      duration: 100,
      intensity: 50,
    );
  }

  Future<void> _sendMove(String from, String to) async {
    await _controller.sendMoveDetected(
      fromSquare: from,
      toSquare: to,
      pieceType: 'pawn',
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('NAOchess Enhanced Controller'),
      ),
      body: Padding(
        padding: EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Setup Status
            Card(
              child: Padding(
                padding: EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Setup Status',
                      style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    SizedBox(height: 8),
                    Text('Status: $_setupStatus'),
                    Text('Message: $_setupMessage'),
                    Text('Connection: $_connectionStatus'),
                  ],
                ),
              ),
            ),
            
            SizedBox(height: 20),
            
            // Control Buttons
            if (_connectionStatus == 'Connected') ...[
              ElevatedButton(
                onPressed: _startAutomaticSetup,
                child: Text('Restart Setup'),
              ),
              SizedBox(height: 10),
              ElevatedButton(
                onPressed: () => _highlightMove('e2', 'e4'),
                child: Text('Highlight Move e2-e4'),
              ),
              SizedBox(height: 10),
              ElevatedButton(
                onPressed: () => _sendHapticFeedback('MOVE'),
                child: Text('Send Haptic Feedback'),
              ),
              SizedBox(height: 10),
              ElevatedButton(
                onPressed: () => _sendMove('e2', 'e4'),
                child: Text('Send Move e2-e4'),
              ),
            ] else ...[
              ElevatedButton(
                onPressed: _startAutomaticSetup,
                child: Text('Start Setup'),
              ),
            ],
            
            SizedBox(height: 20),
            
            // Game State Display
            Expanded(
              child: SingleChildScrollView(
                child: Text(
                  _gameState.isEmpty ? 'No game state data' : _gameState,
                  style: TextStyle(fontFamily: 'monospace'),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _controller.disconnect();
    super.dispose();
  }
}
