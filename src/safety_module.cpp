/**
 * @file safety_module.cpp
 * @brief Implementation of safety and policy enforcement module
 */

#include "safety_module.h"
#include <vector>
#include <cmath>
#include <algorithm>

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

/**
 * @brief Check transmission policy against safety rules
 * 
 * THREAT MITIGATION:
 * - Accidental Replay: Requires confirmation, enforces timeouts
 * - Blind Broadcast: Checks frequency blacklist
 * - User Error: Validates duration limits, prevents policy violations
 * 
 * @param request The transmission request to validate
 * @return PERMIT_ALLOWED if all checks pass, specific denial reason otherwise
 */
TransmitPermission SafetyModule::checkTransmitPolicy(const TransmitRequest& request) {
    // THREAT: Accidental Replay - Check if confirmation expired
    if (checkTimeout()) {
        return PERMIT_DENIED_TIMEOUT;
    }
    
    // THREAT: Accidental Replay - Require explicit user confirmation
    if (requireConfirmation && !request.confirmed) {
        return PERMIT_DENIED_NO_CONFIRMATION;
    }
    
    // THREAT: Blind Broadcast - Validate frequency is not blacklisted
    if (!isFrequencyAllowed(request.frequency)) {
        return PERMIT_DENIED_BLACKLIST;
    }
    
    // THREAT: Accidental Replay - Enforce rate limiting
    if (!isRateLimitOK()) {
        return PERMIT_DENIED_RATE_LIMIT;
    }
    
    // THREAT: User Error - Validate transmission duration within limits
    if (request.duration > maxTransmitDuration) {
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
    return (int)recentTransmits.size() < maxTransmitsPerMinute;
}

void SafetyModule::requestUserConfirmation(const TransmitRequest& request) {
    confirmationPending = true;
    confirmationRequestTime = millis();
    pendingRequest = request;
    
#if ENABLE_SERIAL_LOGGING
    Serial.printf("[Safety] Confirmation requested for %.2f MHz transmission\n", request.frequency);
#endif
}

/**
 * @brief Wait for user confirmation with timeout protection
 * 
 * THREAT MITIGATION:
 * - Accidental Replay: Timeout prevents stale/forgotten confirmations
 * - User Error: Forces fresh intentional action, prevents walk-away scenarios
 * 
 * @param timeout Maximum time to wait for confirmation (milliseconds)
 * @return true if confirmed, false if canceled or timed out
 */
bool SafetyModule::waitForUserConfirmation(unsigned long timeout) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (!confirmationPending) {
            return pendingRequest.confirmed;
        }
        
        // THREAT: Accidental Replay - Check for timeout on each iteration
        if (checkTimeout()) {
            confirmationPending = false;
            return false;
        }
        
        delay(100);
    }
    
    // THREAT: User Error - Timeout expired, deny transmission
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

/**
 * @brief Log transmission attempt to audit trail
 * 
 * THREAT MITIGATION:
 * - Accidental Replay: Audit trail enables detection of unintended replays
 * - All threats: Provides accountability and security review capability
 * 
 * Logs both allowed and denied attempts with timestamps and reasons.
 * 
 * @param request The transmission request
 * @param allowed Whether transmission was allowed
 * @param reason The permission decision reason
 */
void SafetyModule::logTransmitAttempt(const TransmitRequest& request, bool allowed, TransmitPermission reason) {
    TransmitLog log;
    log.timestamp = millis();
    log.frequency = request.frequency;
    log.duration = request.duration;
    log.wasAllowed = allowed;
    log.reason = reason;
    snprintf(log.details, sizeof(log.details), "%.127s", request.reason);
    
    auditLog.push_back(log);
    
    if (allowed) {
        // THREAT: Accidental Replay - Track transmissions for rate limiting
        recentTransmits.push_back(millis());
        lastTransmitTime = millis();
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
