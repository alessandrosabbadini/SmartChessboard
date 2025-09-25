// Flutter Integration Example for NAOchess ESP32 Chessboard
// This file demonstrates how to integrate with the ESP32 chessboard from a Flutter app

import 'dart:convert';
import 'dart:io';
import 'package:http/http.dart' as http;
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';

class ChessboardController {
  String? esp32IP;
  int port = 8080;
  BluetoothConnection? bluetoothConnection;
  bool isConnected = false;

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

  // Connect to ESP32 via Bluetooth LE
  Future<bool> connectBluetooth() async {
    try {
      // Scan for devices
      List<BluetoothDevice> devices = await FlutterBluetoothSerial.instance.getBondedDevices();
      BluetoothDevice? esp32Device;
      
      for (BluetoothDevice device in devices) {
        if (device.name == 'NAOchess Board') {
          esp32Device = device;
          break;
        }
      }
      
      if (esp32Device == null) {
        print('NAOchess Board not found in paired devices');
        return false;
      }
      
      // Connect to device
      bluetoothConnection = await BluetoothConnection.toAddress(esp32Device.address);
      isConnected = true;
      print('Connected to ESP32 via Bluetooth');
      return true;
    } catch (e) {
      print('Bluetooth connection failed: $e');
    }
    return false;
  }

  // Send a message to the ESP32
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
        bluetoothConnection!.output.add(utf8.encode(jsonMessage + '\n'));
        await bluetoothConnection!.output.allSent;
        return true;
      }
    } catch (e) {
      print('Failed to send message: $e');
    }
    return false;
  }

  // Configure WiFi on ESP32
  Future<bool> configureWiFi(String ssid, String password) async {
    Map<String, dynamic> configMessage = {
      'type': 'WIFI_CONFIG',
      'id': 'config_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'ssid': ssid,
        'password': password,
        'securityType': 'WPA2'
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendMessage(configMessage);
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

  // Send ping to check connection
  Future<bool> ping() async {
    Map<String, dynamic> pingMessage = {
      'type': 'PING',
      'id': 'ping_${DateTime.now().millisecondsSinceEpoch}',
      'data': {
        'timestamp': DateTime.now().millisecondsSinceEpoch
      },
      'timestamp': DateTime.now().millisecondsSinceEpoch
    };
    
    return await sendMessage(pingMessage);
  }

  // Get current game state
  Future<Map<String, dynamic>?> getGameState() async {
    if (esp32IP == null) {
      print('WiFi connection required for game state query');
      return null;
    }

    try {
      final response = await http.get(
        Uri.parse('http://$esp32IP:$port/game'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(Duration(seconds: 5));
      
      if (response.statusCode == 200) {
        return jsonDecode(response.body);
      }
    } catch (e) {
      print('Failed to get game state: $e');
    }
    return null;
  }

  // Disconnect from ESP32
  Future<void> disconnect() async {
    if (bluetoothConnection != null) {
      await bluetoothConnection!.close();
      bluetoothConnection = null;
    }
    esp32IP = null;
    isConnected = false;
    print('Disconnected from ESP32');
  }
}

// Example usage in a Flutter widget
class ChessboardWidget extends StatefulWidget {
  @override
  _ChessboardWidgetState createState() => _ChessboardWidgetState();
}

class _ChessboardWidgetState extends State<ChessboardWidget> {
  final ChessboardController _controller = ChessboardController();
  String _connectionStatus = 'Disconnected';
  String _gameState = '';

  @override
  void initState() {
    super.initState();
    _connectToChessboard();
  }

  Future<void> _connectToChessboard() async {
    // Try WiFi connection first
    bool connected = await _controller.connectWiFi('192.168.1.100');
    
    if (!connected) {
      // Fallback to Bluetooth
      connected = await _controller.connectBluetooth();
    }
    
    setState(() {
      _connectionStatus = connected ? 'Connected' : 'Disconnected';
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

  Future<void> _updateGameState() async {
    await _controller.updateGameState(
      fen: 'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1',
      currentPlayer: 'Black',
      isCheck: false,
      isCheckmate: false,
      isStalemate: false,
      lastMove: 'e2e4',
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('NAOchess Controller'),
      ),
      body: Padding(
        padding: EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text(
              'Connection Status: $_connectionStatus',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 20),
            ElevatedButton(
              onPressed: _connectToChessboard,
              child: Text('Reconnect'),
            ),
            SizedBox(height: 20),
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
              onPressed: _updateGameState,
              child: Text('Update Game State'),
            ),
            SizedBox(height: 10),
            ElevatedButton(
              onPressed: () async {
                Map<String, dynamic>? state = await _controller.getGameState();
                if (state != null) {
                  setState(() {
                    _gameState = state.toString();
                  });
                }
              },
              child: Text('Get Game State'),
            ),
            SizedBox(height: 20),
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

