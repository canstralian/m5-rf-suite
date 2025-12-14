/**
 * @file rf_test_workflow.h
 * @brief RF Test Workflow State Machine for ESP32 with RF Modules
 * 
 * Implements structured workflows for RF testing with:
 * - Initialize: System and hardware setup
 * - Passive Observation: Signal capture without transmission
 * - Analysis: Process captured data
 * - Optional Gated Transmission: Multi-stage approved transmission
 * - Cleanup: Resource deallocation and state reset
 * 
 * Supports:
 * - CC1101 at 433 MHz (control packets, pulse analytics)
 * - nRF24L01+ at 2.4 GHz (packet-binding, filtered gate-action)
 */

#ifndef RF_TEST_WORKFLOW_H
#define RF_TEST_WORKFLOW_H

#include <Arduino.h>
#include <vector>
#include "config.h"
#include "rf433_module.h"
#include "rf24_module.h"
#include "safety_module.h"

// ============================================================================
// WORKFLOW CONFIGURATION
// ============================================================================

// Timing parameters (milliseconds)
#define WORKFLOW_INIT_TIMEOUT_MS 5000
#define WORKFLOW_LISTEN_MIN_MS 1000
#define WORKFLOW_LISTEN_MAX_MS 60000
#define WORKFLOW_ANALYZE_TIMEOUT_MS 10000
#define WORKFLOW_READY_TIMEOUT_MS 120000
#define WORKFLOW_TX_GATE_TIMEOUT_MS 10000
#define WORKFLOW_TRANSMIT_MAX_MS 5000
#define WORKFLOW_CLEANUP_TIMEOUT_MS 5000

// Buffer sizes
#define WORKFLOW_SIGNAL_BUFFER_SIZE 100
#define WORKFLOW_PULSE_BUFFER_SIZE 1000

// 433 MHz specific
#define WORKFLOW_433_MIN_PULSES 10
#define WORKFLOW_433_MIN_OBSERVE_SEC 2

// 2.4 GHz specific
#define WORKFLOW_24G_MIN_PACKETS 5
#define WORKFLOW_24G_MIN_OBSERVE_SEC 5

// ============================================================================
// ENUMERATIONS
// ============================================================================

enum WorkflowState {
    WF_IDLE = 0,        // Initial resting state
    WF_INIT = 1,        // Hardware initialization
    WF_LISTENING = 2,   // Passive observation
    WF_ANALYZING = 3,   // Signal analysis
    WF_READY = 4,       // Awaiting user decision
    WF_TX_GATED = 5,    // Multi-stage transmission approval
    WF_TRANSMIT = 6,    // Active transmission
    WF_CLEANUP = 7      // Resource cleanup
};

enum RFBand {
    BAND_433MHZ = 0,
    BAND_24GHZ = 1
};

enum WorkflowError {
    WF_ERROR_NONE = 0,
    WF_ERROR_INIT_FAILED = 1,
    WF_ERROR_HARDWARE_FAILURE = 2,
    WF_ERROR_BUFFER_OVERFLOW = 3,
    WF_ERROR_TIMEOUT = 4,
    WF_ERROR_INVALID_SIGNAL = 5,
    WF_ERROR_TRANSMISSION_FAILED = 6,
    WF_ERROR_GATE_DENIED = 7
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct WorkflowConfig {
    RFBand band;
    uint32_t initTimeout;
    uint32_t listenMinTime;
    uint32_t listenMaxTime;
    uint32_t analyzeTimeout;
    uint32_t readyTimeout;
    uint32_t txGateTimeout;
    uint32_t transmitMaxDuration;
    uint32_t cleanupTimeout;
    uint16_t bufferSize;
    bool dryRunMode;  // Simulate transmissions without RF emission
    
    // Default constructor
    WorkflowConfig() :
        band(BAND_433MHZ),
        initTimeout(WORKFLOW_INIT_TIMEOUT_MS),
        listenMinTime(WORKFLOW_LISTEN_MIN_MS),
        listenMaxTime(WORKFLOW_LISTEN_MAX_MS),
        analyzeTimeout(WORKFLOW_ANALYZE_TIMEOUT_MS),
        readyTimeout(WORKFLOW_READY_TIMEOUT_MS),
        txGateTimeout(WORKFLOW_TX_GATE_TIMEOUT_MS),
        transmitMaxDuration(WORKFLOW_TRANSMIT_MAX_MS),
        cleanupTimeout(WORKFLOW_CLEANUP_TIMEOUT_MS),
        bufferSize(WORKFLOW_SIGNAL_BUFFER_SIZE),
        dryRunMode(DRY_RUN_MODE) {}
};

/**
 * @brief Captured RF signal data with RAII-managed dynamically allocated buffer
 * 
 * MEMORY OWNERSHIP RULES:
 * - This struct owns the pulseTimes buffer (if allocated)
 * - pulseTimes is dynamically allocated on heap when pulseCount > 0
 * - Destructor automatically frees the buffer (RAII pattern)
 * - Deep copy semantics: copying creates independent buffer copy
 * - Move semantics: moving transfers ownership without allocation
 * 
 * LIFETIME GUARANTEES:
 * - pulseTimes buffer lifetime tied to this object's lifetime
 * - pulseTimes is valid as long as this object exists and pulseCount > 0
 * - After move, source object's pulseTimes is nullptr (ownership transferred)
 * 
 * THREAD SAFETY:
 * - NOT thread-safe: external synchronization required for concurrent access
 * 
 * USAGE NOTES:
 * - Prefer move operations (std::move) when transferring ownership
 * - Use allocatePulseBuffer() for safe buffer allocation
 * - Use deallocatePulseBuffer() for explicit cleanup (optional)
 */
struct CapturedSignalData {
    uint32_t captureTime;       // Microseconds
    float frequency;            // MHz
    int8_t rssi;               // dBm
    uint8_t rawData[32];       // Raw data bytes
    uint16_t dataLength;       // Actual data length
    uint16_t* pulseTimes;      // Owned pointer to pulse timing buffer (may be nullptr)
    uint16_t pulseCount;       // Number of pulses in pulseTimes buffer
    char protocol[32];         // Protocol identifier
    char deviceType[32];       // Classified device type
    bool isValid;
    
    // ========================================================================
    // CONSTRUCTOR: Default initialization with no buffer allocated
    // ========================================================================
    CapturedSignalData() : 
        captureTime(0), frequency(0), rssi(0), dataLength(0),
        pulseTimes(nullptr), pulseCount(0), isValid(false) {
        memset(rawData, 0, sizeof(rawData));
        memset(protocol, 0, sizeof(protocol));
        memset(deviceType, 0, sizeof(deviceType));
    }
    
    // ========================================================================
    // COPY CONSTRUCTOR: Creates deep copy with independent buffer
    // Ownership: Allocates NEW buffer, source retains its buffer
    // Exception Safety: Strong guarantee via allocatePulseBuffer()
    // Note: pulseCount initialized to 0, set by allocatePulseBuffer() on success
    // ========================================================================
    CapturedSignalData(const CapturedSignalData& other) :
        captureTime(other.captureTime), frequency(other.frequency),
        rssi(other.rssi), dataLength(other.dataLength),
        pulseTimes(nullptr), pulseCount(0),
        isValid(other.isValid) {
        memcpy(rawData, other.rawData, sizeof(rawData));
        memcpy(protocol, other.protocol, sizeof(protocol));
        memcpy(deviceType, other.deviceType, sizeof(deviceType));
        // Deep copy: Allocate independent buffer with exception safety
        // allocatePulseBuffer() sets pulseCount on successful allocation
        if (other.pulseTimes != nullptr && other.pulseCount > 0) {
            if (allocatePulseBuffer(other.pulseCount)) {
                memcpy(pulseTimes, other.pulseTimes, other.pulseCount * sizeof(uint16_t));
            }
            // If allocation fails, object is still in valid state (nullptr, count=0)
        }
    }
    
    // ========================================================================
    // COPY ASSIGNMENT: Creates deep copy with independent buffer
    // Ownership: Frees existing buffer, allocates NEW buffer
    // Exception Safety: Strong guarantee via allocatePulseBuffer()
    // ========================================================================
    CapturedSignalData& operator=(const CapturedSignalData& other) {
        if (this != &other) {
            // Release current resources (RAII cleanup)
            deallocatePulseBuffer();
            
            // Copy scalar and array data
            captureTime = other.captureTime;
            frequency = other.frequency;
            rssi = other.rssi;
            dataLength = other.dataLength;
            isValid = other.isValid;
            memcpy(rawData, other.rawData, sizeof(rawData));
            memcpy(protocol, other.protocol, sizeof(protocol));
            memcpy(deviceType, other.deviceType, sizeof(deviceType));
            
            // Deep copy: Allocate independent buffer with exception safety
            if (other.pulseTimes != nullptr && other.pulseCount > 0) {
                if (allocatePulseBuffer(other.pulseCount)) {
                    memcpy(pulseTimes, other.pulseTimes, other.pulseCount * sizeof(uint16_t));
                }
                // If allocation fails, object is in valid state (nullptr, count=0)
            }
        }
        return *this;
    }
    
    // ========================================================================
    // MOVE CONSTRUCTOR: Transfers ownership without allocation
    // Ownership: Takes buffer from source, source loses ownership
    // ========================================================================
    CapturedSignalData(CapturedSignalData&& other) noexcept :
        captureTime(other.captureTime), frequency(other.frequency),
        rssi(other.rssi), dataLength(other.dataLength),
        pulseTimes(other.pulseTimes), pulseCount(other.pulseCount),
        isValid(other.isValid) {
        memcpy(rawData, other.rawData, sizeof(rawData));
        memcpy(protocol, other.protocol, sizeof(protocol));
        memcpy(deviceType, other.deviceType, sizeof(deviceType));
        // Transfer ownership: nullify source pointer
        other.pulseTimes = nullptr;
        other.pulseCount = 0;
    }
    
    // ========================================================================
    // MOVE ASSIGNMENT: Transfers ownership without allocation
    // Ownership: Frees existing buffer, takes buffer from source
    // ========================================================================
    CapturedSignalData& operator=(CapturedSignalData&& other) noexcept {
        if (this != &other) {
            // Release current resources (RAII cleanup)
            deallocatePulseBuffer();
            
            // Transfer scalar and array data
            captureTime = other.captureTime;
            frequency = other.frequency;
            rssi = other.rssi;
            dataLength = other.dataLength;
            isValid = other.isValid;
            memcpy(rawData, other.rawData, sizeof(rawData));
            memcpy(protocol, other.protocol, sizeof(protocol));
            memcpy(deviceType, other.deviceType, sizeof(deviceType));
            
            // Transfer ownership: take buffer pointer and count from source
            pulseTimes = other.pulseTimes;
            pulseCount = other.pulseCount;
            
            // Nullify source to prevent double-free
            other.pulseTimes = nullptr;
            other.pulseCount = 0;
        }
        return *this;
    }
    
    // ========================================================================
    // DESTRUCTOR: Automatically frees owned buffer (RAII)
    // ========================================================================
    ~CapturedSignalData() {
        deallocatePulseBuffer();
    }
    
    // ========================================================================
    // HELPER METHODS: Safe buffer management
    // ========================================================================
    
    /**
     * @brief Allocate pulse timing buffer with specified count
     * @param count Number of uint16_t elements to allocate
     * @return true if allocation succeeded, false on failure
     * 
     * Ownership: This object owns the allocated buffer
     * Note: Frees any existing buffer before allocating new one
     */
    bool allocatePulseBuffer(uint16_t count) {
        if (count == 0) {
            deallocatePulseBuffer();
            return true;
        }
        
        // Free existing buffer if present
        deallocatePulseBuffer();
        
        // Allocate new buffer
        try {
            pulseTimes = new uint16_t[count];
            pulseCount = count;
            // Zero-initialize for safety
            memset(pulseTimes, 0, count * sizeof(uint16_t));
            return true;
        } catch (...) {
            // Allocation failed: ensure consistent state
            pulseTimes = nullptr;
            pulseCount = 0;
            return false;
        }
    }
    
    /**
     * @brief Explicitly deallocate pulse timing buffer
     * 
     * Note: Called automatically by destructor, manual call is optional
     * Safe to call multiple times (idempotent)
     */
    void deallocatePulseBuffer() {
        if (pulseTimes != nullptr) {
            delete[] pulseTimes;
            pulseTimes = nullptr;
        }
        pulseCount = 0;
    }
    
    /**
     * @brief Check if pulse buffer is allocated and valid
     * @return true if buffer exists and pulseCount > 0
     */
    bool hasPulseBuffer() const {
        return pulseTimes != nullptr && pulseCount > 0;
    }
};

struct AnalysisResult {
    uint16_t signalCount;
    uint16_t validSignalCount;
    uint16_t uniquePatterns;
    float avgRSSI;
    float minRSSI;
    float maxRSSI;
    uint32_t captureDurationMs;
    uint32_t analysisTime;
    bool complete;
    char summary[256];
    
    AnalysisResult() :
        signalCount(0), validSignalCount(0), uniquePatterns(0),
        avgRSSI(0), minRSSI(0), maxRSSI(0),
        captureDurationMs(0), analysisTime(0), complete(false) {
        memset(summary, 0, sizeof(summary));
    }
};

struct StateTransitionLog {
    WorkflowState fromState;
    WorkflowState toState;
    uint32_t timestamp;
    char reason[64];
    
    StateTransitionLog() : fromState(WF_IDLE), toState(WF_IDLE), timestamp(0) {
        memset(reason, 0, sizeof(reason));
    }
};

// ============================================================================
// RF TEST WORKFLOW CLASS
// ============================================================================

class RFTestWorkflow {
public:
    RFTestWorkflow();
    ~RFTestWorkflow();
    
    // ========================================================================
    // Initialization and Control
    // ========================================================================
    bool initialize(const WorkflowConfig& config, RF433Module* rf433, RF24Module* rf24);
    bool start();
    void abort();
    void reset();
    
    // ========================================================================
    // State Information
    // ========================================================================
    WorkflowState getCurrentState() const;
    const char* getStateName(WorkflowState state) const;
    uint32_t getStateElapsedTime() const;
    bool isRunning() const;
    
    // ========================================================================
    // Results and Data Access
    // ========================================================================
    const AnalysisResult& getAnalysisResult() const;
    int getCapturedSignalCount() const;
    const CapturedSignalData* getCapturedSignal(int index) const;
    
    // ========================================================================
    // User Interaction
    // ========================================================================
    void triggerAnalysis();  // Manual trigger from LISTENING state
    void selectSignalForTransmission(int index);
    void confirmTransmission();
    void cancelTransmission();
    void continueObservation();
    
    // ========================================================================
    // Error Handling
    // ========================================================================
    WorkflowError getLastError() const;
    const char* getErrorString(WorkflowError error) const;
    int getErrorCount() const;
    
    // ========================================================================
    // Logging and Audit
    // ========================================================================
    int getTransitionLogCount() const;
    const StateTransitionLog* getTransitionLog(int index) const;
    void clearLogs();
    
private:
    // ========================================================================
    // State Machine
    // ========================================================================
    void processCurrentState();
    void transitionToState(WorkflowState newState, const char* reason = "");
    bool checkTimeout();
    void checkEmergencyStop();
    
    // ========================================================================
    // State Processors
    // ========================================================================
    void processIdleState();
    void processInitState();
    void processListeningState();
    void processAnalyzingState();
    void processReadyState();
    void processTxGatedState();
    void processTransmitState();
    void processCleanupState();
    
    // ========================================================================
    // Signal Capture
    // ========================================================================
    void captureSignals();
    void capture433MHzSignals();
    void capture24GHzPackets();
    bool validateSignal433MHz(const CapturedSignalData& signal);
    bool validatePacket24GHz(const CapturedSignalData& packet);
    
    // ========================================================================
    // Analysis Functions
    // ========================================================================
    void analyze433MHzSignals();
    void analyze24GHzPackets();
    void classifyDevice433MHz(CapturedSignalData& signal);
    void detectBindingPairs();
    void generateStatistics();
    
    // ========================================================================
    // Transmission Gates
    // ========================================================================
    bool checkPolicyGate();
    bool checkConfirmationGate();
    bool checkRateLimitGate();
    bool check433MHzGate();
    bool check24GHzGate();
    
    // ========================================================================
    // Transmission Execution
    // ========================================================================
    bool transmit433MHz(const CapturedSignalData& signal);
    bool transmit24GHz(const CapturedSignalData& packet);
    
    // ========================================================================
    // Helper Functions
    // ========================================================================
    void logError(WorkflowError error, const char* message);
    void logTransition(WorkflowState fromState, WorkflowState toState, const char* reason);
    uint32_t getTimeoutForState(WorkflowState state) const;
    void handleTimeout();
    bool isFrequencyBlacklisted(float frequency) const;
    uint32_t estimateTransmissionDuration(const CapturedSignalData& signal) const;
    bool wasAddressObserved(const char* address) const;
    
    // ========================================================================
    // Member Variables
    // ========================================================================
    WorkflowConfig config;
    WorkflowState currentState;
    WorkflowState previousState;
    uint32_t stateEntryTime;
    bool running;
    bool emergencyStop;
    
    // Hardware interfaces
    RF433Module* rf433Module;
    RF24Module* rf24Module;
    
    // Captured data
    // MEMORY OWNERSHIP: Vector owns all CapturedSignalData objects and their buffers
    // Each CapturedSignalData manages its own pulseTimes buffer via RAII
    // Vector destruction will automatically free all signal buffers
    std::vector<CapturedSignalData> captureBuffer;
    AnalysisResult analysisResult;
    
    // Transmission state
    int selectedSignalIndex;
    bool userConfirmed;
    bool userCanceled;
    uint8_t transmissionAttempts;
    
    // Error tracking
    WorkflowError lastError;
    int errorCount;
    std::vector<String> errorLog;
    
    // Audit trail
    std::vector<StateTransitionLog> transitionLog;
    
    // Timing
    uint32_t workflowStartTime;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Convert RF433Signal to CapturedSignalData
void convertRF433Signal(const RF433Signal& src, CapturedSignalData& dst);

// Convert CapturedSignalData to RF433Signal
void convertToCapturedSignal(const CapturedSignalData& src, RF433Signal& dst);

// Format workflow state for display
String formatWorkflowState(WorkflowState state, uint32_t elapsedMs);

// Format analysis result summary
String formatAnalysisResult(const AnalysisResult& result);

#endif // RF_TEST_WORKFLOW_H
