# Threat Mapping and Security Defense Documentation

## Overview

This document provides a comprehensive mapping between the security threats that the M5Stack RF Suite addresses and the specific defense mechanisms implemented in the codebase. Each threat class is explicitly linked to the code features that mitigate it, providing clear justification for the design decisions and implementation details.

## Purpose

The RF Suite implements a "defense-in-depth" security architecture where multiple independent layers work together to prevent unauthorized, accidental, or malicious RF transmissions. This document serves to:

1. **Clearly identify** the threat classes we defend against
2. **Explicitly map** each threat to specific mitigation strategies
3. **Document** where in the code these defenses are implemented
4. **Justify** the design decisions made for safety and security

## Threat Classes and Mitigations

### 1. Accidental Replay Attacks

#### Threat Description
**Risk Level**: HIGH

An accidental replay attack occurs when a captured RF signal is unintentionally retransmitted, potentially causing:
- Unintended activation of garage doors, alarms, or other RF-controlled devices
- Interference with legitimate operations
- Security breaches in access control systems
- Privacy violations

**Attack Scenario**: User captures a signal (e.g., garage door opener), then accidentally triggers a replay without realizing the consequences.

#### Defense Mechanisms

##### 1.1 Multi-Stage Gated Transmission (TX_GATED State)
**Implementation**: `src/rf_test_workflow.cpp`, lines 452-515

The workflow enforces a multi-gate approval process before any transmission:

```
Gate 1: Policy Validation    → Frequency blacklist, duration limits, signal validity
Gate 2: User Confirmation     → Explicit user button press with timeout
Gate 3: Rate Limiting         → Prevents rapid-fire replays
Gate 4: Band-Specific Check   → Protocol-specific validation
```

**Code Reference**: 
- `RFTestWorkflow::processTxGatedState()` - Orchestrates all gate checks
- Each gate must explicitly PASS before transmission proceeds

##### 1.2 Mandatory User Confirmation
**Implementation**: `src/safety_module.cpp`, lines 46-73, 89-118

```cpp
// From safety_module.cpp
TransmitPermission SafetyModule::checkTransmitPolicy(const TransmitRequest& request) {
    if (requireConfirmation && !request.confirmed) {
        return PERMIT_DENIED_NO_CONFIRMATION;
    }
    // Additional checks...
}
```

**Features**:
- Orange warning dialog displayed with clear messaging
- Requires explicit button press (Button B)
- Cannot be bypassed through UI
- Default timeout of 10 seconds prevents stale confirmations

**Code Reference**:
- `SafetyModule::requestUserConfirmation()` - Initiates confirmation dialog
- `SafetyModule::waitForUserConfirmation()` - Enforces timeout
- `RFTestWorkflow::checkConfirmationGate()` - Workflow-level confirmation check

##### 1.3 Timeout Protection
**Implementation**: `src/safety_module.cpp`, lines 274-280

**Features**:
- Confirmation requests auto-expire after 10 seconds (configurable)
- Prevents accidental transmission from stale/forgotten dialogs
- User must re-initiate transmission if timeout occurs

**Code Reference**:
- `SafetyModule::checkTimeout()` - Validates confirmation freshness
- `TRANSMIT_TIMEOUT` configuration constant

##### 1.4 Audit Logging
**Implementation**: `src/safety_module.cpp`, lines 133-160

Every transmission attempt is logged with:
- Timestamp
- Frequency and duration
- Allowed/Denied status
- Reason for decision

**Features**:
- Circular buffer of 100 entries
- Tracks both successful and denied attempts
- Provides accountability trail
- Can be reviewed to detect unintended replays

**Code Reference**:
- `SafetyModule::logTransmitAttempt()` - Records all attempts
- `SafetyModule::getRecentLogs()` - Retrieves audit trail

##### 1.5 Rate Limiting
**Implementation**: `src/safety_module.cpp`, lines 84-87, 253-260

**Features**:
- Default limit: 10 transmissions per minute
- Rolling 60-second window
- Prevents rapid accidental replays
- Blocks transmission if limit exceeded

**Code Reference**:
- `SafetyModule::isRateLimitOK()` - Checks current rate
- `SafetyModule::cleanupOldTransmits()` - Maintains rolling window
- `RFTestWorkflow::checkRateLimitGate()` - Workflow-level rate check

#### Summary: Accidental Replay Mitigation
**Defense Layers**: 5 independent mechanisms
**Key Principle**: Multiple explicit human interventions required
**Failure Mode**: Defaults to DENY (fail-safe)

---

### 2. Blind Broadcast Prevention

#### Threat Description
**Risk Level**: HIGH

Blind broadcasting occurs when RF signals are transmitted without prior observation or understanding of the spectrum environment, potentially causing:
- Interference with existing communications
- Jamming of critical services
- Unintended device activation
- Violation of regulatory compliance (spectrum etiquette)

**Attack Scenario**: User transmits on a frequency without first listening to verify it's not in use by others, causing interference.

#### Defense Mechanisms

##### 2.1 Passive-Listening-First Workflow
**Implementation**: `src/rf_test_workflow.cpp`, state machine architecture

The workflow state machine **enforces** observation before transmission:

```
IDLE → INIT → LISTENING → ANALYZING → READY → TX_GATED → TRANSMIT
         ↑                   ↓
         └─── Must observe first
```

**Key Constraints**:
- Cannot reach TRANSMIT state without passing through LISTENING
- Minimum listening duration enforced (configurable)
- Captured signals analyzed before any transmission allowed

**Code Reference**:
- `RFTestWorkflow::processListeningState()` - Passive observation phase
- `WORKFLOW_LISTEN_MIN_MS` (1 second minimum)
- `WORKFLOW_LISTEN_MAX_MS` (60 second maximum)

##### 2.2 Signal Analysis Before Transmission
**Implementation**: `src/rf_test_workflow.cpp`, lines 636-695

**Features**:
- ANALYZING state processes captured signals before user can select for transmission
- Signal classification (device type detection)
- RSSI analysis to assess spectrum congestion
- Pattern detection to identify active communications

**Code Reference**:
- `RFTestWorkflow::processAnalyzingState()` - Analysis orchestration
- `analyze433MHzSignals()` - 433 MHz specific analysis
- `analyze24GHzPackets()` - 2.4 GHz specific analysis
- `classifyDevice433MHz()` - Device identification

##### 2.3 Observed Address Binding (2.4 GHz)
**Implementation**: `src/rf_test_workflow.cpp`, lines 795-811

For 2.4 GHz operations, the system enforces "observed binding":
- Only transmit to addresses that were previously observed during LISTENING
- Prevents blind broadcast to unknown/unobserved devices
- Ensures transmission target was active in recent observation window

**Code Reference**:
- `RFTestWorkflow::check24GHzGate()` - Validates observed binding
- `wasAddressObserved()` - Checks address observation history

##### 2.4 Frequency Validation
**Implementation**: `src/rf_test_workflow.cpp`, lines 728-748

Policy gate validates:
- Signal frequency is not blacklisted
- Frequency falls within legal ISM bands
- Transmission parameters are reasonable

**Code Reference**:
- `RFTestWorkflow::checkPolicyGate()` - Frequency validation
- `isFrequencyBlacklisted()` - Blacklist checking

##### 2.5 Spectrum Analysis Statistics
**Implementation**: `src/rf_test_workflow.cpp`, lines 683-722

System provides visibility into spectrum environment:
- Number of signals detected
- RSSI distribution (min, max, average)
- Capture duration
- Pattern uniqueness metrics

**Benefits**:
- User awareness of spectrum activity
- Informed decision-making before transmission
- Detection of congested/busy frequencies

**Code Reference**:
- `RFTestWorkflow::generateStatistics()` - Computes spectrum metrics
- `AnalysisResult` structure - Holds analysis data

#### Summary: Blind Broadcast Prevention
**Defense Layers**: 5 independent mechanisms
**Key Principle**: Observe first, understand environment, then transmit
**Failure Mode**: Cannot transmit without prior observation

---

### 3. User Error Minimization

#### Threat Description
**Risk Level**: MEDIUM

User errors can lead to unintended transmissions, incorrect signal selection, or misuse of the device:
- Selecting wrong signal for replay
- Misunderstanding device functionality
- Accidental button presses
- Lack of awareness about transmission consequences

**Attack Scenario**: User navigates UI quickly, accidentally confirms a transmission they didn't intend.

#### Defense Mechanisms

##### 3.1 Clear Confirmation Dialogs
**Implementation**: UI layer with safety module integration

**Features**:
- Orange warning background (high visibility)
- "TRANSMIT WARNING" message
- Clear instructions: "Press B to confirm" or "Press A to cancel"
- Countdown timer displayed (if implemented)
- Cannot proceed without explicit choice

**Benefits**:
- Prevents accidental confirmations through clear UI
- User must consciously acknowledge action
- High-contrast warning draws attention

##### 3.2 Timeout on Confirmations
**Implementation**: `src/safety_module.cpp`, lines 99-118

**Features**:
- 10-second timeout on all confirmation dialogs
- Dialog auto-closes after timeout
- No transmission occurs on timeout
- User must re-initiate if distracted

**Benefits**:
- Prevents transmission if user walks away
- Catches situations where user forgot about pending action
- Forces fresh intentional action

**Code Reference**:
- `SafetyModule::waitForUserConfirmation()` - Enforces timeout
- `RFTestWorkflow::checkConfirmationGate()` - Workflow timeout check

##### 3.3 Input Validation
**Implementation**: Throughout codebase

**Signal Validation** (`src/rf_test_workflow.cpp`, lines 609-631):
- 433 MHz: Minimum pulse count, RSSI thresholds
- 2.4 GHz: Packet length validation, RSSI validation
- Invalid signals rejected before user can select them

**Pulse Timing Validation** (`src/rf_test_workflow.cpp`, lines 778-793):
- Pulse duration range checking (100-10000 microseconds)
- Out-of-range pulses rejected
- Prevents transmission of corrupted data

**Benefits**:
- Reduces risk of transmitting invalid/corrupted signals
- Provides early error detection
- Prevents user from selecting bad data

##### 3.4 State Machine Constraints
**Implementation**: `src/rf_test_workflow.cpp`, state machine

**Features**:
- Only valid state transitions allowed
- Cannot skip required states (e.g., LISTENING)
- Each state has timeout protection
- Emergency stop available at any time

**Benefits**:
- Prevents UI navigation errors
- Ensures proper workflow sequence
- Handles firmware faults gracefully

**Code Reference**:
- `RFTestWorkflow::transitionToState()` - Enforces valid transitions
- `checkTimeout()` - Per-state timeout protection
- `checkEmergencyStop()` - User abort capability

##### 3.5 Comprehensive Logging for Review
**Implementation**: `src/rf_test_workflow.cpp`, lines 237-253

**Features**:
- State transition logging
- Timestamps for all transitions
- Reason strings for each transition
- Error logging with context

**Benefits**:
- User can review what happened
- Debugging support for unexpected behavior
- Audit trail for security review

**Code Reference**:
- `RFTestWorkflow::logTransition()` - Records state changes
- `StateTransitionLog` structure - Audit data
- `getTransitionLog()` - Retrieves history

#### Summary: User Error Minimization
**Defense Layers**: 5 independent mechanisms
**Key Principle**: Clear feedback, validation, and timeouts
**Failure Mode**: Timeout or explicit cancellation

---

### 4. Firmware Fault Handling

#### Threat Description
**Risk Level**: MEDIUM

Firmware faults can occur due to:
- Hardware failures (RF module malfunction)
- Memory corruption
- Buffer overflows
- State machine deadlocks
- Unexpected exceptions

**Attack Scenario**: RF module fails during transmission, leaving transmitter enabled indefinitely.

#### Defense Mechanisms

##### 4.1 RAII Memory Management
**Implementation**: `include/rf_test_workflow.h`, lines 114-326

**Features**:
- `CapturedSignalData` uses RAII for pulse buffer management
- Automatic cleanup in destructor
- Move semantics for efficient transfer
- No manual memory management required

**Benefits**:
- Prevents memory leaks even on exceptions
- Buffer freed automatically when signal object destroyed
- Safe even if firmware crashes mid-operation

**Code Reference**:
- `CapturedSignalData::~CapturedSignalData()` - Auto cleanup
- `allocatePulseBuffer()` / `deallocatePulseBuffer()` - Safe allocation
- Deep copy and move constructors - Ownership clarity

##### 4.2 State Machine Timeout Protection
**Implementation**: `src/rf_test_workflow.cpp`, lines 299-310

**Features**:
- Every state has a maximum timeout
- Automatic transition to CLEANUP on timeout
- Prevents infinite loops or deadlocks
- Emergency stop mechanism available

**Code Reference**:
- `RFTestWorkflow::checkTimeout()` - Per-state timeout monitoring
- `getTimeoutForState()` - State-specific timeout values
- `handleTimeout()` - Timeout recovery logic

##### 4.3 Mandatory Cleanup State
**Implementation**: `src/rf_test_workflow.cpp`, lines 544-567

**Features**:
- CLEANUP state **always** executes before returning to IDLE
- Disables transmitter (fail-safe)
- Disables receiver
- Releases resources
- Executes regardless of success/failure

**Benefits**:
- Transmitter never left enabled
- Resources always released
- System returns to safe state
- Works even on errors/timeouts

**Code Reference**:
- `RFTestWorkflow::processCleanupState()` - Cleanup orchestration
- Called on success, failure, timeout, and emergency stop

##### 4.4 Error Counting and Emergency Abort
**Implementation**: `src/rf_test_workflow.cpp`, lines 76-84

**Features**:
- Error counter incremented on each error
- Threshold check: >10 errors forces CLEANUP
- Emergency stop flag checked in main loop
- User can abort at any time

**Benefits**:
- Prevents infinite error loops
- Handles cascading failures gracefully
- User always has emergency exit

**Code Reference**:
- `errorCount` tracking in workflow
- `emergencyStop` flag
- `checkEmergencyStop()` - Monitors abort request

##### 4.5 Hardware Initialization Validation
**Implementation**: `src/rf_test_workflow.cpp`, lines 328-368

**Features**:
- INIT state validates hardware before proceeding
- Checks RF module availability
- Verifies initialization success
- Transitions to CLEANUP on failure (not TRANSMIT)

**Benefits**:
- Prevents operations with failed hardware
- Early detection of hardware faults
- Safe fallback on initialization failure

**Code Reference**:
- `RFTestWorkflow::processInitState()` - Hardware validation
- Module existence checks before use
- Graceful failure handling

##### 4.6 Signal Validation Before Transmission
**Implementation**: `src/rf_test_workflow.cpp`, lines 728-748

Policy gate validates signal before allowing transmission:
- Signal marked as valid
- Frequency not blacklisted
- Duration within limits
- All checks pass before proceeding

**Benefits**:
- Corrupted signals rejected
- Invalid state detected early
- Prevents transmission of garbage data

**Code Reference**:
- `RFTestWorkflow::checkPolicyGate()` - Validates signal integrity
- `signal.isValid` flag checked

##### 4.7 Null Pointer Protection
**Implementation**: Throughout codebase

**Features**:
- All RF module pointers checked before use
- Null checks in every state processor
- Safe returns on null modules
- No undefined behavior on null access

**Examples**:
```cpp
if (rf433Module == nullptr) return false;
if (rf24Module == nullptr) return;
```

**Code Reference**:
- Module pointer validation in all methods
- Defensive programming throughout

#### Summary: Firmware Fault Handling
**Defense Layers**: 7 independent mechanisms
**Key Principle**: Fail-safe defaults, automatic cleanup, resource management
**Failure Mode**: Transition to CLEANUP, disable transmitter, log error

---

## Defense-in-Depth Summary

### Security Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    USER INTENT                               │
│  (User requests transmission)                                │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              PASSIVE OBSERVATION                             │
│  • Must listen first (LISTENING state)                       │
│  • Minimum observation time enforced                         │
│  • Spectrum analysis performed                               │
│  Mitigates: Blind Broadcast                                  │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              SIGNAL ANALYSIS                                 │
│  • Signal validation (pulse timing, RSSI)                    │
│  • Device classification                                     │
│  • Pattern detection                                         │
│  Mitigates: User Error, Firmware Fault                       │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              POLICY GATE (Gate 1)                            │
│  • Frequency blacklist check                                 │
│  • Duration limit validation                                 │
│  • Signal validity check                                     │
│  Mitigates: Blind Broadcast, Accidental Replay              │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              CONFIRMATION GATE (Gate 2)                      │
│  • User confirmation dialog                                  │
│  • 10-second timeout protection                              │
│  • Explicit button press required                            │
│  Mitigates: Accidental Replay, User Error                   │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              RATE LIMIT GATE (Gate 3)                        │
│  • 10 transmissions per minute limit                         │
│  • Rolling 60-second window                                  │
│  Mitigates: Accidental Replay                               │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              BAND-SPECIFIC GATE (Gate 4)                     │
│  • 433 MHz: Pulse timing validation                          │
│  • 2.4 GHz: Address binding validation                       │
│  Mitigates: Blind Broadcast, User Error                     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              TRANSMISSION                                    │
│  • Time-limited execution                                    │
│  • Audit logging                                             │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              MANDATORY CLEANUP                               │
│  • Disable transmitter (always)                              │
│  • Release resources (RAII)                                  │
│  • Return to safe IDLE state                                 │
│  Mitigates: Firmware Fault                                  │
└─────────────────────────────────────────────────────────────┘
```

### Cross-Reference Matrix

| Threat Class         | Defense Mechanism                    | Code Location                          | State/Layer           |
|---------------------|--------------------------------------|----------------------------------------|----------------------|
| Accidental Replay   | Multi-stage gating                   | `rf_test_workflow.cpp:452-515`        | TX_GATED             |
| Accidental Replay   | User confirmation                    | `safety_module.cpp:89-118`            | Gate 2               |
| Accidental Replay   | Timeout protection                   | `safety_module.cpp:274-280`           | Gate 2               |
| Accidental Replay   | Rate limiting                        | `safety_module.cpp:84-87`             | Gate 3               |
| Accidental Replay   | Audit logging                        | `safety_module.cpp:133-160`           | All states           |
| Blind Broadcast     | Passive-listening-first              | `rf_test_workflow.cpp:371-409`        | LISTENING            |
| Blind Broadcast     | Signal analysis                      | `rf_test_workflow.cpp:636-695`        | ANALYZING            |
| Blind Broadcast     | Observed address binding             | `rf_test_workflow.cpp:795-811`        | Gate 4               |
| Blind Broadcast     | Frequency validation                 | `rf_test_workflow.cpp:728-748`        | Gate 1               |
| Blind Broadcast     | Spectrum statistics                  | `rf_test_workflow.cpp:683-722`        | ANALYZING            |
| User Error          | Clear confirmation dialogs           | UI + `safety_module.cpp`              | Gate 2               |
| User Error          | Timeout on confirmations             | `safety_module.cpp:99-118`            | Gate 2               |
| User Error          | Input validation                     | `rf_test_workflow.cpp:609-631`        | ANALYZING            |
| User Error          | State machine constraints            | `rf_test_workflow.cpp:259-310`        | All states           |
| User Error          | Comprehensive logging                | `rf_test_workflow.cpp:237-253`        | All states           |
| Firmware Fault      | RAII memory management               | `rf_test_workflow.h:114-326`          | All states           |
| Firmware Fault      | State timeout protection             | `rf_test_workflow.cpp:299-310`        | All states           |
| Firmware Fault      | Mandatory cleanup                    | `rf_test_workflow.cpp:544-567`        | CLEANUP              |
| Firmware Fault      | Error counting & abort               | `rf_test_workflow.cpp:76-84`          | Main loop            |
| Firmware Fault      | Hardware validation                  | `rf_test_workflow.cpp:328-368`        | INIT                 |
| Firmware Fault      | Signal validation                    | `rf_test_workflow.cpp:728-748`        | Gate 1               |
| Firmware Fault      | Null pointer protection              | Throughout codebase                   | All states           |

## Implementation Guidelines

### For Developers

When modifying code that relates to transmission safety:

1. **Always maintain gate order**: Policy → Confirmation → Rate Limit → Band-Specific
2. **Never bypass gates**: All four gates must pass for transmission to proceed
3. **Maintain fail-safe defaults**: Any gate failure must result in transmission denial
4. **Preserve state machine flow**: Never allow direct IDLE → TRANSMIT transitions
5. **Keep audit logging**: Every transmission attempt (success or failure) must be logged
6. **Test timeout paths**: Ensure all timeout scenarios lead to safe states
7. **Validate all inputs**: Assume all external data is potentially corrupted

### For Code Reviewers

When reviewing PRs that touch safety-critical code:

1. **Verify gate integrity**: Confirm all four gates still execute in order
2. **Check fail-safe behavior**: Ensure failures default to denial
3. **Review state transitions**: No shortcuts that bypass LISTENING or gates
4. **Audit log coverage**: All decision points should be logged
5. **Timeout handling**: All blocking operations must have timeouts
6. **CLEANUP guarantee**: Ensure CLEANUP always executes before IDLE
7. **Memory safety**: Check RAII patterns are maintained

### For Security Auditors

When performing security review:

1. **Threat coverage**: Verify all four threat classes are addressed
2. **Defense depth**: Confirm multiple layers exist for each threat
3. **Bypass resistance**: Attempt to find paths that skip gates or states
4. **Timeout validation**: Test all timeout values are reasonable
5. **Logging completeness**: Ensure audit trail is comprehensive
6. **Failure modes**: Test behavior under fault injection
7. **Compliance**: Verify frequency blacklist includes prohibited bands

## Testing Threat Mitigations

See `TESTING.md` for existing test scenarios. Additional threat-specific tests are documented in the test suite (when implemented).

### Recommended Test Cases

1. **Accidental Replay Prevention**:
   - Test timeout on confirmation dialog
   - Test rate limiting (11th transmission in 60 seconds)
   - Test cancellation prevents transmission
   - Verify audit log captures all attempts

2. **Blind Broadcast Prevention**:
   - Test cannot transmit without LISTENING state
   - Test observed address binding enforcement
   - Verify frequency blacklist is enforced
   - Test signal analysis completes before READY

3. **User Error Handling**:
   - Test rapid button presses during confirmation
   - Test invalid signal selection is rejected
   - Test timeout recovers gracefully
   - Test emergency stop works in all states

4. **Firmware Fault Resilience**:
   - Test CLEANUP executes on error
   - Test state timeout recovery
   - Test hardware initialization failure
   - Test memory cleanup (no leaks)
   - Test null pointer handling

## Compliance and Regulatory Notes

### FCC Compliance (United States)

- Frequency blacklist includes aviation emergency (121.5 MHz)
- Frequency blacklist includes military emergency (243.0 MHz)
- Frequency blacklist includes marine distress (156.8 MHz)
- Rate limiting prevents excessive spectrum use
- Duration limits prevent jamming scenarios

### ETSI Compliance (European Union)

- 433 MHz operations limited to ISM band
- Duration limits comply with duty cycle restrictions
- Rate limiting supports fair spectrum access

### General Best Practices

- Passive observation before transmission (spectrum etiquette)
- Audit logging for regulatory investigation support
- Frequency validation prevents prohibited band usage
- User confirmation ensures intentional operation

## Changelog

### Version 1.0.0 (Initial Release)
- Documented all four threat classes
- Mapped defenses to code locations
- Created cross-reference matrix
- Established testing guidelines

## References

- **SECURITY.md**: General security policy and guidelines
- **WORKFLOWS.md**: State machine diagrams and flow documentation
- **TESTING.md**: Test procedures and scenarios
- **OWASP IoT Security Project**: Best practices for IoT device security
- **NIST Cybersecurity Framework**: Risk management framework

## Conclusion

The M5Stack RF Suite implements a comprehensive, defense-in-depth security architecture that addresses four major threat classes through 20+ distinct mitigation mechanisms. Each threat is addressed by multiple independent layers, ensuring that a single failure does not compromise security.

The state machine architecture enforces "passive-listening-first" behavior, multi-stage gated transmission approval, and mandatory cleanup, making it extremely difficult to bypass safety mechanisms either accidentally or intentionally.

All defense mechanisms are explicitly mapped to code locations, enabling straightforward verification, testing, and security auditing.
