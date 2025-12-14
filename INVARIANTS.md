# Workflow Invariants and Safety Guarantees

This document formalizes the implicit safety model of the M5Stack RF Suite workflow system. These invariants must hold at all times during system operation.

## Purpose

This document serves multiple audiences:
- **Reviewers**: Understand safety properties without inferring from control flow
- **Auditors**: Verify compliance with safety requirements
- **Contributors**: Maintain safety properties during modifications
- **Users**: Understand system guarantees

## Document Structure

1. State Machine Invariants
2. Safety Guarantees
3. Resource Management Invariants
4. Transmission Control Invariants
5. Timing and Timeout Invariants
6. Error Handling Invariants
7. Debug Assertions

---

## 1. State Machine Invariants

### INV-SM-1: State Validity
**Invariant**: `currentState` is always one of the defined `WorkflowState` enum values.

**Why it matters**: Invalid states could bypass safety checks or cause undefined behavior.

**Enforcement**:
- All state transitions use `transitionToState()` which validates the target state
- State is never modified directly
- Debug assertion verifies state is in valid range

### INV-SM-2: Single Active State
**Invariant**: The workflow is in exactly one state at any given time.

**Why it matters**: Multiple active states could lead to race conditions or conflicting operations.

**Enforcement**:
- `currentState` is a single atomic value
- State transitions are sequential, never concurrent
- No parallel state machines

### INV-SM-3: Deterministic Transitions
**Invariant**: State transitions are deterministic and logged. For any given state and set of conditions, the next state is predictable.

**Why it matters**: Non-deterministic behavior makes the system impossible to audit or debug.

**Enforcement**:
- All transitions go through `transitionToState()`
- Each transition is logged with reason
- Transition guards are pure functions of observable state

### INV-SM-4: IDLE is Terminal
**Invariant**: IDLE state is only entered from CLEANUP. Once in IDLE, workflow stays there until explicitly started.

**Why it matters**: Ensures clean workflow boundaries and prevents partial state carry-over.

**Enforcement**:
- Only CLEANUP state transitions to IDLE
- IDLE state processor does nothing (no side effects)
- Starting from IDLE requires explicit `start()` call

### INV-SM-5: CLEANUP Always Executes
**Invariant**: All workflow terminations (success, failure, timeout, emergency) pass through CLEANUP state.

**Why it matters**: Guarantees resources are released and hardware is returned to safe state.

**Enforcement**:
- All error paths transition to CLEANUP
- All success paths transition to CLEANUP
- Emergency stop forces transition to CLEANUP
- Timeout handlers transition to CLEANUP

---

## 2. Safety Guarantees

### SAFE-TX-1: Transmitter Disabled by Default
**Guarantee**: TX hardware is disabled in all states except TRANSMIT.

**Why it matters**: Prevents accidental transmissions during passive observation or analysis.

**Enforcement**:
- INIT state explicitly disables transmitter
- LISTENING state verifies transmitter is disabled
- Only TRANSMIT state enables transmitter
- CLEANUP state unconditionally disables transmitter
- Debug assertion checks transmitter state in each state processor

**States and TX status**:
```
IDLE:      TX disabled (no hardware access)
INIT:      TX explicitly disabled
LISTENING: TX must be disabled (passive observation)
ANALYZING: TX disabled (no hardware access)
READY:     TX disabled (awaiting decision)
TX_GATED:  TX disabled (approval process)
TRANSMIT:  TX enabled (controlled transmission)
CLEANUP:   TX explicitly disabled
```

### SAFE-TX-2: Passive Listening First
**Guarantee**: No transmission is possible before observation phase completes.

**Why it matters**: Ensures we understand the RF environment before modifying it.

**Enforcement**:
- LISTENING state always precedes any transmission path
- TRANSMIT state is only reachable through LISTENING → ANALYZING → READY → TX_GATED
- Cannot skip observation phases
- Minimum observation time enforced

### SAFE-TX-3: Multi-Gate Transmission Approval
**Guarantee**: Transmission requires passing all four independent gates in sequence.

**Why it matters**: Defense-in-depth prevents single point of failure.

**Enforcement**:
- Gate 1: Policy validation (frequency, duration, signal validity)
- Gate 2: User confirmation (explicit button press within timeout)
- Gate 3: Rate limiting (recent transmission count)
- Gate 4: Band-specific validation (protocol compliance, binding verification)
- All gates must pass; any failure returns to READY
- Gates cannot be skipped or reordered

### SAFE-TX-4: Fail-Safe Default
**Guarantee**: System defaults to safe state (IDLE, TX disabled) on any error.

**Why it matters**: Ensures safety even in unexpected conditions.

**Enforcement**:
- All error handlers transition to CLEANUP → IDLE
- Timeout handlers transition to safe states
- Emergency stop forces CLEANUP
- Hardware reset puts system in IDLE

### SAFE-TX-5: Observable Audit Trail
**Guarantee**: All state transitions and transmission attempts are logged with microsecond timestamps.

**Why it matters**: Enables forensics, debugging, and compliance verification.

**Enforcement**:
- Every state transition logged in `transitionLog`
- Transition includes: from state, to state, timestamp, reason
- Logs persist until explicit clear or power cycle
- Serial output mirrors audit trail

---

## 3. Resource Management Invariants

### RES-BUF-1: Buffer Bounds Protection
**Invariant**: Capture buffer never exceeds `config.bufferSize`.

**Why it matters**: Prevents buffer overflow and memory corruption.

**Enforcement**:
- LISTENING state checks buffer size before adding signals
- Buffer allocation uses `reserve()` for capacity guarantee
- Buffer usage checked against 90% threshold
- Debug assertion verifies buffer size ≤ configured maximum

### RES-BUF-2: Pulse Memory Lifecycle
**Invariant**: Dynamically allocated pulse arrays in `CapturedSignalData` are properly managed.

**Why it matters**: Prevents memory leaks and use-after-free bugs.

**Enforcement**:
- Constructor initializes `pulseTimes` to nullptr
- Destructor deletes pulse array if non-null
- Copy constructor performs deep copy
- Assignment operator handles self-assignment and frees old memory

### RES-BUF-3: Buffer Persistence
**Invariant**: Captured data persists through workflow until CLEANUP or explicit clear.

**Why it matters**: Allows analysis and replay without data loss.

**Enforcement**:
- CLEANUP state does NOT clear capture buffer by default
- Buffer only cleared on explicit `reset()` or new workflow start
- Data available in READY and TX_GATED states

### RES-HW-1: Hardware Initialization
**Invariant**: RF hardware is initialized in INIT state before any use.

**Why it matters**: Prevents operations on uninitialized hardware.

**Enforcement**:
- INIT state checks hardware module pointers are non-null
- Hardware methods called only after successful initialization
- Initialization failure transitions to CLEANUP
- Debug assertion verifies hardware ready before use

### RES-HW-2: Hardware Cleanup
**Invariant**: RF hardware is cleanly shut down in CLEANUP state.

**Why it matters**: Ensures hardware is in known state for next workflow.

**Enforcement**:
- CLEANUP disables transmitter (step 1)
- CLEANUP disables receiver (step 2)
- CLEANUP operations are unconditional
- Cleanup attempts multiple times if needed

---

## 4. Transmission Control Invariants

### TX-CONF-1: Confirmation Required
**Invariant**: User confirmation is mandatory before any transmission.

**Why it matters**: Prevents accidental or unauthorized transmissions.

**Enforcement**:
- Gate 2 (confirmation gate) cannot be bypassed
- Confirmation flag checked in `checkConfirmationGate()`
- Timeout occurs if not confirmed within `config.txGateTimeout`
- Debug assertion verifies confirmation flag is set before TRANSMIT

### TX-CONF-2: Single Use Confirmation
**Invariant**: Each confirmation is valid for exactly one transmission attempt.

**Why it matters**: Prevents replay of stale confirmations.

**Enforcement**:
- Confirmation flag reset after use (success or failure)
- Each transmission requires fresh confirmation
- Timeout invalidates pending confirmation

### TX-RATE-1: Rate Limit Enforcement
**Invariant**: Transmission rate does not exceed configured maximum per minute.

**Why it matters**: Prevents spectrum abuse and DoS attacks.

**Enforcement**:
- Gate 3 (rate limit gate) uses SafetyModule
- SafetyModule tracks recent transmissions with timestamps
- Old transmissions cleaned up (rolling window)
- Denial logged when rate exceeded

### TX-POL-1: Frequency Blacklist
**Invariant**: Transmission on blacklisted frequencies is blocked.

**Why it matters**: Prevents interference with critical services.

**Enforcement**:
- Gate 1 (policy gate) checks `isFrequencyBlacklisted()`
- Blacklist defined in config.h
- Check uses tolerance (±100 kHz)
- Blocked attempts logged

### TX-POL-2: Duration Limits
**Invariant**: Single transmission duration does not exceed `config.transmitMaxDuration`.

**Why it matters**: Prevents jamming and excessive spectrum occupancy.

**Enforcement**:
- Gate 1 checks estimated duration
- TRANSMIT state has timeout protection
- Watchdog triggers emergency stop on overrun
- Transmitter forcibly disabled after timeout

### TX-BAND-1: Band-Specific Validation (433 MHz)
**Invariant**: 433 MHz transmissions have valid pulse timing (100-10000 μs).

**Why it matters**: Ensures protocol compliance and receiver compatibility.

**Enforcement**:
- Gate 4 (433 MHz gate) validates each pulse time
- Out-of-range pulses cause gate failure
- Validation happens after capture but before transmission

### TX-BAND-2: Band-Specific Validation (2.4 GHz)
**Invariant**: 2.4 GHz transmissions only target observed addresses (binding verification).

**Why it matters**: Prevents unauthorized address spoofing.

**Enforcement**:
- Gate 4 (2.4 GHz gate) checks address was observed
- Observed addresses tracked during LISTENING
- Unobserved addresses cause gate failure

---

## 5. Timing and Timeout Invariants

### TIME-OUT-1: Every State Has Timeout
**Invariant**: Each non-IDLE state has a maximum duration timeout.

**Why it matters**: Prevents indefinite hang in any state.

**Enforcement**:
- `getTimeoutForState()` returns timeout for each state
- `checkTimeout()` called in main loop
- Timeout handler transitions to safe state or forces cleanup
- IDLE has no timeout (intentional)

**Timeout values**:
```
INIT:      5 seconds
LISTENING: 60 seconds (max observation)
ANALYZING: 10 seconds
READY:     120 seconds (user decision)
TX_GATED:  10 seconds (confirmation window)
TRANSMIT:  5 seconds (hard limit)
CLEANUP:   5 seconds (force transition if stuck)
```

### TIME-OUT-2: Timeout is Fail-Safe
**Invariant**: Timeout always transitions to safe state or CLEANUP.

**Why it matters**: Ensures timeout doesn't leave system in dangerous state.

**Enforcement**:
- INIT timeout → CLEANUP
- LISTENING timeout → ANALYZING (force analysis)
- ANALYZING timeout → READY (partial results OK)
- READY timeout → CLEANUP
- TX_GATED timeout → READY (deny transmission)
- TRANSMIT timeout → emergency stop → CLEANUP
- CLEANUP timeout → IDLE (force exit)

### TIME-MIN-1: Minimum Observation Time
**Invariant**: LISTENING state has minimum duration before analysis.

**Why it matters**: Ensures sufficient observation data.

**Enforcement**:
- `config.listenMinTime` enforced in LISTENING processor
- Early transitions blocked until minimum time elapses
- Default: 1 second (configurable)

### TIME-MIN-2: Minimum Analysis Time
**Invariant**: ANALYZING state completes basic analysis before transition.

**Why it matters**: Ensures analysis results are meaningful.

**Enforcement**:
- Analysis steps always execute
- Minimum processing ensures data validity
- Cannot skip analysis steps

---

## 6. Error Handling Invariants

### ERR-LOG-1: All Errors Logged
**Invariant**: Every error is logged with error code and message.

**Why it matters**: Enables debugging and root cause analysis.

**Enforcement**:
- All error paths call `logError()`
- Error includes: error code, message, timestamp
- Error log persists until cleared
- Serial output mirrors error log

### ERR-CNT-1: Error Threshold Protection
**Invariant**: Workflow aborts after too many errors (threshold: 10).

**Why it matters**: Prevents infinite error loops.

**Enforcement**:
- Error counter increments on each error
- Main loop checks error count
- Exceeding threshold forces CLEANUP
- Counter resets on successful workflow completion

### ERR-EMRG-1: Emergency Stop
**Invariant**: Emergency stop is always honored immediately.

**Why it matters**: Provides last-resort safety mechanism.

**Enforcement**:
- Emergency flag checked in main loop
- Emergency stop forces CLEANUP regardless of state
- Transmitter disabled immediately
- Cannot be ignored or delayed

### ERR-RECOV-1: All Failures Exit via CLEANUP
**Invariant**: No error path bypasses CLEANUP state.

**Why it matters**: Guarantees resource release and safe state.

**Enforcement**:
- Init failures → CLEANUP → IDLE
- Runtime errors → CLEANUP → IDLE
- Timeout errors → CLEANUP (or safe state)
- Emergency stop → CLEANUP → IDLE
- All error handlers use `transitionToState(WF_CLEANUP, reason)`

---

## 7. Debug Assertions

Debug assertions are enabled in debug builds to verify invariants at runtime. They help catch violations early during development and testing.

### Assertion Levels

**Level 1 (ASSERT_LEVEL_CRITICAL)**: Always enabled in debug builds
- Safety-critical invariants
- Transmitter state verification
- State validity checks

**Level 2 (ASSERT_LEVEL_STANDARD)**: Enabled by default in debug builds
- Resource management checks
- Buffer bounds verification
- Timing constraints

**Level 3 (ASSERT_LEVEL_VERBOSE)**: Enabled only when explicitly requested
- Detailed state consistency checks
- Performance-impacting validations
- Diagnostic assertions

### Assertion Implementation

Assertions are implemented using compile-time guards:

```cpp
#ifdef DEBUG_ASSERTIONS
    // Critical safety check
    WF_ASSERT(condition, "Invariant violation message");
#endif
```

Assertion failure behavior:
- Logs violation to serial output
- Logs error with full context
- Forces emergency stop
- Transitions to CLEANUP
- In severe cases, may halt execution (configurable)

### Key Assertions

**State Machine Assertions**:
- `WF_ASSERT(currentState >= WF_IDLE && currentState <= WF_CLEANUP)` - INV-SM-1
- `WF_ASSERT(currentState != previousState || justEntered)` - Transition occurred

**Safety Assertions**:
- `WF_ASSERT(!txEnabled || currentState == WF_TRANSMIT)` - SAFE-TX-1
- `WF_ASSERT(userConfirmed || currentState != WF_TRANSMIT)` - TX-CONF-1

**Resource Assertions**:
- `WF_ASSERT(captureBuffer.size() <= config.bufferSize)` - RES-BUF-1
- `WF_ASSERT(rf433Module != nullptr || config.band != BAND_433MHZ)` - RES-HW-1

**Timing Assertions**:
- `WF_ASSERT(getStateElapsedTime() < timeout || timeout == 0)` - TIME-OUT-1
- `WF_ASSERT(getStateElapsedTime() >= config.listenMinTime || !transitionRequested)` - TIME-MIN-1

**Error Handling Assertions**:
- `WF_ASSERT(errorCount < 100)` - Sanity check for error explosion
- `WF_ASSERT(transitionLog.size() < 1000)` - Prevent log memory exhaustion

---

## Verification Checklist

Use this checklist when reviewing code changes:

### State Machine
- [ ] All state transitions use `transitionToState()`
- [ ] All transitions are logged
- [ ] IDLE is only entered from CLEANUP
- [ ] All termination paths go through CLEANUP

### Safety
- [ ] Transmitter disabled in all states except TRANSMIT
- [ ] Observation phase cannot be skipped
- [ ] All four transmission gates are enforced
- [ ] Failures default to IDLE with TX disabled

### Resources
- [ ] Buffer bounds checked before insertion
- [ ] Dynamic memory properly freed
- [ ] Hardware initialized before use
- [ ] Hardware cleaned up in CLEANUP

### Transmission
- [ ] User confirmation required
- [ ] Rate limiting enforced
- [ ] Frequency blacklist checked
- [ ] Duration limits enforced
- [ ] Band-specific validation performed

### Timing
- [ ] Each state has timeout
- [ ] Timeout handlers transition safely
- [ ] Minimum times enforced where required

### Errors
- [ ] All errors logged with context
- [ ] Error threshold checked
- [ ] Emergency stop honored
- [ ] All failures exit via CLEANUP

### Debug Assertions
- [ ] Critical assertions present for safety invariants
- [ ] Assertions enabled in debug builds
- [ ] Assertion failures handled safely

---

## Known Limitations

### Hardware State Verification

**Limitation**: The current implementation of `isTransmitterEnabled()` in `rf_test_workflow.cpp` performs state-based checking rather than querying actual hardware TX enable state.

**Impact**: 
- Assertions verify state consistency but cannot detect hardware-level bugs
- If code bypasses state management and directly enables TX, assertions won't catch it
- SAFE-TX-1 verification is limited to state-level consistency

**Mitigation**:
- Code review emphasis on all TX enable/disable calls
- Future enhancement to add hardware query capability
- RF433Module should expose `getTransmitEnabled()` method

**Tracking**: See TODO comments in `rf_test_workflow.cpp` lines 1085-1093

### Confirmation Flag Lifecycle

**Limitation**: TX-CONF-1 assertion cannot be verified in `verifySafetyInvariants()` because the confirmation flag is reset after gate checks complete.

**Impact**:
- Assertion would always fail after successful gate passage
- Cannot verify confirmation in post-TRANSMIT state

**Mitigation**:
- Confirmation checked in TX_GATED gate processing (line 738-756)
- Gate logging provides audit trail of confirmation
- Multi-gate architecture ensures confirmation cannot be bypassed

**Future Enhancement**: Add separate flag to track "was confirmed" vs "is pending confirmation"

## Maintenance Notes

### Adding New States
When adding a new workflow state:
1. Update `WorkflowState` enum
2. Add state name to `getStateName()`
3. Add timeout value to `getTimeoutForState()`
4. Add state processor function
5. Add timeout handler to `handleTimeout()`
6. Document invariants for new state
7. Add debug assertions
8. Update state diagram in WORKFLOWS.md

### Modifying Invariants
When modifying an invariant:
1. Update this document with rationale
2. Update affected debug assertions
3. Update related code comments
4. Update test cases
5. Review impact on safety guarantees
6. Get peer review for safety-critical changes

### Adding Assertions
When adding new assertions:
1. Choose appropriate assertion level
2. Provide clear violation message
3. Document which invariant it enforces
4. Consider performance impact
5. Test assertion can trigger (negative testing)

---

## Testing Invariants

### Unit Testing
Each invariant should have test cases that:
- Verify the invariant holds in normal operation
- Attempt to violate the invariant (negative testing)
- Verify assertion triggers on violation (in debug builds)

### Integration Testing
Integration tests should verify:
- Invariants hold across state transitions
- Error paths maintain invariants
- Timeout handling preserves invariants
- Emergency stop maintains safety

### Stress Testing
Stress tests should verify:
- Invariants hold under resource pressure
- Error threshold protection works
- Memory doesn't leak over extended operation
- Buffer management stays within bounds

---

## Related Documents

- **WORKFLOWS.md**: State machine diagrams and flow descriptions
- **SECURITY.md**: Security threat model and mitigations
- **TESTING.md**: Test procedures and acceptance criteria
- **README.md**: High-level architecture overview

---

**Document Version**: 1.0  
**Last Updated**: 2025-12-14  
**Status**: Complete  
**Reviewers**: Security team, RF experts, embedded systems engineers
