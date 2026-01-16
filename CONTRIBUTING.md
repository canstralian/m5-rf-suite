# Contributing to M5Stack RF Suite

Thank you for your interest in contributing to M5Stack RF Suite! This document provides guidelines for extending the project while maintaining its core architectural principles.

## Table of Contents

1. [Core Architectural Principles](#core-architectural-principles)
2. [Before You Start](#before-you-start)
3. [How to Add Features](#how-to-add-features)
4. [State Machine Workflow Guidelines](#state-machine-workflow-guidelines)
5. [Safety Module Integration](#safety-module-integration)
6. [Testing Requirements](#testing-requirements)
7. [Code Review Process](#code-review-process)
8. [Examples](#examples)
9. [Resources](#resources)

---

## Core Architectural Principles

This project is built on three fundamental safety principles that **MUST NOT** be violated:

### 1. Passive-First Design

**Principle**: All workflows must begin with passive observation before any transmission is permitted.

**Requirements**:
- New features MUST capture/observe RF signals before allowing transmission
- Transmitter MUST be disabled during initialization and observation phases
- No shortcuts that bypass the observation phase
- State machine MUST enforce LISTENING state before TX_GATED state

**Why**: This ensures complete environmental awareness before modifying the RF spectrum, preventing interference and enabling informed decision-making.

### 2. Gated-TX (Gated Transmission)

**Principle**: All transmissions must pass through multiple independent approval gates.

**Required Gates** (minimum):
1. **Policy Gate**: Frequency blacklist, duration limits, data validation
2. **User Confirmation Gate**: Explicit button press with timeout
3. **Rate Limit Gate**: Time-based transmission throttling
4. **Band-Specific Gate**: Additional validation per frequency band

**Requirements**:
- New transmission features MUST integrate with `SafetyModule`
- All gates must be independent and fail-safe
- Denial at any gate MUST prevent transmission
- Gate failures MUST be logged with reason
- NO bypass mechanisms in user-facing code

**Why**: Defense-in-depth approach ensures multiple independent safety checks prevent unauthorized or dangerous transmissions.

### 3. Fail-Closed Design

**Principle**: System defaults to safe state on any error, timeout, or unexpected condition.

**Requirements**:
- Timeouts MUST transition to safe states (disable TX, cleanup)
- Errors MUST NOT allow transmission to proceed
- Default state is IDLE with transmitter disabled
- Emergency stop MUST immediately disable transmitter
- Cleanup phase ALWAYS executes regardless of success/failure

**Why**: Ensures the system never "fails open" and accidentally transmits due to bugs or unexpected conditions.

---

## Before You Start

### 1. Read the Documentation

Familiarize yourself with:
- **README.md**: Project overview and features
- **WORKFLOWS.md**: State machine diagrams and flows
- **PSEUDOCODE.md**: Detailed implementation logic
- **SECURITY.md**: Security policies and threat model
- **TESTING.md**: Testing procedures and requirements

### 2. Understand the State Machine

The RF Test Workflow state machine is the core of this project:

```
IDLE → INIT → LISTENING → ANALYZING → READY → TX_GATED → TRANSMIT → CLEANUP → IDLE
         ↓        ↓           ↓          ↓         ↓           ↓          ↓
      [FAIL]  [timeout]   [timeout]  [timeout] [denied]    [error]   [always]
         ↓        ↓           ↓          ↓         ↓           ↓          ↓
      CLEANUP  CLEANUP    CLEANUP    CLEANUP   READY      CLEANUP      IDLE
```

Key points:
- States are strictly ordered
- Multiple exit paths ensure fail-safe operation
- CLEANUP always executes before returning to IDLE
- Timeouts are enforced at every state

### 3. Set Up Development Environment

```bash
# Clone the repository
git clone https://github.com/canstralian/m5-rf-suite.git
cd m5-rf-suite

# Install PlatformIO
pip install platformio

# Build the project
pio run

# Run tests (when available)
pio test
```

---

## How to Add Features

### Feature Categories

#### Category A: Passive Features (Low Risk)
Features that only receive/analyze signals without transmission capability.

**Examples**:
- New signal decoders
- Additional analysis algorithms
- Enhanced visualization
- New scanning modes

**Requirements**:
- Implement in LISTENING or ANALYZING states
- Do not touch transmitter controls
- Follow existing module patterns
- Add appropriate logging

#### Category B: Active Features (High Risk)
Features that involve RF transmission.

**Examples**:
- New transmission protocols
- Signal replay with modifications
- Automated transmission sequences

**Requirements**:
- MUST integrate with state machine workflow
- MUST pass through all TX gates in SafetyModule
- MUST respect passive-first (observe before transmit)
- MUST include comprehensive error handling
- MUST add band-specific validation gate if needed
- REQUIRES security review before merge

#### Category C: Infrastructure Features (Medium Risk)
Features that modify core systems.

**Examples**:
- New state machine states
- Safety policy modifications
- Configuration system changes

**Requirements**:
- Discuss design in issue before implementation
- Maintain backward compatibility when possible
- Update all relevant documentation
- Add migration guides if breaking changes
- Extensive testing required

### Step-by-Step Process

1. **Open an Issue**
   - Describe the feature and its purpose
   - Explain how it respects the three core principles
   - Get feedback from maintainers

2. **Fork and Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Implement Your Feature**
   - Follow the patterns in existing code
   - Maintain consistency with code style
   - Add appropriate comments
   - Update configuration if needed

4. **Test Thoroughly**
   - Test normal operation
   - Test error conditions
   - Test timeouts
   - Test integration with existing features
   - Document test procedures

5. **Update Documentation**
   - Update README.md if user-facing
   - Update WORKFLOWS.md if modifying state machine
   - Update SECURITY.md if affecting security
   - Add inline code documentation

6. **Submit Pull Request**
   - Clear description of changes
   - Reference related issues
   - Include test results
   - List any breaking changes

---

## State Machine Workflow Guidelines

### Adding a New State (Rarely Needed)

If you need to add a state to the workflow FSM:

1. **Justify the Need**
   - Explain why existing states cannot be extended
   - Show how the new state fits in the workflow
   - Ensure it maintains passive-first and fail-closed principles

2. **Implementation Checklist**
   - [ ] Add state to `WorkflowState` enum
   - [ ] Implement `process[NewState]State()` method
   - [ ] Add timeout handling
   - [ ] Define entry/exit conditions
   - [ ] Implement fail-safe transitions
   - [ ] Add state to `getStateName()` function
   - [ ] Update `getTimeoutForState()` function
   - [ ] Add state to documentation in WORKFLOWS.md
   - [ ] Update state diagram

3. **Safety Considerations**
   - New state MUST have timeout protection
   - New state MUST have error handling
   - Define clear path to CLEANUP on failure
   - Ensure transmitter is disabled unless explicitly in TRANSMIT state

### Extending Existing States

Preferred over adding new states. Example:

```cpp
void RFTestWorkflow::processListeningState() {
    // Existing validation
    if (hardware.isTransmitterDisabled() == false) {
        logError(WF_ERROR_HARDWARE_FAILURE, "Transmitter must be disabled");
        transitionToState(WF_CLEANUP, "Safety violation");
        return;
    }
    
    // Your new feature: additional signal type capture
    captureSignals();  // Existing function
    captureYourNewSignalType();  // New function
    
    // Existing transition logic
    if (bufferFull() || elapsed > config.listenMaxTime) {
        transitionToState(WF_ANALYZING, "Buffer full or timeout");
    }
}
```

### Adding Transmission Gates

If your feature requires additional validation before transmission:

1. **Implement in `processTxGatedState()`**

```cpp
void RFTestWorkflow::processTxGatedState() {
    // Existing gates...
    if (!checkPolicyGate()) { /* fail */ }
    if (!checkConfirmationGate()) { /* fail */ }
    if (!checkRateLimitGate()) { /* fail */ }
    
    // Your new gate
    if (!checkYourNewValidationGate()) {
        Serial.println("Your new gate FAILED");
        transitionToState(WF_READY, "Gate denied");
        return;
    }
    LOG_INFO("Your new gate: PASSED");
    
    // Proceed to transmission only if ALL gates pass
    transitionToState(WF_TRANSMIT, "All gates passed");
}
```

2. **Gate Implementation Rules**
   - Must return boolean (true = pass, false = fail)
   - Must log failure reason
   - Must be independent of other gates
   - Must not have side effects
   - Must respect timeouts

---

## Safety Module Integration

### When to Use SafetyModule

ANY feature that involves RF transmission MUST use the SafetyModule:

```cpp
#include "safety_module.h"

// Create transmission request
TransmitRequest request;
request.frequency = 433.92;
request.duration = 500;  // milliseconds
request.timestamp = millis();
request.confirmed = false;
snprintf(request.reason, sizeof(request.reason), "User requested replay");

// Check policy
TransmitPermission permission = Safety.checkTransmitPolicy(request);
if (permission != PERMIT_ALLOWED) {
    // Handle denial
    Serial.printf("Transmission denied: %d\n", permission);
    return false;
}

// Request user confirmation
Safety.requestUserConfirmation(request);
bool confirmed = Safety.waitForUserConfirmation(10000);  // 10 second timeout

if (!confirmed) {
    Safety.cancelConfirmation();
    return false;
}

// Check rate limit
if (!Safety.isRateLimitOK()) {
    Serial.println("Rate limit exceeded");
    return false;
}

// All checks passed - safe to transmit
// Log the attempt
Safety.logTransmitAttempt(request, true, PERMIT_ALLOWED);

// Perform transmission
yourTransmitFunction();
```

### Bypassing SafetyModule is PROHIBITED

**DO NOT**:
- Call hardware transmit functions directly
- Add configuration options to disable safety checks
- Implement transmission without user confirmation
- Skip rate limiting "just this once"
- Add backdoors or debug modes that bypass safety

**If you need testing without confirmations**, modify the test environment, not the production code.

---

## Testing Requirements

### For Passive Features

1. **Unit Tests**
   - Test signal parsing correctness
   - Test error handling
   - Test boundary conditions

2. **Integration Tests**
   - Test integration with existing modules
   - Verify no interference with workflow states
   - Test with real hardware if possible

### For Active Features (Transmission)

1. **Safety Tests** (MANDATORY)
   - Verify all gates are called
   - Test gate failure handling
   - Verify user confirmation is required
   - Test timeout behavior
   - Verify rate limiting works
   - Test frequency blacklist enforcement

2. **State Machine Tests**
   - Verify passive observation occurs before TX
   - Test state transitions
   - Verify cleanup always executes
   - Test emergency stop

3. **Hardware Tests**
   - Verify transmitter is disabled during observation
   - Measure actual transmission duration
   - Verify frequency accuracy
   - Test with real RF equipment

### Test Documentation

Document your tests in TESTING.md:

```markdown
## Test Scenario X: Your Feature

**Purpose**: Brief description

**Setup**:
1. Hardware configuration
2. Software configuration
3. Initial state

**Procedure**:
1. Step-by-step test procedure

**Expected Results**:
- What should happen

**Safety Verification**:
- Confirm transmitter disabled during observation
- Confirm user confirmation required
- Confirm rate limiting enforced
- Confirm timeout protection works
```

---

## Code Review Process

### What Reviewers Will Check

1. **Architecture Compliance**
   - Does it respect passive-first?
   - Does it use gated-TX properly?
   - Does it fail-closed?

2. **Safety Integration**
   - Are all transmission paths protected?
   - Is user confirmation required?
   - Is rate limiting enforced?
   - Are frequency blacklists respected?

3. **State Machine Integrity**
   - Are states used correctly?
   - Are timeouts implemented?
   - Does cleanup always execute?
   - Are error paths safe?

4. **Code Quality**
   - Consistent style
   - Appropriate comments
   - Error handling
   - Resource management (no leaks)

5. **Documentation**
   - Updated README if user-facing
   - Updated WORKFLOWS.md if state machine changed
   - Updated SECURITY.md if security-relevant
   - Inline documentation adequate

### Review Timeframe

- Passive features: 1-3 days
- Active features: 1-2 weeks (includes security review)
- Infrastructure changes: 2-4 weeks (requires design discussion)

### Addressing Feedback

- Respond to all comments
- Ask questions if unclear
- Make requested changes
- Re-request review after updates

---

## Examples

### ✅ GOOD: Adding a New Signal Type (Passive)

```cpp
// In rf433_module.cpp
void RF433Module::captureWeatherStationData() {
    // Passive observation only - no transmission
    if (rcswitch.available()) {
        unsigned long value = rcswitch.getReceivedValue();
        int protocol = rcswitch.getReceivedProtocol();
        
        // Check if it's a weather station protocol
        if (protocol == WEATHER_STATION_PROTOCOL) {
            RF433Signal signal;
            signal.value = value;
            signal.protocol = protocol;
            signal.type = SIGNAL_WEATHER_STATION;
            signal.timestamp = micros();
            
            // Store signal
            capturedSignals.push_back(signal);
            
            // Log observation
            Serial.printf("Weather station signal captured: %lu\n", value);
        }
        
        rcswitch.resetAvailable();
    }
}
```

**Why this is good**:
- Only observes, never transmits
- Uses existing patterns
- Appropriate logging
- No safety concerns

### ✅ GOOD: Adding Validation Gate

```cpp
// In rf_test_workflow.cpp
bool RFTestWorkflow::checkProtocolSafetyGate() {
    CapturedSignalData& signal = captureBuffer[selectedSignalIndex];
    
    // Verify protocol is known and safe
    const char* safeProtocols[] = {"RCSwitch-1", "RCSwitch-2", "RCSwitch-5"};
    bool isSafe = false;
    
    for (int i = 0; i < 3; i++) {
        if (strcmp(signal.protocol, safeProtocols[i]) == 0) {
            isSafe = true;
            break;
        }
    }
    
    if (!isSafe) {
        LOG_WARNING("Unknown or unsafe protocol: %s", signal.protocol);
        return false;
    }
    
    LOG("Protocol safety gate: PASSED");
    return true;
}

// Then call it in processTxGatedState()
if (!checkProtocolSafetyGate()) {
    transitionToState(WF_READY, "Protocol safety gate failed");
    return;
}
```

**Why this is good**:
- Adds additional safety check
- Follows existing gate pattern
- Logs failure reason
- Returns to safe state on failure

### ❌ BAD: Bypassing Safety (DO NOT DO THIS)

```cpp
// WRONG - DO NOT DO THIS
void quickTransmit(unsigned long value) {
    // Direct transmission without safety checks
    rcswitch.send(value, 24);
}

// WRONG - DO NOT DO THIS
void transmitWithoutConfirmation(RF433Signal signal) {
    // Skipping user confirmation "for convenience"
    hardware.enableTransmitter();
    hardware.transmitSignal(signal, /*requireConfirmation=*/false);
    hardware.disableTransmitter();
}

// WRONG - DO NOT DO THIS
if (debugMode) {
    // Bypassing safety in debug mode
    directTransmit();
} else {
    // Normal safe path
    safeTransmit();
}
```

**Why this is bad**:
- Bypasses gated-TX principle
- No user confirmation
- No rate limiting
- No audit logging
- Creates dangerous precedent
- WILL BE REJECTED in code review

### ❌ BAD: Transmitting During Observation (DO NOT DO THIS)

```cpp
// WRONG - DO NOT DO THIS
void RFTestWorkflow::processListeningState() {
    captureSignals();
    
    // VIOLATION: Transmitting during passive observation phase
    if (someCondition) {
        transmitTestBeacon();  // NO! Violates passive-first!
    }
    
    if (bufferFull()) {
        transitionToState(WF_ANALYZING);
    }
}
```

**Why this is bad**:
- Violates passive-first principle
- Transmits without going through TX gates
- Corrupts observation data
- WILL BE REJECTED in code review

### ✅ GOOD: Safe Test Beacon (Correct Approach)

If you need a test beacon feature:

```cpp
// Implement as a separate workflow that respects the state machine
void RFTestWorkflow::transmitTestBeacon() {
    // Only callable from READY state after observation
    if (currentState != WF_READY) {
        LOG_ERROR("Test beacon only available after observation");
        return;
    }
    
    // Create test signal
    CapturedSignalData beacon;
    beacon.frequency = 433.92;
    beacon.dataLength = 3;
    beacon.rawData[0] = 0xAA;
    beacon.rawData[1] = 0xBB;
    beacon.rawData[2] = 0xCC;
    strcpy(beacon.protocol, "Test-Beacon");
    beacon.isValid = true;
    
    // Store as "captured" signal (even though it's synthetic)
    captureBuffer.push_back(beacon);
    if (captureBuffer.size() > 0) {
        selectedSignalIndex = captureBuffer.size() - 1;
    }
    
    // Go through normal TX gate process
    transitionToState(WF_TX_GATED, "Test beacon requested");
    
    // User will still need to confirm
    // Rate limiting still applies
    // All gates must pass
}
```

**Why this is good**:
- Respects state machine flow
- Goes through all safety gates
- User confirmation still required
- Maintains audit trail

---

## Resources

### Documentation
- [WORKFLOWS.md](WORKFLOWS.md) - State machine details
- [PSEUDOCODE.md](PSEUDOCODE.md) - Implementation details
- [SECURITY.md](SECURITY.md) - Security policies
- [TESTING.md](TESTING.md) - Test procedures
- [HARDWARE.md](HARDWARE.md) - Hardware setup

### Code References
- `include/rf_test_workflow.h` - State machine interface
- `src/rf_test_workflow.cpp` - State machine implementation
- `include/safety_module.h` - Safety interface
- `src/safety_module.cpp` - Safety implementation

### Getting Help

- **GitHub Issues**: Ask questions or discuss designs
- **Pull Requests**: Request feedback during development
- **Documentation**: Reference existing patterns and examples

### Legal and Regulatory

Remember:
- Comply with local RF regulations (FCC, CE, etc.)
- This is an educational/research tool
- You are responsible for legal use
- Do not implement features for illegal purposes

---

## Summary: The Golden Rules

1. **Always observe before transmitting** (passive-first)
2. **All transmissions must pass through TX gates** (gated-TX)
3. **System must fail to safe state** (fail-closed)
4. **Never bypass SafetyModule** (no backdoors)
5. **User confirmation is mandatory** (no auto-transmit)
6. **Rate limiting must be enforced** (no DoS)
7. **Cleanup must always execute** (no resource leaks)
8. **Timeouts must be respected** (no hangs)
9. **Test safety features thoroughly** (prove it's safe)
10. **Document your changes** (help future contributors)

---

## Questions?

If you're unsure whether your feature idea complies with these principles:

1. Open an issue describing your idea
2. Explain how it respects passive-first, gated-TX, and fail-closed
3. Ask for feedback from maintainers
4. Wait for design approval before implementing

We're here to help you contribute successfully while maintaining the project's safety and security standards.

**Thank you for contributing responsibly!**
