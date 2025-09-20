#ifndef MKR_BLE_H
#define MKR_BLE_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include "config.h"

class MKRBLE {
public:
    MKRBLE();
    void begin();
    void handleEvents();
    void sendMessage(String message);
    void sendStatus(String status, String message = "");
    bool isConnected();
    void setMessageCallback(void (*callback)(String));
    
private:
    static MKRBLE* instance;
    static void onCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic);
    
    void (*messageCallback)(String);
    bool deviceConnected;
    bool oldDeviceConnected;
    unsigned long lastAdvertiseTime;
    
    // BLE service and characteristics
    BLEService* chessboardService;
    BLEStringCharacteristic* messageCharacteristic;
    BLEStringCharacteristic* statusCharacteristic;
};

#endif // MKR_BLE_H
