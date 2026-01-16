# Deterministic Logging Demo

This example demonstrates the deterministic state transition logging feature added to the RF Test Workflow system.

## Overview

Deterministic logging provides structured, machine-readable logs of every state transition, user action, and system event. This enables:

- **Replay**: Recreate exact workflow execution for debugging
- **Debugging**: Trace state transitions with precise timing
- **Compliance**: Audit trail for regulatory review
- **Testing**: Automated validation of workflow behavior

## Features Demonstrated

### 1. State Entry/Exit Logging

Every state transition is logged with both entry and exit events:

```
[DET_LOG] seq=0 ts_ms=1000 ts_us=1000000 type=STATE_EXIT state=IDLE prev=IDLE event=EXIT_IDLE reason=User started workflow
[DET_LOG] seq=1 ts_ms=1001 ts_us=1001000 type=TRANSITION state=INIT prev=IDLE event=TRANSITION reason=User started workflow
[DET_LOG] seq=2 ts_ms=1002 ts_us=1002000 type=STATE_ENTRY state=INIT prev=IDLE event=ENTER_INIT reason=User started workflow
```

### 2. Transition Cause Recording

Each transition includes the reason/cause:

- User actions (trigger analysis, confirm transmission, etc.)
- Timeouts (listen timeout, gate timeout, etc.)
- System events (buffer full, analysis complete, etc.)
- Errors (init failed, transmission failed, etc.)

### 3. Machine-Readable Format

Logs can be exported in two formats:

#### JSON Format

```json
{
  "workflow_logs": [
    {
      "seq": 0,
      "timestamp_ms": 1000,
      "timestamp_us": 1000000,
      "event_type": "STATE_ENTRY",
      "state": "INIT",
      "prev_state": "IDLE",
      "event": "ENTER_INIT",
      "reason": "User started workflow",
      "data": ""
    }
  ]
}
```

#### CSV Format

```csv
sequence,timestamp_ms,timestamp_us,event_type,state,prev_state,event,reason,data
0,1000,1000000,STATE_ENTRY,INIT,IDLE,ENTER_INIT,User started workflow,
```

### 4. Event Types

The logging system tracks multiple event types:

- `STATE_ENTRY`: Entering a workflow state
- `STATE_EXIT`: Exiting a workflow state
- `TRANSITION`: State machine transition
- `ERROR`: Error conditions
- `USER_ACTION`: User interactions
- `TIMEOUT`: Timeout events

## How to Use

### Running the Demo

1. Upload the sketch to your M5Stack Core2
2. Follow the on-screen prompts
3. Monitor the Serial output at 115200 baud
4. Press buttons to interact with the demo

### Button Controls

- **Button A**: Previous/Cancel
- **Button B**: Select/Confirm
- **Button C**: Next/Skip

### Demo Flow

1. **Intro Screen**: Overview of deterministic logging
2. **Workflow Demo**: Simulated workflow with logging
3. **Log Summary**: Review captured log entries
4. **Export**: Export logs to Serial in JSON and CSV formats

## API Usage

### Enable/Disable Logging

```cpp
RFTestWorkflow workflow;

// Enable deterministic logging
workflow.enableDeterministicLogging(true);

// Check if enabled
if (workflow.isDeterministicLoggingEnabled()) {
    Serial.println("Logging is active");
}
```

### Access Log Entries

```cpp
// Get number of log entries
int count = workflow.getDeterministicLogCount();

// Access individual entries
for (int i = 0; i < count; i++) {
    const DeterministicLogEntry* entry = workflow.getDeterministicLog(i);
    if (entry) {
        Serial.printf("Event: %s, Reason: %s\n", entry->event, entry->reason);
    }
}
```

### Export Logs

```cpp
// Export as JSON
String jsonLogs = workflow.exportDeterministicLogsJSON();
Serial.println(jsonLogs);

// Export as CSV
String csvLogs = workflow.exportDeterministicLogsCSV();
Serial.println(csvLogs);
```

### Clear Logs

```cpp
// Clear all logs
workflow.clearLogs();

// Or clear just deterministic logs
workflow.clearDeterministicLogs();
```

## Configuration

The deterministic logging feature can be configured in `include/config.h`:

```cpp
// Enable/disable at compile time
#define ENABLE_DETERMINISTIC_LOGGING true

// Maximum number of log entries (FIFO buffer)
#define DETERMINISTIC_LOG_MAX_ENTRIES 200
```

## Use Cases

### 1. Debugging Workflow Issues

When a workflow fails or behaves unexpectedly, export the deterministic logs to see the exact sequence of events, including timing information.

### 2. Automated Testing

Use the structured log format to validate workflow behavior in automated tests. Compare expected vs actual state transitions.

### 3. Compliance Auditing

For regulatory compliance, the deterministic logs provide a complete audit trail of all RF operations, including user confirmations and safety checks.

### 4. Performance Analysis

The microsecond-precision timestamps enable detailed timing analysis of workflow execution.

### 5. Replay and Simulation

The structured logs can be used to replay workflow scenarios for training or simulation purposes.

## Log Entry Structure

```cpp
struct DeterministicLogEntry {
    uint32_t sequenceNumber;     // Sequential entry number
    uint32_t timestampMs;        // Millisecond timestamp
    uint32_t timestampUs;        // Microsecond timestamp (precision)
    DeterministicEventType eventType;  // Type of event
    WorkflowState state;         // Current state
    WorkflowState prevState;     // Previous state
    char event[32];              // Event identifier
    char reason[64];             // Reason/cause
    char data[64];               // Additional data
};
```

## Serial Output Example

```
=== Deterministic Logging Demo ===
[DET_LOG] seq=0 ts_ms=1000 ts_us=1000000 type=STATE_EXIT state=IDLE prev=IDLE event=EXIT_IDLE reason=User started workflow data=
[DET_LOG] seq=1 ts_ms=1001 ts_us=1001100 type=TRANSITION state=INIT prev=IDLE event=TRANSITION reason=User started workflow data=from=IDLE to=INIT
[DET_LOG] seq=2 ts_ms=1002 ts_us=1002200 type=STATE_ENTRY state=INIT prev=IDLE event=ENTER_INIT reason=User started workflow data=
[DET_LOG] seq=3 ts_ms=2000 ts_us=2000000 type=USER_ACTION state=LISTENING prev=INIT event=TRIGGER_ANALYSIS reason=User manually triggered analysis data=
```

## Integration with Other Systems

The structured log format makes it easy to integrate with:

- Log aggregation systems (ELK stack, Splunk, etc.)
- Database storage for historical analysis
- Monitoring and alerting systems
- Test automation frameworks
- Compliance tracking tools

## Best Practices

1. **Enable for Development**: Always enable deterministic logging during development and testing
2. **Export Regularly**: Export logs after each test run for analysis
3. **Review Transitions**: Pay attention to unexpected state transitions
4. **Check Timing**: Use microsecond timestamps to identify performance issues
5. **Store for Audit**: Save logs for compliance review when operating in regulated environments

## Troubleshooting

### No Logs Appearing

- Check if deterministic logging is enabled: `workflow.isDeterministicLoggingEnabled()`
- Verify Serial monitor is connected at 115200 baud
- Ensure workflow has been executed (logs only created during workflow)

### Buffer Full

- If you exceed `DETERMINISTIC_LOG_MAX_ENTRIES`, oldest entries are removed (FIFO)
- Export logs regularly to avoid losing historical data
- Increase `DETERMINISTIC_LOG_MAX_ENTRIES` in config.h if needed

### Export Issues

- Large log exports may take time on Serial
- Consider using SD card or other storage for large exports
- JSON format is more verbose than CSV

## Related Documentation

- [WORKFLOWS.md](../../WORKFLOWS.md) - Complete workflow state machine documentation
- [TESTING.md](../../TESTING.md) - Testing guide for RF Suite
- [rf_test_workflow.h](../../include/rf_test_workflow.h) - API reference

## License

Part of M5Stack RF Suite - See main LICENSE file for details.
