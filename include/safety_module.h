/**
 * @file safety_module.h
 * @brief Safety and policy enforcement module
 * 
 * This module implements the "safe-by-default" workflow:
 * - All transmissions require explicit user confirmation
 * - Policy checks before any RF transmission
 * - Audit logging of all transmission attempts
 * - Time-based restrictions and rate limiting
 */

#ifndef SAFETY_MODULE_H
#define SAFETY_MODULE_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// SAFETY POLICY STRUCTURES
// ============================================================================

enum TransmitPermission {
    PERMIT_ALLOWED = 0,
    PERMIT_DENIED_NO_CONFIRMATION = 1,
    PERMIT_DENIED_BLACKLIST = 2,
    PERMIT_DENIED_RATE_LIMIT = 3,
    PERMIT_DENIED_POLICY = 4,
    PERMIT_DENIED_TIMEOUT = 5
};

struct TransmitRequest {
    float frequency;            // MHz
    unsigned long duration;     // milliseconds
    unsigned long timestamp;    // When requested
    bool confirmed;             // User confirmed
    char reason[64];           // Reason for request
};

struct TransmitLog {
    unsigned long timestamp;
    float frequency;
    unsigned long duration;
    bool wasAllowed;
    TransmitPermission reason;
    char details[128];
};

// ============================================================================
// SAFETY MODULE CLASS
// ============================================================================

class SafetyModule {
public:
    SafetyModule();
    ~SafetyModule();
    
    // Initialization
    void begin();
    
    // Policy checks
    TransmitPermission checkTransmitPolicy(const TransmitRequest& request);
    bool isFrequencyAllowed(float frequency);
    bool isRateLimitOK();
    
    // User confirmation
    void requestUserConfirmation(const TransmitRequest& request);
    bool waitForUserConfirmation(unsigned long timeout = TRANSMIT_TIMEOUT);
    void cancelConfirmation();
    bool isConfirmationPending();
    
    // Audit logging
    void logTransmitAttempt(const TransmitRequest& request, bool allowed, TransmitPermission reason);
    int getRecentLogs(TransmitLog* logs, int maxCount, unsigned long since = 0);
    void clearLogs();
    
    // Configuration
    void setRequireConfirmation(bool required);
    bool getRequireConfirmation();
    void setTransmitTimeout(unsigned long timeout);
    void setMaxTransmitDuration(unsigned long duration);
    
    // Blacklist management
    bool addFrequencyToBlacklist(float frequency);
    bool removeFrequencyFromBlacklist(float frequency);
    int getBlacklistedFrequencies(float* frequencies, int maxCount);
    
    // Rate limiting
    void setRateLimit(int maxTransmitsPerMinute);
    int getRateLimit();
    int getRecentTransmitCount();
    
    // Status
    String getStatusString();
    bool isTransmitAllowed();
    unsigned long getLastTransmitTime();
    
private:
    bool requireConfirmation;
    unsigned long transmitTimeout;
    unsigned long maxTransmitDuration;
    bool confirmationPending;
    unsigned long confirmationRequestTime;
    TransmitRequest pendingRequest;
    
    // Rate limiting
    int maxTransmitsPerMinute;
    std::vector<unsigned long> recentTransmits;
    
    // Blacklist
    std::vector<float> blacklistedFrequencies;
    
    // Audit log
    std::vector<TransmitLog> auditLog;
    unsigned long lastTransmitTime;
    
    // Helper functions
    void cleanupOldTransmits();
    const char* getPermissionName(TransmitPermission perm);
    bool checkTimeout();
};

// ============================================================================
// GLOBAL SAFETY MODULE INSTANCE
// ============================================================================
extern SafetyModule Safety;

#endif // SAFETY_MODULE_H
