/**
 * @file rf_test_workflow.cpp
 * @brief Implementation of RF Test Workflow State Machine
 */

#include "rf_test_workflow.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// Constructor and Destructor
// ============================================================================

RFTestWorkflow::RFTestWorkflow() :
    currentState(WF_IDLE),
    previousState(WF_IDLE),
    stateEntryTime(0),
    running(false),
    emergencyStop(false),
    rf433Module(nullptr),
    rf24Module(nullptr),
    selectedSignalIndex(-1),
    userConfirmed(false),
    userCanceled(false),
    transmissionAttempts(0),
    lastError(WF_ERROR_NONE),
    errorCount(0),
    workflowStartTime(0) {
}

RFTestWorkflow::~RFTestWorkflow() {
    if (running) {
        abort();
    }
}

// ============================================================================
// Initialization and Control
// ============================================================================

bool RFTestWorkflow::initialize(const WorkflowConfig& cfg, RF433Module* rf433, RF24Module* rf24) {
    config = cfg;
    rf433Module = rf433;
    rf24Module = rf24;
    
    currentState = WF_IDLE;
    running = false;
    emergencyStop = false;
    errorCount = 0;
    
    Serial.println("[Workflow] Initialized");
    Serial.printf("[Workflow] Band: %s\n", config.band == BAND_433MHZ ? "433 MHz" : "2.4 GHz");
    
    return true;
}

bool RFTestWorkflow::start() {
    if (currentState != WF_IDLE) {
        Serial.println("[Workflow] Cannot start: not in IDLE state");
        return false;
    }
    
    if ((config.band == BAND_433MHZ && rf433Module == nullptr) ||
        (config.band == BAND_24GHZ && rf24Module == nullptr)) {
        Serial.println("[Workflow] Cannot start: RF module not initialized");
        logError(WF_ERROR_INIT_FAILED, "RF module not available");
        return false;
    }
    
    Serial.println("[Workflow] Starting workflow");
    workflowStartTime = millis();
    running = true;
    transitionToState(WF_INIT, "User started workflow");
    
    // Run state machine
    while (currentState != WF_IDLE && !emergencyStop) {
        processCurrentState();
        checkTimeout();
        checkEmergencyStop();
        
        if (errorCount > 10) {
            Serial.println("[Workflow] Too many errors, forcing cleanup");
            transitionToState(WF_CLEANUP, "Error threshold exceeded");
        }
        
        delay(10);  // Small delay to prevent busy loop
    }
    
    running = false;
    Serial.println("[Workflow] Workflow completed");
    return errorCount == 0;
}

void RFTestWorkflow::abort() {
    Serial.println("[Workflow] Abort requested");
    emergencyStop = true;
    transitionToState(WF_CLEANUP, "Emergency abort");
}

void RFTestWorkflow::reset() {
    currentState = WF_IDLE;
    previousState = WF_IDLE;
    running = false;
    emergencyStop = false;
    // MEMORY OWNERSHIP: Clearing captureBuffer destroys all CapturedSignalData objects
    // Each object's destructor will automatically free its pulseTimes buffer (RAII)
    captureBuffer.clear();
    errorLog.clear();
    errorCount = 0;
    selectedSignalIndex = -1;
    userConfirmed = false;
    userCanceled = false;
    transmissionAttempts = 0;
}

// ============================================================================
// State Information
// ============================================================================

WorkflowState RFTestWorkflow::getCurrentState() const {
    return currentState;
}

const char* RFTestWorkflow::getStateName(WorkflowState state) const {
    switch (state) {
        case WF_IDLE: return "IDLE";
        case WF_INIT: return "INIT";
        case WF_LISTENING: return "LISTENING";
        case WF_ANALYZING: return "ANALYZING";
        case WF_READY: return "READY";
        case WF_TX_GATED: return "TX_GATED";
        case WF_TRANSMIT: return "TRANSMIT";
        case WF_CLEANUP: return "CLEANUP";
        default: return "UNKNOWN";
    }
}

uint32_t RFTestWorkflow::getStateElapsedTime() const {
    return millis() - stateEntryTime;
}

bool RFTestWorkflow::isRunning() const {
    return running;
}

// ============================================================================
// Results and Data Access
// ============================================================================

const AnalysisResult& RFTestWorkflow::getAnalysisResult() const {
    return analysisResult;
}

int RFTestWorkflow::getCapturedSignalCount() const {
    return captureBuffer.size();
}

const CapturedSignalData* RFTestWorkflow::getCapturedSignal(int index) const {
    if (index >= 0 && index < (int)captureBuffer.size()) {
        // MEMORY OWNERSHIP: Returns pointer to object owned by captureBuffer
        // Returned pointer is valid only while captureBuffer exists and is not modified
        // Caller MUST NOT delete the returned pointer
        // Caller should not hold pointer across operations that might modify captureBuffer
        return &captureBuffer[index];
    }
    return nullptr;
}

// ============================================================================
// User Interaction
// ============================================================================

void RFTestWorkflow::triggerAnalysis() {
    if (currentState == WF_LISTENING) {
        Serial.println("[Workflow] User triggered analysis");
        transitionToState(WF_ANALYZING, "User trigger");
    }
}

void RFTestWorkflow::selectSignalForTransmission(int index) {
    if (currentState == WF_READY && index >= 0 && index < (int)captureBuffer.size()) {
        selectedSignalIndex = index;
        Serial.printf("[Workflow] Signal %d selected for transmission\n", index);
        transitionToState(WF_TX_GATED, "User requested transmission");
    }
}

void RFTestWorkflow::confirmTransmission() {
    if (currentState == WF_TX_GATED) {
        userConfirmed = true;
        Serial.println("[Workflow] Transmission confirmed by user");
    }
}

void RFTestWorkflow::cancelTransmission() {
    if (currentState == WF_TX_GATED || currentState == WF_READY) {
        userCanceled = true;
        Serial.println("[Workflow] Transmission canceled by user");
    }
}

void RFTestWorkflow::continueObservation() {
    if (currentState == WF_READY) {
        Serial.println("[Workflow] Continuing observation");
        transitionToState(WF_LISTENING, "User requested more observation");
    }
}

// ============================================================================
// Error Handling
// ============================================================================

WorkflowError RFTestWorkflow::getLastError() const {
    return lastError;
}

const char* RFTestWorkflow::getErrorString(WorkflowError error) const {
    switch (error) {
        case WF_ERROR_NONE: return "No error";
        case WF_ERROR_INIT_FAILED: return "Initialization failed";
        case WF_ERROR_HARDWARE_FAILURE: return "Hardware failure";
        case WF_ERROR_BUFFER_OVERFLOW: return "Buffer overflow";
        case WF_ERROR_TIMEOUT: return "Timeout";
        case WF_ERROR_INVALID_SIGNAL: return "Invalid signal";
        case WF_ERROR_TRANSMISSION_FAILED: return "Transmission failed";
        case WF_ERROR_GATE_DENIED: return "Transmission gate denied";
        default: return "Unknown error";
    }
}

int RFTestWorkflow::getErrorCount() const {
    return errorCount;
}

// ============================================================================
// Logging and Audit
// ============================================================================

int RFTestWorkflow::getTransitionLogCount() const {
    return transitionLog.size();
}

const StateTransitionLog* RFTestWorkflow::getTransitionLog(int index) const {
    if (index >= 0 && index < (int)transitionLog.size()) {
        return &transitionLog[index];
    }
    return nullptr;
}

void RFTestWorkflow::clearLogs() {
    transitionLog.clear();
    errorLog.clear();
}

// ============================================================================
// State Machine
// ============================================================================

void RFTestWorkflow::processCurrentState() {
    switch (currentState) {
        case WF_IDLE:
            processIdleState();
            break;
        case WF_INIT:
            processInitState();
            break;
        case WF_LISTENING:
            processListeningState();
            break;
        case WF_ANALYZING:
            processAnalyzingState();
            break;
        case WF_READY:
            processReadyState();
            break;
        case WF_TX_GATED:
            processTxGatedState();
            break;
        case WF_TRANSMIT:
            processTransmitState();
            break;
        case WF_CLEANUP:
            processCleanupState();
            break;
    }
}

void RFTestWorkflow::transitionToState(WorkflowState newState, const char* reason) {
    Serial.printf("[Workflow] State transition: %s -> %s (%s)\n",
                  getStateName(currentState), getStateName(newState), reason);
    
    logTransition(currentState, newState, reason);
    
    previousState = currentState;
    currentState = newState;
    stateEntryTime = millis();
}

bool RFTestWorkflow::checkTimeout() {
    uint32_t elapsed = millis() - stateEntryTime;
    uint32_t timeout = getTimeoutForState(currentState);
    
    if (timeout > 0 && elapsed > timeout) {
        Serial.printf("[Workflow] Timeout in state %s\n", getStateName(currentState));
        handleTimeout();
        return true;
    }
    
    return false;
}

void RFTestWorkflow::checkEmergencyStop() {
    if (emergencyStop) {
        Serial.println("[Workflow] Emergency stop activated");
        if (rf433Module) rf433Module->setTransmitEnabled(false);
        transitionToState(WF_CLEANUP, "Emergency stop");
    }
}

// ============================================================================
// State Processors
// ============================================================================

void RFTestWorkflow::processIdleState() {
    // Do nothing in IDLE state
}

void RFTestWorkflow::processInitState() {
    Serial.println("[Workflow] === INITIALIZATION PHASE ===");
    
    // Step 1: Hardware initialization
    Serial.println("[Workflow] Step 1: Initialize hardware");
    
    bool success = false;
    if (config.band == BAND_433MHZ) {
        if (rf433Module != nullptr) {
            rf433Module->startReceiving();
            rf433Module->setTransmitEnabled(false);
            success = true;
            Serial.println("[Workflow] 433 MHz module: OK");
        } else {
            Serial.println("[Workflow] 433 MHz module: FAILED (not available)");
        }
    } else if (config.band == BAND_24GHZ) {
        if (rf24Module != nullptr) {
            success = true;
            Serial.println("[Workflow] 2.4 GHz module: OK");
        } else {
            Serial.println("[Workflow] 2.4 GHz module: FAILED (not available)");
        }
    }
    
    if (!success) {
        logError(WF_ERROR_INIT_FAILED, "Hardware initialization failed");
        transitionToState(WF_CLEANUP, "Init failed");
        return;
    }
    
    // Step 2: Allocate buffers
    Serial.println("[Workflow] Step 2: Allocate buffers");
    captureBuffer.clear();
    captureBuffer.reserve(config.bufferSize);
    Serial.printf("[Workflow] Buffer allocated: %d slots\n", config.bufferSize);
    
    // Step 3: Initialize statistics
    memset(&analysisResult, 0, sizeof(analysisResult));
    errorCount = 0;
    
    Serial.println("[Workflow] Initialization complete");
    transitionToState(WF_LISTENING, "Init successful");
}

/**
 * @brief Process LISTENING state - Passive observation phase
 * 
 * THREAT MITIGATION: Prevents Blind Broadcast
 * - Enforces "observe first" principle before any transmission
 * - Minimum observation time ensures adequate spectrum awareness
 * - Transmitter explicitly disabled during this phase (fail-safe)
 * 
 * This state is MANDATORY - cannot reach TRANSMIT without passing through LISTENING.
 */
void RFTestWorkflow::processListeningState() {
    // THREAT: Blind Broadcast - Ensure transmitter is disabled during observation
    if (config.band == BAND_433MHZ && rf433Module) {
        rf433Module->setTransmitEnabled(false);
    }
    
    uint32_t elapsed = millis() - stateEntryTime;
    
    // THREAT: Blind Broadcast - Enforce minimum observation time
    // Prevents hasty transmission without understanding spectrum environment
    if (elapsed < config.listenMinTime) {
        captureSignals();
        return;
    }
    
    // Check for transition triggers
    float bufferUsage = (float)captureBuffer.size() / config.bufferSize;
    
    if (bufferUsage >= 0.9f) {
        Serial.println("[Workflow] Buffer 90% full, triggering analysis");
        transitionToState(WF_ANALYZING, "Buffer full");
        return;
    }
    
    if (elapsed >= config.listenMaxTime) {
        Serial.println("[Workflow] Maximum observation time reached");
        transitionToState(WF_ANALYZING, "Max time reached");
        return;
    }
    
    // Continue passive observation
    captureSignals();
}

void RFTestWorkflow::processAnalyzingState() {
    Serial.println("[Workflow] === ANALYSIS PHASE ===");
    
    if (captureBuffer.empty()) {
        Serial.println("[Workflow] No signals captured, returning to LISTENING");
        transitionToState(WF_LISTENING, "No data");
        return;
    }
    
    Serial.printf("[Workflow] Analyzing %d captured signals\n", captureBuffer.size());
    
    // Initialize analysis result
    analysisResult.signalCount = captureBuffer.size();
    analysisResult.analysisTime = millis();
    
    if (config.band == BAND_433MHZ) {
        analyze433MHzSignals();
    } else if (config.band == BAND_24GHZ) {
        analyze24GHzPackets();
    }
    
    generateStatistics();
    
    analysisResult.complete = true;
    
    Serial.println("[Workflow] Analysis complete");
    Serial.printf("[Workflow]   Valid signals: %d\n", analysisResult.validSignalCount);
    Serial.printf("[Workflow]   Unique patterns: %d\n", analysisResult.uniquePatterns);
    
    transitionToState(WF_READY, "Analysis complete");
}

void RFTestWorkflow::processReadyState() {
    Serial.println("[Workflow] === READY PHASE ===");
    Serial.println("[Workflow] Awaiting user decision");
    
    // This state is typically driven by user interaction
    // In automated mode, could auto-transition after timeout
    
    // Check for timeout
    if (getStateElapsedTime() >= config.readyTimeout) {
        Serial.println("[Workflow] Ready timeout, ending workflow");
        transitionToState(WF_CLEANUP, "Inactivity timeout");
    }
}

/**
 * @brief Process TX_GATED state - Multi-stage transmission approval
 * 
 * THREAT MITIGATION: This function implements the core defense against:
 * - Accidental Replay Attacks (multi-gate approval, user confirmation, rate limiting)
 * - Blind Broadcast (frequency validation, signal analysis requirement)
 * - User Errors (explicit confirmation, input validation)
 * 
 * All four gates must pass sequentially for transmission to proceed.
 * Any gate failure results in denial and return to READY state (fail-safe).
 */
void RFTestWorkflow::processTxGatedState() {
    Serial.println("[Workflow] === GATED TRANSMISSION PHASE ===");
    
    transmissionAttempts++;
    
    // THREAT: User Error - Prevent infinite retry loops
    if (transmissionAttempts > 3) {
        Serial.println("[Workflow] Too many transmission attempts");
        transitionToState(WF_READY, "Max attempts");
        return;
    }
    
    // THREAT: User Error - Validate signal selection
    if (selectedSignalIndex < 0 || selectedSignalIndex >= (int)captureBuffer.size()) {
        Serial.println("[Workflow] Invalid signal selection");
        transitionToState(WF_READY, "Invalid selection");
        return;
    }
    
    // Gate 1: Policy Check
    // THREAT: Blind Broadcast, Accidental Replay
    // Validates frequency blacklist, duration limits, signal validity
    Serial.println("[Workflow] Gate 1: Policy validation");
    if (!checkPolicyGate()) {
        Serial.println("[Workflow] Gate 1: FAILED");
        transitionToState(WF_READY, "Policy denied");
        return;
    }
    Serial.println("[Workflow] Gate 1: PASSED");
    
    // Gate 2: Safety Confirmation
    // THREAT: Accidental Replay, User Error
    // Requires explicit user button press with timeout protection
    Serial.println("[Workflow] Gate 2: User confirmation");
    if (!checkConfirmationGate()) {
        Serial.println("[Workflow] Gate 2: FAILED");
        transitionToState(WF_READY, "Not confirmed");
        return;
    }
    Serial.println("[Workflow] Gate 2: PASSED");
    
    // Gate 3: Rate Limit Check
    // THREAT: Accidental Replay
    // Prevents rapid-fire replays (max 10/minute)
    Serial.println("[Workflow] Gate 3: Rate limiting");
    if (!checkRateLimitGate()) {
        Serial.println("[Workflow] Gate 3: FAILED");
        transitionToState(WF_READY, "Rate limit");
        return;
    }
    Serial.println("[Workflow] Gate 3: PASSED");
    
    // Gate 4: Band-Specific Validation
    // THREAT: Blind Broadcast, User Error
    // 433 MHz: Pulse timing validation, 2.4 GHz: Observed address binding
    Serial.println("[Workflow] Gate 4: Band-specific validation");
    bool gatePass = false;
    if (config.band == BAND_433MHZ) {
        gatePass = check433MHzGate();
    } else if (config.band == BAND_24GHZ) {
        gatePass = check24GHzGate();
    }
    
    if (!gatePass) {
        Serial.println("[Workflow] Gate 4: FAILED");
        transitionToState(WF_READY, "Band validation failed");
        return;
    }
    Serial.println("[Workflow] Gate 4: PASSED");
    
    // All gates passed - proceed to transmission
    Serial.println("[Workflow] ALL GATES PASSED");
    transitionToState(WF_TRANSMIT, "All gates passed");
}

void RFTestWorkflow::processTransmitState() {
    Serial.println("[Workflow] === TRANSMISSION PHASE ===");
    
    const CapturedSignalData& signal = captureBuffer[selectedSignalIndex];
    
    Serial.printf("[Workflow] Transmitting signal %d\n", selectedSignalIndex);
    Serial.printf("[Workflow]   Frequency: %.2f MHz\n", signal.frequency);
    Serial.printf("[Workflow]   Protocol: %s\n", signal.protocol);
    
    bool success = false;
    
    if (config.band == BAND_433MHZ) {
        success = transmit433MHz(signal);
    } else if (config.band == BAND_24GHZ) {
        success = transmit24GHz(signal);
    }
    
    if (success) {
        Serial.println("[Workflow] Transmission completed successfully");
    } else {
        Serial.println("[Workflow] Transmission failed");
        logError(WF_ERROR_TRANSMISSION_FAILED, "Transmission execution failed");
    }
    
    transitionToState(WF_CLEANUP, success ? "Transmit success" : "Transmit failed");
}

/**
 * @brief Process CLEANUP state - Mandatory resource cleanup
 * 
 * THREAT MITIGATION: Handles Firmware Faults
 * - Always executes before returning to IDLE (guaranteed cleanup)
 * - Disables transmitter regardless of success/failure (fail-safe)
 * - Prevents transmitter being left enabled due to errors or timeouts
 * - RAII memory management ensures buffer cleanup even on exceptions
 * 
 * This state executes on: success, failure, timeout, and emergency stop.
 */
void RFTestWorkflow::processCleanupState() {
    Serial.println("[Workflow] === CLEANUP PHASE ===");
    
    // Step 1: Disable transmitter
    Serial.println("[Workflow] Step 1: Disable transmitter");
    if (rf433Module) {
        rf433Module->setTransmitEnabled(false);
    }
    
    // Step 2: Disable receiver
    Serial.println("[Workflow] Step 2: Disable receiver");
    if (rf433Module) {
        rf433Module->stopReceiving();
    }
    
    // Step 3: Clear sensitive data (optional, depends on requirements)
    // MEMORY OWNERSHIP: Clearing vector will call destructor on each CapturedSignalData
    // which will automatically free all pulseTimes buffers (RAII)
    // captureBuffer.clear(); // Uncomment if data should not persist
    
    Serial.println("[Workflow] Cleanup complete");
    
    transitionToState(WF_IDLE, "Cleanup done");
}

// ============================================================================
// Signal Capture
// ============================================================================

void RFTestWorkflow::captureSignals() {
    if (config.band == BAND_433MHZ) {
        capture433MHzSignals();
    } else if (config.band == BAND_24GHZ) {
        capture24GHzPackets();
    }
}

void RFTestWorkflow::capture433MHzSignals() {
    if (rf433Module == nullptr) return;
    
    while (rf433Module->isSignalAvailable() && captureBuffer.size() < config.bufferSize) {
        RF433Signal rfSignal = rf433Module->receiveSignal();
        
        if (!rfSignal.isValid) continue;
        
        CapturedSignalData captured;
        convertRF433Signal(rfSignal, captured);
        
        if (validateSignal433MHz(captured)) {
            // MEMORY OWNERSHIP: Move captured signal into vector for efficient transfer
            // After this move, 'captured' will have nullptr pulseTimes (ownership transferred)
            captureBuffer.push_back(std::move(captured));
            Serial.printf("[Workflow] Captured 433 MHz signal: %lu (%d bits)\n",
                         rfSignal.value, rfSignal.bitLength);
        }
        // If not valid, captured's destructor will free any allocated buffer
    }
}

void RFTestWorkflow::capture24GHzPackets() {
    // 2.4 GHz packet capture would be implemented here
    // This would use rf24Module for ESP-NOW or nRF24 operations
    // Placeholder for future implementation
}

bool RFTestWorkflow::validateSignal433MHz(const CapturedSignalData& signal) {
    if (signal.pulseCount < WORKFLOW_433_MIN_PULSES) {
        return false;
    }
    
    if (signal.rssi != 0 && signal.rssi < -100) {
        return false;
    }
    
    return true;
}

bool RFTestWorkflow::validatePacket24GHz(const CapturedSignalData& packet) {
    if (packet.dataLength < 1 || packet.dataLength > 32) {
        return false;
    }
    
    if (packet.rssi < -90) {
        return false;
    }
    
    return true;
}

// ============================================================================
// Analysis Functions
// ============================================================================

void RFTestWorkflow::analyze433MHzSignals() {
    Serial.println("[Workflow] Performing 433 MHz analysis");
    
    for (auto& signal : captureBuffer) {
        if (signal.isValid) {
            classifyDevice433MHz(signal);
            analysisResult.validSignalCount++;
        }
    }
}

void RFTestWorkflow::analyze24GHzPackets() {
    Serial.println("[Workflow] Performing 2.4 GHz packet analysis");
    
    detectBindingPairs();
    
    for (const auto& packet : captureBuffer) {
        if (packet.isValid) {
            analysisResult.validSignalCount++;
        }
    }
}

void RFTestWorkflow::classifyDevice433MHz(CapturedSignalData& signal) {
    // Simple classification based on pulse characteristics
    float avgPulse = 0;
    // MEMORY SAFETY: Check buffer validity before accessing
    // signal owns the pulseTimes buffer, guaranteed valid during this function
    if (signal.hasPulseBuffer()) {
        for (uint16_t i = 0; i < signal.pulseCount; i++) {
            avgPulse += signal.pulseTimes[i];
        }
        avgPulse /= signal.pulseCount;
    }
    
    if (avgPulse > 400 && signal.pulseCount >= 48) {
        strncpy(signal.deviceType, "Garage Door", sizeof(signal.deviceType) - 1);
        signal.deviceType[sizeof(signal.deviceType) - 1] = '\0';
    } else if (avgPulse < 350 && signal.pulseCount < 48) {
        strncpy(signal.deviceType, "Doorbell", sizeof(signal.deviceType) - 1);
        signal.deviceType[sizeof(signal.deviceType) - 1] = '\0';
    } else if (signal.pulseCount >= 128) {
        strncpy(signal.deviceType, "Car Remote", sizeof(signal.deviceType) - 1);
        signal.deviceType[sizeof(signal.deviceType) - 1] = '\0';
    } else {
        strncpy(signal.deviceType, "Unknown", sizeof(signal.deviceType) - 1);
        signal.deviceType[sizeof(signal.deviceType) - 1] = '\0';
    }
}

void RFTestWorkflow::detectBindingPairs() {
    // Detect address pairs for 2.4 GHz binding
    // Placeholder for future implementation
}

void RFTestWorkflow::generateStatistics() {
    if (captureBuffer.empty()) return;
    
    float rssiSum = 0;
    int rssiCount = 0;
    analysisResult.minRSSI = 0;
    analysisResult.maxRSSI = -999;
    
    for (const auto& signal : captureBuffer) {
        if (signal.rssi != 0) {
            rssiSum += signal.rssi;
            rssiCount++;
            analysisResult.minRSSI = std::min(analysisResult.minRSSI, (float)signal.rssi);
            analysisResult.maxRSSI = std::max(analysisResult.maxRSSI, (float)signal.rssi);
        }
    }
    
    if (rssiCount > 0) {
        analysisResult.avgRSSI = rssiSum / rssiCount;
    }
    
    if (!captureBuffer.empty()) {
        uint32_t firstTime = captureBuffer[0].captureTime;
        uint32_t lastTime = captureBuffer[captureBuffer.size() - 1].captureTime;
        analysisResult.captureDurationMs = (lastTime - firstTime) / 1000;
    }
    
    snprintf(analysisResult.summary, sizeof(analysisResult.summary),
             "%d signals, %d valid, avg RSSI: %.1f dBm",
             analysisResult.signalCount, analysisResult.validSignalCount, analysisResult.avgRSSI);
}

// ============================================================================
// Transmission Gates
// ============================================================================

bool RFTestWorkflow::checkPolicyGate() {
    const CapturedSignalData& signal = captureBuffer[selectedSignalIndex];
    
    if (isFrequencyBlacklisted(signal.frequency)) {
        Serial.printf("[Workflow] Frequency %.2f MHz is blacklisted\n", signal.frequency);
        return false;
    }
    
    uint32_t estimatedDuration = estimateTransmissionDuration(signal);
    if (estimatedDuration > config.transmitMaxDuration) {
        Serial.printf("[Workflow] Duration %lu ms exceeds limit\n", estimatedDuration);
        return false;
    }
    
    if (!signal.isValid) {
        Serial.println("[Workflow] Signal is not valid");
        return false;
    }
    
    return true;
}

bool RFTestWorkflow::checkConfirmationGate() {
    // Wait for user confirmation or timeout
    uint32_t startTime = millis();
    
    while ((millis() - startTime) < config.txGateTimeout) {
        if (userConfirmed) {
            userConfirmed = false;  // Reset flag
            return true;
        }
        
        if (userCanceled) {
            userCanceled = false;  // Reset flag
            return false;
        }
        
        delay(10);
    }
    
    Serial.println("[Workflow] Confirmation timeout");
    return false;
}

bool RFTestWorkflow::checkRateLimitGate() {
    // Use safety module for rate limiting
    extern SafetyModule Safety;
    return Safety.isRateLimitOK();
}

bool RFTestWorkflow::check433MHzGate() {
    const CapturedSignalData& signal = captureBuffer[selectedSignalIndex];
    
    // MEMORY SAFETY: Verify pulse timing is within acceptable range
    // Use hasPulseBuffer() to safely check buffer validity
    if (signal.hasPulseBuffer()) {
        for (uint16_t i = 0; i < signal.pulseCount; i++) {
            if (signal.pulseTimes[i] < 100 || signal.pulseTimes[i] > 10000) {
                Serial.printf("[Workflow] Pulse %d out of range: %d us\n", i, signal.pulseTimes[i]);
                return false;
            }
        }
    }
    
    return true;
}

bool RFTestWorkflow::check24GHzGate() {
    const CapturedSignalData& signal = captureBuffer[selectedSignalIndex];
    
    // Verify packet structure
    if (signal.dataLength < 1 || signal.dataLength > 32) {
        Serial.printf("[Workflow] Invalid packet length: %d\n", signal.dataLength);
        return false;
    }
    
    // Verify address was observed (binding)
    if (!wasAddressObserved(signal.protocol)) {
        Serial.println("[Workflow] Address not in observed bindings");
        return false;
    }
    
    return true;
}

// ============================================================================
// Transmission Execution
// ============================================================================

bool RFTestWorkflow::transmit433MHz(const CapturedSignalData& signal) {
    if (rf433Module == nullptr) return false;
    
    // Convert to RF433Signal
    RF433Signal rfSignal;
    convertToCapturedSignal(signal, rfSignal);
    
    // Transmit through safety-checked module
    return rf433Module->transmitSignal(rfSignal, false);  // Confirmation already done in gate
}

bool RFTestWorkflow::transmit24GHz(const CapturedSignalData& packet) {
    if (rf24Module == nullptr) return false;
    
    // 2.4 GHz transmission would be implemented here
    // Placeholder for future implementation
    return false;
}

// ============================================================================
// Helper Functions
// ============================================================================

void RFTestWorkflow::logError(WorkflowError error, const char* message) {
    lastError = error;
    errorCount++;
    errorLog.push_back(String(message));
    Serial.printf("[Workflow] ERROR: %s\n", message);
}

void RFTestWorkflow::logTransition(WorkflowState fromState, WorkflowState toState, const char* reason) {
    StateTransitionLog log;
    log.fromState = fromState;
    log.toState = toState;
    log.timestamp = millis();
    strncpy(log.reason, reason, sizeof(log.reason) - 1);
    
    transitionLog.push_back(log);
}

uint32_t RFTestWorkflow::getTimeoutForState(WorkflowState state) const {
    switch (state) {
        case WF_INIT: return config.initTimeout;
        case WF_LISTENING: return config.listenMaxTime;
        case WF_ANALYZING: return config.analyzeTimeout;
        case WF_READY: return config.readyTimeout;
        case WF_TX_GATED: return config.txGateTimeout;
        case WF_TRANSMIT: return config.transmitMaxDuration;
        case WF_CLEANUP: return config.cleanupTimeout;
        default: return 0;
    }
}

void RFTestWorkflow::handleTimeout() {
    logError(WF_ERROR_TIMEOUT, "State timeout");
    
    switch (currentState) {
        case WF_INIT:
            transitionToState(WF_CLEANUP, "Init timeout");
            break;
        case WF_LISTENING:
            transitionToState(WF_ANALYZING, "Listen timeout");
            break;
        case WF_ANALYZING:
            transitionToState(WF_READY, "Analysis timeout");
            break;
        case WF_READY:
            transitionToState(WF_CLEANUP, "Ready timeout");
            break;
        case WF_TX_GATED:
            transitionToState(WF_READY, "Gate timeout");
            break;
        case WF_TRANSMIT:
            emergencyStop = true;
            transitionToState(WF_CLEANUP, "Transmit timeout");
            break;
        case WF_CLEANUP:
            transitionToState(WF_IDLE, "Cleanup timeout");
            break;
        default:
            break;
    }
}

bool RFTestWorkflow::isFrequencyBlacklisted(float frequency) const {
    for (size_t i = 0; i < sizeof(FREQ_BLACKLIST) / sizeof(FREQ_BLACKLIST[0]); i++) {
        if (std::fabs(frequency - FREQ_BLACKLIST[i]) < 0.1f) {
            return true;
        }
    }
    return false;
}

uint32_t RFTestWorkflow::estimateTransmissionDuration(const CapturedSignalData& signal) const {
    if (config.band == BAND_433MHZ) {
        uint32_t totalPulse = 0;
        // MEMORY SAFETY: Use hasPulseBuffer() to safely check buffer validity
        if (signal.hasPulseBuffer()) {
            for (uint16_t i = 0; i < signal.pulseCount; i++) {
                totalPulse += signal.pulseTimes[i];
            }
        }
        return (totalPulse * 10) / 1000;  // 10 repeats, convert to ms
    }
    return 10;  // Default 10ms for 2.4 GHz
}

bool RFTestWorkflow::wasAddressObserved(const char* address) const {
    for (const auto& signal : captureBuffer) {
        if (strcmp(signal.protocol, address) == 0) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Helper Functions (Global)
// ============================================================================

/**
 * @brief Convert RF433Signal to CapturedSignalData with proper memory management
 * 
 * MEMORY OWNERSHIP: Allocates new pulseTimes buffer owned by dst
 * Caller responsibility: Ensure dst object's lifetime manages the buffer
 * 
 * @param src Source RF433 signal (read-only)
 * @param dst Destination captured signal (takes ownership of allocated buffer)
 */
void convertRF433Signal(const RF433Signal& src, CapturedSignalData& dst) {
    dst.captureTime = src.timestamp * 1000;  // Convert to microseconds
    dst.frequency = 433.92f;
    dst.rssi = src.rssi;
    
    // Copy raw data
    dst.rawData[0] = (src.value >> 24) & 0xFF;
    dst.rawData[1] = (src.value >> 16) & 0xFF;
    dst.rawData[2] = (src.value >> 8) & 0xFF;
    dst.rawData[3] = src.value & 0xFF;
    dst.dataLength = 4;
    
    // MEMORY OWNERSHIP: Allocate pulse timing buffer
    // dst takes ownership of the allocated buffer via allocatePulseBuffer()
    // Buffer will be automatically freed when dst is destroyed (RAII)
    if (src.bitLength > 0) {
        if (dst.allocatePulseBuffer(src.bitLength)) {
            // Fill buffer with pulse timing data
            for (uint16_t i = 0; i < dst.pulseCount; i++) {
                dst.pulseTimes[i] = src.pulseLength;
            }
        } else {
            // Allocation failed: dst.pulseCount is already 0, pulseTimes is nullptr
            Serial.println("[Workflow] WARNING: Failed to allocate pulse buffer");
        }
    }
    
    snprintf(dst.protocol, sizeof(dst.protocol), "RCSwitch-%d", src.protocol);
    strncpy(dst.deviceType, src.description, sizeof(dst.deviceType) - 1);
    dst.deviceType[sizeof(dst.deviceType) - 1] = '\0';
    dst.isValid = src.isValid;
}

/**
 * @brief Convert CapturedSignalData back to RF433Signal (lossy conversion)
 * 
 * MEMORY SAFETY: Reads from src.pulseTimes but does not take ownership
 * dst is a simple struct with no dynamic allocation
 * 
 * @param src Source captured signal (read-only, retains ownership of buffers)
 * @param dst Destination RF433 signal (fixed-size, no dynamic allocation)
 */
void convertToCapturedSignal(const CapturedSignalData& src, RF433Signal& dst) {
    dst.value = (src.rawData[0] << 24) | (src.rawData[1] << 16) | 
                (src.rawData[2] << 8) | src.rawData[3];
    dst.bitLength = src.pulseCount;
    dst.protocol = 1;  // Default protocol
    // MEMORY SAFETY: Use hasPulseBuffer() to safely access pulse data
    dst.pulseLength = src.hasPulseBuffer() ? src.pulseTimes[0] : 350;
    dst.timestamp = src.captureTime / 1000;
    dst.rssi = src.rssi;
    strncpy(dst.description, src.deviceType, sizeof(dst.description) - 1);
    // Defensive: Explicitly null-terminate even though strncpy should handle it
    // Protects against non-null-terminated source strings
    dst.description[sizeof(dst.description) - 1] = '\0';
    dst.isValid = src.isValid;
}

String formatWorkflowState(WorkflowState state, uint32_t elapsedMs) {
    const char* stateName;
    switch (state) {
        case WF_IDLE: stateName = "IDLE"; break;
        case WF_INIT: stateName = "INIT"; break;
        case WF_LISTENING: stateName = "LISTENING"; break;
        case WF_ANALYZING: stateName = "ANALYZING"; break;
        case WF_READY: stateName = "READY"; break;
        case WF_TX_GATED: stateName = "TX_GATED"; break;
        case WF_TRANSMIT: stateName = "TRANSMIT"; break;
        case WF_CLEANUP: stateName = "CLEANUP"; break;
        default: stateName = "UNKNOWN"; break;
    }
    
    return String(stateName) + " (" + String(elapsedMs / 1000) + "s)";
}

String formatAnalysisResult(const AnalysisResult& result) {
    return String(result.summary);
}
