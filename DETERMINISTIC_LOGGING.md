# Deterministic State Transition Logging

## Overview

The deterministic state transition logging mode provides structured, machine-readable logs of every state entry, exit, and transition in the RF Test Workflow system. This feature enables replay, debugging, and compliance review.

## Why Deterministic Logging?

While microsecond timestamps are useful for timing analysis, deterministic logs focusing on **state**, **event**, and **reason** provide unique benefits:

- **Replay**: Recreate exact workflow execution sequences
- **Debugging**: Trace state transitions with explicit causes
- **Compliance**: Complete audit trail for regulatory review
- **Testing**: Automated validation of workflow behavior

## Features

### 1. State Entry/Exit Tracking

Every state transition logs both entry and exit events:

```
EXIT_IDLE → TRANSITION → ENTER_INIT
```

This provides complete visibility into state boundaries, making it easy to identify:
- How long a state was active
- What triggered the exit
- What caused the entry

### 2. Structured Event Types

Six event types provide semantic meaning:

- `STATE_ENTRY`: Entering a workflow state
- `STATE_EXIT`: Exiting a workflow state  
- `TRANSITION`: State machine transition
- `ERROR`: Error conditions
- `USER_ACTION`: User interactions
- `TIMEOUT`: Timeout events

### 3. Machine-Readable Format

Logs are structured for programmatic access:

#### Serial Format
```
[DET_LOG] seq=0 ts_ms=1000 ts_us=1000000 type=STATE_ENTRY state=INIT prev=IDLE event=ENTER_INIT reason=User started workflow data=
```

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

### 4. Microsecond Precision

Timestamps include both millisecond and microsecond resolution for precise timing analysis.

## API Reference

### Enable/Disable Logging

```cpp
RFTestWorkflow workflow;

// Enable deterministic logging (enabled by default)
workflow.enableDeterministicLogging(true);

// Disable if needed
workflow.enableDeterministicLogging(false);

// Check status
if (workflow.isDeterministicLoggingEnabled()) {
    Serial.println("Deterministic logging is active");
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
        Serial.printf("Seq: %lu, Event: %s, Reason: %s\n", 
                     entry->sequenceNumber, 
                     entry->event, 
                     entry->reason);
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

// Save to SD card or transmit over network
// (implementation depends on your storage/network setup)
```

### Clear Logs

```cpp
// Clear all logs (including deterministic logs)
workflow.clearLogs();

// Clear only deterministic logs
workflow.clearDeterministicLogs();
```

## Configuration

Edit `include/config.h` to configure deterministic logging:

```cpp
// Enable/disable at compile time
#define ENABLE_DETERMINISTIC_LOGGING true

// Maximum number of log entries (FIFO buffer)
#define DETERMINISTIC_LOG_MAX_ENTRIES 200
```

When the buffer is full, oldest entries are removed (FIFO) to make room for new entries.

## Data Structure

```cpp
enum DeterministicEventType {
    DET_EVENT_STATE_ENTRY = 0,
    DET_EVENT_STATE_EXIT = 1,
    DET_EVENT_TRANSITION = 2,
    DET_EVENT_ERROR = 3,
    DET_EVENT_USER_ACTION = 4,
    DET_EVENT_TIMEOUT = 5
};

struct DeterministicLogEntry {
    uint32_t sequenceNumber;        // Sequential entry number for ordering
    uint32_t timestampMs;           // Millisecond timestamp
    uint32_t timestampUs;           // Microsecond timestamp (for precision)
    DeterministicEventType eventType;
    WorkflowState state;            // Current state
    WorkflowState prevState;        // Previous state (for transitions)
    char event[32];                 // Event identifier
    char reason[64];                // Reason/cause for event
    char data[64];                  // Additional data (JSON fragment or key=value)
};
```

## Use Cases

### 1. Debugging Workflow Issues

When troubleshooting unexpected behavior:

```cpp
// Run workflow
workflow.start();

// Export logs
String logs = workflow.exportDeterministicLogsJSON();

// Analyze the sequence of events
// Look for:
// - Unexpected transitions
// - Missing user actions
// - Timeout events
// - Error conditions
```

### 2. Automated Testing

Validate workflow behavior in tests:

```cpp
void testWorkflowSequence() {
    workflow.clearLogs();
    workflow.start();
    
    // Check expected sequence
    assert(workflow.getDeterministicLogCount() > 0);
    
    const DeterministicLogEntry* firstEntry = workflow.getDeterministicLog(0);
    assert(firstEntry->eventType == DET_EVENT_STATE_EXIT);
    assert(firstEntry->state == WF_IDLE);
}
```

### 3. Compliance Auditing

For regulatory compliance:

```cpp
// Run RF operations
workflow.start();

// Export complete audit trail
String auditLog = workflow.exportDeterministicLogsJSON();

// Save to secure storage
saveToSDCard("audit_log_" + getTimestamp() + ".json", auditLog);

// The log includes:
// - All state transitions
// - User confirmations
// - Safety gate results
// - Transmission events
// - Error conditions
```

### 4. Performance Analysis

Analyze timing characteristics:

```cpp
// Export logs as CSV for spreadsheet analysis
String csvLogs = workflow.exportDeterministicLogsCSV();

// Analyze in Excel/Python/R:
// - State durations (exit timestamp - entry timestamp)
// - Transition frequencies
// - User response times
// - Timeout occurrences
```

### 5. Replay and Simulation

Recreate workflow scenarios:

```cpp
// Load previous log
String previousLog = loadFromSDCard("previous_run.json");

// Parse log and extract events
// Use events to drive simulation or replay
// Useful for:
// - Training scenarios
// - Regression testing
// - Incident recreation
```

## Example Log Sequence

Here's a complete workflow execution captured in deterministic logs:

```
[DET_LOG] seq=0 ts_ms=1000 ts_us=1000000 type=STATE_EXIT state=IDLE prev=IDLE event=EXIT_IDLE reason=User started workflow data=
[DET_LOG] seq=1 ts_ms=1001 ts_us=1001100 type=TRANSITION state=INIT prev=IDLE event=TRANSITION reason=User started workflow data=from=IDLE to=INIT
[DET_LOG] seq=2 ts_ms=1002 ts_us=1002200 type=STATE_ENTRY state=INIT prev=IDLE event=ENTER_INIT reason=User started workflow data=
[DET_LOG] seq=3 ts_ms=2000 ts_us=2000000 type=STATE_EXIT state=INIT prev=IDLE event=EXIT_INIT reason=Init successful data=
[DET_LOG] seq=4 ts_ms=2001 ts_us=2001100 type=TRANSITION state=LISTENING prev=INIT event=TRANSITION reason=Init successful data=from=INIT to=LISTENING
[DET_LOG] seq=5 ts_ms=2002 ts_us=2002200 type=STATE_ENTRY state=LISTENING prev=INIT event=ENTER_LISTENING reason=Init successful data=
[DET_LOG] seq=6 ts_ms=5000 ts_us=5000000 type=USER_ACTION state=LISTENING prev=INIT event=TRIGGER_ANALYSIS reason=User manually triggered analysis data=
[DET_LOG] seq=7 ts_ms=5001 ts_us=5001100 type=STATE_EXIT state=LISTENING prev=INIT event=EXIT_LISTENING reason=User trigger data=
[DET_LOG] seq=8 ts_ms=5002 ts_us=5002200 type=TRANSITION state=ANALYZING prev=LISTENING event=TRANSITION reason=User trigger data=from=LISTENING to=ANALYZING
[DET_LOG] seq=9 ts_ms=5003 ts_us=5003300 type=STATE_ENTRY state=ANALYZING prev=LISTENING event=ENTER_ANALYZING reason=User trigger data=
[DET_LOG] seq=10 ts_ms=7000 ts_us=7000000 type=STATE_EXIT state=ANALYZING prev=LISTENING event=EXIT_ANALYZING reason=Analysis complete data=
[DET_LOG] seq=11 ts_ms=7001 ts_us=7001100 type=TRANSITION state=READY prev=ANALYZING event=TRANSITION reason=Analysis complete data=from=ANALYZING to=READY
[DET_LOG] seq=12 ts_ms=7002 ts_us=7002200 type=STATE_ENTRY state=READY prev=ANALYZING event=ENTER_READY reason=Analysis complete data=
[DET_LOG] seq=13 ts_ms=10000 ts_us=10000000 type=USER_ACTION state=READY prev=ANALYZING event=SELECT_SIGNAL reason=User selected signal for transmission data=signal_index=0
[DET_LOG] seq=14 ts_ms=10001 ts_us=10001100 type=STATE_EXIT state=READY prev=ANALYZING event=EXIT_READY reason=User requested transmission data=
[DET_LOG] seq=15 ts_ms=10002 ts_us=10002200 type=TRANSITION state=TX_GATED prev=READY event=TRANSITION reason=User requested transmission data=from=READY to=TX_GATED
[DET_LOG] seq=16 ts_ms=10003 ts_us=10003300 type=STATE_ENTRY state=TX_GATED prev=READY event=ENTER_TX_GATED reason=User requested transmission data=
[DET_LOG] seq=17 ts_ms=12000 ts_us=12000000 type=USER_ACTION state=TX_GATED prev=READY event=CONFIRM_TX reason=User confirmed transmission data=
[DET_LOG] seq=18 ts_ms=15000 ts_us=15000000 type=STATE_EXIT state=TX_GATED prev=READY event=EXIT_TX_GATED reason=All gates passed data=
[DET_LOG] seq=19 ts_ms=15001 ts_us=15001100 type=TRANSITION state=TRANSMIT prev=TX_GATED event=TRANSITION reason=All gates passed data=from=TX_GATED to=TRANSMIT
[DET_LOG] seq=20 ts_ms=15002 ts_us=15002200 type=STATE_ENTRY state=TRANSMIT prev=TX_GATED event=ENTER_TRANSMIT reason=All gates passed data=
[DET_LOG] seq=21 ts_ms=16000 ts_us=16000000 type=STATE_EXIT state=TRANSMIT prev=TX_GATED event=EXIT_TRANSMIT reason=Transmit success data=
[DET_LOG] seq=22 ts_ms=16001 ts_us=16001100 type=TRANSITION state=CLEANUP prev=TRANSMIT event=TRANSITION reason=Transmit success data=from=TRANSMIT to=CLEANUP
[DET_LOG] seq=23 ts_ms=16002 ts_us=16002200 type=STATE_ENTRY state=CLEANUP prev=TRANSMIT event=ENTER_CLEANUP reason=Transmit success data=
[DET_LOG] seq=24 ts_ms=17000 ts_us=17000000 type=STATE_EXIT state=CLEANUP prev=TRANSMIT event=EXIT_CLEANUP reason=Cleanup done data=
[DET_LOG] seq=25 ts_ms=17001 ts_us=17001100 type=TRANSITION state=IDLE prev=CLEANUP event=TRANSITION reason=Cleanup done data=from=CLEANUP to=IDLE
[DET_LOG] seq=26 ts_ms=17002 ts_us=17002200 type=STATE_ENTRY state=IDLE prev=CLEANUP event=ENTER_IDLE reason=Cleanup done data=
```

## Best Practices

1. **Enable for Development**: Always enable during development and testing
2. **Export Regularly**: Export logs after each test run for analysis
3. **Review Sequences**: Look for unexpected patterns in state transitions
4. **Check Timing**: Use microsecond timestamps to identify performance issues
5. **Store for Audit**: Save logs when operating in regulated environments
6. **Automate Testing**: Use logs to validate workflow behavior automatically
7. **Buffer Management**: Monitor buffer usage and export before overflow
8. **Integration**: Parse logs into monitoring/alerting systems

## Performance Considerations

- Log entries are stored in RAM (default: 200 entries max)
- Each entry uses approximately 200 bytes
- Total memory usage: ~40KB for 200 entries
- FIFO buffer automatically removes oldest entries when full
- Minimal overhead: logging only occurs during state transitions
- Export operations may take time on Serial (consider SD card for large logs)

## Troubleshooting

### No Logs Generated

- Check if logging is enabled: `workflow.isDeterministicLoggingEnabled()`
- Verify workflow has been executed
- Ensure Serial monitor is connected at 115200 baud

### Missing Events

- Check if buffer is full (oldest entries removed)
- Increase `DETERMINISTIC_LOG_MAX_ENTRIES` in config.h
- Export logs more frequently

### Export Issues

- JSON format is verbose - may be slow on Serial
- Use CSV for faster exports
- Consider SD card storage for large logs
- Break large exports into chunks if needed

## Integration Examples

### With ELK Stack

```cpp
// Export JSON and send to Logstash
String jsonLogs = workflow.exportDeterministicLogsJSON();
httpClient.post("http://logstash:5000/", jsonLogs);
```

### With Database

```cpp
// Parse and store in database
for (int i = 0; i < workflow.getDeterministicLogCount(); i++) {
    const DeterministicLogEntry* entry = workflow.getDeterministicLog(i);
    db.execute("INSERT INTO workflow_logs VALUES (?, ?, ?, ?, ?)",
               entry->sequenceNumber,
               entry->timestampMs,
               entry->event,
               entry->reason,
               entry->data);
}
```

### With Test Framework

```cpp
TEST(WorkflowTest, CorrectTransitionSequence) {
    workflow.clearLogs();
    workflow.start();
    
    // Verify expected event sequence
    ASSERT_EQ(workflow.getDeterministicLogCount(), 27);
    
    const DeterministicLogEntry* firstExit = workflow.getDeterministicLog(0);
    EXPECT_EQ(firstExit->eventType, DET_EVENT_STATE_EXIT);
    EXPECT_EQ(firstExit->state, WF_IDLE);
    
    const DeterministicLogEntry* firstEntry = workflow.getDeterministicLog(2);
    EXPECT_EQ(firstEntry->eventType, DET_EVENT_STATE_ENTRY);
    EXPECT_EQ(firstEntry->state, WF_INIT);
}
```

## Related Documentation

- [WORKFLOWS.md](WORKFLOWS.md) - Complete workflow state machine documentation
- [TESTING.md](TESTING.md) - Testing guide for RF Suite
- [examples/deterministic_logging_demo/](examples/deterministic_logging_demo/) - Demo application
- [include/rf_test_workflow.h](include/rf_test_workflow.h) - API reference

## License

Part of M5Stack RF Suite - See LICENSE file for details.

---

**Last Updated**: 2025-12-14  
**Version**: 1.0.0
