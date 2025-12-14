# m5-rf-suite  
**M5Stack-based RF Tool Suite for 433 MHz and 2.4 GHz**

---

## Overview

`m5-rf-suite` is a modular RF experimentation and analysis platform built on the **M5Stack (ESP32)** ecosystem. It provides a unified, device-local interface for interacting with **433 MHz (sub-GHz)** and **2.4 GHz** radio systems using external RF modules and the ESP32’s native radio capabilities.

This project is designed as an **instrument**, not a gadget: a tight feedback loop where RF behavior can be observed, decoded, modified, and retransmitted with minimal abstraction leakage.

Primary goals:

- Build intuition about real-world RF behavior  
- Maintain clear separation between hardware drivers, RF logic, and UI  
- Support multiple RF front-ends with a shared control surface  
- Enable low-impact, inspectable experimentation  

---

## Supported Frequency Bands

### 433 MHz (Sub-GHz)

Typical targets:
- Remote controls
- Weather sensors
- Smart outlets
- Doorbells
- Simple telemetry devices

Characteristics:
- Narrowband
- Simple modulation schemes
- Often stateless
- Replay-friendly

Common modulations:
- OOK (On-Off Keying)
- ASK (Amplitude Shift Keying)
- FSK (Frequency Shift Keying)

Primary module:
- **CC1101**

---

### 2.4 GHz

Typical targets:
- Proprietary wireless peripherals
- IoT devices
- Short-range control links
- Telemetry systems

Characteristics:
- Packetized communication
- Channelized spectrum
- Strict timing constraints
- Heavy coexistence (Wi-Fi, BLE, proprietary protocols)

Primary modules:
- **nRF24L01+**
- ESP32 internal radio (select modes)

---

## Hardware Requirements

### Core Platform
- M5Stack Core / Core2 / CoreS3 (ESP32-based)
- USB-C or Micro-USB cable
- MicroSD card (recommended for logging)

### RF Modules
- CC1101 (433 MHz)
- nRF24L01+ (2.4 GHz)

### Optional
- External antennas (433 MHz / 2.4 GHz)
- Logic analyzer
- RF shielding enclosure (lab use)

---

## Software Architecture

The system is structured in layers to prevent RF logic, hardware access, and UI from collapsing into a single monolith.

### High-Level Layers

UI Layer
└── RF Control Layer
├── Protocol Logic
└── Radio Abstraction
├── CC1101 Driver
└── nRF24 / ESP32 Radio Driver

### Key Principles

- **Radio-agnostic core**: Shared logic where possible  
- **Module isolation**: Each RF front-end owns its timing and constraints  
- **State visibility**: No “magic” background behavior  
- **Fail-loud defaults**: Invalid states surface immediately  

---

## Core Features

### RF Scanning
- Frequency or channel scanning
- RSSI observation
- Activity visualization

### Signal Capture
- Raw pulse timing (433 MHz)
- Packet capture (2.4 GHz)
- Timestamped buffers

### Protocol Analysis
- Pulse width statistics
- Symbol classification
- Basic protocol fingerprinting

### Transmission
- Controlled replay
- Parameterized payload injection
- Power and timing adjustment

### UI & Control
- On-device display
- Physical button input
- Real-time parameter tuning

---

## Safety & Ethics

This tool is intended for **authorized experimentation only**.

- Operate only on devices you own or have explicit permission to test  
- Respect local RF regulations and power limits  
- Avoid interference with safety-critical systems  
- Use shielding and low-power modes during development  

RF is shared infrastructure. Treat it accordingly.

---

## Build & Flash

### Toolchain
- Arduino IDE or PlatformIO
- ESP32 board support package
- Required libraries listed in `platformio.ini` or `lib/`

### General Steps
1. Clone repository  
2. Configure target M5Stack variant  
3. Select enabled RF modules  
4. Build and flash via USB  

---

## Extensibility

`m5-rf-suite` is intentionally modular.

You can extend it by:
- Adding new RF front-ends
- Implementing additional protocol decoders
- Enhancing visualization modes
- Exporting captured data for offline analysis

New modules should conform to the radio abstraction interface to ensure compatibility with the UI and logging subsystems.

---

## Project Philosophy

RF is not clean.  
It is noisy, lossy, probabilistic, and shaped by physics as much as code.

This project treats RF systems the way debuggers treat software:  
observe first, hypothesize second, act deliberately.

---

## License

Specify license here (MIT, GPL-v3, BSD, etc.)

---

## Disclaimer

This project is provided **as-is**, without warranty.  
The authors are not responsible for misuse, regulatory violations, or unintended interference.

Experiment carefully.
