/**
 * @file rf433_module.h
 * @brief 433 MHz RF module for receiving, classifying, and transmitting signals
 * 
 * This module handles all 433 MHz operations including:
 * - Signal reception and decoding (OOK/ASK)
 * - Signal classification and analysis
 * - Safe transmission with policy enforcement
 */

#ifndef RF433_MODULE_H
#define RF433_MODULE_H

#include <Arduino.h>
#include <RCSwitch.h>
#include "config.h"

// ============================================================================
// SIGNAL TYPES AND STRUCTURES
// ============================================================================

enum SignalType {
    SIGNAL_UNKNOWN = 0,
    SIGNAL_DOOR_BELL = 1,
    SIGNAL_GARAGE_DOOR = 2,
    SIGNAL_LIGHT_SWITCH = 3,
    SIGNAL_WEATHER_STATION = 4,
    SIGNAL_CAR_REMOTE = 5,
    SIGNAL_ALARM_SYSTEM = 6,
    SIGNAL_OTHER = 99
};

/**
 * @brief 433 MHz RF signal data (fixed-size, no dynamic allocation)
 * 
 * MEMORY OWNERSHIP: Self-contained, no heap allocations
 * Copy/move semantics: Default (shallow copy sufficient)
 * Safe to pass by value and return from functions
 * 
 * NOTE: This is a simplified representation. Actual pulse timing data
 * is not stored here but can be captured separately in CapturedSignalData
 * for more detailed analysis.
 */
struct RF433Signal {
    unsigned long value;        // Decoded value
    unsigned int bitLength;     // Number of bits
    unsigned int protocol;      // Protocol number
    unsigned int pulseLength;   // Pulse length in microseconds
    unsigned long timestamp;    // When signal was captured
    SignalType type;           // Classified signal type
    int rssi;                  // Signal strength (if available)
    char description[64];      // Human-readable description
    bool isValid;              // Whether signal is valid
};

// ============================================================================
// RF433 MODULE CLASS
// ============================================================================

class RF433Module {
public:
    RF433Module();
    ~RF433Module();
    
    // Initialization and setup
    bool begin(int rxPin = RF_433_RX_PIN, int txPin = RF_433_TX_PIN);
    void end();
    
    // Receiver functions
    bool isSignalAvailable();
    RF433Signal receiveSignal();
    void startReceiving();
    void stopReceiving();
    
    // Signal classification
    SignalType classifySignal(const RF433Signal& signal);
    const char* getSignalTypeName(SignalType type);
    
    // Transmitter functions (with safety checks)
    bool canTransmit(const RF433Signal& signal);
    bool transmitSignal(const RF433Signal& signal, bool requireConfirmation = true);
    void setTransmitEnabled(bool enabled);
    bool isTransmitEnabled();
    
    // Signal storage
    bool saveSignal(const RF433Signal& signal, const char* name);
    bool loadSignal(const char* name, RF433Signal& signal);
    int listSavedSignals(char** names, int maxCount);
    
    // Statistics and info
    unsigned long getReceivedCount();
    unsigned long getTransmittedCount();
    void resetStatistics();
    
    // Configuration
    void setProtocol(int protocol);
    void setPulseLength(int length);
    void setRepeatTransmit(int repeat);
    
private:
    RCSwitch rcSwitch;
    int rxPin;
    int txPin;
    bool transmitEnabled;
    unsigned long receivedCount;
    unsigned long transmittedCount;
    
    // Internal helper functions
    unsigned long calculateTransmissionDuration(const RF433Signal& signal);
    bool checkTransmitPolicy(const RF433Signal& signal);
    void logSignal(const RF433Signal& signal, bool isTransmit);
    bool isFrequencyBlacklisted(float frequency);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Convert signal to JSON string for storage/transmission
String signalToJson(const RF433Signal& signal);

// Parse JSON string to signal structure
bool jsonToSignal(const String& json, RF433Signal& signal);

// Format signal for display
String formatSignalInfo(const RF433Signal& signal);

#endif // RF433_MODULE_H
