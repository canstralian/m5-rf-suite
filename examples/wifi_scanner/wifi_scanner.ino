/**
 * @file wifi_scanner.ino
 * @brief Wi-Fi network scanner example
 * 
 * This example demonstrates how to use the RF24 module
 * to scan for Wi-Fi networks.
 */

#include <M5Core2.h>
#include "../../include/config.h"
#include "../../include/rf24_module.h"

RF24Module rf24;

void setup() {
    M5.begin();
    Serial.begin(115200);
    
    Serial.println("Wi-Fi Scanner Example");
    Serial.println("=====================");
    
    // Initialize RF24 module
    if (!rf24.begin()) {
        Serial.println("Failed to initialize RF24 module!");
        M5.Lcd.println("RF24 Init Failed!");
        while (1) delay(1000);
    }
    
    M5.Lcd.println("Wi-Fi Scanner");
    M5.Lcd.println("\nPress B to scan");
}

void loop() {
    M5.update();
    
    if (M5.BtnB.wasPressed()) {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.println("Scanning...");
        
        Serial.println("\nScanning for Wi-Fi networks...");
        rf24.startWiFiScan(false); // Synchronous scan
        
        int count = rf24.getWiFiNetworkCount();
        
        Serial.printf("\nFound %d networks:\n", count);
        Serial.println("-----------------------------------");
        
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.printf("Found %d networks:\n\n", count);
        
        for (int i = 0; i < count && i < 10; i++) {
            WiFiNetworkInfo network = rf24.getWiFiNetwork(i);
            
            Serial.printf("%d. %s\n", i + 1, network.ssid);
            Serial.printf("   RSSI: %d dBm, Ch: %d, Enc: %s\n",
                         network.rssi,
                         network.channel,
                         rf24.getEncryptionTypeName(network.encryptionType).c_str());
            
            M5.Lcd.printf("%d. %s (%ddBm)\n", i + 1, network.ssid, network.rssi);
        }
        
        Serial.println("-----------------------------------");
        M5.Lcd.println("\nPress B to rescan");
    }
    
    delay(10);
}
