# Hardware Setup Guide

This guide covers the hardware requirements and setup for the M5Stack RF Suite.

## Supported Devices

### Primary Devices

#### M5Stack Core2
- **Processor**: ESP32-D0WDQ6-V3
- **Display**: 2.0" IPS LCD (320x240)
- **Touch**: FT6336U capacitive touch
- **Built-in**: Wi-Fi, Bluetooth, BLE
- **Recommended for**: Full-featured deployment

#### M5Stack Fire
- **Processor**: ESP32
- **Display**: 2.0" IPS LCD (320x240)
- **Built-in**: Wi-Fi, Bluetooth, BLE, IMU, Microphone
- **Recommended for**: General purpose use

#### M5Stack Basic/Gray
- **Processor**: ESP32
- **Display**: 2.0" IPS LCD (320x240)
- **Built-in**: Wi-Fi, Bluetooth, BLE
- **Recommended for**: Budget builds

#### M5Stick-C / M5Stick-C Plus
- **Processor**: ESP32-PICO-D4
- **Display**: 0.96" / 1.14" LCD
- **Built-in**: Wi-Fi, Bluetooth, BLE, IMU
- **Recommended for**: Portable/compact deployment
- **Note**: Smaller screen limits UI complexity

## Required External Components

### 433 MHz Receiver Module

**Recommended Models**:
- RXB6 (superheterodyne, best sensitivity)
- SYN470R (low cost alternative)
- RX470-4 (good performance)

**Specifications**:
- Frequency: 433.92 MHz (adjustable: 433-434 MHz)
- Operating Voltage: 5V (some models: 3.3-5V)
- Sensitivity: -105 to -110 dBm
- Operating Current: 2.5-5 mA

**Connections**:
```
Receiver    ->  M5Stack
VCC         ->  5V
GND         ->  GND
DATA        ->  GPIO 36 (default, configurable)
```

**Notes**:
- Use a module with 3.3V logic output, or use level shifter
- Add a 17.3cm wire antenna for better reception
- Keep away from power lines and motors

### 433 MHz Transmitter Module

**Recommended Models**:
- FS1000A / XY-FST (common, cheap)
- SYN115 (better range)
- HC-12 (long range, up to 1km)

**Specifications**:
- Frequency: 433.92 MHz
- Operating Voltage: 3.3-12V (higher voltage = more range)
- Transmission Power: 10-40 mW
- Operating Current: 20-40 mA
- Range: 20-100 meters (line of sight)

**Connections**:
```
Transmitter ->  M5Stack
VCC         ->  5V (for maximum power)
GND         ->  GND
DATA        ->  GPIO 26 (default, configurable)
```

**Notes**:
- Antenna length affects range (17.3cm optimal)
- Higher voltage increases transmission power and range
- Ensure adequate power supply (avoid USB power for transmission)
- Consider using external power source for transmitter

## Wiring Diagrams

### M5Stack Core2 - Complete Setup

```
┌─────────────────────────────────┐
│      M5Stack Core2              │
│                                 │
│  ┌──┐ ┌──┐ ┌──┐                │
│  │A │ │B │ │C │  (Buttons)     │
│  └──┘ └──┘ └──┘                │
│                                 │
│   [Display Area]                │
│                                 │
└─────────────────────────────────┘
         │ │ │ │ │
         │ │ │ │ └─ 5V ──────┐
         │ │ │ └─── GND ──┐   │
         │ │ └───── GPIO26 │  │
         │ └─────── GPIO36 │  │
         └───────── GND ────┼──┼──┐
                            │  │  │
        ┌───────────────────┘  │  │
        │  433 MHz Receiver    │  │
        │  ┌────┐              │  │
        │  │ RX │              │  │
        │  └────┘              │  │
        │   │ │ │              │  │
        └───┤ │ └──────────────┼──┘
            │ └────────────────┘
            └── DATA out
            
            ┌──────────────────┐
            │  433 MHz Transmit│
            │  ┌────┐          │
            │  │ TX │          │
            │  └────┘          │
            │   │ │ │          │
            └───┤ │ └──────────┘
                │ └── DATA in
                └────── to GPIO26
```

### Port Connector Pinout (M5Stack Bottom)

```
Port A (I2C):
Pin 1: GND
Pin 2: GPIO 22 (SCL)
Pin 3: GPIO 21 (SDA)
Pin 4: 5V

Port B (I/O):
Pin 1: GND
Pin 2: GPIO 26 (TX433 default)
Pin 3: GPIO 36 (RX433 default)
Pin 4: 5V

Port C (UART):
Pin 1: GND
Pin 2: GPIO 16 (RX)
Pin 3: GPIO 17 (TX)
Pin 4: 5V
```

## Assembly Instructions

### Option 1: Using Proto Module

1. **Acquire M5Stack Proto Module**
   - Provides easy access to GPIO pins
   - Stackable design

2. **Solder Receiver and Transmitter**
   ```
   Step 1: Solder receiver VCC to 5V rail
   Step 2: Solder receiver GND to GND rail
   Step 3: Solder receiver DATA to GPIO 36 via proto pad
   Step 4: Repeat for transmitter on GPIO 26
   ```

3. **Add Antennas**
   - Cut 17.3cm wire for each module
   - Solder to ANT pad on modules
   - Keep antennas perpendicular to each other

4. **Stack Modules**
   - M5Stack Core2 (bottom)
   - Proto module with RF circuits (middle)
   - Optional battery module (top)

### Option 2: Using Grove Connectors

1. **Use Grove Cables**
   - Connect receiver to Port B
   - May need custom cable for pinout

2. **External Mounting**
   - Keep RF modules external for better signal
   - Use appropriate cable length

### Option 3: Custom PCB (Advanced)

Design considerations:
- Separate RF grounds from digital grounds
- Use RF-rated PCB material
- Add proper impedance matching
- Include filtering capacitors
- ESD protection on antenna lines

## Power Considerations

### Current Requirements

**Idle Operation**:
- M5Stack Core2: ~100 mA
- 433 MHz Receiver: ~3 mA
- Total: ~103 mA

**During Reception**:
- M5Stack Core2: ~120 mA
- 433 MHz Receiver: ~5 mA
- Total: ~125 mA

**During Transmission**:
- M5Stack Core2: ~120 mA
- 433 MHz Receiver: ~3 mA
- 433 MHz Transmitter: ~40 mA
- Total: ~163 mA

**During Wi-Fi/BLE Operation**:
- M5Stack Core2: ~200-400 mA
- Total: ~200-400 mA (plus RF modules)

### Power Supply Options

#### USB Power
- **Sufficient for**: Reception, Wi-Fi, BLE
- **May be insufficient for**: Continuous transmission at high power
- **Recommendation**: Use for development and testing

#### Battery Power
- **M5Stack Battery Module**: 150-700 mAh
- **Runtime**: 2-6 hours depending on usage
- **Best for**: Portable operation, low-power modes

#### External Power Supply
- **Recommended**: 5V 2A USB power adapter
- **Best for**: Stationary use, continuous operation
- **Allows**: Maximum transmission power

### Power Optimization Tips

1. **Reduce Display Brightness**
   ```cpp
   M5.Axp.SetLcdVoltage(2500); // Lower voltage = less brightness
   ```

2. **Sleep When Idle**
   ```cpp
   esp_wifi_set_ps(WIFI_PS_MAX_MODEM); // Wi-Fi power save
   ```

3. **Disable Unused Modules**
   - Turn off Wi-Fi when using only 433 MHz
   - Disable BLE when not scanning

## Antenna Design

### 433 MHz Antenna

**Quarter Wave Antenna** (simplest):
- Length: 17.3 cm (¼ wavelength)
- Wire gauge: 18-22 AWG
- Position: Vertical for best omnidirectional coverage

**Calculations**:
```
Wavelength (λ) = c / f
λ = 299,792,458 m/s / 433,920,000 Hz
λ = 0.691 m = 69.1 cm
Quarter wave = 69.1 / 4 = 17.3 cm
```

**Coil Antenna** (compact):
- More compact than wire
- Slightly less efficient
- Good for portable applications

### Antenna Placement

**DO**:
- ✅ Mount antenna vertically
- ✅ Keep antenna clear of metal
- ✅ Use proper antenna length
- ✅ Separate TX and RX antennas

**DON'T**:
- ❌ Coil or fold antenna
- ❌ Place near metal surfaces
- ❌ Use random wire lengths
- ❌ Point antenna at ground

## Enclosure Recommendations

### RF Considerations
- Use plastic enclosure (not metal)
- Provide opening for antennas
- Keep antennas outside metal parts
- Consider IP rating for outdoor use

### Mounting Options
1. **Desktop Stand**: Use M5Stack base
2. **Wall Mount**: 3D printed bracket
3. **Portable Case**: With battery module
4. **Vehicle Mount**: With external power

## Testing and Calibration

### Signal Quality Test

1. **Receiver Test**:
   ```
   - Place known 433 MHz transmitter at 1 meter
   - Verify signal received
   - Check RSSI value
   - Gradually increase distance
   - Note maximum range
   ```

2. **Transmitter Test**:
   ```
   - Use another receiver at 1 meter
   - Transmit test signal
   - Verify reception
   - Check signal strength
   - Test at various distances
   ```

### Expected Performance

**433 MHz Range**:
- Indoor: 10-30 meters
- Outdoor: 50-100 meters
- With better antenna: 100-300 meters

**Wi-Fi Range**:
- Indoor: 20-50 meters
- Outdoor: 100-200 meters

**BLE Range**:
- Indoor: 10-30 meters
- Outdoor: 50-100 meters

## Troubleshooting Hardware Issues

### No Signal Reception

1. Check connections:
   - Verify wiring to GPIO 36
   - Ensure proper power supply
   - Check GND connection

2. Test receiver module:
   - Connect LED to DATA pin
   - Should blink when receiving
   - If no blink, module may be faulty

3. Verify antenna:
   - Should be 17.3 cm
   - Should be straight, not coiled
   - Check for broken connections

### Poor Reception Range

1. **Check antenna**:
   - Verify length (17.3 cm)
   - Ensure good solder connection
   - Try different antenna types

2. **Check power**:
   - Measure voltage at receiver VCC
   - Should be stable 5V
   - Check for voltage drops

3. **Reduce interference**:
   - Move away from computers
   - Avoid power supplies
   - Keep away from metal

### Transmission Issues

1. **No transmission**:
   - Verify GPIO 26 connection
   - Check power supply current capacity
   - Measure voltage during transmission
   - Ensure software transmit enabled

2. **Weak transmission**:
   - Increase transmitter VCC to 5V
   - Check antenna connection
   - Verify antenna length
   - Use external power supply

3. **Interference**:
   - Other devices on 433 MHz
   - Harmonics from switching supplies
   - Try different location

## Safety Warnings

⚠️ **RF Exposure**:
- Keep transmitter antenna away from body
- Do not operate transmitter continuously at high power
- Follow local RF safety guidelines

⚠️ **Electrical**:
- Disconnect power before modifying connections
- Use proper ESD precautions
- Verify polarity before connecting power

⚠️ **Legal**:
- Check local regulations before transmitting
- 433 MHz ISM band rules vary by country
- Some frequencies may require license

## Accessories and Optional Components

### Recommended Accessories

1. **External Antenna** (SMA connector)
   - Better range than wire antenna
   - Various types available (whip, directional)

2. **Antenna Extension Cable**
   - Allows antenna placement away from device
   - Use low-loss RF cable (RG316, RG174)

3. **RF Bandpass Filter**
   - Improves selectivity
   - Reduces interference
   - 433 MHz centered filter

4. **RF Amplifier**
   - Increases transmission power
   - Requires FCC/CE compliance
   - Use with caution

5. **Spectrum Analyzer Module**
   - Visualize RF spectrum
   - Helps with debugging
   - Educational tool

## Advanced Modifications

### Adding External LNA (Low Noise Amplifier)

Benefits:
- Improves receiver sensitivity
- Increases reception range
- Better weak signal performance

Considerations:
- May increase noise
- Adds complexity
- Power consumption increase

### Directional Antenna

Types:
- Yagi antenna: High gain, directional
- Patch antenna: Compact, directional
- Helical antenna: Circular polarization

Benefits:
- Increased range in one direction
- Better signal quality
- Reduced interference from sides

## Component Sources

### Reputable Suppliers

**433 MHz Modules**:
- SparkFun
- Adafruit
- Amazon (various sellers)
- AliExpress (low cost)

**M5Stack Products**:
- Official M5Stack store
- Authorized distributors
- Amazon

**Antennas**:
- Mouser Electronics
- Digi-Key
- RF solutions specialists

## Bill of Materials (BOM)

### Basic Setup

| Item | Quantity | Est. Cost |
|------|----------|-----------|
| M5Stack Core2 | 1 | $50 |
| 433 MHz Receiver (RXB6) | 1 | $3 |
| 433 MHz Transmitter (FS1000A) | 1 | $2 |
| Wire for antennas (22 AWG) | 50 cm | $1 |
| Proto module (optional) | 1 | $15 |
| USB-C cable | 1 | $5 |
| **Total** | | **~$76** |

### Advanced Setup

Add to basic setup:
| Item | Quantity | Est. Cost |
|------|----------|-----------|
| External antenna with SMA | 2 | $20 |
| RF bandpass filter | 1 | $10 |
| Enclosure | 1 | $20 |
| Battery module | 1 | $25 |
| **Additional** | | **~$75** |

## Conclusion

With proper hardware setup, the M5Stack RF Suite provides a powerful and flexible platform for RF experimentation and security testing. Follow this guide carefully for best results.
