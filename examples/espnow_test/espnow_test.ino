/**
 * @file espnow_test.ino
 * @brief ESP-NOW communication test
 * 
 * This example demonstrates how to use the RF24 module
 * for ESP-NOW peer-to-peer communication.
 */

#include <M5Core2.h>
#include "../../include/config.h"
#include "../../include/rf24_module.h"

RF24Module rf24;

// Broadcast address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup() {
    M5.begin();
    Serial.begin(115200);
    
    Serial.println("ESP-NOW Test Example");
    Serial.println("====================");
    
    // Initialize RF24 module
    if (!rf24.begin()) {
        Serial.println("Failed to initialize RF24 module!");
        M5.Lcd.println("RF24 Init Failed!");
        while (1) delay(1000);
    }
    
    // Initialize ESP-NOW
    if (!rf24.initESPNow()) {
        Serial.println("Failed to initialize ESP-NOW!");
        M5.Lcd.println("ESP-NOW Init Failed!");
        while (1) delay(1000);
    }
    
    uint8_t mac[6];
    rf24.getLocalMAC(mac);
    
    Serial.println("ESP-NOW initialized");
    Serial.printf("Local MAC: %s\n", rf24.getMACString(mac).c_str());
    
    M5.Lcd.println("ESP-NOW Ready");
    M5.Lcd.println("\nLocal MAC:");
    M5.Lcd.println(rf24.getMACString(mac));
    M5.Lcd.println("\nB: Send broadcast");
}

void loop() {
    M5.update();
    
    // Send broadcast message on button press
    if (M5.BtnB.wasPressed()) {
        String message = "Hello from M5Stack!";
        uint8_t data[200];
        message.getBytes(data, message.length() + 1);
        
        Serial.println("Sending broadcast message...");
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.println("Sending...");
        
        if (rf24.broadcastMessage(data, message.length() + 1)) {
            Serial.println("Message sent successfully");
            M5.Lcd.println("Message sent!");
        } else {
            Serial.println("Failed to send message");
            M5.Lcd.println("Send failed!");
        }
        
        delay(1000);
    }
    
    // Check for received messages
    if (rf24.hasReceivedMessage()) {
        ESPNowMessage msg = rf24.getReceivedMessage();
        
        Serial.println("\n=== Message Received ===");
        Serial.printf("From: %s\n", rf24.getMACString(msg.senderId).c_str());
        Serial.printf("Data: %s\n", (char*)msg.data);
        Serial.println("=======================\n");
        
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.println("Message received!");
        M5.Lcd.printf("From:\n%s\n", rf24.getMACString(msg.senderId).c_str());
        M5.Lcd.printf("\nData:\n%s", (char*)msg.data);
    }
    
    delay(10);
}
