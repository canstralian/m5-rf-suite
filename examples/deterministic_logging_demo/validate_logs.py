#!/usr/bin/env python3
"""
Validate Deterministic Logs

This script validates the format and consistency of deterministic logs
exported from the RF Test Workflow system.

Usage:
    python validate_logs.py <log_file.json>
    python validate_logs.py <log_file.csv>

The script checks:
- Sequence numbers are monotonically increasing
- Timestamps are monotonically increasing
- State transitions are valid
- Event types are recognized
- Entry/exit pairs match
"""

import sys
import json
import csv
import argparse
from typing import List, Dict, Optional

# Valid event types
VALID_EVENT_TYPES = {
    "STATE_ENTRY",
    "STATE_EXIT",
    "TRANSITION",
    "ERROR",
    "USER_ACTION",
    "TIMEOUT"
}

# Valid workflow states
VALID_STATES = {
    "IDLE",
    "INIT",
    "LISTENING",
    "ANALYZING",
    "READY",
    "TX_GATED",
    "TRANSMIT",
    "CLEANUP"
}

class LogValidator:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.stats = {
            "total_entries": 0,
            "state_entries": 0,
            "state_exits": 0,
            "transitions": 0,
            "user_actions": 0,
            "timeouts": 0,
            "errors": 0
        }
        
    def validate_json(self, filename: str) -> bool:
        """Validate JSON format log file"""
        try:
            with open(filename, 'r') as f:
                data = json.load(f)
                
            if "workflow_logs" not in data:
                self.errors.append("Missing 'workflow_logs' key in JSON")
                return False
                
            logs = data["workflow_logs"]
            return self.validate_entries(logs)
            
        except json.JSONDecodeError as e:
            self.errors.append(f"Invalid JSON: {e}")
            return False
        except Exception as e:
            self.errors.append(f"Error reading file: {e}")
            return False
            
    def validate_csv(self, filename: str) -> bool:
        """Validate CSV format log file"""
        try:
            logs = []
            with open(filename, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    # Convert string values to appropriate types
                    entry = {
                        "seq": int(row["sequence"]),
                        "timestamp_ms": int(row["timestamp_ms"]),
                        "timestamp_us": int(row["timestamp_us"]),
                        "event_type": row["event_type"],
                        "state": row["state"],
                        "prev_state": row["prev_state"],
                        "event": row["event"],
                        "reason": row["reason"],
                        "data": row["data"]
                    }
                    logs.append(entry)
                    
            return self.validate_entries(logs)
            
        except Exception as e:
            self.errors.append(f"Error reading CSV: {e}")
            return False
            
    def validate_entries(self, entries: List[Dict]) -> bool:
        """Validate log entries"""
        if not entries:
            self.errors.append("No log entries found")
            return False
            
        self.stats["total_entries"] = len(entries)
        
        prev_seq = -1
        prev_ts_ms = -1
        prev_ts_us = -1
        state_stack = []
        
        for i, entry in enumerate(entries):
            # Validate sequence number
            seq = entry.get("seq", entry.get("sequenceNumber", -1))
            if seq <= prev_seq:
                self.errors.append(f"Entry {i}: Sequence number {seq} not increasing (prev: {prev_seq})")
            prev_seq = seq
            
            # Validate timestamps
            ts_ms = entry.get("timestamp_ms", entry.get("timestampMs", -1))
            ts_us = entry.get("timestamp_us", entry.get("timestampUs", -1))
            
            if ts_ms < prev_ts_ms:
                self.warnings.append(f"Entry {i}: Timestamp MS {ts_ms} < previous {prev_ts_ms}")
            if ts_us < prev_ts_us:
                self.warnings.append(f"Entry {i}: Timestamp US {ts_us} < previous {prev_ts_us}")
                
            prev_ts_ms = ts_ms
            prev_ts_us = ts_us
            
            # Validate event type
            event_type = entry.get("event_type", entry.get("eventType", ""))
            if event_type not in VALID_EVENT_TYPES:
                self.errors.append(f"Entry {i}: Invalid event type: {event_type}")
                
            # Update stats
            if event_type == "STATE_ENTRY":
                self.stats["state_entries"] += 1
            elif event_type == "STATE_EXIT":
                self.stats["state_exits"] += 1
            elif event_type == "TRANSITION":
                self.stats["transitions"] += 1
            elif event_type == "USER_ACTION":
                self.stats["user_actions"] += 1
            elif event_type == "TIMEOUT":
                self.stats["timeouts"] += 1
            elif event_type == "ERROR":
                self.stats["errors"] += 1
                
            # Validate state
            state = entry.get("state", "")
            prev_state = entry.get("prev_state", entry.get("prevState", ""))
            
            if state not in VALID_STATES:
                self.errors.append(f"Entry {i}: Invalid state: {state}")
            if prev_state and prev_state not in VALID_STATES:
                self.errors.append(f"Entry {i}: Invalid prev_state: {prev_state}")
                
            # Track state entry/exit pairs
            if event_type == "STATE_ENTRY":
                state_stack.append((state, i))
            elif event_type == "STATE_EXIT":
                if state_stack:
                    entry_state, entry_idx = state_stack.pop()
                    if entry_state != state:
                        self.warnings.append(
                            f"Entry {i}: EXIT_{state} doesn't match ENTRY_{entry_state} at {entry_idx}"
                        )
                        
        # Check for unmatched entries
        if state_stack:
            for state, idx in state_stack:
                self.warnings.append(f"Entry {idx}: ENTER_{state} has no matching EXIT")
                
        # Validate entry/exit balance
        if self.stats["state_entries"] != self.stats["state_exits"]:
            self.warnings.append(
                f"Unbalanced state entries/exits: {self.stats['state_entries']} entries, "
                f"{self.stats['state_exits']} exits"
            )
            
        return len(self.errors) == 0
        
    def print_report(self):
        """Print validation report"""
        print("\n" + "=" * 60)
        print("DETERMINISTIC LOG VALIDATION REPORT")
        print("=" * 60)
        
        print(f"\nTotal Entries: {self.stats['total_entries']}")
        print(f"  State Entries: {self.stats['state_entries']}")
        print(f"  State Exits: {self.stats['state_exits']}")
        print(f"  Transitions: {self.stats['transitions']}")
        print(f"  User Actions: {self.stats['user_actions']}")
        print(f"  Timeouts: {self.stats['timeouts']}")
        print(f"  Errors: {self.stats['errors']}")
        
        if self.errors:
            print(f"\n❌ ERRORS ({len(self.errors)}):")
            for error in self.errors:
                print(f"  - {error}")
        else:
            print("\n✅ No errors found")
            
        if self.warnings:
            print(f"\n⚠️  WARNINGS ({len(self.warnings)}):")
            for warning in self.warnings:
                print(f"  - {warning}")
        else:
            print("✅ No warnings")
            
        print("\n" + "=" * 60)
        
        if self.errors:
            print("RESULT: ❌ FAILED")
        elif self.warnings:
            print("RESULT: ⚠️  PASSED WITH WARNINGS")
        else:
            print("RESULT: ✅ PASSED")
            
        print("=" * 60 + "\n")

def main():
    parser = argparse.ArgumentParser(
        description="Validate deterministic log files from RF Test Workflow"
    )
    parser.add_argument("logfile", help="Log file to validate (JSON or CSV)")
    parser.add_argument("-v", "--verbose", action="store_true", 
                       help="Verbose output")
    
    args = parser.parse_args()
    
    validator = LogValidator()
    
    # Detect file format
    if args.logfile.endswith(".json"):
        print(f"Validating JSON log file: {args.logfile}")
        success = validator.validate_json(args.logfile)
    elif args.logfile.endswith(".csv"):
        print(f"Validating CSV log file: {args.logfile}")
        success = validator.validate_csv(args.logfile)
    else:
        print("Error: Unknown file format. Use .json or .csv")
        sys.exit(1)
        
    validator.print_report()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
