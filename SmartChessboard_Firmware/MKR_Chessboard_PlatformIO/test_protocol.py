#!/usr/bin/env python3
"""
NAOchess ESP32 Chessboard Protocol Test Script

This script tests the communication protocol with the ESP32 chessboard.
It can test both WiFi and Bluetooth connections and verify message handling.

Usage:
    python test_protocol.py --ip 192.168.1.100
    python test_protocol.py --bluetooth
"""

import json
import time
import requests
import argparse
import sys
from datetime import datetime

class ChessboardTester:
    def __init__(self, ip=None, port=8080):
        self.ip = ip
        self.port = port
        self.base_url = f"http://{ip}:{port}" if ip else None
        self.message_id = 0

    def generate_message_id(self):
        self.message_id += 1
        return f"test_{self.message_id}_{int(time.time())}"

    def send_message(self, message_type, data):
        """Send a message to the ESP32"""
        if not self.base_url:
            print("No IP address configured")
            return False

        message = {
            "type": message_type,
            "id": self.generate_message_id(),
            "data": data,
            "timestamp": int(time.time() * 1000)
        }

        try:
            response = requests.post(
                f"{self.base_url}/message",
                json=message,
                headers={'Content-Type': 'application/json'},
                timeout=5
            )
            
            if response.status_code == 200:
                print(f"✓ {message_type} message sent successfully")
                return True
            else:
                print(f"✗ {message_type} message failed: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"✗ {message_type} message failed: {e}")
            return False

    def test_ping(self):
        """Test ping/pong functionality"""
        print("\n=== Testing Ping/Pong ===")
        try:
            response = requests.get(f"{self.base_url}/ping", timeout=5)
            if response.status_code == 200:
                print("✓ Ping successful")
                return True
            else:
                print(f"✗ Ping failed: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"✗ Ping failed: {e}")
            return False

    def test_wifi_config(self):
        """Test WiFi configuration"""
        print("\n=== Testing WiFi Configuration ===")
        data = {
            "ssid": "TestNetwork",
            "password": "testpassword",
            "securityType": "WPA2"
        }
        return self.send_message("WIFI_CONFIG", data)

    def test_led_control(self):
        """Test LED control"""
        print("\n=== Testing LED Control ===")
        
        # Test move highlight
        data = {
            "pattern": "MOVE_HIGHLIGHT",
            "squares": ["e2", "e4"],
            "color": "blue",
            "duration": 2000,
            "intensity": 100
        }
        success1 = self.send_message("LED_CONTROL", data)
        
        time.sleep(1)
        
        # Test check pattern
        data = {
            "pattern": "CHECK",
            "color": "red",
            "duration": 500,
            "intensity": 80
        }
        success2 = self.send_message("LED_CONTROL", data)
        
        return success1 and success2

    def test_haptic_feedback(self):
        """Test haptic feedback"""
        print("\n=== Testing Haptic Feedback ===")
        
        # Test move feedback
        data = {
            "pattern": "MOVE",
            "duration": 100,
            "intensity": 50
        }
        success1 = self.send_message("HAPTIC_FEEDBACK", data)
        
        time.sleep(1)
        
        # Test capture feedback
        data = {
            "pattern": "CAPTURE",
            "duration": 150,
            "intensity": 75
        }
        success2 = self.send_message("HAPTIC_FEEDBACK", data)
        
        return success1 and success2

    def test_game_state(self):
        """Test game state updates"""
        print("\n=== Testing Game State ===")
        
        data = {
            "fen": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            "currentPlayer": "Black",
            "isCheck": False,
            "isCheckmate": False,
            "isStalemate": False,
            "lastMove": "e2e4"
        }
        return self.send_message("GAME_STATE", data)

    def test_move_detection(self):
        """Test move detection (simulated)"""
        print("\n=== Testing Move Detection ===")
        
        data = {
            "fromSquare": "e2",
            "toSquare": "e4",
            "pieceType": "pawn",
            "capturedPiece": None,
            "isPromotion": False,
            "promotionPiece": None
        }
        return self.send_message("MOVE_DETECTED", data)

    def test_device_info(self):
        """Test device information request"""
        print("\n=== Testing Device Info ===")
        
        data = {
            "request": "DEVICE_INFO"
        }
        return self.send_message("DEVICE_INFO", data)

    def test_setup_status(self):
        """Test setup status messages"""
        print("\n=== Testing Setup Status ===")
        
        data = {
            "status": "SCANNING",
            "message": "Looking for chessboard..."
        }
        return self.send_message("SETUP_STATUS", data)

    def get_status(self):
        """Get system status"""
        print("\n=== Getting System Status ===")
        try:
            response = requests.get(f"{self.base_url}/status", timeout=5)
            if response.status_code == 200:
                print("✓ Status retrieved successfully")
                print("Status:")
                print(response.text)
                return True
            else:
                print(f"✗ Status request failed: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"✗ Status request failed: {e}")
            return False

    def get_game_state(self):
        """Get current game state"""
        print("\n=== Getting Game State ===")
        try:
            response = requests.get(f"{self.base_url}/game", timeout=5)
            if response.status_code == 200:
                print("✓ Game state retrieved successfully")
                game_state = response.json()
                print("Game State:")
                print(json.dumps(game_state, indent=2))
                return True
            else:
                print(f"✗ Game state request failed: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"✗ Game state request failed: {e}")
            return False

    def run_all_tests(self):
        """Run all protocol tests"""
        print("NAOchess ESP32 Chessboard Protocol Test")
        print("=" * 50)
        print(f"Testing connection to: {self.base_url}")
        print(f"Test started at: {datetime.now()}")
        
        if not self.base_url:
            print("Error: No IP address provided")
            return False

        # Test basic connectivity
        if not self.test_ping():
            print("\n❌ Basic connectivity test failed. Check IP address and connection.")
            return False

        # Run all tests
        tests = [
            ("WiFi Configuration", self.test_wifi_config),
            ("LED Control", self.test_led_control),
            ("Haptic Feedback", self.test_haptic_feedback),
            ("Game State", self.test_game_state),
            ("Move Detection", self.test_move_detection),
            ("Device Info", self.test_device_info),
            ("Setup Status", self.test_setup_status),
            ("System Status", self.get_status),
            ("Game State Query", self.get_game_state),
        ]

        results = []
        for test_name, test_func in tests:
            try:
                result = test_func()
                results.append((test_name, result))
                time.sleep(1)  # Small delay between tests
            except Exception as e:
                print(f"✗ {test_name} test failed with exception: {e}")
                results.append((test_name, False))

        # Print summary
        print("\n" + "=" * 50)
        print("TEST SUMMARY")
        print("=" * 50)
        
        passed = 0
        total = len(results)
        
        for test_name, result in results:
            status = "✓ PASS" if result else "✗ FAIL"
            print(f"{test_name:20} {status}")
            if result:
                passed += 1

        print(f"\nResults: {passed}/{total} tests passed")
        
        if passed == total:
            print("🎉 All tests passed! Protocol is working correctly.")
            return True
        else:
            print("⚠️  Some tests failed. Check ESP32 logs for details.")
            return False

def main():
    parser = argparse.ArgumentParser(description='Test NAOchess ESP32 Chessboard Protocol')
    parser.add_argument('--ip', type=str, help='ESP32 IP address')
    parser.add_argument('--port', type=int, default=8080, help='ESP32 port (default: 8080)')
    parser.add_argument('--bluetooth', action='store_true', help='Test Bluetooth connection (not implemented)')
    
    args = parser.parse_args()
    
    if not args.ip and not args.bluetooth:
        print("Error: Please provide either --ip or --bluetooth option")
        parser.print_help()
        sys.exit(1)
    
    if args.bluetooth:
        print("Bluetooth testing not implemented in this script.")
        print("Use a BLE scanner or the Flutter app for Bluetooth testing.")
        sys.exit(1)
    
    tester = ChessboardTester(args.ip, args.port)
    success = tester.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()

