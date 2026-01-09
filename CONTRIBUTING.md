# Contributing to M5Stack RF Suite

Thank you for your interest in contributing to M5Stack RF Suite! This document provides guidelines for adding features and making changes while maintaining the project's security-first architecture.

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Core Principles](#core-principles)
- [Adding Features](#adding-features)
- [Common Pitfalls](#common-pitfalls)
- [Development Workflow](#development-workflow)
- [Code Review Checklist](#code-review-checklist)
- [Testing Requirements](#testing-requirements)

## Architecture Overview

M5Stack RF Suite is built around three fundamental architectural principles:

1. **Passive-First**: Always observe before transmitting
2. **Gated-TX**: Multi-stage approval for all transmissions
3. **Fail-Closed**: Default to safe state on any error

These principles are enforced through a **Finite State Machine (FSM)** workflow that cannot be bypassed.

### The FSM Workflow

```
IDLE → INIT → LISTENING → ANALYZING → READY → TX_GATED → TRANSMIT → CLEANUP → IDLE
```

**Key Points**:
- Transmitter is physically disabled until `TX_GATED` state
- `LISTENING` state is mandatory and passive-only
- All transmission requests must pass through `TX_GATED` gates
- System always returns to `IDLE` through `CLEANUP`

For detailed state diagrams, see [WORKFLOWS.md](WORKFLOWS.md).

## Core Principles

### 1. Passive-First Design

**What it means**: The system must always observe RF signals passively before any transmission occurs.

**Why it matters**:
- Prevents interference with unknown systems
- Allows analysis before action
- Supports regulatory compliance
- Reduces accidental transmissions

**Implementation Requirements**:
- All features must start in `LISTENING` state
- Transmitter must be disabled during observation
- Minimum observation time enforced
- No shortcuts to skip observation phase

**Example - Correct**:
```cpp
void newFeature() {
    // ✅ CORRECT: Start with passive observation
    workflow.start();  // Enters LISTENING state first

    // The FSM will automatically progress through:
    // INIT → LISTENING → ANALYZING → READY
}
```

**Example - Incorrect**:
```cpp
void newFeature() {
    // ❌ WRONG: Direct transmission bypasses observation
    rf433.transmitSignal(signal);  // Violates passive-first!

    // ❌ WRONG: Skipping LISTENING state
    workflow.transitionToState(TX_GATED);  // Bypasses FSM!
}
```

### 2. Gated-TX (Multi-Stage Transmission Approval)

**What it means**: All transmissions require passing through multiple independent safety gates.

**The Four Gates** (from `TX_GATED` state):

1. **Policy Gate**: Frequency blacklist, duration limits, data integrity
2. **Confirmation Gate**: User button press within timeout
3. **Rate Limit Gate**: Prevents excessive transmissions
4. **Band-Specific Gate**: Protocol validation, binding verification

**Why it matters**:
- Multiple independent safety layers
- Single failure doesn't compromise security
- User awareness and consent
- Regulatory compliance

**Implementation Requirements**:
- ALL transmissions must go through `SafetyModule::checkTransmitPolicy()`
- User confirmation cannot be bypassed
- Rate limiting must be enforced
- Band-specific validation required

**Example - Correct**:
```cpp
void requestTransmission(RF433Signal signal) {
    // ✅ CORRECT: Prepare a request for the FSM.
    TransmitRequest request;
    request.signal = signal;
    request.frequency = 433.92; // Should be from config
    request.duration = calculateDuration(signal);

    // The application then triggers the FSM to enter the TX_GATED state.
    // The FSM automatically processes all four safety gates. The contributor's
    // code does not need to implement the gate checks directly.
    // For example (conceptual):
    // workflow.processEvent(Event::TransmitRequested, request);
}
```

**Example - Incorrect**:
```cpp
void requestTransmission(RF433Signal signal) {
    // ❌ WRONG: Direct hardware access bypasses all gates
    digitalWrite(RF_433_TX_PIN, HIGH);
    RCSwitch.send(signal.value, signal.bitLength);
    digitalWrite(RF_433_TX_PIN, LOW);

    // ❌ WRONG: Bypassing safety module
    if (userClickedButton()) {  // Only checking one gate!
        rf433.transmitSignal(signal);
    }

    // ❌ WRONG: Hardcoding approval
    safety.userConfirmed = true;  // Never do this!
}
```

### 3. Fail-Closed Design

**What it means**: On any error, timeout, or unexpected state, the system defaults to a safe state (IDLE) with transmitter disabled.

**Why it matters**:
- Prevents dangerous states from persisting
- Protects against software bugs
- Ensures cleanup always occurs
- Safe default behavior

**Implementation Requirements**:
- All error paths must lead to `CLEANUP` state
- Transmitter must be disabled on error
- Timeouts must be enforced for every state
- Resources must be freed even on failure

**Example - Correct**:
```cpp
void processState() {
    try {
        // Attempt operation
        if (!performOperation()) {
            // ✅ CORRECT: Fail-safe on error
            logError("Operation failed");
            workflow.transitionToState(CLEANUP);
            return;
        }

        // ✅ CORRECT: Check timeout
        if (hasTimedOut()) {
            logWarning("Timeout in state");
            workflow.transitionToState(CLEANUP);
            return;
        }

    } catch (Exception e) {
        // ✅ CORRECT: Exception safety
        logError("Exception: " + e.getMessage());
        hardware.disableTransmitter();
        workflow.transitionToState(CLEANUP);
    }
}
```

**Example - Incorrect**:
```cpp
void processState() {
    // ❌ WRONG: No error handling
    performOperation();  // What if this fails?

    // ❌ WRONG: No timeout checking
    while (!isComplete()) {
        doWork();  // Infinite loop possible!
    }

    // ❌ WRONG: Leaving transmitter enabled on error
    try {
        transmitSignal();
    } catch (Exception e) {
        logError(e);
        // Transmitter still enabled! 😱
    }
}
```

## Adding Features

### Before You Start

Ask yourself:
1. Does my feature involve RF transmission? → Must use FSM workflow
2. Does my feature modify the state machine? → Needs architectural review
3. Does my feature bypass any safety checks? → Redesign required

### Feature Categories

#### Category A: Passive Features (Safe)

Features that only observe or analyze without transmitting.

**Examples**:
- New signal classification algorithms
- Additional RSSI metrics
- Enhanced protocol decoding
- Data export functionality

**Guidelines**:
- Can be added to `LISTENING` or `ANALYZING` states
- No safety module interaction required
- Should not allocate resources that aren't freed in `CLEANUP`

**Template**:
```cpp
// Add to LISTENING state
void captureNewMetric() {
    if (hardware.hasSignal()) {
        signal.newMetric = calculateNewMetric();
        // Store in capture buffer (freed in CLEANUP)
    }
}

// Add to ANALYZING state
void analyzeWithNewAlgorithm() {
    for (auto& signal : captureBuffer) {
        signal.classification = newClassifier(signal);
    }
}
```

#### Category B: UI/Display Features (Mostly Safe)

Features that enhance user interface without affecting RF operations.

**Examples**:
- New menu items
- Additional status displays
- Configuration screens
- Statistics views

**Guidelines**:
- Must not bypass user confirmation dialogs
- Must not auto-trigger transmissions
- Should respect existing button mappings
- Follow safe-by-default UI patterns

**Template**:
```cpp
void displayNewScreen() {
    M5.Lcd.clear();
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("New Feature Screen");

    // Display information, but don't trigger actions
    // User must explicitly select and confirm
}
```

#### Category C: Transmission Features (Requires FSM Integration)

Features that involve any form of RF transmission.

**Examples**:
- New modulation schemes
- Custom protocols
- Replay functionality
- Test signal generation

**Guidelines**:
- **MUST** integrate with FSM workflow
- **MUST** pass through all TX_GATED gates
- **MUST** have timeout protection
- **MUST** disable transmitter in CLEANUP

**Template**:
```cpp
class NewTransmissionFeature {
public:
    bool execute() {
        // 1. Start workflow (enters LISTENING first)
        if (!workflow.start()) {
            return false;
        }

        // 2. FSM automatically progresses to READY
        // 3. User requests transmission
        // 4. TX_GATED gates are checked automatically
        // 5. If approved, TRANSMIT state executes

        // Your custom transmission logic goes in TRANSMIT state:
        return performCustomTransmission();
    }

private:
    bool performCustomTransmission() {
        // Called from TRANSMIT state only
        // Transmitter is already enabled by FSM
        // Must complete within transmitMaxDuration

        hardware.setCustomParameters();
        bool success = hardware.transmit();

        // FSM will automatically disable transmitter in CLEANUP
        return success;
    }
};
```

#### Category D: Safety/Policy Features (Critical)

Features that modify safety checks or policies.

**Examples**:
- New validation rules
- Additional blacklist entries
- Rate limiting changes
- Timeout adjustments

**Guidelines**:
- Requires architectural review
- Must not weaken existing protections
- Should add defense-in-depth
- Needs comprehensive testing

**Template**:
```cpp
bool newSafetyCheck(TransmitRequest request) {
    // Add additional safety check (defense-in-depth)
    if (!existingCheck(request)) {
        return false;  // Fail-closed
    }

    // New validation
    if (!newValidation(request)) {
        logDenial("New safety check failed");
        return false;  // Fail-closed
    }

    return true;
}

// Integrate into TX_GATED state
bool checkPolicyGate() {
    return existingChecks() && newSafetyCheck();
}
```

## Common Pitfalls

### ❌ Pitfall #1: "Just This Once" Bypass

**Scenario**: "My feature needs to transmit a quick beacon, I'll just call `transmit()` directly."

**Why it's wrong**: Every bypass creates a security hole and sets a bad precedent.

**Correct approach**: Integrate with FSM or use existing workflow.

### ❌ Pitfall #2: Timeout Removal

**Scenario**: "The timeout is too short for my use case, I'll remove it."

**Why it's wrong**: Timeouts prevent infinite loops and stuck states.

**Correct approach**: Make timeout configurable or increase the limit with justification.

### ❌ Pitfall #3: State Machine Shortcuts

**Scenario**: "I can skip LISTENING since I already know what to transmit."

**Why it's wrong**: Violates passive-first principle and safety guarantees.

**Correct approach**: Even with known signals, follow FSM workflow. Add a "quick replay" mode that still enters LISTENING but exits quickly.

### ❌ Pitfall #4: Silent Failures

**Scenario**: "If the safety check fails, I'll just try again without logging."

**Why it's wrong**: Hides security violations and prevents debugging.

**Correct approach**: Log all denials, return to safe state, inform user.

### ❌ Pitfall #5: Hardcoded Confirmations

**Scenario**: "For automated testing, I'll set `userConfirmed = true`."

**Why it's wrong**: Creates a backdoor that could be exploited.

**Correct approach**: Create a proper test mode that simulates button presses or use dependency injection for testing.

## Development Workflow

### 1. Design Phase

- [ ] Read [WORKFLOWS.md](WORKFLOWS.md) and [PSEUDOCODE.md](PSEUDOCODE.md)
- [ ] Identify which FSM states your feature affects
- [ ] Determine feature category (A, B, C, or D)
- [ ] Check if existing states can accommodate your feature
- [ ] Document any new states or transitions needed

### 2. Implementation Phase

- [ ] Start with passive observation (if applicable)
- [ ] Implement error handling for all code paths
- [ ] Add timeout protection
- [ ] Use SafetyModule for any transmissions
- [ ] Ensure transmitter is disabled in CLEANUP
- [ ] Add logging for debugging

### 3. Testing Phase

See [Testing Requirements](#testing-requirements) below.

### 4. Documentation Phase

- [ ] Update README.md if adding user-facing features
- [ ] Update WORKFLOWS.md if adding states
- [ ] Update PSEUDOCODE.md if adding algorithms
- [ ] Add code comments explaining safety-critical sections
- [ ] Update CHANGELOG.md

### 5. Submission Phase

- [ ] Create feature branch from `main`
- [ ] Commit with clear, descriptive messages
- [ ] Run all tests
- [ ] Self-review using Code Review Checklist
- [ ] Submit pull request with description

## Code Review Checklist

Use this checklist when submitting or reviewing pull requests:

### FSM Compliance

- [ ] All transmissions go through FSM workflow
- [ ] No direct hardware access bypassing SafetyModule
- [ ] State transitions are valid per WORKFLOWS.md
- [ ] LISTENING state is passive-only (no transmission)
- [ ] CLEANUP state always executes

### Safety Checks

- [ ] All transmissions pass through TX_GATED gates
- [ ] User confirmation cannot be bypassed
- [ ] Rate limiting is enforced
- [ ] Frequency blacklist is checked
- [ ] Duration limits are enforced

### Error Handling

- [ ] All error paths lead to CLEANUP state
- [ ] Timeouts are implemented for all states
- [ ] Exceptions are caught and handled safely
- [ ] Transmitter is disabled on error
- [ ] Errors are logged with context

### Resource Management

- [ ] Allocated memory is freed in CLEANUP
- [ ] No memory leaks in error paths
- [ ] Buffers have bounds checking
- [ ] Resources are released on timeout

### Code Quality

- [ ] No buffer overflows (use length-limited functions)
- [ ] No floating-point comparison without tolerance
- [ ] Proper use of `std::` namespace
- [ ] No hardcoded values (use config.h)
- [ ] Clear variable names and comments

### Testing

- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Safety features tested (denial scenarios)
- [ ] Timeout handling tested
- [ ] Error recovery tested

## Testing Requirements

### Required Tests for All Features

1. **Happy Path Test**: Feature works as intended
2. **Error Path Test**: Feature handles errors gracefully
3. **Timeout Test**: Feature respects state timeouts
4. **Cleanup Test**: Resources are freed on exit

### Additional Tests for Transmission Features

5. **Denial Test**: Transmission is blocked when policy fails
6. **No Confirmation Test**: Transmission is blocked without user confirmation
7. **Rate Limit Test**: Transmission is blocked when rate limited
8. **Emergency Stop Test**: Transmission stops immediately on abort

### Test Template

```cpp
void testNewFeature() {
    // Setup
    RFTestWorkflow workflow;
    WorkflowConfig config = getTestConfig();
    workflow.initialize(config);

    // Test 1: Happy path
    assert(workflow.start() == true);
    assert(workflow.getCurrentState() == IDLE);

    // Test 2: Error handling
    injectError();
    assert(workflow.start() == false);
    assert(transmitterIsDisabled());

    // Test 3: Timeout
    setShortTimeout();
    assert(workflow.start() == false);
    assert(workflow.getCurrentState() == IDLE);

    // Test 4: Cleanup
    workflow.abort();
    assert(resourcesAreFreed());
    assert(transmitterIsDisabled());
}
```

See [TESTING.md](TESTING.md) for comprehensive test scenarios.

## Architecture Decisions

When proposing architectural changes:

### Questions to Answer

1. **Does this change affect safety?** → Requires security review
2. **Does this add new states?** → Update state diagram
3. **Does this change gate logic?** → Needs comprehensive testing
4. **Does this weaken protections?** → Will likely be rejected

### Proposal Template

Create a GitHub issue with:

```markdown
## Proposed Change
[Clear description of the architectural change]

## Motivation
[Why is this change needed?]

## Impact Analysis
- States affected: [List]
- Safety implications: [Analysis]
- Backward compatibility: [Yes/No and explanation]

## Implementation Plan
1. [Step 1]
2. [Step 2]
...

## Testing Strategy
[How will this be tested?]

## Alternatives Considered
[What other approaches were considered and why were they rejected?]
```

## Examples of Good Contributions

### Example 1: Adding Signal Strength Indicator (Category A)

```cpp
// File: src/rf433_module.cpp

// Added to LISTENING state
void RF433Module::captureWithRSSI() {
    while (rcSwitch.available()) {
        RF433Signal signal;
        signal.value = rcSwitch.getReceivedValue();
        signal.bitLength = rcSwitch.getReceivedBitlength();
        signal.protocol = rcSwitch.getReceivedProtocol();

        // NEW: Capture RSSI
        signal.rssi = measureRSSI();  // New metric
        signal.signalQuality = calculateQuality(signal.rssi);

        capturedSignals.push_back(signal);
    }
}

// No FSM changes required - passive observation only
```

### Example 2: Adding Custom Protocol Support (Category C)

```cpp
// File: src/rf433_module.cpp

// Integrated with FSM in TRANSMIT state
bool RF433Module::transmitCustomProtocol(RF433Signal signal) {
    // This function is called from TRANSMIT state
    // All gates have been passed

    // Validate protocol is safe
    if (!isSafeCustomProtocol(signal)) {
        logError("Unsafe custom protocol");
        return false;  // Fail-closed
    }

    // Configure for custom protocol
    configureCustomProtocol(signal.customParams);

    // Transmit (transmitter already enabled by FSM)
    bool success = executeCustomTransmission(signal);

    // FSM will disable transmitter in CLEANUP
    return success;
}

// Properly integrated - no FSM bypass
```

### Example 3: Adding Frequency Validation (Category D)

```cpp
// File: src/safety_module.cpp

// Added to TX_GATED state, Gate 1 (Policy Check)
bool SafetyModule::checkFrequencyRange(float frequency) {
    // Additional safety check (defense-in-depth)

    // Check if frequency is in valid ISM band
    const float ISM_MIN = 433.05;
    const float ISM_MAX = 434.79;

    if (frequency < ISM_MIN || frequency > ISM_MAX) {
        logDenial("Frequency outside ISM band", frequency);
        return false;  // Fail-closed
    }

    // Existing blacklist check still applies
    return !isFrequencyBlacklisted(frequency);
}

// Strengthens safety without weakening existing checks
```

## Getting Help

- **Questions about architecture?** Open a GitHub Discussion
- **Found a security issue?** See [SECURITY.md](SECURITY.md)
- **Need clarification?** Comment on relevant issue/PR
- **Want to propose major changes?** Create an RFC issue first

## License

By contributing to M5Stack RF Suite, you agree that your contributions will be licensed under the same license as the project.

## Code of Conduct

- Be respectful and professional
- Focus on technical merit
- Accept constructive criticism
- Prioritize safety and security
- Help others learn

## Thank You!

Your contributions help make RF security research more accessible and safer. By following these guidelines, you help maintain the project's integrity and protect users.

**Remember**: When in doubt, err on the side of safety. The FSM exists for a reason - work with it, not around it.

---

**Document Version**: 1.0
**Last Updated**: 2026-01-09
**Maintainers**: M5Stack RF Suite Team
