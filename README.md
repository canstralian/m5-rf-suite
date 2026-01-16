# M5Stack RF Suite

A comprehensive, modular RF tool suite running on M5Stack/ESP32-class devices, supporting both 433 MHz and 2.4 GHz operations with safe-by-default workflows.

## Overview

M5Stack RF Suite is a security-focused toolkit for working with RF signals across multiple frequency bands. It provides:

- **433 MHz Operations**: Receive, classify, and optionally transmit signals (OOK/ASK modulation for consumer devices like remotes, doorbells, garage doors)
- **2.4 GHz Operations**: ESP-NOW peer-to-peer communication, Wi-Fi scanning, and BLE device scanning
- **Safe-by-Default Design**: All transmissions require explicit user confirmation with policy enforcement

## Features

### 433 MHz Module
- **Signal Reception**: Decode OOK/ASK signals from common consumer devices
- **Signal Classification**: Automatic classification of signal types (doorbells, garage doors, remotes, etc.)
- **Signal Storage**: Save and replay captured signals
- **Safe Transmission**: Transmit signals only with explicit user confirmation
- **Protocol Support**: Multiple protocols via RCSwitch library

### 2.4 GHz Module
- **ESP-NOW**: Peer-to-peer communication between ESP32 devices
- **Wi-Fi Scanner**: Scan and analyze nearby Wi-Fi networks
- **BLE Scanner**: Discover and list Bluetooth Low Energy devices
- **Multi-mode Support**: Switch between different 2.4 GHz modes

### Safety Features
- **Mandatory Confirmation**: All transmissions require button press confirmation
- **Frequency Blacklist**: Block transmission on prohibited frequencies
- **Rate Limiting**: Prevent excessive transmissions
- **Audit Logging**: Track all transmission attempts
- **Timeout Protection**: Auto-cancel pending transmissions
- **Policy Enforcement**: Configurable transmission policies

## Hardware Requirements

### Supported Devices
- M5Stack Core2
- M5Stack Fire
- M5Stick-C
- Other ESP32-based M5Stack devices

### Required Components
- **433 MHz Receiver**: Connect to GPIO 36 (configurable in `config.h`)
- **433 MHz Transmitter**: Connect to GPIO 26 (configurable in `config.h`)
- Built-in 2.4 GHz radio (ESP32 Wi-Fi/BLE)

### Pin Configuration
Default pins (adjust in `include/config.h`):
```cpp
#define RF_433_RX_PIN 36  // 433 MHz receiver
#define RF_433_TX_PIN 26  // 433 MHz transmitter
```

## Installation

### Using PlatformIO (Recommended)

1. Clone this repository:
```bash
git clone https://github.com/canstralian/m5-rf-suite.git
cd m5-rf-suite
```

2. Build and upload:
```bash
pio run --target upload
```

3. Monitor serial output:
```bash
pio device monitor
```

### Using Arduino IDE

1. Install required libraries:
   - M5Core2 (or M5Stack/M5StickC depending on your device)
   - RCSwitch
   - ArduinoJson
   - ESP32Servo

2. Copy the `src/` and `include/` files to your Arduino project

3. Select your M5Stack board and upload

## Usage

### Main Application

The main application provides a menu-driven interface with the following options:

1. **433MHz Scanner**: Passively receive and decode 433 MHz signals
2. **433MHz Transmit**: Replay captured signals (requires confirmation)
3. **WiFi Scanner**: Scan for nearby Wi-Fi networks
4. **BLE Scanner**: Discover Bluetooth devices
5. **ESP-NOW**: Peer-to-peer communication
6. **Settings**: View statistics and configuration
7. **About**: Version and system information

### Button Controls

- **Button A (Left)**: Previous / Back
- **Button B (Center)**: Select / Action
- **Button C (Right)**: Next / Forward

### Safety Workflow

When transmitting signals:

1. Select a captured signal
2. Press transmit button
3. **WARNING** dialog appears
4. Press B to confirm (or A to cancel within timeout)
5. Transmission occurs only after confirmation
6. Result displayed on screen

## Examples

### 433 MHz Scanner
```cpp
#include "rf433_module.h"

RF433Module rf433;

void setup() {
    rf433.begin();
}

void loop() {
    if (rf433.isSignalAvailable()) {
        RF433Signal signal = rf433.receiveSignal();
        // Process signal...
    }
}
```

### Wi-Fi Scanner
```cpp
#include "rf24_module.h"

RF24Module rf24;

void setup() {
    rf24.begin();
    rf24.startWiFiScan(true);
}

void loop() {
    if (rf24.isWiFiScanComplete()) {
        int count = rf24.getWiFiNetworkCount();
        for (int i = 0; i < count; i++) {
            WiFiNetworkInfo network = rf24.getWiFiNetwork(i);
            // Process network...
        }
    }
}
```

### ESP-NOW Communication
```cpp
#include "rf24_module.h"

RF24Module rf24;

void setup() {
    rf24.begin();
    rf24.initESPNow();
}

void loop() {
    if (rf24.hasReceivedMessage()) {
        ESPNowMessage msg = rf24.getReceivedMessage();
        // Process message...
    }
}
```

See the `examples/` directory for complete working examples.

## Configuration

Edit `include/config.h` to customize:

- Pin assignments
- Protocol parameters
- Safety settings
- UI colors and layout
- Logging options
- Storage settings

### Key Configuration Options

```cpp
// Safety settings
#define REQUIRE_USER_CONFIRMATION true
#define TRANSMIT_TIMEOUT 10000  // ms
#define MAX_TRANSMIT_DURATION 5000  // ms

// 433 MHz settings
#define RF_433_RX_PIN 36
#define RF_433_TX_PIN 26
#define RF_433_PROTOCOL_DEFAULT 1
#define RF_433_PULSE_LENGTH 350

// Rate limiting
maxTransmitsPerMinute = 10;  // In safety_module.cpp
```

## RF Test Workflows

The suite includes a comprehensive workflow system for structured RF testing:

### Workflow States

1. **IDLE**: Initial resting state
2. **INIT**: Hardware initialization
3. **LISTENING**: Passive observation (no transmission)
4. **ANALYZING**: Signal processing and classification
5. **READY**: Awaiting user decision
6. **TX_GATED**: Multi-stage transmission approval
7. **TRANSMIT**: Controlled RF transmission
8. **CLEANUP**: Resource deallocation

### Key Features

- **Passive-Listening First**: Always observe before transmitting
- **Fail-Safe Modes**: Timeout protection at every state
- **Gated Transmission**: Multi-stage approval (Policy → Confirmation → Rate Limit → Band-Specific)
- **Complete Traceability**: Microsecond-precision timing logs

See `WORKFLOWS.md` for detailed state diagrams and `PSEUDOCODE.md` for implementation details.

### Example Usage

```cpp
#include "rf_test_workflow.h"

RFTestWorkflow workflow;

void setup() {
    // Configure for 433 MHz testing
    WorkflowConfig config;
    config.band = BAND_433MHZ;
    config.listenMinTime = 5000;   // 5 seconds
    config.listenMaxTime = 60000;  // 60 seconds
    
    workflow.initialize(config, &rf433, &rf24);
    workflow.start();  // Runs complete workflow
}
```

See `examples/rf_workflow_test/` for a complete working example.

## Architecture

### Module Structure

```
m5-rf-suite/
├── include/                    # Header files
│   ├── config.h               # Configuration constants
│   ├── rf433_module.h         # 433 MHz operations
│   ├── rf24_module.h          # 2.4 GHz operations
│   ├── safety_module.h        # Safety and policy enforcement
│   ├── rf_test_workflow.h     # RF test workflow state machine
│   └── ui_module.h            # User interface (header only)
├── src/                       # Implementation files
│   ├── main.cpp               # Main application
│   ├── rf433_module.cpp       # 433 MHz implementation
│   ├── rf24_module.cpp        # 2.4 GHz implementation
│   ├── safety_module.cpp      # Safety implementation
│   └── rf_test_workflow.cpp   # Workflow implementation
├── examples/                  # Example applications
│   ├── 433mhz_scanner/
│   ├── wifi_scanner/
│   ├── espnow_test/
│   └── rf_workflow_test/      # Workflow demonstration
├── WORKFLOWS.md               # State diagrams and flows
├── PSEUDOCODE.md              # Structured pseudocode
└── platformio.ini             # Build configuration
```

### Safety Architecture

The safety module enforces a multi-layered approach:

1. **User Confirmation Layer**: Requires explicit button press
2. **Policy Layer**: Checks frequency blacklists and duration limits
3. **Rate Limiting Layer**: Prevents excessive transmissions
4. **Audit Layer**: Logs all transmission attempts
5. **Timeout Layer**: Auto-cancels stale requests

## Legal and Safety Notice

⚠️ **IMPORTANT**: This tool is for educational and authorized testing purposes only.

- Always comply with local RF regulations (FCC, CE, etc.)
- Obtain proper authorization before testing in production environments
- Be aware of frequency allocations and restrictions in your country
- Transmitting on certain frequencies may be illegal without a license
- The developers assume no liability for misuse of this software

### Responsible Use Guidelines

- **DO**: Use for learning, authorized security testing, and research
- **DO**: Respect privacy and property rights
- **DON'T**: Jam, disrupt, or interfere with critical services
- **DON'T**: Transmit on emergency or aviation frequencies
- **DON'T**: Use for unauthorized access or malicious purposes

## Security Considerations

The M5Stack RF Suite implements comprehensive security measures to prevent unauthorized and accidental transmissions. For detailed information about how the system defends against specific threats, see the **[Threat Mapping Documentation](docs/threat-mapping.md)**.

### Key Security Features

- **Multi-layered Defense**: 4 independent transmission gates (Policy, Confirmation, Rate Limit, Band-Specific)
- **Passive-Listening-First**: Mandatory observation phase before any transmission
- **Audit Logging**: Complete transmission history for accountability
- **Timeout Protection**: Auto-expiring confirmations prevent stale transmissions
- **Frequency Blacklist**: Prevents transmission on prohibited bands

### Threat Classes Addressed

1. **Accidental Replay Attacks**: Multi-gate approval, user confirmation, rate limiting
2. **Blind Broadcast**: Enforced observation, spectrum analysis, address binding
3. **User Errors**: Input validation, clear confirmations, timeout protection
4. **Firmware Faults**: RAII memory management, mandatory cleanup, state timeouts

For complete details on each threat class and its mitigations, see [docs/threat-mapping.md](docs/threat-mapping.md).

### General Security Notes

- The 433 MHz module can capture and replay signals from nearby devices
- Ensure physical security of the device when containing sensitive signals
- Be aware that many consumer RF devices use insecure protocols
- This tool demonstrates security weaknesses in RF communication
- Use responsibly and ethically

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

See LICENSE file for details.

## Acknowledgments

- M5Stack for hardware platform
- RCSwitch library for 433 MHz protocol support
- ESP32 community for ESP-NOW and BLE examples
- Security researchers who inspire responsible disclosure

## Support

For issues, questions, or contributions:
- GitHub Issues: [Report bugs or request features](https://github.com/canstralian/m5-rf-suite/issues)
- Documentation: See comments in header files
- Examples: Check the `examples/` directory

## Changelog

### Version 1.0.0 (2025-12-14)
- Initial release
- 433 MHz receive/transmit with classification
- 2.4 GHz Wi-Fi, BLE, and ESP-NOW support
- Safe-by-default transmission workflow
- Multi-layered safety enforcement
- Menu-driven UI for M5Stack
- Example applications included
