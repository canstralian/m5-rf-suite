/**
 * @file rf24_module.cpp
 * @brief Implementation of 2.4 GHz RF module (ESP-NOW, Wi-Fi, BLE)
 */

#include "rf24_module.h"
#include <algorithm>

// Static callback data
static RF24Module* g_instance = nullptr;

RF24Module::RF24Module() {
    espNowInitialized = false;
    wifiScanInProgress = false;
    bleInitialized = false;
    pBLEScan = nullptr;
    peerCount = 0;
    g_instance = this;
}

RF24Module::~RF24Module() {
    end();
}

bool RF24Module::begin() {
    // Initialize Wi-Fi in station mode for ESP-NOW and scanning
    WiFi.mode(WIFI_STA);
    
#if ENABLE_SERIAL_LOGGING
    Serial.println("[RF24] Module initialized");
    uint8_t mac[6];
    getLocalMAC(mac);
    Serial.printf("[RF24] MAC: %s\n", getMACString(mac).c_str());
#endif
    
    return true;
}

void RF24Module::end() {
    stopESPNow();
    stopBLE();
}

// ============================================================================
// ESP-NOW Functions
// ============================================================================

bool RF24Module::initESPNow() {
    if (espNowInitialized) {
        return true;
    }
    
    if (esp_now_init() != ESP_OK) {
#if ENABLE_SERIAL_LOGGING
        Serial.println("[RF24] ESP-NOW init failed");
#endif
        return false;
    }
    
    // Register callbacks
    esp_now_register_recv_cb(onESPNowDataReceived);
    esp_now_register_send_cb(onESPNowDataSent);
    
    espNowInitialized = true;
    
#if ENABLE_SERIAL_LOGGING
    Serial.println("[RF24] ESP-NOW initialized");
#endif
    
    return true;
}

void RF24Module::stopESPNow() {
    if (espNowInitialized) {
        esp_now_deinit();
        espNowInitialized = false;
        receivedMessages.clear();
        peerCount = 0;
    }
}

bool RF24Module::addPeer(const uint8_t* peerAddress, uint8_t channel) {
    if (!espNowInitialized) {
        return false;
    }
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = channel;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
#if ENABLE_SERIAL_LOGGING
        Serial.println("[RF24] Failed to add ESP-NOW peer");
#endif
        return false;
    }
    
    peerCount++;
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF24] Added ESP-NOW peer: %s\n", getMACString(peerAddress).c_str());
#endif
    
    return true;
}

bool RF24Module::removePeer(const uint8_t* peerAddress) {
    if (!espNowInitialized) {
        return false;
    }
    
    if (esp_now_del_peer(peerAddress) == ESP_OK) {
        peerCount--;
        return true;
    }
    
    return false;
}

bool RF24Module::sendMessage(const uint8_t* peerAddress, const uint8_t* data, size_t len) {
    if (!espNowInitialized) {
        return false;
    }
    
    if (len > 250) {
#if ENABLE_SERIAL_LOGGING
        Serial.println("[RF24] Message too large for ESP-NOW");
#endif
        return false;
    }
    
    esp_err_t result = esp_now_send(peerAddress, data, len);
    
#if ENABLE_SERIAL_LOGGING
    if (result == ESP_OK) {
        Serial.printf("[RF24] Sent ESP-NOW message (%d bytes)\n", len);
    } else {
        Serial.println("[RF24] ESP-NOW send failed");
    }
#endif
    
    return result == ESP_OK;
}

bool RF24Module::broadcastMessage(const uint8_t* data, size_t len) {
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return sendMessage(broadcastAddress, data, len);
}

bool RF24Module::hasReceivedMessage() {
    return !receivedMessages.empty();
}

ESPNowMessage RF24Module::getReceivedMessage() {
    ESPNowMessage msg = {};
    
    if (!receivedMessages.empty()) {
        msg = receivedMessages.front();
        receivedMessages.erase(receivedMessages.begin());
    }
    
    return msg;
}

int RF24Module::getPeerCount() {
    return peerCount;
}

void RF24Module::onESPNowDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
    if (g_instance == nullptr) return;
    
    // MEMORY OWNERSHIP: Create message on stack (no dynamic allocation in ESPNowMessage)
    ESPNowMessage msg;
    memcpy(msg.senderId, mac, 6);
    msg.timestamp = millis();
    msg.dataLen = std::min((size_t)len, sizeof(msg.data));
    memcpy(msg.data, data, msg.dataLen);
    
    if (len > 0) {
        msg.messageType = data[0];
    } else {
        msg.messageType = 0;
    }
    
    // MEMORY OWNERSHIP: Copy message into vector (vector takes ownership of copy)
    // ESPNowMessage has no dynamic allocations, so copy is safe and efficient
    g_instance->receivedMessages.push_back(msg);
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF24] Received ESP-NOW message from %s (%d bytes)\n",
                  g_instance->getMACString(mac).c_str(), len);
#endif
}

void RF24Module::onESPNowDataSent(const uint8_t* mac, esp_now_send_status_t status) {
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF24] ESP-NOW send to %s: %s\n",
                  g_instance->getMACString(mac).c_str(),
                  status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
#endif
}

// ============================================================================
// Wi-Fi Scanner Functions
// ============================================================================

bool RF24Module::startWiFiScan(bool async) {
    if (wifiScanInProgress) {
        return false;
    }
    
    wifiNetworks.clear();
    wifiScanInProgress = true;
    
    int result = WiFi.scanNetworks(async);
    
    if (!async) {
        parseWiFiScanResults();
        wifiScanInProgress = false;
    }
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF24] Wi-Fi scan started (%s mode)\n", async ? "async" : "sync");
#endif
    
    return true;
}

bool RF24Module::isWiFiScanComplete() {
    if (!wifiScanInProgress) {
        return true;
    }
    
    int n = WiFi.scanComplete();
    if (n >= 0) {
        parseWiFiScanResults();
        wifiScanInProgress = false;
        return true;
    }
    
    return false;
}

int RF24Module::getWiFiNetworkCount() {
    return wifiNetworks.size();
}

WiFiNetworkInfo RF24Module::getWiFiNetwork(int index) {
    if (index >= 0 && index < (int)wifiNetworks.size()) {
        return wifiNetworks[index];
    }
    return WiFiNetworkInfo{};
}

String RF24Module::getEncryptionTypeName(wifi_auth_mode_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "Unknown";
    }
}

void RF24Module::sortNetworksByRSSI() {
    std::sort(wifiNetworks.begin(), wifiNetworks.end(),
              [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) {
                  return a.rssi > b.rssi;
              });
}

void RF24Module::parseWiFiScanResults() {
    int n = WiFi.scanComplete();
    
    if (n < 0) {
        return; // Scan not complete or error
    }
    
    wifiNetworks.clear();
    
    for (int i = 0; i < n; i++) {
        WiFiNetworkInfo info;
        
        String ssid = WiFi.SSID(i);
        strncpy(info.ssid, ssid.c_str(), sizeof(info.ssid) - 1);
        info.ssid[sizeof(info.ssid) - 1] = '\0';
        
        WiFi.BSSID(i, info.bssid);
        info.rssi = WiFi.RSSI(i);
        info.channel = WiFi.channel(i);
        info.encryptionType = WiFi.encryptionType(i);
        info.isHidden = (ssid.length() == 0);
        
        wifiNetworks.push_back(info);
    }
    
    sortNetworksByRSSI();
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF24] Found %d Wi-Fi networks\n", n);
#endif
}

// ============================================================================
// BLE Scanner Functions
// ============================================================================

bool RF24Module::initBLE(const char* deviceName) {
    if (bleInitialized) {
        return true;
    }
    
    BLEDevice::init(deviceName);
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(BLE_SCAN_INTERVAL);
    pBLEScan->setWindow(BLE_SCAN_WINDOW);
    
    bleInitialized = true;
    
#if ENABLE_SERIAL_LOGGING
    Serial.println("[RF24] BLE initialized");
#endif
    
    return true;
}

void RF24Module::stopBLE() {
    if (bleInitialized) {
        BLEDevice::deinit(false);
        bleInitialized = false;
        pBLEScan = nullptr;
        bleDevices.clear();
    }
}

bool RF24Module::startBLEScan(int duration) {
    if (!bleInitialized) {
        initBLE();
    }
    
    bleDevices.clear();
    
    BLEScanResults foundDevices = pBLEScan->start(duration, false);
    
    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        
        BLEDeviceInfo info;
        
        if (device.haveName()) {
            strncpy(info.name, device.getName().c_str(), sizeof(info.name) - 1);
            info.hasName = true;
        } else {
            strcpy(info.name, "[Unknown]");
            info.hasName = false;
        }
        
        strncpy(info.address, device.getAddress().toString().c_str(), sizeof(info.address) - 1);
        info.rssi = device.getRSSI();
        info.appearance = device.haveAppearance() ? device.getAppearance() : 0;
        
        bleDevices.push_back(info);
    }
    
    pBLEScan->clearResults();
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF24] BLE scan complete: %d devices found\n", bleDevices.size());
#endif
    
    return true;
}

bool RF24Module::isBLEScanComplete() {
    return pBLEScan != nullptr && pBLEScan->isScanning() == false;
}

int RF24Module::getBLEDeviceCount() {
    return bleDevices.size();
}

BLEDeviceInfo RF24Module::getBLEDevice(int index) {
    if (index >= 0 && index < (int)bleDevices.size()) {
        return bleDevices[index];
    }
    return BLEDeviceInfo{};
}

void RF24Module::clearBLEResults() {
    bleDevices.clear();
}

// ============================================================================
// General Functions
// ============================================================================

void RF24Module::getLocalMAC(uint8_t* mac) {
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
}

String RF24Module::getMACString(const uint8_t* mac) {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buffer);
}

int RF24Module::getCurrentChannel() {
    return WiFi.channel();
}

void RF24Module::setChannel(int channel) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

// ============================================================================
// BLE Scan Callback Implementation
// ============================================================================

BLEScanCallback::BLEScanCallback(std::vector<BLEDeviceInfo>* devices) {
    deviceList = devices;
}

void BLEScanCallback::onResult(BLEAdvertisedDevice advertisedDevice) {
    // This callback is called for each device found during scan
    // The actual processing is done in startBLEScan()
}
