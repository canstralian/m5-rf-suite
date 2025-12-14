/**
 * @file main.cpp
 * @brief Main application for M5Stack RF Suite
 * 
 * This is the primary entry point that initializes all modules
 * and manages the main application loop.
 */

#include <M5Core2.h>
#include <algorithm>
#include "config.h"
#include "rf433_module.h"
#include "rf24_module.h"
#include "safety_module.h"

// Global module instances
RF433Module rf433;
RF24Module rf24;

// Application state
MenuMode currentMode = MENU_MAIN;
int menuIndex = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100; // ms

// Signal buffers
RF433Signal capturedSignals[RF_433_MAX_SIGNALS];
int capturedSignalCount = 0;
int selectedSignalIndex = 0;

// Forward declarations
void displayMainMenu();
void displayRF433Scanner();
void displayRF433Transmit();
void displayWiFiScanner();
void displayBLEScanner();
void displayESPNowMenu();
void displaySettings();
void handleButtonPress();
void updateDisplay();

void setup() {
    // Initialize M5Stack
    M5.begin();
    M5.Axp.SetLcdVoltage(2800);
    
    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println("\n\n" RF_SUITE_NAME " v" RF_SUITE_VERSION);
    Serial.println("====================================");
    
    // Initialize display
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
    M5.Lcd.setTextSize(TEXT_SIZE);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println(RF_SUITE_NAME);
    M5.Lcd.println("v" RF_SUITE_VERSION);
    M5.Lcd.println("\nInitializing...");
    
    // Initialize safety module first
    Safety.begin();
    
    // Initialize RF modules
    M5.Lcd.println("- 433 MHz module");
    if (rf433.begin()) {
        Serial.println("[Main] RF433 module ready");
    } else {
        Serial.println("[Main] RF433 module failed!");
    }
    
    M5.Lcd.println("- 2.4 GHz module");
    if (rf24.begin()) {
        Serial.println("[Main] RF24 module ready");
    } else {
        Serial.println("[Main] RF24 module failed!");
    }
    
    M5.Lcd.println("\nReady!");
    delay(2000);
    
    // Clear captured signals
    capturedSignalCount = 0;
    
    // Show main menu
    currentMode = MENU_MAIN;
    displayMainMenu();
    
    Serial.println("[Main] System ready");
}

void loop() {
    M5.update();
    
    // Handle button presses
    handleButtonPress();
    
    // Update display periodically
    if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    // Process based on current mode
    switch (currentMode) {
        case MENU_RF433_SCAN:
            // Check for incoming signals
            if (rf433.isSignalAvailable()) {
                RF433Signal signal = rf433.receiveSignal();
                if (signal.isValid && capturedSignalCount < RF_433_MAX_SIGNALS) {
                    capturedSignals[capturedSignalCount++] = signal;
                    displayRF433Scanner(); // Update display
                    
                    // Beep on M5Stack
                    M5.Axp.SetLDOEnable(3, true);
                    delay(50);
                    M5.Axp.SetLDOEnable(3, false);
                }
            }
            break;
            
        case MENU_WIFI_SCAN:
            // Check if scan is complete
            if (rf24.isWiFiScanComplete()) {
                displayWiFiScanner();
            }
            break;
            
        default:
            break;
    }
    
    delay(10);
}

void handleButtonPress() {
    // Button A (left) - Back or previous
    if (M5.BtnA.wasPressed()) {
        Serial.println("[Main] Button A pressed");
        
        switch (currentMode) {
            case MENU_MAIN:
                // Cycle through menu
                menuIndex = (menuIndex > 0) ? menuIndex - 1 : 6;
                displayMainMenu();
                break;
                
            case MENU_RF433_SCAN:
            case MENU_RF433_TRANSMIT:
            case MENU_WIFI_SCAN:
            case MENU_BLE_SCAN:
            case MENU_ESPNOW:
            case MENU_SETTINGS:
                // Return to main menu
                currentMode = MENU_MAIN;
                menuIndex = 0;
                displayMainMenu();
                break;
                
            default:
                break;
        }
    }
    
    // Button B (middle) - Select or action
    if (M5.BtnB.wasPressed()) {
        Serial.println("[Main] Button B pressed");
        
        switch (currentMode) {
            case MENU_MAIN:
                // Select menu item
                switch (menuIndex) {
                    case 0:
                        currentMode = MENU_RF433_SCAN;
                        capturedSignalCount = 0;
                        rf433.startReceiving();
                        displayRF433Scanner();
                        break;
                    case 1:
                        currentMode = MENU_RF433_TRANSMIT;
                        displayRF433Transmit();
                        break;
                    case 2:
                        currentMode = MENU_WIFI_SCAN;
                        rf24.startWiFiScan(true);
                        displayWiFiScanner();
                        break;
                    case 3:
                        currentMode = MENU_BLE_SCAN;
                        displayBLEScanner();
                        break;
                    case 4:
                        currentMode = MENU_ESPNOW;
                        displayESPNowMenu();
                        break;
                    case 5:
                        currentMode = MENU_SETTINGS;
                        displaySettings();
                        break;
                    case 6:
                        // About/Info
                        M5.Lcd.fillScreen(COLOR_BACKGROUND);
                        M5.Lcd.setCursor(10, 10);
                        M5.Lcd.println(RF_SUITE_NAME);
                        M5.Lcd.println("Version: " RF_SUITE_VERSION);
                        M5.Lcd.println("\nSafe-by-default RF tool");
                        M5.Lcd.println("433 MHz & 2.4 GHz");
                        M5.Lcd.println("\nPress A to return");
                        break;
                }
                break;
                
            case MENU_RF433_TRANSMIT:
                // Transmit selected signal with confirmation
                if (capturedSignalCount > 0) {
                    RF433Signal& signal = capturedSignals[selectedSignalIndex];
                    
                    M5.Lcd.fillScreen(COLOR_WARNING);
                    M5.Lcd.setCursor(10, 60);
                    M5.Lcd.setTextColor(BLACK);
                    M5.Lcd.println("TRANSMIT WARNING");
                    M5.Lcd.println("\nPress B to confirm");
                    M5.Lcd.println("Press A to cancel");
                    M5.Lcd.setTextColor(COLOR_TEXT);
                    
                    // Wait for confirmation
                    unsigned long startTime = millis();
                    bool confirmed = false;
                    
                    while (millis() - startTime < TRANSMIT_TIMEOUT) {
                        M5.update();
                        
                        if (M5.BtnB.wasPressed()) {
                            confirmed = true;
                            break;
                        }
                        if (M5.BtnA.wasPressed()) {
                            break;
                        }
                        
                        delay(10);
                    }
                    
                    if (confirmed) {
                        rf433.setTransmitEnabled(true);
                        bool success = rf433.transmitSignal(signal, false);
                        rf433.setTransmitEnabled(false);
                        
                        M5.Lcd.fillScreen(success ? COLOR_SUCCESS : COLOR_DANGER);
                        M5.Lcd.setCursor(10, 100);
                        M5.Lcd.println(success ? "Transmitted!" : "Failed!");
                        delay(1000);
                    }
                    
                    displayRF433Transmit();
                }
                break;
                
            case MENU_WIFI_SCAN:
                // Rescan
                rf24.startWiFiScan(true);
                displayWiFiScanner();
                break;
                
            case MENU_BLE_SCAN:
                // Start BLE scan
                M5.Lcd.fillScreen(COLOR_BACKGROUND);
                M5.Lcd.setCursor(10, 100);
                M5.Lcd.println("Scanning BLE...");
                rf24.startBLEScan(BLE_SCAN_TIME);
                displayBLEScanner();
                break;
                
            default:
                break;
        }
    }
    
    // Button C (right) - Forward or next
    if (M5.BtnC.wasPressed()) {
        Serial.println("[Main] Button C pressed");
        
        switch (currentMode) {
            case MENU_MAIN:
                // Cycle through menu
                menuIndex = (menuIndex + 1) % 7;
                displayMainMenu();
                break;
                
            case MENU_RF433_TRANSMIT:
                // Next signal
                if (capturedSignalCount > 0) {
                    selectedSignalIndex = (selectedSignalIndex + 1) % capturedSignalCount;
                    displayRF433Transmit();
                }
                break;
                
            default:
                break;
        }
    }
}

void updateDisplay() {
    // Update status bar or other dynamic elements
    // Currently minimal, can be expanded
}

void displayMainMenu() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Main Menu");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_HIGHLIGHT);
    
    const char* menuItems[] = {
        "1. 433MHz Scanner",
        "2. 433MHz Transmit",
        "3. WiFi Scanner",
        "4. BLE Scanner",
        "5. ESP-NOW",
        "6. Settings",
        "7. About"
    };
    
    int y = 50;
    for (int i = 0; i < 7; i++) {
        if (i == menuIndex) {
            M5.Lcd.fillRect(5, y - 5, 310, 25, COLOR_HIGHLIGHT);
            M5.Lcd.setTextColor(BLACK);
        } else {
            M5.Lcd.setTextColor(COLOR_TEXT);
        }
        M5.Lcd.setCursor(10, y);
        M5.Lcd.println(menuItems[i]);
        y += 25;
        M5.Lcd.setTextColor(COLOR_TEXT);
    }
    
    // Button labels
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Prev");
    M5.Lcd.setCursor(140, 225);
    M5.Lcd.print("Select");
    M5.Lcd.setCursor(285, 225);
    M5.Lcd.print("Next");
    M5.Lcd.setTextSize(2);
}

void displayRF433Scanner() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("433MHz Scanner");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_SUCCESS);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.printf("Signals: %d/%d", capturedSignalCount, RF_433_MAX_SIGNALS);
    M5.Lcd.setCursor(10, 55);
    M5.Lcd.printf("Total RX: %lu", rf433.getReceivedCount());
    
    // Display last few signals
    int y = 75;
    M5.Lcd.setCursor(10, y);
    M5.Lcd.println("Recent signals:");
    y += 15;
    
    int startIdx = std::max(0, capturedSignalCount - 8);
    for (int i = startIdx; i < capturedSignalCount && y < 220; i++) {
        M5.Lcd.setCursor(10, y);
        M5.Lcd.printf("%d. %s", i + 1, capturedSignals[i].description);
        y += 15;
    }
    
    // Button label
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Back");
}

void displayRF433Transmit() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("433MHz Transmit");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_WARNING);
    
    M5.Lcd.setTextSize(1);
    
    if (capturedSignalCount == 0) {
        M5.Lcd.setCursor(10, 100);
        M5.Lcd.println("No signals captured yet!");
        M5.Lcd.println("\nGo to scanner first.");
    } else {
        RF433Signal& signal = capturedSignals[selectedSignalIndex];
        
        M5.Lcd.setCursor(10, 40);
        M5.Lcd.printf("Signal %d/%d", selectedSignalIndex + 1, capturedSignalCount);
        
        int y = 60;
        M5.Lcd.setCursor(10, y); y += 15;
        M5.Lcd.printf("Type: %s", signal.description);
        M5.Lcd.setCursor(10, y); y += 15;
        M5.Lcd.printf("Value: %lu", signal.value);
        M5.Lcd.setCursor(10, y); y += 15;
        M5.Lcd.printf("Bits: %u", signal.bitLength);
        M5.Lcd.setCursor(10, y); y += 15;
        M5.Lcd.printf("Protocol: %u", signal.protocol);
        M5.Lcd.setCursor(10, y); y += 15;
        M5.Lcd.printf("Pulse: %u us", signal.pulseLength);
        
        M5.Lcd.setCursor(10, 160);
        M5.Lcd.setTextColor(COLOR_DANGER);
        M5.Lcd.println("WARNING: Transmit requires");
        M5.Lcd.setCursor(10, 175);
        M5.Lcd.println("explicit confirmation!");
        M5.Lcd.setTextColor(COLOR_TEXT);
    }
    
    // Button labels
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Back");
    M5.Lcd.setCursor(120, 225);
    M5.Lcd.print("Transmit");
    M5.Lcd.setCursor(285, 225);
    M5.Lcd.print("Next");
}

void displayWiFiScanner() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("WiFi Scanner");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_INFO);
    
    M5.Lcd.setTextSize(1);
    
    int networkCount = rf24.getWiFiNetworkCount();
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.printf("Networks found: %d", networkCount);
    
    int y = 60;
    for (int i = 0; i < std::min(networkCount, 10) && y < 220; i++) {
        WiFiNetworkInfo network = rf24.getWiFiNetwork(i);
        M5.Lcd.setCursor(10, y);
        M5.Lcd.printf("%s (%ddBm)", network.ssid, network.rssi);
        y += 15;
    }
    
    // Button labels
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Back");
    M5.Lcd.setCursor(140, 225);
    M5.Lcd.print("Scan");
}

void displayBLEScanner() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("BLE Scanner");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_INFO);
    
    M5.Lcd.setTextSize(1);
    
    int deviceCount = rf24.getBLEDeviceCount();
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.printf("Devices found: %d", deviceCount);
    
    int y = 60;
    for (int i = 0; i < std::min(deviceCount, 10) && y < 220; i++) {
        BLEDeviceInfo device = rf24.getBLEDevice(i);
        M5.Lcd.setCursor(10, y);
        M5.Lcd.printf("%s (%ddBm)", device.name, device.rssi);
        y += 15;
    }
    
    // Button labels
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Back");
    M5.Lcd.setCursor(140, 225);
    M5.Lcd.print("Scan");
}

void displayESPNowMenu() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("ESP-NOW");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_HIGHLIGHT);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println("ESP-NOW peer-to-peer");
    M5.Lcd.println("communication module");
    M5.Lcd.println("\nFeature coming soon!");
    
    uint8_t mac[6];
    rf24.getLocalMAC(mac);
    M5.Lcd.setCursor(10, 120);
    M5.Lcd.printf("Local MAC:\n%s", rf24.getMACString(mac).c_str());
    
    // Button label
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Back");
}

void displaySettings() {
    M5.Lcd.fillScreen(COLOR_BACKGROUND);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Settings");
    M5.Lcd.drawLine(0, 30, 320, 30, COLOR_HIGHLIGHT);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println(Safety.getStatusString());
    M5.Lcd.println();
    M5.Lcd.printf("RX Count: %lu\n", rf433.getReceivedCount());
    M5.Lcd.printf("TX Count: %lu\n", rf433.getTransmittedCount());
    M5.Lcd.println();
    M5.Lcd.println("Safety: ENABLED");
    M5.Lcd.println("Confirmation: REQUIRED");
    
    // Button label
    M5.Lcd.setCursor(5, 225);
    M5.Lcd.print("Back");
}
