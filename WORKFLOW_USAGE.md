# RF Test Workflow Usage Guide

This guide explains how to use the RF Test Workflow system in the M5Stack RF Suite.

## Overview

The RF Test Workflow system provides a structured, safe approach to RF testing with the following key features:

- **Passive-First Methodology**: Always observe before transmitting
- **State Machine Control**: Explicit states with fail-safe transitions
- **Multi-Gate Approval**: Multiple independent safety checks before transmission
- **Complete Traceability**: Detailed logging of all operations

## Quick Start

### Basic 433 MHz Workflow

```cpp
#include "rf_test_workflow.h"

RFTestWorkflow workflow;
RF433Module rf433;
RF24Module rf24;

void setup() {
    // Initialize hardware
    rf433.begin();
    rf24.begin();
    
    // Configure workflow for 433 MHz
    WorkflowConfig config;
    config.band = BAND_433MHZ;
    config.listenMinTime = 5000;   // 5 seconds minimum observation
    config.listenMaxTime = 60000;  // 60 seconds maximum observation
    
    // Initialize and run workflow
    workflow.initialize(config, &rf433, &rf24);
    workflow.start();  // Blocks until workflow completes
    
    // Access results
    const AnalysisResult& result = workflow.getAnalysisResult();
    Serial.printf("Captured %d signals\n", result.signalCount);
}
```

### Basic 2.4 GHz Workflow

```cpp
WorkflowConfig config;
config.band = BAND_24GHZ;
config.listenMinTime = 5000;
config.listenMaxTime = 30000;

workflow.initialize(config, &rf433, &rf24);
workflow.start();
```

## Workflow States

### 1. IDLE
- Initial state, no resources allocated
- Waiting for workflow to start

### 2. INIT
- Hardware initialization
- Buffer allocation
- Configuration setup
- **Timeout**: 5 seconds
- **On Success**: Transitions to LISTENING
- **On Failure**: Transitions to CLEANUP

### 3. LISTENING (Passive Observation)
- Captures RF signals without any transmission
- Transmitter is disabled and locked
- Records signal characteristics and timing
- **Minimum Duration**: 1 second (configurable)
- **Maximum Duration**: 60 seconds (configurable)
- **Transitions**:
  - Buffer 90% full → ANALYZING
  - Maximum time reached → ANALYZING
  - User trigger → ANALYZING
  - Error → CLEANUP

### 4. ANALYZING
- Processes captured signals
- Classifies device types (433 MHz)
- Detects binding pairs (2.4 GHz)
- Generates statistics
- **Timeout**: 10 seconds
- **Transitions**:
  - Analysis complete → READY
  - Need more data → LISTENING
  - Error → CLEANUP

### 5. READY
- Results available for review
- Awaiting user decision
- **Timeout**: 120 seconds (inactivity)
- **User Options**:
  - Request transmission → TX_GATED
  - Cancel → CLEANUP
  - Continue observation → LISTENING

### 6. TX_GATED (Multi-Stage Approval)
- Gate 1: Policy Check
  - Frequency blacklist
  - Duration limits
  - Data validity
- Gate 2: User Confirmation
  - Explicit button press required
  - 10-second timeout
- Gate 3: Rate Limiting
  - Checks transmission rate
- Gate 4: Band-Specific Validation
  - 433 MHz: Pulse timing validation
  - 2.4 GHz: Address binding verification
- **Transitions**:
  - All gates pass → TRANSMIT
  - Any gate fails → READY
  - Timeout → READY

### 7. TRANSMIT
- Controlled RF transmission
- Monitored execution
- Maximum duration enforced
- **Maximum Duration**: 5 seconds
- **Transitions**:
  - Success → CLEANUP
  - Error → CLEANUP (emergency)

### 8. CLEANUP
- Resource deallocation
- Transmitter disable (always)
- Log saving
- State reset
- **Always Executes**: Regardless of success/failure
- **Transitions**: Always → IDLE

## Configuration Options

### WorkflowConfig Structure

```cpp
struct WorkflowConfig {
    RFBand band;                    // BAND_433MHZ or BAND_24GHZ
    uint32_t initTimeout;           // Init phase timeout (ms)
    uint32_t listenMinTime;         // Min observation time (ms)
    uint32_t listenMaxTime;         // Max observation time (ms)
    uint32_t analyzeTimeout;        // Analysis timeout (ms)
    uint32_t readyTimeout;          // Inactivity timeout (ms)
    uint32_t txGateTimeout;         // Confirmation timeout (ms)
    uint32_t transmitMaxDuration;   // Max TX duration (ms)
    uint32_t cleanupTimeout;        // Cleanup timeout (ms)
    uint16_t bufferSize;            // Signal buffer size
};
```

### Default Values

```cpp
WorkflowConfig config;  // Uses these defaults:
// initTimeout = 5000 ms
// listenMinTime = 1000 ms
// listenMaxTime = 60000 ms
// analyzeTimeout = 10000 ms
// readyTimeout = 120000 ms
// txGateTimeout = 10000 ms
// transmitMaxDuration = 5000 ms
// cleanupTimeout = 5000 ms
// bufferSize = 100
```

## User Interaction Methods

### During Workflow Execution

```cpp
// Trigger analysis from LISTENING state
workflow.triggerAnalysis();

// Select signal for transmission (in READY state)
workflow.selectSignalForTransmission(signalIndex);

// Confirm transmission (in TX_GATED state)
workflow.confirmTransmission();

// Cancel transmission
workflow.cancelTransmission();

// Continue observation (from READY state)
workflow.continueObservation();

// Emergency abort (any state)
workflow.abort();
```

## Accessing Results

### Get Current State

```cpp
WorkflowState state = workflow.getCurrentState();
const char* stateName = workflow.getStateName(state);
uint32_t elapsed = workflow.getStateElapsedTime();
```

### Get Captured Signals

```cpp
int count = workflow.getCapturedSignalCount();
for (int i = 0; i < count; i++) {
    const CapturedSignalData* signal = workflow.getCapturedSignal(i);
    if (signal != nullptr) {
        Serial.printf("Signal %d: %.2f MHz, RSSI: %d dBm\n",
                     i, signal->frequency, signal->rssi);
        Serial.printf("  Protocol: %s\n", signal->protocol);
        Serial.printf("  Device: %s\n", signal->deviceType);
    }
}
```

### Get Analysis Results

```cpp
const AnalysisResult& result = workflow.getAnalysisResult();
Serial.printf("Total signals: %d\n", result.signalCount);
Serial.printf("Valid signals: %d\n", result.validSignalCount);
Serial.printf("Average RSSI: %.1f dBm\n", result.avgRSSI);
Serial.printf("Capture duration: %lu ms\n", result.captureDurationMs);
Serial.printf("Summary: %s\n", result.summary);
```

### Error Handling

```cpp
if (workflow.getErrorCount() > 0) {
    WorkflowError error = workflow.getLastError();
    const char* errorMsg = workflow.getErrorString(error);
    Serial.printf("Error: %s\n", errorMsg);
}
```

### Audit Trail

```cpp
int logCount = workflow.getTransitionLogCount();
for (int i = 0; i < logCount; i++) {
    const StateTransitionLog* log = workflow.getTransitionLog(i);
    if (log != nullptr) {
        Serial.printf("Transition %d: %s -> %s (%s)\n",
                     i,
                     workflow.getStateName(log->fromState),
                     workflow.getStateName(log->toState),
                     log->reason);
    }
}
```

## Band-Specific Features

### 433 MHz (Control Packets, Pulse Analytics)

The 433 MHz workflow focuses on:
- **OOK/ASK modulation** signal capture
- **Pulse timing analysis** (microsecond precision)
- **Device classification** (garage doors, doorbells, remotes, etc.)
- **Protocol detection** (RCSwitch, Manchester, PWM)

#### 433 MHz Validation Rules

- Minimum 10 pulses per signal
- Pulse width: 100-10,000 microseconds
- RSSI threshold: > -100 dBm

#### 433 MHz Transmission Gate

Additional validation before transmission:
- All pulse timings within safe range (100-10,000 μs)
- Protocol is known and safe
- Signal integrity verified

### 2.4 GHz (Packet-Binding, Filtered Gate-Action)

The 2.4 GHz workflow focuses on:
- **Packet capture** with complete structure
- **Address binding detection** (paired devices)
- **Channel analysis** (frequency hopping patterns)
- **Data rate detection** (250kbps - 2Mbps)

#### 2.4 GHz Validation Rules

- Packet length: 1-32 bytes
- CRC verification
- RSSI threshold: > -90 dBm

#### 2.4 GHz Transmission Gate (Filtered Gate-Action)

Additional validation before transmission:
- **Address binding verification**: Destination address must have been observed
- Channel validity check (1-80)
- Packet structure validation
- Only allows transmission to observed addresses

## Safety Features

### Passive-Listening First

The workflow **always** begins with passive observation:
1. Transmitter is disabled at workflow start
2. Transmitter remains disabled through INIT and LISTENING states
3. No transmission possible until TX_GATED state
4. Multiple independent locks prevent premature transmission

### Fail-Safe Modes

Every state has timeout protection:
- If a state exceeds its timeout, automatic transition occurs
- Default behavior is always safe (no transmission)
- Emergency stop available at any time
- Cleanup state always executes, even on errors

### Multi-Gate Approval

Four independent gates must pass for transmission:

1. **Policy Gate**: System-level rules
   - Frequency not blacklisted
   - Duration within limits
   - Signal data valid

2. **User Confirmation Gate**: Human approval
   - Explicit button press required
   - 10-second timeout
   - Cancel option available

3. **Rate Limit Gate**: Transmission throttling
   - Prevents excessive transmissions
   - Configurable limit (default: 10/minute)
   - Time-window based

4. **Band-Specific Gate**: Technical validation
   - 433 MHz: Pulse timing validation
   - 2.4 GHz: Address binding verification

### Complete Traceability

All operations are logged:
- State transitions with timestamps
- Captured signals with microsecond timestamps
- Transmission attempts (approved and denied)
- Error conditions
- User actions

## Best Practices

### 1. Always Check Results

```cpp
if (workflow.start()) {
    // Workflow completed successfully
    const AnalysisResult& result = workflow.getAnalysisResult();
    // Process results...
} else {
    // Workflow had errors
    Serial.printf("Errors: %d\n", workflow.getErrorCount());
}
```

### 2. Set Appropriate Timeouts

For short tests:
```cpp
config.listenMinTime = 2000;   // 2 seconds
config.listenMaxTime = 10000;  // 10 seconds
```

For comprehensive scans:
```cpp
config.listenMinTime = 10000;  // 10 seconds
config.listenMaxTime = 300000; // 5 minutes
```

### 3. Handle User Interaction

In a UI application, respond to workflow state:
```cpp
void loop() {
    switch (workflow.getCurrentState()) {
        case WF_LISTENING:
            display.print("Listening... press B to analyze");
            if (buttonB.pressed()) {
                workflow.triggerAnalysis();
            }
            break;
            
        case WF_READY:
            display.print("Results ready. A=Exit, B=Transmit, C=Continue");
            // Handle buttons...
            break;
            
        // ... handle other states
    }
}
```

### 4. Review Captured Data Before Transmission

```cpp
// After ANALYZING completes
int count = workflow.getCapturedSignalCount();
for (int i = 0; i < count; i++) {
    const CapturedSignalData* signal = workflow.getCapturedSignal(i);
    // Display signal info to user
    // Let user choose which signal to transmit
}
```

### 5. Respect Rate Limits

```cpp
// Check rate limit before starting a new workflow
extern SafetyModule Safety;
if (!Safety.isRateLimitOK()) {
    Serial.println("Rate limit exceeded, please wait");
    delay(60000);  // Wait 1 minute
}
```

### 6. Save Important Data

```cpp
// After workflow completes, save data if needed
const AnalysisResult& result = workflow.getAnalysisResult();
// Save to SPIFFS, SD card, or transmit via WiFi
saveAnalysisResults(result);
```

## Error Recovery

### Common Errors and Solutions

#### WF_ERROR_INIT_FAILED
- **Cause**: Hardware initialization failed
- **Solution**: Check RF module connections, verify power supply

#### WF_ERROR_TIMEOUT
- **Cause**: State exceeded maximum duration
- **Solution**: Adjust timeout configuration, or indicates hardware issue

#### WF_ERROR_BUFFER_OVERFLOW
- **Cause**: Captured more signals than buffer can hold
- **Solution**: Increase `bufferSize` in config, or reduce `listenMaxTime`

#### WF_ERROR_TRANSMISSION_FAILED
- **Cause**: Hardware failure during transmission
- **Solution**: Check transmitter connections, verify transmit is enabled

#### WF_ERROR_GATE_DENIED
- **Cause**: One or more transmission gates failed
- **Solution**: Check logs to see which gate failed, adjust parameters

### Recovery Procedure

```cpp
if (workflow.getErrorCount() > 0) {
    // Log error details
    WorkflowError error = workflow.getLastError();
    Serial.printf("Error: %s\n", workflow.getErrorString(error));
    
    // Reset workflow
    workflow.reset();
    
    // Adjust configuration if needed
    WorkflowConfig newConfig;
    // ... modify config based on error type ...
    
    // Try again
    workflow.initialize(newConfig, &rf433, &rf24);
}
```

## Advanced Usage

### Custom State Handling

For more control, manually step through states:

```cpp
// Initialize workflow
workflow.initialize(config, &rf433, &rf24);

// Don't call start(), manually control states
while (workflow.getCurrentState() != WF_IDLE) {
    WorkflowState state = workflow.getCurrentState();
    
    switch (state) {
        case WF_LISTENING:
            // Custom listening logic
            if (customCondition()) {
                workflow.triggerAnalysis();
            }
            break;
            
        case WF_READY:
            // Custom ready logic
            int selectedSignal = getUserSelection();
            workflow.selectSignalForTransmission(selectedSignal);
            break;
            
        // ... handle other states
    }
    
    delay(100);
}
```

### Integration with UI

See `examples/rf_workflow_test/rf_workflow_test.ino` for a complete example with M5Stack UI integration.

## Compliance Notes

The workflow system is designed to support regulatory compliance:

- **Passive-first approach** prevents accidental interference
- **Multi-gate approval** ensures deliberate, authorized transmissions
- **Complete audit trail** provides evidence of responsible use
- **Frequency blacklist** supports restricted frequency enforcement
- **Rate limiting** prevents excessive transmissions

Users are responsible for:
- Compliance with local regulations (FCC, CE, etc.)
- Obtaining necessary licenses
- Operating within authorized frequency bands
- Respecting privacy and property rights

## Troubleshooting

### Workflow Doesn't Start
- Check that workflow is in IDLE state
- Verify RF module initialization
- Check for error messages in serial output

### No Signals Captured
- Verify RF module is connected and working
- Check that frequency band is correct
- Ensure there are actual signals in the area
- Try increasing `listenMaxTime`

### Transmission Denied
- Check which gate failed (see console output)
- Verify user confirmed transmission
- Check rate limit hasn't been exceeded
- Ensure frequency isn't blacklisted

### State Timeouts
- Adjust timeout values in configuration
- Check for hardware issues
- Verify sufficient processing power

## Dry-Run / Simulation Mode

The workflow system includes a special dry-run mode that allows you to test the complete TX_GATED → TRANSMIT workflow without actually emitting RF signals.

### What is Dry-Run Mode?

Dry-run mode executes the entire workflow including all safety gates and state transitions, but simulates the actual RF transmission. This provides:

- **Safe Testing**: No RF emissions during development or debugging
- **CI/CD Integration**: Run automated tests without hardware setup
- **Demonstrations**: Show workflow behavior safely in presentations
- **Development**: Test workflow logic without RF equipment

### Enabling Dry-Run Mode

Enable dry-run mode in the workflow configuration:

```cpp
WorkflowConfig config;
config.band = BAND_433MHZ;
config.dryRunMode = true;  // *** ENABLE DRY-RUN MODE ***

RFTestWorkflow workflow;
workflow.initialize(config, &rf433, &rf24);
workflow.start();
```

Or configure it globally in `include/config.h`:

```cpp
#define DRY_RUN_MODE true  // Enable by default
```

### What Happens in Dry-Run Mode?

When dry-run mode is enabled:

1. **All States Execute Normally**
   - INIT → LISTENING → ANALYZING → READY → TX_GATED → TRANSMIT → CLEANUP
   - State transitions occur as they would in normal operation
   - Timing and delays are preserved

2. **All Gates Are Checked**
   - Policy Gate: Validates frequency, duration, signal data
   - Confirmation Gate: Checks user confirmation
   - Rate Limit Gate: Enforces transmission limits
   - Band-Specific Gate: Performs 433 MHz or 2.4 GHz validation

3. **Transmission is Simulated**
   - No hardware module calls are made
   - RF transmitter is not enabled
   - Realistic transmission delay is simulated
   - Success is always returned (unless gates fail)

4. **Detailed Logging is Provided**
   ```
   [Workflow] *** DRY-RUN MODE ENABLED ***
   [Workflow] *** NO RF EMISSIONS WILL OCCUR ***
   [Workflow] === GATED TRANSMISSION PHASE ===
   [Workflow] *** DRY-RUN MODE - SIMULATING GATES ***
   [Workflow] Gate 1: PASSED
   [Workflow] Gate 2: PASSED
   [Workflow] Gate 3: PASSED
   [Workflow] Gate 4: PASSED
   [Workflow] === TRANSMISSION PHASE ===
   [Workflow] *** DRY-RUN MODE ACTIVE - NO RF EMISSION ***
   [Workflow] DRY-RUN: Simulating 433 MHz transmission
   [Workflow] DRY-RUN: Would transmit 433.92 MHz, 48 pulses
   [Workflow] DRY-RUN: Protocol: Protocol 1, Device: Remote Control
   [Workflow] DRY-RUN: Simulated transmission complete
   [Workflow] Simulated transmission completed successfully
   ```

### Example: Dry-Run Test

See the complete working example in `examples/dry_run_test/`:

```cpp
#include <M5Core2.h>
#include "rf_test_workflow.h"

RFTestWorkflow workflow;
RF433Module rf433;
RF24Module rf24;

void setup() {
    M5.begin();
    rf433.begin();
    rf24.begin();
    
    // Configure for dry-run
    WorkflowConfig config;
    config.band = BAND_433MHZ;
    config.dryRunMode = true;  // No RF emission
    
    workflow.initialize(config, &rf433, &rf24);
    
    // ... rest of test logic ...
}
```

### Use Cases

**Development & Testing**
- Test workflow logic without RF equipment
- Debug state transitions and gate checks
- Validate timing and error handling
- Develop new features safely

**CI/CD Pipelines**
- Automated testing without hardware
- Verify workflow integrity on every commit
- Test configuration changes
- Regression testing

**Demonstrations**
- Show workflow behavior in presentations
- Train users on the system
- Demo safety features without RF
- Educational purposes

**Debugging**
- Isolate workflow issues from RF hardware
- Test edge cases safely
- Verify gate logic
- Analyze state transition logs

### Dry-Run vs Normal Operation

| Aspect | Normal Operation | Dry-Run Mode |
|--------|------------------|--------------|
| RF Emission | ✅ Actual transmission | ❌ No RF emitted |
| State Machine | ✅ Full execution | ✅ Full execution |
| Gate Checks | ✅ All gates | ✅ All gates |
| Logging | ✅ Standard logs | ✅ Enhanced logs with "DRY-RUN" markers |
| Hardware Access | ✅ RF modules used | ❌ Hardware bypassed |
| Success Rate | Depends on conditions | ✅ Always succeeds (if gates pass) |
| Timing | ✅ Actual RF timing | ⏱️ Simulated (capped) |

### Important Notes

- Dry-run mode does NOT skip safety checks - all gates are still validated
- The workflow behaves identically except for actual RF emission
- Hardware modules must still be initialized (but won't be used for transmission)
- Dry-run mode is ideal for testing, but always test with real RF before deployment
- State transitions, timeouts, and error handling work the same way

## Additional Resources

- **WORKFLOWS.md**: Detailed state diagrams and transition rules
- **PSEUDOCODE.md**: Complete pseudocode implementation
- **examples/rf_workflow_test/**: Working example with UI
- **examples/dry_run_test/**: Dry-run mode demonstration
- **TESTING.md**: Test procedures and scenarios
- **SECURITY.md**: Security considerations and responsible use

---

**Version**: 1.0.0  
**Last Updated**: 2025-12-14  
**Status**: Complete
