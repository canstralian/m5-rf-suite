/**
 * @file rf24_module.h
 * @brief 2.4 GHz RF module for ESP-NOW, Wi-Fi, and BLE operations
 * 
 * This module handles all 2.4 GHz operations including:
 * - ESP-NOW peer-to-peer communication
 * - Wi-Fi scanning and analysis
 * - BLE device scanning and interaction
 */

#ifndef RF24_MODULE_H
#define RF24_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "config.h"

// ============================================================================
// WIFI STRUCTURES
// ============================================================================

/**
 * @brief Wi-Fi network information (fixed-size, no dynamic allocation)
 * 
 * MEMORY OWNERSHIP: Self-contained, no heap allocations
 * Copy/move semantics: Default (shallow copy sufficient)
 * Safe to store in std::vector and pass by value
 */
struct WiFiNetworkInfo {
    char ssid[33];
    uint8_t bssid[6];
    int8_t rssi;
    uint8_t channel;
    wifi_auth_mode_t encryptionType;
    bool isHidden;
};

// ============================================================================
// ESP-NOW STRUCTURES
// ============================================================================

/**
 * @brief ESP-NOW received message (fixed-size, no dynamic allocation)
 * 
 * MEMORY OWNERSHIP: Self-contained, no heap allocations
 * Copy/move semantics: Default (shallow copy sufficient)
 * Safe to store in std::vector and pass by value
 */
struct ESPNowMessage {
    uint8_t senderId[6];
    uint8_t messageType;
    uint32_t timestamp;
    uint8_t data[200];
    size_t dataLen;
};

// ============================================================================
// BLE STRUCTURES
// ============================================================================

/**
 * @brief BLE device information (fixed-size, no dynamic allocation)
 * 
 * MEMORY OWNERSHIP: Self-contained, no heap allocations
 * Copy/move semantics: Default (shallow copy sufficient)
 * Safe to store in std::vector and pass by value
 */
struct BLEDeviceInfo {
    char name[64];
    char address[18];
    int8_t rssi;
    uint16_t appearance;
    bool hasName;
};

// ============================================================================
// RF24 MODULE CLASS
// ============================================================================

class RF24Module {
public:
    RF24Module();
    ~RF24Module();
    
    // Initialization
    bool begin();
    void end();
    
    // ========================================================================
    // ESP-NOW Functions
    // ========================================================================
    bool initESPNow();
    void stopESPNow();
    bool addPeer(const uint8_t* peerAddress, uint8_t channel = ESPNOW_CHANNEL);
    bool removePeer(const uint8_t* peerAddress);
    bool sendMessage(const uint8_t* peerAddress, const uint8_t* data, size_t len);
    bool broadcastMessage(const uint8_t* data, size_t len);
    bool hasReceivedMessage();
    ESPNowMessage getReceivedMessage();
    int getPeerCount();
    
    // ========================================================================
    // Wi-Fi Scanner Functions
    // ========================================================================
    bool startWiFiScan(bool async = true);
    bool isWiFiScanComplete();
    int getWiFiNetworkCount();
    WiFiNetworkInfo getWiFiNetwork(int index);
    String getEncryptionTypeName(wifi_auth_mode_t type);
    void sortNetworksByRSSI();
    
    // ========================================================================
    // BLE Scanner Functions
    // ========================================================================
    bool initBLE(const char* deviceName = "M5-RF-Suite");
    void stopBLE();
    bool startBLEScan(int duration = BLE_SCAN_TIME);
    bool isBLEScanComplete();
    int getBLEDeviceCount();
    BLEDeviceInfo getBLEDevice(int index);
    void clearBLEResults();
    
    // ========================================================================
    // General Functions
    // ========================================================================
    void getLocalMAC(uint8_t* mac);
    String getMACString(const uint8_t* mac);
    int getCurrentChannel();
    void setChannel(int channel);
    
private:
    // ESP-NOW state
    bool espNowInitialized;
    // MEMORY OWNERSHIP: Vector owns all ESPNowMessage objects
    // Messages contain fixed-size data array (no dynamic allocation)
    std::vector<ESPNowMessage> receivedMessages;
    int peerCount;
    
    // Wi-Fi state
    bool wifiScanInProgress;
    // MEMORY OWNERSHIP: Vector owns all WiFiNetworkInfo objects
    // Network info contains fixed-size data (no dynamic allocation)
    std::vector<WiFiNetworkInfo> wifiNetworks;
    
    // BLE state
    bool bleInitialized;
    BLEScan* pBLEScan;  // Managed by BLEDevice library (not owned by us)
    // MEMORY OWNERSHIP: Vector owns all BLEDeviceInfo objects
    // Device info contains fixed-size data (no dynamic allocation)
    std::vector<BLEDeviceInfo> bleDevices;
    
    // Callbacks
    static void onESPNowDataReceived(const uint8_t* mac, const uint8_t* data, int len);
    static void onESPNowDataSent(const uint8_t* mac, esp_now_send_status_t status);
    
    // Helper functions
    void parseWiFiScanResults();
};

// ============================================================================
// BLE SCAN CALLBACK CLASS
// ============================================================================

class BLEScanCallback : public BLEAdvertisedDeviceCallbacks {
public:
    BLEScanCallback(std::vector<BLEDeviceInfo>* devices);
    void onResult(BLEAdvertisedDevice advertisedDevice);
    
private:
    std::vector<BLEDeviceInfo>* deviceList;
};

#endif // RF24_MODULE_H
