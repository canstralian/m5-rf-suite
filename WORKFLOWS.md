# RF Test Workflows - Process Flows and State Diagrams

This document defines the high-level process flows and explicit state diagrams for RF test workflows targeting ESP32 with attached RF modules (CC1101 at 433 MHz, nRF24L01+ at 2.4 GHz).

## Overview

The RF test workflow system implements a "passive-listening first" methodology with structured safeguards that:
- Prioritize observation before transmission
- Implement fail-safe modes for every state transition
- Enforce timing constraints and overrun protection
- Ensure complete observational capture before any transmission
- Provide RF timing/pattern traceability

## Workflow Architecture

### Core Principles

1. **Passive-First Design**: All workflows begin with passive observation mode
2. **Fail-Safe Transitions**: Each state transition includes timeout and error handling
3. **Gated Transmission**: Transmissions require explicit multi-stage approval
4. **Complete Cleanup**: Resources are always released, regardless of success/failure

## State Machine Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    RF TEST WORKFLOW STATES                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────┐                                                   │
│  │   IDLE    │ ◄──────────────────────────────────────┐         │
│  └─────┬─────┘                                         │         │
│        │                                               │         │
│        │ start()                                       │         │
│        ▼                                               │         │
│  ┌───────────┐                                         │         │
│  │  INIT     │ ──────► [FAIL] ──────────────────────► │         │
│  └─────┬─────┘         timeout                         │         │
│        │                                               │         │
│        │ success                                       │         │
│        ▼                                               │         │
│  ┌───────────┐                                         │         │
│  │ LISTENING │ ◄──────┐                                │         │
│  └─────┬─────┘        │                                │         │
│        │              │ continue                       │         │
│        │ timeout/     │ observation                    │         │
│        │ trigger      │                                │         │
│        ▼              │                                │         │
│  ┌───────────┐        │                                │         │
│  │ ANALYZING │ ───────┘                                │         │
│  └─────┬─────┘                                         │         │
│        │                                               │         │
│        │ analysis complete                             │         │
│        ▼                                               │         │
│  ┌───────────┐                                         │         │
│  │  READY    │ ──────► skip transmission ───────────► │         │
│  └─────┬─────┘                                         │         │
│        │                                               │         │
│        │ request transmit                              │         │
│        ▼                                               │         │
│  ┌───────────┐                                         │         │
│  │ TX_GATED  │ ──────► [DENIED] ───────────────────► │         │
│  └─────┬─────┘         policy/timeout                  │         │
│        │                                               │         │
│        │ approved                                      │         │
│        ▼                                               │         │
│  ┌───────────┐                                         │         │
│  │TRANSMIT   │ ──────► [ERROR] ────────────────────► │         │
│  └─────┬─────┘         failure                         │         │
│        │                                               │         │
│        │ success                                       │         │
│        ▼                                               │         │
│  ┌───────────┐                                         │         │
│  │  CLEANUP  │ ──────────────────────────────────────►│         │
│  └───────────┘         always executes                 │         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Workflow States

### 1. IDLE State

**Purpose**: Initial resting state, no resources allocated

**Entry Conditions**:
- System startup
- Workflow completion (successful or failed)
- Manual reset

**Exit Conditions**:
- `start()` method called
- Transition to INIT

**Fail-Safe**:
- No timeout (can remain indefinitely)
- All resources released

### 2. INIT State

**Purpose**: Initialize hardware, allocate buffers, configure RF modules

**Entry Conditions**:
- Transition from IDLE

**Activities**:
1. Initialize RF module (433 MHz or 2.4 GHz)
2. Allocate signal capture buffers
3. Configure timing parameters
4. Set initial frequency/channel
5. Enable receiver circuits

**Exit Conditions**:
- Success → LISTENING
- Failure → IDLE
- Timeout (5 seconds) → IDLE

**Fail-Safe**:
- Timeout protection: 5 seconds maximum
- Resource cleanup on failure
- Error logging

**Transition Guards**:
```
if (initSuccessful && resourcesAllocated && receiverEnabled)
    → LISTENING
else if (timeout || error)
    → IDLE (cleanup)
```

### 3. LISTENING State

**Purpose**: Passive observation of RF signals, no transmission

**Entry Conditions**:
- Successful INIT
- Return from ANALYZING (continuous observation)

**Activities**:
1. **Passive Reception Only** - No transmission permitted
2. Capture RF signals to buffer
3. Record timestamps for each signal
4. Measure signal strength (RSSI)
5. Decode protocols
6. Store raw pulse data

**Exit Conditions**:
- Buffer full → ANALYZING
- Observation timeout → ANALYZING
- Manual trigger → ANALYZING
- Error → CLEANUP → IDLE

**Fail-Safe**:
- Maximum observation time: 60 seconds (configurable)
- Buffer overflow protection
- Automatic transition on buffer 90% full
- Continuous error monitoring

**Transition Guards**:
```
if (bufferFull || observationTimeout || manualTrigger)
    → ANALYZING
else if (error || systemFailure)
    → CLEANUP → IDLE
```

**Timing Constraints**:
- Minimum observation: 1 second
- Maximum observation: 60 seconds
- Buffer size: Configurable per band

### 4. ANALYZING State

**Purpose**: Process captured signals, classify patterns, prepare data

**Entry Conditions**:
- Sufficient data collected in LISTENING

**Activities**:
1. Signal classification (device type identification)
2. Protocol analysis
3. Pulse timing analysis
4. Pattern recognition
5. Statistical analysis (frequency, repetition rate)
6. Calculate signal quality metrics
7. Generate analysis report

**Exit Conditions**:
- Analysis complete → READY
- Continue observation → LISTENING
- Error → CLEANUP → IDLE

**Fail-Safe**:
- Analysis timeout: 10 seconds maximum
- Memory bounds checking
- Exception handling

**Transition Guards**:
```
if (analysisComplete && dataValid)
    → READY
else if (needMoreData)
    → LISTENING
else if (timeout || error)
    → CLEANUP → IDLE
```

### 5. READY State

**Purpose**: Analysis complete, awaiting user decision

**Entry Conditions**:
- Successful ANALYZING

**Activities**:
1. Display analysis results
2. Present transmission options
3. Wait for user input
4. Maintain captured data

**Exit Conditions**:
- User requests transmission → TX_GATED
- User cancels/exits → CLEANUP → IDLE
- Timeout (120 seconds) → CLEANUP → IDLE

**Fail-Safe**:
- Inactivity timeout: 120 seconds
- Data preservation during wait
- Clean cancellation path

**Transition Guards**:
```
if (userRequestsTransmit)
    → TX_GATED
else if (userCancels || timeout)
    → CLEANUP → IDLE
```

### 6. TX_GATED State

**Purpose**: Multi-stage approval gate for transmission

**Entry Conditions**:
- User requested transmission from READY

**Activities (Sequential Gates)**:

**Gate 1: Policy Check**
- Verify frequency is allowed
- Check blacklist
- Validate duration limits
- Confirm data integrity

**Gate 2: Safety Confirmation**
- Display warning to user
- Require explicit button confirmation
- 10-second timeout for confirmation

**Gate 3: Rate Limit Check**
- Verify rate limit not exceeded
- Check recent transmission history
- Calculate transmission window

**Gate 4: Final Validation**
- Re-verify all conditions
- Lock transmission parameters
- Prepare for transmission

**Exit Conditions**:
- All gates pass → TRANSMIT
- Any gate fails → READY (with error message)
- Timeout → READY (denied)
- User cancels → READY

**Fail-Safe**:
- Each gate has independent timeout (10 seconds)
- Default deny on timeout
- Audit logging of all decisions
- Complete state preservation for retry

**Transition Guards**:
```
if (allGatesPass && userConfirmed && !timeout)
    → TRANSMIT
else
    → READY (log denial reason)
```

### 7. TRANSMIT State

**Purpose**: Execute controlled RF transmission

**Entry Conditions**:
- All gates passed in TX_GATED

**Activities**:
1. Enable transmitter
2. Set transmission parameters
3. Execute transmission
4. Monitor transmission progress
5. Verify transmission completion
6. Record transmission metrics

**Exit Conditions**:
- Success → CLEANUP → IDLE
- Error → CLEANUP → IDLE

**Fail-Safe**:
- Maximum transmission duration enforced
- Watchdog timer protection
- Emergency stop capability
- Automatic transmitter disable

**Timing Constraints**:
- Maximum duration: 5 seconds (hard limit)
- Minimum spacing: 100ms between transmissions
- Enforced by hardware and software

**Transition Guards**:
```
if (transmissionComplete && verified)
    → CLEANUP → IDLE
else if (error || timeout)
    → CLEANUP → IDLE (emergency shutdown)
```

### 8. CLEANUP State

**Purpose**: Resource deallocation and state reset

**Entry Conditions**:
- Any terminal transition
- Error conditions
- Normal completion

**Activities (Always Execute)**:
1. Disable transmitter (if enabled)
2. Disable receiver
3. Free allocated buffers
4. Save logs
5. Clear sensitive data
6. Reset hardware to safe state
7. Update statistics

**Exit Conditions**:
- Always → IDLE

**Fail-Safe**:
- Unconditional execution
- No dependencies on prior state
- Multiple cleanup attempts if needed
- Timeout: 5 seconds, then force transition

**Transition Guards**:
```
// Always transitions to IDLE, no conditions
→ IDLE
```

## Frequency-Specific Workflows

### 433 MHz Workflow (CC1101)

**Focus**: Control packets, pulse analytics

#### Signal Characteristics
- OOK/ASK modulation
- Typical pulse lengths: 200-500 μs
- Protocol families: RCSwitch, Manchester, PWM
- Device types: Remotes, sensors, alarms

#### Workflow Specialization

**LISTENING Phase**:
- Capture minimum 10 pulses per signal
- Record precise timing (μs resolution)
- Store pulse train patterns
- Minimum observation: 2 seconds

**ANALYZING Phase**:
- Pulse width analysis
- Protocol detection
- Device classification
- Timing pattern recognition
- Repetition analysis

**TX_GATED Phase**:
- Additional gate: Pulse validation
- Verify pulse timing within tolerances
- Check for protocol compliance

**Priority**: Control Packets
- Door remotes
- Garage door openers
- Light switches
- Security system controls

### 2.4 GHz Workflow (nRF24L01+)

**Focus**: Packet-binding, filtered gate-action

#### Signal Characteristics
- GFSK modulation
- Packet-based communication
- Address-based filtering
- Data rate: 250kbps - 2Mbps

#### Workflow Specialization

**LISTENING Phase**:
- Capture complete packets
- Record packet addressing
- Monitor multiple channels
- Minimum observation: 5 seconds

**ANALYZING Phase**:
- Packet structure analysis
- Address pattern recognition
- Channel usage analysis
- Data rate detection
- Binding pair identification

**TX_GATED Phase**:
- Additional gate: Address binding verification
- Validate destination address
- Check packet structure
- Confirm channel availability

**Filtered Gate-Action**:
- Only allow transmission to observed addresses
- Verify binding establishment
- Restrict to captured channels
- Require packet ACK verification

**Priority**: Packet-Binding
- Wireless keyboards/mice
- Game controllers
- IoT devices
- Custom RF links

## State Transition Timing

### Timeout Values

| Transition | Timeout | Fail-Safe Action |
|------------|---------|------------------|
| IDLE → INIT | - | None (user triggered) |
| INIT → LISTENING | 5 sec | Return to IDLE |
| LISTENING → ANALYZING | 60 sec | Force analysis |
| ANALYZING → READY | 10 sec | Return to LISTENING |
| READY → TX_GATED | 120 sec | Return to IDLE |
| TX_GATED → TRANSMIT | 10 sec | Deny, return to READY |
| TRANSMIT → CLEANUP | 5 sec | Emergency cleanup |
| CLEANUP → IDLE | 5 sec | Force return |

### State Durations

| State | Min Duration | Max Duration | Typical |
|-------|--------------|--------------|---------|
| IDLE | 0 | ∞ | Variable |
| INIT | 0.5 sec | 5 sec | 1 sec |
| LISTENING | 1 sec | 60 sec | 10 sec |
| ANALYZING | 0.1 sec | 10 sec | 2 sec |
| READY | 0 | 120 sec | Variable |
| TX_GATED | 0.5 sec | 10 sec | 3 sec |
| TRANSMIT | 0.01 sec | 5 sec | 0.5 sec |
| CLEANUP | 0.1 sec | 5 sec | 0.5 sec |

## Error Handling and Recovery

### Error Categories

#### 1. Initialization Errors
- Hardware not responding
- Resource allocation failure
- Configuration error

**Recovery**: Return to IDLE, log error, retry available

#### 2. Runtime Errors
- Buffer overflow
- Signal decoding error
- Timing violation

**Recovery**: Transition to CLEANUP, preserve partial data, log error

#### 3. Transmission Errors
- Hardware failure during TX
- Verification failure
- Emergency stop triggered

**Recovery**: Immediate transmitter disable, CLEANUP, comprehensive logging

#### 4. Timeout Errors
- State duration exceeded
- User response timeout
- System watchdog

**Recovery**: State-specific timeout handler, graceful degradation

### Recovery Procedures

```
For each error:
1. Log error details (type, state, timestamp)
2. Disable any active transmission
3. Preserve captured data if possible
4. Execute CLEANUP state
5. Transition to IDLE
6. Set error flag for next workflow
```

## Safety Guarantees

### Passive-Listening First

**Guarantee**: No transmission possible before observation completes

**Implementation**:
- Transmitter disabled until TX_GATED state
- Physical separation of RX/TX enable signals
- Software locks preventing premature transmission
- State machine enforcement

### Complete Observational Capture

**Guarantee**: All observation data captured before any modification

**Implementation**:
- Immutable capture buffers
- Copy-on-write for analysis
- Original data preserved through workflow
- Traceability maintained

### Fail-Safe Modes

**Guarantee**: System defaults to safe state on any error

**Implementation**:
- Default state is IDLE (no transmission)
- Watchdog timers at every state
- Exception handlers
- Hardware-level transmitter disable

### Structured Safeguards

**Guarantee**: Multiple independent safety layers

**Implementation**:
- Hardware: Physical transmitter enable
- Firmware: State machine guards
- Software: Policy enforcement
- User: Explicit confirmation

### RF Timing Traceability

**Guarantee**: Complete timing history for forensics/debugging

**Implementation**:
- Microsecond timestamp resolution
- Signal-level timing capture
- State transition logging
- Audit trail preservation

## Workflow Usage Examples

### Example 1: 433 MHz Signal Analysis (No Transmission)

```
1. User initiates workflow
   IDLE → INIT

2. System initializes 433 MHz receiver
   INIT → LISTENING (success)

3. System passively captures signals for 10 seconds
   State: LISTENING
   - Captures 5 door remote signals
   - Records pulse timings
   - Measures RSSI

4. Analysis triggered (manual or timeout)
   LISTENING → ANALYZING

5. System analyzes captured data
   State: ANALYZING
   - Identifies protocol: RCSwitch protocol 1
   - Classifies: Garage door remote
   - Calculates: Pulse length 350μs ±5μs

6. Results presented to user
   ANALYZING → READY

7. User decides not to transmit
   READY → CLEANUP → IDLE
```

### Example 2: 2.4 GHz Packet Capture with Gated Transmission

```
1. User initiates workflow
   IDLE → INIT

2. System initializes nRF24 in receive mode
   INIT → LISTENING (success)

3. System captures packets on multiple channels
   State: LISTENING
   - Scans channels 1-80
   - Captures packets on channel 42
   - Records addresses: 0xE7E7E7E7E7

4. Sufficient data collected
   LISTENING → ANALYZING

5. System analyzes packet structure
   State: ANALYZING
   - Identifies binding pair
   - Extracts packet format
   - Validates CRC

6. Results ready
   ANALYZING → READY

7. User requests transmission
   READY → TX_GATED

8. Gate sequence:
   State: TX_GATED
   - Gate 1: Policy check → PASS
   - Gate 2: User confirmation → PASS (button pressed)
   - Gate 3: Rate limit → PASS
   - Gate 4: Address binding verification → PASS

9. Transmission approved
   TX_GATED → TRANSMIT

10. Execute transmission
    State: TRANSMIT
    - Set channel 42
    - Set address 0xE7E7E7E7E7
    - Transmit packet
    - Verify ACK received

11. Cleanup
    TRANSMIT → CLEANUP → IDLE
```

### Example 3: Fail-Safe on Timeout

```
1. User initiates workflow
   IDLE → INIT

2. System initializes
   INIT → LISTENING

3. Observation in progress
   State: LISTENING
   - Capturing signals...
   - User walks away
   - 60 seconds elapsed

4. Automatic timeout
   LISTENING → ANALYZING (forced)

5. Analysis with available data
   State: ANALYZING
   - Processes partial capture
   - Generates results

6. Waiting for user response
   ANALYZING → READY
   - User still absent
   - 120 seconds elapsed

7. Inactivity timeout
   READY → CLEANUP → IDLE
   - No transmission occurred
   - Data preserved in log
```

## Integration with Existing Modules

### RF433Module Integration

The workflow uses `RF433Module` for:
- Signal reception: `receiveSignal()`
- Transmission: `transmitSignal()` (with safety checks)
- Classification: `classifySignal()`

### RF24Module Integration

The workflow uses `RF24Module` for:
- Packet reception: ESP-NOW or custom protocols
- Channel scanning: `setChannel()`
- Transmission: `sendMessage()` (with binding verification)

### SafetyModule Integration

The workflow integrates with `SafetyModule` at TX_GATED state:
- Policy checks: `checkTransmitPolicy()`
- User confirmation: `waitForUserConfirmation()`
- Rate limiting: `isRateLimitOK()`
- Audit logging: `logTransmitAttempt()`

## Configuration Parameters

### Workflow Configuration

```cpp
// Timing parameters
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
```

## Logging and Audit Trail

### State Transition Logging

Every state transition is logged with:
- Timestamp (microsecond precision)
- Previous state
- New state
- Trigger reason
- Data snapshot

### Signal Capture Logging

Every captured signal includes:
- Reception timestamp
- Signal characteristics
- RSSI
- Decoding result
- Classification

### Transmission Logging

Every transmission attempt logs:
- Request timestamp
- Gate results (pass/fail for each)
- Final decision
- Transmission timestamp (if approved)
- Verification result

## Testing and Validation

### State Machine Testing

1. Verify all state transitions
2. Test timeout handling for each state
3. Validate fail-safe transitions
4. Confirm cleanup always executes

### Timing Testing

1. Measure state durations
2. Verify timeout enforcement
3. Test watchdog functionality
4. Validate timing constraints

### Safety Testing

1. Attempt transmission without observation
2. Test gate rejection scenarios
3. Verify emergency stop
4. Validate audit trail completeness

## Compliance and Regulatory Notes

This workflow system is designed to support regulatory compliance:

- **FCC Part 15**: Observation-first prevents interference
- **CE RED**: Fail-safe design ensures safety
- **Local Regulations**: Blacklist support for restricted frequencies
- **Responsible Use**: Multiple confirmation layers

Users remain responsible for compliance with applicable regulations.

## Formal Invariants and Safety Guarantees

The workflow system enforces formal invariants that guarantee safety properties. These invariants are:

1. **Documented** in `INVARIANTS.md` for reviewers, auditors, and contributors
2. **Enforced** through code structure and state machine design
3. **Verified** by runtime assertions in debug builds

Key invariants include:
- TX hardware disabled in all states except TRANSMIT (SAFE-TX-1)
- All failures exit via CLEANUP (INV-SM-5, ERR-RECOV-1)
- User confirmation mandatory for transmission (TX-CONF-1)
- Buffer overflow protection (RES-BUF-1)
- Rate limiting enforcement (TX-RATE-1)

For complete invariant documentation, see **INVARIANTS.md**.

---

**Document Version**: 1.1  
**Last Updated**: 2025-12-14  
**Status**: Complete
