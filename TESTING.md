# Testing Guide for M5Stack RF Suite

This document describes how to test the RF Suite functionality.

## Prerequisites

- M5Stack device (Core2, Fire, or Stick-C)
- 433 MHz receiver module connected to GPIO 36
- 433 MHz transmitter module connected to GPIO 26
- USB cable for programming and monitoring
- PlatformIO or Arduino IDE installed

## Building and Uploading

### Using PlatformIO

```bash
# Build for M5Stack Core2
pio run -e m5stack-core2

# Upload to device
pio run -e m5stack-core2 --target upload

# Monitor serial output
pio device monitor
```

### Using Arduino IDE

1. Install required libraries via Library Manager:
   - M5Core2 (or M5Stack/M5StickC)
   - RCSwitch
   - ArduinoJson
   - ESP32Servo

2. Select board: Tools > Board > M5Stack-Core2

3. Select port: Tools > Port > (your device)

4. Click Upload

## Test Scenarios

### Test 1: Basic System Boot

**Objective**: Verify system initialization

**Steps**:
1. Upload firmware to device
2. Observe serial output
3. Check LCD display

**Expected Results**:
- Serial output shows module initialization messages
- Display shows "M5Stack RF Suite v1.0.0"
- "Initializing..." message appears
- Main menu displays after 2 seconds

**Pass Criteria**:
- ✅ No error messages in serial
- ✅ All modules report "ready"
- ✅ Display shows main menu

### Test 2: 433 MHz Signal Reception

**Objective**: Verify RF433 receiver functionality

**Steps**:
1. Navigate to "433MHz Scanner" from main menu
2. Press Button B to select
3. Press a 433 MHz remote control near the receiver
4. Observe display and serial output

**Expected Results**:
- Display shows "Scanning 433 MHz..."
- When signal received:
  - Beep sound
  - Signal details displayed
  - Serial output shows decoded value
- Signal counter increments

**Pass Criteria**:
- ✅ Signals are detected and decoded
- ✅ Signal details match remote (value, bits, protocol)
- ✅ Classification is reasonable (type detected)

### Test 3: 433 MHz Safe Transmission

**Objective**: Verify safe transmission workflow

**Steps**:
1. Capture at least one signal (Test 2)
2. Navigate to "433MHz Transmit"
3. Press Button B on a captured signal
4. Observe warning dialog
5. Press Button A to cancel
6. Repeat steps 3-4, press Button B to confirm

**Expected Results**:
- Warning dialog appears with orange background
- "TRANSMIT WARNING" message shown
- Cancellation (Button A) returns to menu
- Confirmation (Button B) transmits signal
- Success message displayed
- Audit log records attempt

**Pass Criteria**:
- ✅ Cannot transmit without confirmation
- ✅ Cancellation works correctly
- ✅ Confirmation triggers transmission
- ✅ Safety module logs the event

### Test 4: Wi-Fi Scanner

**Objective**: Verify Wi-Fi scanning capability

**Steps**:
1. Navigate to "WiFi Scanner"
2. Press Button B to start scan
3. Wait for scan to complete
4. Review results

**Expected Results**:
- "Scanning..." message appears
- After 3-5 seconds, networks displayed
- Networks sorted by signal strength (highest first)
- SSID, RSSI, and encryption shown
- Up to 10 networks displayed

**Pass Criteria**:
- ✅ Networks are discovered
- ✅ Information is accurate (compare with phone)
- ✅ RSSI values are reasonable

### Test 5: BLE Scanner

**Objective**: Verify BLE scanning capability

**Steps**:
1. Ensure nearby BLE devices (phone, watch, etc.)
2. Navigate to "BLE Scanner"
3. Press Button B to start scan
4. Wait for scan (5 seconds)
5. Review results

**Expected Results**:
- "Scanning BLE..." message appears
- Devices discovered and listed
- Device names or addresses shown
- RSSI values displayed

**Pass Criteria**:
- ✅ BLE devices detected
- ✅ Device information displayed correctly
- ✅ Multiple scans work consistently

### Test 6: ESP-NOW Initialization

**Objective**: Verify ESP-NOW setup

**Steps**:
1. Navigate to "ESP-NOW"
2. Press Button B
3. Observe local MAC address display

**Expected Results**:
- ESP-NOW initialized successfully
- Local MAC address displayed in format XX:XX:XX:XX:XX:XX
- No error messages

**Pass Criteria**:
- ✅ ESP-NOW initializes without error
- ✅ Valid MAC address shown

### Test 7: Safety Module Rate Limiting

**Objective**: Verify rate limiting enforcement

**Steps**:
1. Capture multiple signals
2. Attempt to transmit rapidly (>10 times per minute)
3. Observe behavior after limit reached

**Expected Results**:
- First 10 transmissions succeed (within 1 minute)
- 11th transmission is blocked
- Error or denial message shown
- Rate limit resets after 1 minute

**Pass Criteria**:
- ✅ Rate limiting activates after threshold
- ✅ User notified of rate limit
- ✅ Limit resets correctly

### Test 8: Timeout Protection

**Objective**: Verify transmission timeout

**Steps**:
1. Navigate to transmit mode
2. Press Button B to initiate transmission
3. Wait without pressing confirm or cancel
4. Wait for timeout (10 seconds default)

**Expected Results**:
- Warning dialog appears
- After 10 seconds, dialog closes automatically
- No transmission occurs
- Returns to previous screen

**Pass Criteria**:
- ✅ Timeout occurs after configured duration
- ✅ No transmission on timeout
- ✅ System remains stable

### Test 9: Menu Navigation

**Objective**: Verify UI navigation

**Steps**:
1. Use Button C to cycle through main menu items
2. Use Button A to go back from each submenu
3. Test all menu transitions

**Expected Results**:
- Button C moves selection down
- Button A moves selection up (in main menu)
- Button B enters selected item
- Button A exits to main menu from submenus
- Selection highlighting works correctly

**Pass Criteria**:
- ✅ All menu items accessible
- ✅ Navigation is intuitive
- ✅ No crashes or freezes

### Test 10: Statistics and Logging

**Objective**: Verify statistics tracking

**Steps**:
1. Perform various operations (scan, transmit)
2. Navigate to "Settings"
3. Review displayed statistics

**Expected Results**:
- RX Count shows received signals
- TX Count shows transmitted signals
- Safety status displayed
- All values accurate

**Pass Criteria**:
- ✅ Counters increment correctly
- ✅ Statistics persist during session
- ✅ Settings menu displays all info

## Integration Testing

### Test 11: 433 MHz End-to-End

**Objective**: Verify complete receive-transmit cycle

**Requirements**: Two M5Stack devices with RF modules

**Steps**:
1. Device A: Set to scanner mode
2. Device B: Transmit a known signal
3. Device A: Capture the signal
4. Device A: Switch to transmit mode
5. Device B: Switch to scanner mode
6. Device A: Transmit captured signal
7. Device B: Verify received signal matches

**Expected Results**:
- Signal captured on Device A
- Signal replayed successfully
- Device B receives identical signal
- Values match exactly

**Pass Criteria**:
- ✅ Signal integrity maintained
- ✅ Values match between devices
- ✅ No data corruption

### Test 12: ESP-NOW Communication

**Objective**: Test peer-to-peer communication

**Requirements**: Two M5Stack devices

**Steps**:
1. Both devices: Initialize ESP-NOW
2. Device A: Note MAC address
3. Device B: Add Device A as peer (requires code modification)
4. Device B: Send message to Device A
5. Device A: Verify message received

**Expected Results**:
- Peer added successfully
- Message transmitted
- Message received on Device A
- Data integrity maintained

**Pass Criteria**:
- ✅ ESP-NOW communication works
- ✅ Messages delivered reliably
- ✅ No packet loss

## Performance Testing

### Memory Usage

Monitor heap usage:
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

**Expected**: >100KB free heap during operation

### Signal Processing Speed

- 433 MHz signal decode: <10ms
- Wi-Fi scan: 3-5 seconds
- BLE scan: 5 seconds (configurable)
- UI update: <100ms

### Reliability

- Device should run for hours without crash
- Memory leaks: None expected
- Signal buffer: Should handle 50 signals without overflow

## Troubleshooting

### No 433 MHz signals received

- Check receiver connection (GPIO 36)
- Verify power supply to receiver
- Test with known working remote
- Check serial output for errors

### Cannot transmit

- Verify transmitter connection (GPIO 26)
- Check safety module status
- Ensure transmit is enabled in code
- Verify confirmation dialog appears

### Wi-Fi scan fails

- Check ESP32 is not in AP mode
- Ensure Wi-Fi radio not disabled
- Try power cycling device
- Check for nearby networks with phone

### BLE scan issues

- Verify BLE enabled on nearby devices
- Check device supports BLE (all M5Stack do)
- Ensure sufficient memory available
- Try reducing scan duration

### Display issues

- Check LCD initialization in setup()
- Verify voltage settings (M5.Axp.SetLcdVoltage)
- Test with simple display commands
- Check for memory issues affecting graphics

## Security Testing

### Transmission Safety

- ✅ Verify confirmation required
- ✅ Test timeout mechanism
- ✅ Confirm rate limiting works
- ✅ Check audit logging

### Blacklist Enforcement

1. Add frequency to blacklist in config.h
2. Attempt transmission on that frequency
3. Verify transmission blocked

### Unauthorized Access

- Ensure no remote access without authentication
- Verify serial commands are safe
- Check for buffer overflows in input handling

## Regression Testing

After any code changes, run:
1. Basic boot test (Test 1)
2. RF reception test (Test 2)
3. Safety workflow test (Test 3)
4. Menu navigation test (Test 9)

## Test Results Template

```
Test Date: ___________
Firmware Version: ___________
Device: ___________
Tester: ___________

| Test # | Test Name | Result | Notes |
|--------|-----------|--------|-------|
| 1 | System Boot | ☐ Pass ☐ Fail | |
| 2 | RF Reception | ☐ Pass ☐ Fail | |
| 3 | Safe Transmission | ☐ Pass ☐ Fail | |
| 4 | Wi-Fi Scanner | ☐ Pass ☐ Fail | |
| 5 | BLE Scanner | ☐ Pass ☐ Fail | |
| 6 | ESP-NOW Init | ☐ Pass ☐ Fail | |
| 7 | Rate Limiting | ☐ Pass ☐ Fail | |
| 8 | Timeout Protection | ☐ Pass ☐ Fail | |
| 9 | Menu Navigation | ☐ Pass ☐ Fail | |
| 10 | Statistics | ☐ Pass ☐ Fail | |

Overall Result: ☐ Pass ☐ Fail

Additional Notes:
_________________________________
_________________________________
```

## Automated Testing (Future)

Consider implementing:
- Unit tests for signal processing
- Mock RF inputs for CI/CD
- Automated regression suite
- Memory leak detection
- Fuzzing for input validation

## Compliance Testing

Before deployment, verify:
- FCC compliance (if in USA)
- CE marking (if in EU)
- Local RF regulations
- Frequency allocation rules
- Power output limits

## References

- M5Stack documentation: https://docs.m5stack.com/
- ESP32 datasheet: https://www.espressif.com/
- RCSwitch library: https://github.com/sui77/rc-switch
- FCC regulations: https://www.fcc.gov/
