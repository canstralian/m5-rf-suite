/**
 * @file safety_module.cpp
 * @brief Implementation of safety and policy enforcement module
 */

#include "safety_module.h"
#include <vector>
#include <cmath>
#include <algorithm>

// Debug assertion macros for safety module
#if DEBUG_ASSERTIONS >= ASSERT_LEVEL_CRITICAL
    #define SAFETY_ASSERT_CRITICAL(condition, message) \
        do { \
            if (!(condition)) { \
                Serial.printf("[SAFETY ASSERT CRITICAL] %s:%d - %s\n", __FILE__, __LINE__, message); \
                Serial.printf("[SAFETY ASSERT CRITICAL] Condition failed: %s\n", #condition); \
            } \
        } while(0)
#else
    #define SAFETY_ASSERT_CRITICAL(condition, message) ((void)0)
#endif

#if DEBUG_ASSERTIONS >= ASSERT_LEVEL_STANDARD
    #define SAFETY_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                Serial.printf("[SAFETY ASSERT] %s:%d - %s\n", __FILE__, __LINE__, message); \
                Serial.printf("[SAFETY ASSERT] Condition failed: %s\n", #condition); \
            } \
        } while(0)
#else
    #define SAFETY_ASSERT(condition, message) ((void)0)
#endif

SafetyModule Safety; // Global instance

SafetyModule::SafetyModule() {
    requireConfirmation = REQUIRE_USER_CONFIRMATION;
    transmitTimeout = TRANSMIT_TIMEOUT;
    maxTransmitDuration = MAX_TRANSMIT_DURATION;
    confirmationPending = false;
    confirmationRequestTime = 0;
    lastTransmitTime = 0;
    maxTransmitsPerMinute = 10; // Default rate limit
}

SafetyModule::~SafetyModule() {
    auditLog.clear();
    recentTransmits.clear();
    blacklistedFrequencies.clear();
}

void SafetyModule::begin() {
    // Load blacklist from configuration
#if FREQ_BLACKLIST_ENABLED
    for (size_t i = 0; i < sizeof(FREQ_BLACKLIST) / sizeof(FREQ_BLACKLIST[0]); i++) {
        blacklistedFrequencies.push_back(FREQ_BLACKLIST[i]);
    }
#endif
    
#if ENABLE_SERIAL_LOGGING
    Serial.println("[Safety] Safety module initialized");
    Serial.printf("[Safety] Require confirmation: %s\n", requireConfirmation ? "YES" : "NO");
    Serial.printf("[Safety] Transmit timeout: %lu ms\n", transmitTimeout);
    Serial.printf("[Safety] Max transmit duration: %lu ms\n", maxTransmitDuration);
    Serial.printf("[Safety] Rate limit: %d/min\n", maxTransmitsPerMinute);
#endif
}

TransmitPermission SafetyModule::checkTransmitPolicy(const TransmitRequest& request) {
    // TX-CONF-1: Verify confirmation requirement is enforced (not bypassed at runtime)
    SAFETY_ASSERT_CRITICAL(!REQUIRE_USER_CONFIRMATION || requireConfirmation,
                          "TX-CONF-1: Confirmation requirement bypassed at runtime");
    
    // Check timeout first
    if (checkTimeout()) {
        return PERMIT_DENIED_TIMEOUT;
    }
    
    // TX-CONF-1: Check if confirmation is required and not yet given
    if (requireConfirmation && !request.confirmed) {
        SAFETY_ASSERT(request.confirmed == false,
                     "TX-CONF-1: Confirmation state inconsistent");
        return PERMIT_DENIED_NO_CONFIRMATION;
    }
    
    // TX-POL-1: Check frequency blacklist
    if (!isFrequencyAllowed(request.frequency)) {
        return PERMIT_DENIED_BLACKLIST;
    }
    
    // TX-RATE-1: Check rate limiting
    if (!isRateLimitOK()) {
        SAFETY_ASSERT(getRecentTransmitCount() >= maxTransmitsPerMinute,
                     "TX-RATE-1: Rate limit check inconsistent");
        return PERMIT_DENIED_RATE_LIMIT;
    }
    
    // TX-POL-2: Check duration limits
    if (request.duration > maxTransmitDuration) {
        SAFETY_ASSERT(maxTransmitDuration > 0,
                     "TX-POL-2: Duration limit not properly configured");
        return PERMIT_DENIED_POLICY;
    }
    
    return PERMIT_ALLOWED;
}

bool SafetyModule::isFrequencyAllowed(float frequency) {
    for (float blacklisted : blacklistedFrequencies) {
        if (fabs(frequency - blacklisted) < 0.1) { // Within 100 kHz
            return false;
        }
    }
    return true;
}

bool SafetyModule::isRateLimitOK() {
    cleanupOldTransmits();
    
    int currentCount = (int)recentTransmits.size();
    bool withinLimit = currentCount < maxTransmitsPerMinute;
    
    // TX-RATE-1: Verify rate limit enforcement
    SAFETY_ASSERT(currentCount >= 0 && currentCount <= maxTransmitsPerMinute + 1,
                 "TX-RATE-1: Rate limit count out of expected range");
    
    return withinLimit;
}

void SafetyModule::requestUserConfirmation(const TransmitRequest& request) {
    confirmationPending = true;
    confirmationRequestTime = millis();
    pendingRequest = request;
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[Safety] Confirmation requested for %.2f MHz transmission\n", request.frequency);
#endif
}

bool SafetyModule::waitForUserConfirmation(unsigned long timeout) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (!confirmationPending) {
            return pendingRequest.confirmed;
        }
        
        // Check for timeout
        if (checkTimeout()) {
            confirmationPending = false;
            return false;
        }
        
        delay(100);
    }
    
    confirmationPending = false;
    return false;
}

void SafetyModule::cancelConfirmation() {
    confirmationPending = false;
    confirmationRequestTime = 0;
    
#if ENABLE_SERIAL_LOGGING
    Serial.println("[Safety] Confirmation cancelled");
#endif
}

bool SafetyModule::isConfirmationPending() {
    return confirmationPending;
}

void SafetyModule::logTransmitAttempt(const TransmitRequest& request, bool allowed, TransmitPermission reason) {
    TransmitLog log;
    log.timestamp = millis();
    log.frequency = request.frequency;
    log.duration = request.duration;
    log.wasAllowed = allowed;
    log.reason = reason;
    snprintf(log.details, sizeof(log.details), "%.127s", request.reason);
    
    // SAFE-TX-5: Verify audit trail is being maintained
    SAFETY_ASSERT(auditLog.size() < 1000, "SAFE-TX-5: Audit log size exceeded safe limit");
    
    auditLog.push_back(log);
    
    if (allowed) {
        // TX-CONF-2: Single use confirmation - record transmission
        recentTransmits.push_back(millis());
        lastTransmitTime = millis();
        
        // Verify transmission was actually allowed
        SAFETY_ASSERT(reason == PERMIT_ALLOWED,
                     "Inconsistent: transmission allowed but reason not PERMIT_ALLOWED");
    } else {
        // Verify denied transmission has appropriate reason
        SAFETY_ASSERT(reason != PERMIT_ALLOWED,
                     "Inconsistent: transmission denied but reason is PERMIT_ALLOWED");
    }
    
    // Keep log size manageable
    if (auditLog.size() > 100) {
        auditLog.erase(auditLog.begin());
    }
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[Safety] Log: %.2f MHz for %lu ms - %s (%s)\n",
                  log.frequency, log.duration,
                  allowed ? "ALLOWED" : "DENIED",
                  getPermissionName(reason));
#endif
}

int SafetyModule::getRecentLogs(TransmitLog* logs, int maxCount, unsigned long since) {
    int count = 0;
    for (int i = auditLog.size() - 1; i >= 0 && count < maxCount; i--) {
        if (auditLog[i].timestamp >= since) {
            logs[count++] = auditLog[i];
        }
    }
    return count;
}

void SafetyModule::clearLogs() {
    auditLog.clear();
#if ENABLE_SERIAL_LOGGING
    Serial.println("[Safety] Audit logs cleared");
#endif
}

void SafetyModule::setRequireConfirmation(bool required) {
    requireConfirmation = required;
}

bool SafetyModule::getRequireConfirmation() {
    return requireConfirmation;
}

void SafetyModule::setTransmitTimeout(unsigned long timeout) {
    transmitTimeout = timeout;
}

void SafetyModule::setMaxTransmitDuration(unsigned long duration) {
    maxTransmitDuration = duration;
}

bool SafetyModule::addFrequencyToBlacklist(float frequency) {
    if (!isFrequencyAllowed(frequency)) {
        return false; // Already blacklisted
    }
    blacklistedFrequencies.push_back(frequency);
    return true;
}

bool SafetyModule::removeFrequencyFromBlacklist(float frequency) {
    for (auto it = blacklistedFrequencies.begin(); it != blacklistedFrequencies.end(); ++it) {
        if (fabs(*it - frequency) < 0.1) {
            blacklistedFrequencies.erase(it);
            return true;
        }
    }
    return false;
}

int SafetyModule::getBlacklistedFrequencies(float* frequencies, int maxCount) {
    int count = 0;
    for (float freq : blacklistedFrequencies) {
        if (count >= maxCount) break;
        frequencies[count++] = freq;
    }
    return count;
}

void SafetyModule::setRateLimit(int maxTransmitsPerMinute) {
    this->maxTransmitsPerMinute = maxTransmitsPerMinute;
}

int SafetyModule::getRateLimit() {
    return maxTransmitsPerMinute;
}

int SafetyModule::getRecentTransmitCount() {
    cleanupOldTransmits();
    return recentTransmits.size();
}

String SafetyModule::getStatusString() {
    String status = "Safety: ";
    status += requireConfirmation ? "LOCKED" : "UNLOCKED";
    status += " | Rate: ";
    status += String(getRecentTransmitCount());
    status += "/";
    status += String(maxTransmitsPerMinute);
    return status;
}

bool SafetyModule::isTransmitAllowed() {
    return !confirmationPending && isRateLimitOK();
}

unsigned long SafetyModule::getLastTransmitTime() {
    return lastTransmitTime;
}

void SafetyModule::cleanupOldTransmits() {
    unsigned long cutoff = millis() - 60000; // 1 minute ago
    recentTransmits.erase(
        std::remove_if(recentTransmits.begin(), recentTransmits.end(),
                      [cutoff](unsigned long t) { return t < cutoff; }),
        recentTransmits.end()
    );
}

const char* SafetyModule::getPermissionName(TransmitPermission perm) {
    switch (perm) {
        case PERMIT_ALLOWED: return "ALLOWED";
        case PERMIT_DENIED_NO_CONFIRMATION: return "NO_CONFIRMATION";
        case PERMIT_DENIED_BLACKLIST: return "BLACKLISTED";
        case PERMIT_DENIED_RATE_LIMIT: return "RATE_LIMITED";
        case PERMIT_DENIED_POLICY: return "POLICY_VIOLATION";
        case PERMIT_DENIED_TIMEOUT: return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

bool SafetyModule::checkTimeout() {
    if (confirmationPending && millis() - confirmationRequestTime > transmitTimeout) {
        confirmationPending = false;
        return true;
    }
    return false;
}
