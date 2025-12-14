# Quick Start Guide

Get up and running with M5Stack RF Suite in 10 minutes!

## Prerequisites

- M5Stack Core2, Fire, or similar ESP32-based M5Stack device
- USB-C cable
- 433 MHz receiver module (e.g., RXB6)
- 433 MHz transmitter module (e.g., FS1000A)
- PlatformIO installed (or Arduino IDE with libraries)

## Hardware Setup (5 minutes)

### 1. Connect RF Modules

Connect your RF modules to the M5Stack:

**433 MHz Receiver:**
```
RXB6 VCC  → M5Stack 5V
RXB6 GND  → M5Stack GND
RXB6 DATA → M5Stack GPIO 36
```

**433 MHz Transmitter:**
```
FS1000A VCC  → M5Stack 5V
FS1000A GND  → M5Stack GND
FS1000A DATA → M5Stack GPIO 26
```

### 2. Add Antennas

Cut two pieces of wire, each **17.3 cm** long, and solder one to each module's antenna pad. Keep them straight (don't coil them).

## Software Setup (5 minutes)

### Option A: PlatformIO (Recommended)

```bash
# Clone the repository
git clone https://github.com/canstralian/m5-rf-suite.git
cd m5-rf-suite

# Build and upload
pio run -e m5stack-core2 --target upload

# Monitor serial output
pio device monitor
```

### Option B: Arduino IDE

1. Install libraries from Library Manager:
   - M5Core2
   - RCSwitch
   - ArduinoJson
   - ESP32Servo

2. Open `src/main.cpp` in Arduino IDE

3. Select **Tools > Board > M5Stack-Core2**

4. Select **Tools > Port > [your device]**

5. Click **Upload**

## First Use

### 1. Power On

When you first power on the device, you'll see:
```
M5Stack RF Suite v1.0.0
Initializing...
- 433 MHz module
- 2.4 GHz module
Ready!
```

### 2. Scan for 433 MHz Signals

1. From main menu, press **Button C** to navigate to "433MHz Scanner"
2. Press **Button B** to select
3. Press a 433 MHz remote control near your device
4. You'll see signal details on the screen
5. Press **Button A** to return to main menu

### 3. Scan for Wi-Fi Networks

1. Navigate to "WiFi Scanner" (use Button C)
2. Press **Button B** to scan
3. Wait 3-5 seconds
4. Networks will be listed by signal strength
5. Press **Button B** to rescan, or **Button A** to exit

### 4. Try Safe Transmission (Optional)

⚠️ **WARNING**: Only transmit if you own the device you're controlling!

1. First capture a signal using the scanner
2. Navigate to "433MHz Transmit"
3. Press **Button B** to select
4. You'll see a warning dialog
5. Press **Button B** again to confirm transmission
6. The signal will be transmitted

## Button Reference

```
┌──────────┬──────────┬──────────┐
│ Button A │ Button B │ Button C │
├──────────┼──────────┼──────────┤
│   Back   │  Select  │   Next   │
│ Previous │  Action  │ Forward  │
└──────────┴──────────┴──────────┘
```

## Common Issues

### No signals received
- Check receiver connection to GPIO 36
- Verify 5V power to receiver
- Ensure antenna is 17.3 cm and straight
- Try with a known working remote

### Cannot see Wi-Fi networks
- Make sure device has booted completely
- Check that you're in an area with Wi-Fi
- Try rescanning (press Button B in Wi-Fi scanner)

### Display not working
- Check USB-C connection
- Try unplugging and replugging
- Verify M5Stack is powered on (LED should be lit)

### Compilation errors
- Ensure all libraries are installed
- Check that you selected the correct board
- Verify PlatformIO/Arduino IDE is up to date

## What's Next?

### Learn More
- Read [README.md](README.md) for full documentation
- Check [HARDWARE.md](HARDWARE.md) for advanced setup
- Review [SECURITY.md](SECURITY.md) for responsible use
- Try examples in `examples/` directory

### Customize
- Edit `include/config.h` to change pin assignments
- Adjust safety settings (confirmation timeout, rate limits)
- Add frequencies to blacklist
- Modify UI colors and layout

### Experiment Safely
- ✅ Scan and analyze RF signals
- ✅ Test with your own devices
- ✅ Learn about RF protocols
- ❌ Don't transmit without permission
- ❌ Don't interfere with others' devices
- ❌ Don't ignore local regulations

## Example Workflows

### Workflow 1: Learn About Your Remote
```
1. Start 433MHz Scanner
2. Press each button on your remote
3. Observe the decoded values
4. Note the protocol and pulse length
5. Compare different remotes
```

### Workflow 2: Check Wi-Fi Security
```
1. Start WiFi Scanner
2. Look at encryption types
3. Note signal strengths
4. Identify your network
5. Verify proper security (WPA2/WPA3)
```

### Workflow 3: Discover BLE Devices
```
1. Start BLE Scanner
2. Press Button B to scan
3. See nearby Bluetooth devices
4. Check signal strengths
5. Learn what devices are around
```

## Help and Support

- **Issues**: https://github.com/canstralian/m5-rf-suite/issues
- **Documentation**: Check the `*.md` files in repository
- **Examples**: See `examples/` directory

## Safety Reminder

🔒 **This tool is designed with safety as the top priority:**

- All transmissions require explicit confirmation
- Rate limiting prevents abuse
- Frequency blacklist blocks dangerous frequencies
- Complete audit trail of all actions
- Timeout protection on pending transmissions

**Use responsibly and ethically. Know your local laws.**

## Next Steps

Now that you're up and running:

1. ✅ Scan for signals and learn about RF protocols
2. ✅ Explore the different menu options
3. ✅ Read the full documentation
4. ✅ Experiment with the examples
5. ✅ Understand the safety features

Happy exploring! 🎉
