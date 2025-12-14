/**
 * @file config.h
 * @brief Configuration and constants for M5Stack RF Suite
 * 
 * This file contains all configuration parameters, pin definitions,
 * and constants used throughout the RF tool suite.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// VERSION INFORMATION
// ============================================================================
#define RF_SUITE_VERSION "1.0.0"
#define RF_SUITE_NAME "M5Stack RF Suite"

// ============================================================================
// 433 MHz RADIO CONFIGURATION
// ============================================================================
// Default pin for 433 MHz receiver (adjust based on your hardware setup)
#define RF_433_RX_PIN 36
// Default pin for 433 MHz transmitter (adjust based on your hardware setup)
#define RF_433_TX_PIN 26

// Protocol settings
#define RF_433_PROTOCOL_DEFAULT 1
#define RF_433_PULSE_LENGTH 350
#define RF_433_REPEAT_TRANSMIT 10

// Signal capture settings
#define RF_433_MAX_SIGNALS 50          // Maximum signals to store in memory
#define RF_433_SIGNAL_TIMEOUT 5000     // ms before clearing capture buffer

// ============================================================================
// 2.4 GHz RADIO CONFIGURATION
// ============================================================================
// ESP-NOW settings
#define ESPNOW_CHANNEL 1
#define ESPNOW_MAX_PEERS 20

// Wi-Fi scanner settings
#define WIFI_SCAN_INTERVAL 5000        // ms between scans
#define WIFI_MAX_NETWORKS 30

// BLE scanner settings
#define BLE_SCAN_TIME 5                // seconds
#define BLE_SCAN_INTERVAL 100          // ms
#define BLE_SCAN_WINDOW 99             // ms
#define BLE_MAX_DEVICES 30

// ============================================================================
// SAFETY AND POLICY CONFIGURATION
// ============================================================================
// Transmission safety settings
#define REQUIRE_USER_CONFIRMATION true  // Require button press before transmit
#define TRANSMIT_TIMEOUT 10000         // ms before auto-canceling transmit mode
#define MAX_TRANSMIT_DURATION 5000     // ms maximum single transmission duration

// Blacklist/whitelist for frequencies (in MHz)
#define FREQ_BLACKLIST_ENABLED false
const float FREQ_BLACKLIST[] = {
    // Add any frequencies that should never be transmitted on
    // Example: 121.5, 243.0  // Aviation emergency frequencies
};

// ============================================================================
// DISPLAY CONFIGURATION
// ============================================================================
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define TEXT_SIZE 2
#define STATUS_BAR_HEIGHT 20

// UI Colors (RGB565 format)
#define COLOR_BACKGROUND 0x0000    // Black
#define COLOR_TEXT 0xFFFF          // White
#define COLOR_HIGHLIGHT 0x07E0     // Green
#define COLOR_WARNING 0xFD20       // Orange
#define COLOR_DANGER 0xF800        // Red
#define COLOR_INFO 0x001F          // Blue
#define COLOR_SUCCESS 0x07E0       // Green

// ============================================================================
// DEBUG AND ASSERTIONS CONFIGURATION
// ============================================================================
// Enable debug assertions (safety invariant checks)
// Set to 0 to disable, 1 for critical only, 2 for standard, 3 for verbose
#ifndef DEBUG_ASSERTIONS
    #ifdef DEBUG
        #define DEBUG_ASSERTIONS 2  // Standard level in debug builds
    #else
        #define DEBUG_ASSERTIONS 0  // Disabled in release builds
    #endif
#endif

// Assertion levels
#define ASSERT_LEVEL_NONE 0
#define ASSERT_LEVEL_CRITICAL 1   // Safety-critical only (always in debug)
#define ASSERT_LEVEL_STANDARD 2   // Standard checks (default debug)
#define ASSERT_LEVEL_VERBOSE 3    // Verbose/performance-impacting checks

// ============================================================================
// LOGGING CONFIGURATION
// ============================================================================
#define ENABLE_SERIAL_LOGGING true
#define SERIAL_BAUD_RATE 115200
#define LOG_BUFFER_SIZE 512

// Log levels
enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3
};

#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO

// ============================================================================
// MENU CONFIGURATION
// ============================================================================
enum MenuMode {
    MENU_MAIN = 0,
    MENU_RF433_SCAN = 1,
    MENU_RF433_TRANSMIT = 2,
    MENU_RF433_REPLAY = 3,
    MENU_WIFI_SCAN = 4,
    MENU_BLE_SCAN = 5,
    MENU_ESPNOW = 6,
    MENU_SETTINGS = 7
};

// ============================================================================
// STORAGE CONFIGURATION
// ============================================================================
#define USE_SPIFFS true
#define SIGNAL_STORAGE_PATH "/signals"
#define CONFIG_STORAGE_PATH "/config"
#define MAX_SAVED_SIGNALS 100

#endif // CONFIG_H
