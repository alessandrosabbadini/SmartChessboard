#!/usr/bin/env python3
"""
Test script per verificare il protocollo BLE del NAOchess Board
"""

import asyncio
import json
import time
from bleak import BleakClient, BleakScanner

# UUIDs del dispositivo
SERVICE_UUID = "12345678-1234-1234-1234-123456789ABC"
CHARACTERISTIC_UUID = "87654321-4321-4321-4321-CBA987654321"
DEVICE_NAME = "NAOchess Board"

class ChessboardTester:
    def __init__(self):
        self.client = None
        self.characteristic = None
        
    async def scan_and_connect(self):
        """Scansiona e connetti al dispositivo"""
        print("Scansionando dispositivi BLE...")
        devices = await BleakScanner.discover(timeout=10.0)
        
        target_device = None
        for device in devices:
            if device.name == DEVICE_NAME:
                target_device = device
                break
                
        if not target_device:
            print(f"Dispositivo '{DEVICE_NAME}' non trovato!")
            return False
            
        print(f"Trovato: {target_device.name} ({target_device.address})")
        
        # Connetti al dispositivo
        self.client = BleakClient(target_device.address)
        await self.client.connect()
        print("Connesso al dispositivo!")
        
        # Trova la caratteristica
        services = await self.client.get_services()
        for service in services:
            if str(service.uuid).upper() == SERVICE_UUID.upper():
                for char in service.characteristics:
                    if str(char.uuid).upper() == CHARACTERISTIC_UUID.upper():
                        self.characteristic = char
                        print(f"Caratteristica trovata: {char.uuid}")
                        break
                        
        if not self.characteristic:
            print("Caratteristica non trovata!")
            return False
            
        return True
        
    async def send_message(self, message_type, data):
        """Invia un messaggio al dispositivo"""
        message = {
            "type": message_type,
            "id": str(int(time.time())),
            "timestamp": int(time.time() * 1000),
            "data": data
        }
        
        message_json = json.dumps(message)
        print(f"Inviando: {message_json}")
        
        await self.client.write_gatt_char(
            self.characteristic, 
            message_json.encode('utf-8')
        )
        
    async def listen_for_messages(self, duration=10):
        """Ascolta messaggi dal dispositivo"""
        print(f"Ascoltando messaggi per {duration} secondi...")
        
        def notification_handler(sender, data):
            message = data.decode('utf-8')
            print(f"Ricevuto: {message}")
            
        await self.client.start_notify(self.characteristic, notification_handler)
        await asyncio.sleep(duration)
        await self.client.stop_notify(self.characteristic)
        
    async def test_ping(self):
        """Test del messaggio PING"""
        print("\n=== Test PING ===")
        await self.send_message("PING", {"message": "Hello from Python test"})
        await self.listen_for_messages(3)
        
    async def test_wifi_config(self):
        """Test della configurazione WiFi"""
        print("\n=== Test WiFi Config ===")
        await self.send_message("WIFI_CONFIG", {
            "ssid": "TestNetwork",
            "password": "TestPassword"
        })
        await self.listen_for_messages(5)
        
    async def test_led_control(self):
        """Test del controllo LED"""
        print("\n=== Test LED Control ===")
        await self.send_message("LED_CONTROL", {
            "pattern": "blink",
            "squares": ["e4", "d4"],
            "color": "red",
            "duration": 1000,
            "intensity": 255
        })
        await self.listen_for_messages(3)
        
    async def run_tests(self):
        """Esegue tutti i test"""
        if not await self.scan_and_connect():
            return
            
        try:
            # Aspetta il messaggio DEVICE_INFO automatico
            print("\nAspettando messaggio DEVICE_INFO...")
            await self.listen_for_messages(5)
            
            # Esegui i test
            await self.test_ping()
            await self.test_wifi_config()
            await self.test_led_control()
            
        finally:
            if self.client:
                await self.client.disconnect()
                print("Disconnesso dal dispositivo")

async def main():
    tester = ChessboardTester()
    await tester.run_tests()

if __name__ == "__main__":
    print("Test BLE Protocol per NAOchess Board")
    print("Assicurati che il dispositivo sia acceso e in modalità advertising")
    print("=" * 50)
    
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nTest interrotto dall'utente")
    except Exception as e:
        print(f"Errore: {e}")
