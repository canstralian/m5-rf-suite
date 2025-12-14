/**
 * @file 433mhz_scanner.ino
 * @brief Simple 433 MHz scanner example
 * 
 * This example demonstrates how to use the RF433 module
 * to scan for and decode 433 MHz signals.
 */

#include <M5Core2.h>
#include "../../include/config.h"
#include "../../include/rf433_module.h"
#include "../../include/safety_module.h"

RF433Module rf433;

void setup() {
    M5.begin();
    Serial.begin(115200);
    
    Serial.println("433 MHz Scanner Example");
    Serial.println("=======================");
    
    // Initialize safety module
    Safety.begin();
    
    // Initialize RF433 module
    if (!rf433.begin()) {
        Serial.println("Failed to initialize RF433 module!");
        M5.Lcd.println("RF433 Init Failed!");
        while (1) delay(1000);
    }
    
    Serial.println("Scanning for 433 MHz signals...");
    M5.Lcd.println("Scanning 433 MHz...");
    M5.Lcd.println("Waiting for signals");
}

void loop() {
    M5.update();
    
    if (rf433.isSignalAvailable()) {
        RF433Signal signal = rf433.receiveSignal();
        
        if (signal.isValid) {
            // Print to serial
            Serial.println("\n=== Signal Received ===");
            Serial.printf("Type: %s\n", signal.description);
            Serial.printf("Value: %lu (0x%lX)\n", signal.value, signal.value);
            Serial.printf("Bits: %u\n", signal.bitLength);
            Serial.printf("Protocol: %u\n", signal.protocol);
            Serial.printf("Pulse Length: %u us\n", signal.pulseLength);
            Serial.println("======================\n");
            
            // Display on screen
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setCursor(10, 10);
            M5.Lcd.printf("Type: %s\n", signal.description);
            M5.Lcd.printf("Value: %lu\n", signal.value);
            M5.Lcd.printf("Bits: %u\n", signal.bitLength);
            M5.Lcd.printf("Protocol: %u\n", signal.protocol);
        }
    }
    
    delay(10);
}
