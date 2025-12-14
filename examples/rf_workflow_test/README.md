# RF Workflow Test Example

This example demonstrates the RF Test Workflow system with comprehensive gate failure feedback for educational purposes.

## Features

### Educational Gate Outcomes

The example rotates through different transmission gate scenarios to help users understand why transmissions succeed or fail:

1. **Scenario 0: Success** - All gates pass, transmission proceeds
2. **Scenario 1: Policy Gate - Blacklisted Frequency** - Demonstrates frequency restrictions
3. **Scenario 2: Policy Gate - Duration Exceeded** - Shows transmission time limits
4. **Scenario 3: Rate Limit Gate** - Illustrates transmission rate throttling
5. **Scenario 4: Band-Specific Gate - Invalid Pulse Timing** - Shows 433 MHz validation

The scenario cycles based on the number of signals captured, so running the workflow multiple times will demonstrate different gate outcomes.

### Detailed Failure Information

When a gate fails, the display shows:
- **Gate Name**: Which gate failed
- **Reason**: Specific failure cause in simple terms
- **Details**: Technical information about the failure
- **Context**: Why this safety check exists

This helps users learn the safety model instead of just seeing generic "aborted" messages.

## Usage

1. Upload the sketch to your M5Stack Core2
2. Select "1. 433MHz Workflow" from the main menu
3. Press B to start capture
4. Press B again to analyze after capturing signals
5. Press B to attempt transmission (if signals were captured)
6. Observe the gate approval process and specific outcomes

Run the workflow multiple times to see different gate failure scenarios.

## Example Output

### Policy Gate Failure (Blacklisted Frequency)
```
Gate 1: Policy: FAILED

Reason: Blacklisted Frequency

Details:
Signal frequency 433.05 MHz
is on the restricted list.

Emergency services and
aviation bands are blocked
for safety.

This demonstrates why
transmissions fail.

Press any button...
```

### Rate Limit Gate Failure
```
Gate 3: Rate Limit: FAILED

Reason: Too Many Transmissions

Details:
Rate: 12 TX in last 60s
Limit: 10 TX per 60s

Rate limiting prevents
excessive RF usage and
ensures fair spectrum access.

This demonstrates why
transmissions fail.

Press any button...
```

### Band-Specific Gate Failure
```
Gate 4: Band Check: FAILED

Reason: Invalid Pulse Timing

Details:
Pulse #23: 45us (too short)
Valid range: 100-10000us

Malformed signals can cause
interference or device damage.

This demonstrates why
transmissions fail.

Press any button...
```

## Hardware Requirements

- M5Stack Core2 (or compatible)
- 433 MHz receiver module (GPIO 36)
- 433 MHz transmitter module (GPIO 26)
- USB cable for programming

## Learning Objectives

After using this example, users will understand:

1. **Multi-Gate Approval System**: Four independent gates protect transmission
2. **Policy Checks**: Frequency blacklists and duration limits
3. **User Confirmation**: Human oversight requirement
4. **Rate Limiting**: Throttling to prevent excessive transmissions
5. **Band-Specific Validation**: Technical checks for signal integrity

## Code Structure

- `displayGateFailure()`: Helper function to show detailed gate failure information
- `executeGatedTransmission()`: Demonstrates the gate approval process with scenario-based outcomes
- `manualWorkflowExecution()`: Simulates the complete workflow states

## Related Documentation

- [RF Test Workflow Usage Guide](../../WORKFLOW_USAGE.md)
- [Workflow Theory](../../WORKFLOWS.md)
- [Testing Guide](../../TESTING.md)
- [Security and Responsible Use](../../SECURITY.md)

## Version History

- **1.1.0**: Added specific gate outcome feedback with educational scenarios
- **1.0.0**: Initial example with basic workflow demonstration
