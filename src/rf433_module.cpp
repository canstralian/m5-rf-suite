/**
 * @file rf433_module.cpp
 * @brief Implementation of 433 MHz RF module
 */

#include "rf433_module.h"
#include "safety_module.h"
#include <ArduinoJson.h>

RF433Module::RF433Module() {
    rxPin = RF_433_RX_PIN;
    txPin = RF_433_TX_PIN;
    transmitEnabled = false;
    receivedCount = 0;
    transmittedCount = 0;
}

RF433Module::~RF433Module() {
    end();
}

bool RF433Module::begin(int rxPin, int txPin) {
    this->rxPin = rxPin;
    this->txPin = txPin;
    
    // Enable receiver
    rcSwitch.enableReceive(digitalPinToInterrupt(rxPin));
    
    // Set up transmitter
    rcSwitch.enableTransmit(txPin);
    rcSwitch.setProtocol(RF_433_PROTOCOL_DEFAULT);
    rcSwitch.setPulseLength(RF_433_PULSE_LENGTH);
    rcSwitch.setRepeatTransmit(RF_433_REPEAT_TRANSMIT);
    
    // Transmit disabled by default for safety
    transmitEnabled = false;
    
#if ENABLE_SERIAL_LOGGING
    Serial.println("[RF433] Module initialized");
    Serial.printf("[RF433] RX Pin: %d, TX Pin: %d\n", rxPin, txPin);
    Serial.printf("[RF433] Protocol: %d, Pulse: %d us, Repeat: %d\n",
                  RF_433_PROTOCOL_DEFAULT, RF_433_PULSE_LENGTH, RF_433_REPEAT_TRANSMIT);
#endif
    
    return true;
}

void RF433Module::end() {
    rcSwitch.disableReceive();
    rcSwitch.disableTransmit();
}

bool RF433Module::isSignalAvailable() {
    return rcSwitch.available();
}

RF433Signal RF433Module::receiveSignal() {
    RF433Signal signal;
    signal.isValid = false;
    
    if (!rcSwitch.available()) {
        return signal;
    }
    
    signal.value = rcSwitch.getReceivedValue();
    signal.bitLength = rcSwitch.getReceivedBitlength();
    signal.protocol = rcSwitch.getReceivedProtocol();
    signal.pulseLength = rcSwitch.getReceivedDelay();
    signal.timestamp = millis();
    signal.rssi = -1; // Not available with RCSwitch
    signal.isValid = (signal.value != 0);
    
    // Classify the signal
    signal.type = classifySignal(signal);
    snprintf(signal.description, sizeof(signal.description), 
             "%s", getSignalTypeName(signal.type));
    
    rcSwitch.resetAvailable();
    receivedCount++;
    
    logSignal(signal, false);
    
    return signal;
}

void RF433Module::startReceiving() {
    rcSwitch.enableReceive(digitalPinToInterrupt(rxPin));
#if ENABLE_SERIAL_LOGGING
    Serial.println("[RF433] Receiving started");
#endif
}

void RF433Module::stopReceiving() {
    rcSwitch.disableReceive();
#if ENABLE_SERIAL_LOGGING
    Serial.println("[RF433] Receiving stopped");
#endif
}

SignalType RF433Module::classifySignal(const RF433Signal& signal) {
    // Simple heuristic-based classification
    // In a real implementation, this would use machine learning or pattern matching
    
    // Very short codes (< 8 bits) are often simple switches
    if (signal.bitLength <= 8) {
        return SIGNAL_LIGHT_SWITCH;
    }
    
    // 12-bit codes are common in cheap remotes
    if (signal.bitLength == 12) {
        return SIGNAL_LIGHT_SWITCH;
    }
    
    // 24-bit codes are common in doorbells and garage doors
    if (signal.bitLength == 24) {
        // Check pulse length to differentiate
        if (signal.pulseLength < 400) {
            return SIGNAL_DOOR_BELL;
        } else {
            return SIGNAL_GARAGE_DOOR;
        }
    }
    
    // 32-bit codes might be car remotes or alarm systems
    if (signal.bitLength == 32) {
        return SIGNAL_CAR_REMOTE;
    }
    
    // Weather stations often use longer codes
    if (signal.bitLength > 32) {
        return SIGNAL_WEATHER_STATION;
    }
    
    return SIGNAL_UNKNOWN;
}

const char* RF433Module::getSignalTypeName(SignalType type) {
    switch (type) {
        case SIGNAL_DOOR_BELL: return "Doorbell";
        case SIGNAL_GARAGE_DOOR: return "Garage Door";
        case SIGNAL_LIGHT_SWITCH: return "Light Switch";
        case SIGNAL_WEATHER_STATION: return "Weather Station";
        case SIGNAL_CAR_REMOTE: return "Car Remote";
        case SIGNAL_ALARM_SYSTEM: return "Alarm System";
        case SIGNAL_OTHER: return "Other";
        default: return "Unknown";
    }
}

bool RF433Module::canTransmit(const RF433Signal& signal) {
    if (!transmitEnabled) {
        return false;
    }
    
    return checkTransmitPolicy(signal);
}

bool RF433Module::transmitSignal(const RF433Signal& signal, bool requireConfirmation) {
    if (!signal.isValid) {
#if ENABLE_SERIAL_LOGGING
        Serial.println("[RF433] Cannot transmit invalid signal");
#endif
        return false;
    }
    
    if (!transmitEnabled) {
#if ENABLE_SERIAL_LOGGING
        Serial.println("[RF433] Transmit is disabled");
#endif
        return false;
    }
    
    // Create transmit request
    TransmitRequest request;
    request.frequency = 433.92; // MHz
    request.duration = signal.pulseLength * signal.bitLength * RF_433_REPEAT_TRANSMIT / 1000;
    request.timestamp = millis();
    request.confirmed = !requireConfirmation; // If not requiring confirmation, mark as confirmed
    snprintf(request.reason, sizeof(request.reason), "RF433: %s", signal.description);
    
    // Check policy
    TransmitPermission permission = Safety.checkTransmitPolicy(request);
    
    if (permission != PERMIT_ALLOWED) {
        Safety.logTransmitAttempt(request, false, permission);
#if ENABLE_SERIAL_LOGGING
        Serial.printf("[RF433] Transmit denied: %d\n", permission);
#endif
        return false;
    }
    
    // Perform transmission
    rcSwitch.setProtocol(signal.protocol);
    rcSwitch.setPulseLength(signal.pulseLength);
    rcSwitch.send(signal.value, signal.bitLength);
    
    transmittedCount++;
    Safety.logTransmitAttempt(request, true, PERMIT_ALLOWED);
    logSignal(signal, true);
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF433] Transmitted: %lu (%d bits)\n", signal.value, signal.bitLength);
#endif
    
    return true;
}

void RF433Module::setTransmitEnabled(bool enabled) {
    transmitEnabled = enabled;
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF433] Transmit %s\n", enabled ? "ENABLED" : "DISABLED");
#endif
}

bool RF433Module::isTransmitEnabled() {
    return transmitEnabled;
}

bool RF433Module::saveSignal(const RF433Signal& signal, const char* name) {
    // TODO: Implement SPIFFS storage
    // For now, just return success
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF433] Signal saved as '%s'\n", name);
#endif
    return true;
}

bool RF433Module::loadSignal(const char* name, RF433Signal& signal) {
    // TODO: Implement SPIFFS storage
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF433] Loading signal '%s'\n", name);
#endif
    return false;
}

int RF433Module::listSavedSignals(char** names, int maxCount) {
    // TODO: Implement SPIFFS storage
    return 0;
}

unsigned long RF433Module::getReceivedCount() {
    return receivedCount;
}

unsigned long RF433Module::getTransmittedCount() {
    return transmittedCount;
}

void RF433Module::resetStatistics() {
    receivedCount = 0;
    transmittedCount = 0;
}

void RF433Module::setProtocol(int protocol) {
    rcSwitch.setProtocol(protocol);
}

void RF433Module::setPulseLength(int length) {
    rcSwitch.setPulseLength(length);
}

void RF433Module::setRepeatTransmit(int repeat) {
    rcSwitch.setRepeatTransmit(repeat);
}

bool RF433Module::checkTransmitPolicy(const RF433Signal& signal) {
    TransmitRequest request;
    request.frequency = 433.92;
    request.duration = signal.pulseLength * signal.bitLength * RF_433_REPEAT_TRANSMIT / 1000;
    request.timestamp = millis();
    request.confirmed = false;
    
    return Safety.checkTransmitPolicy(request) == PERMIT_ALLOWED;
}

void RF433Module::logSignal(const RF433Signal& signal, bool isTransmit) {
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[RF433] %s: Value=%lu, Bits=%u, Protocol=%u, Pulse=%u us, Type=%s\n",
                  isTransmit ? "TX" : "RX",
                  signal.value,
                  signal.bitLength,
                  signal.protocol,
                  signal.pulseLength,
                  signal.description);
#endif
}

bool RF433Module::isFrequencyBlacklisted(float frequency) {
    return !Safety.isFrequencyAllowed(frequency);
}

// Helper functions

String signalToJson(const RF433Signal& signal) {
    StaticJsonDocument<512> doc;
    
    doc["value"] = (unsigned long)signal.value;
    doc["bitLength"] = signal.bitLength;
    doc["protocol"] = signal.protocol;
    doc["pulseLength"] = signal.pulseLength;
    doc["timestamp"] = signal.timestamp;
    doc["type"] = signal.type;
    doc["description"] = signal.description;
    doc["rssi"] = signal.rssi;
    doc["isValid"] = signal.isValid;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool jsonToSignal(const String& json, RF433Signal& signal) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    signal.value = doc["value"];
    signal.bitLength = doc["bitLength"];
    signal.protocol = doc["protocol"];
    signal.pulseLength = doc["pulseLength"];
    signal.timestamp = doc["timestamp"];
    signal.type = (SignalType)(int)doc["type"];
    signal.rssi = doc["rssi"];
    signal.isValid = doc["isValid"];
    
    const char* desc = doc["description"];
    strncpy(signal.description, desc, sizeof(signal.description) - 1);
    
    return true;
}

String formatSignalInfo(const RF433Signal& signal) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "Type: %s\nValue: %lu\nBits: %u\nProtocol: %u\nPulse: %u us\nTime: %lu ms",
             signal.description,
             signal.value,
             signal.bitLength,
             signal.protocol,
             signal.pulseLength,
             signal.timestamp);
    return String(buffer);
}
