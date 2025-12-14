/**
 * @file ui_module.h
 * @brief User interface module for M5Stack displays
 * 
 * This module handles:
 * - Menu navigation and display
 * - Signal visualization
 * - User confirmations and dialogs
 * - Status information display
 */

#ifndef UI_MODULE_H
#define UI_MODULE_H

#include <Arduino.h>
#include "config.h"
#include "rf433_module.h"
#include "rf24_module.h"

// ============================================================================
// UI STATE AND MENU STRUCTURES
// ============================================================================

struct MenuItem {
    const char* label;
    MenuMode targetMode;
    void (*action)();
};

// ============================================================================
// UI MODULE CLASS
// ============================================================================

class UIModule {
public:
    UIModule();
    ~UIModule();
    
    // Initialization
    bool begin();
    void update();
    
    // Menu management
    void showMainMenu();
    void showMenu(MenuMode mode);
    MenuMode getCurrentMode();
    void handleButtonPress(int button); // Button A=0, B=1, C=2
    
    // Display functions
    void clearScreen();
    void drawStatusBar();
    void drawSignalList(RF433Signal* signals, int count, int selectedIndex);
    void drawWiFiList(WiFiNetworkInfo* networks, int count, int selectedIndex);
    void drawBLEList(BLEDeviceInfo* devices, int count, int selectedIndex);
    void drawSignalInfo(const RF433Signal& signal);
    
    // Dialog functions
    bool showConfirmDialog(const char* title, const char* message, unsigned long timeout);
    void showInfoDialog(const char* title, const char* message, unsigned long duration = 2000);
    void showErrorDialog(const char* title, const char* error);
    
    // Visualization
    void drawSignalGraph(const RF433Signal& signal);
    void drawRSSIBar(int rssi, int x, int y, int width, int height);
    void drawProgressBar(int percent, int x, int y, int width, int height);
    
    // Text display helpers
    void printCentered(const char* text, int y);
    void printWrapped(const char* text, int x, int y, int maxWidth);
    int getTextWidth(const char* text);
    
    // Status indicators
    void showReceiving();
    void showTransmitting();
    void showScanning();
    void clearIndicators();
    
private:
    MenuMode currentMode;
    int menuIndex;
    int scrollOffset;
    unsigned long lastUpdateTime;
    bool dialogActive;
    bool confirmResult;
    
    // Button state tracking
    bool buttonState[3];
    unsigned long buttonPressTime[3];
    
    // Helper functions
    void drawButton(const char* label, int position); // position: 0=left, 1=center, 2=right
    void drawTitle(const char* title);
    void handleMenuNavigation(int button);
    void executeMenuAction();
    int getVisibleItemCount();
};

// ============================================================================
// GLOBAL UI MODULE INSTANCE
// ============================================================================
extern UIModule UI;

#endif // UI_MODULE_H
