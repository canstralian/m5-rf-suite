# Security Policy and Guidelines

## Overview

The M5Stack RF Suite is designed with security as a primary concern. This document outlines the security features, responsible use guidelines, and vulnerability reporting procedures.

## Security Philosophy

### Safe-by-Default Design

The RF Suite implements multiple layers of security controls:

1. **No Transmission Without Confirmation**: All RF transmissions require explicit user button press
2. **Policy Enforcement**: Configurable policies prevent unauthorized or dangerous transmissions
3. **Audit Logging**: All transmission attempts are logged for review
4. **Rate Limiting**: Prevents abuse through rapid-fire transmissions
5. **Timeout Protection**: Pending transmissions auto-cancel after timeout period

### Defense in Depth

Multiple independent safety mechanisms ensure that a single failure doesn't compromise security:

```
User Intent → Confirmation Dialog → Policy Check → Rate Limit → Frequency Blacklist → Transmission
     ↓              ↓                    ↓              ↓               ↓
  Timeout      Button Press         Duration        Time-based      Frequency
  Protection   Required             Limit           Tracking        Validation
```

## Security Features

### 1. Mandatory User Confirmation

**Implementation**:
```cpp
// In safety_module.cpp
#define REQUIRE_USER_CONFIRMATION true
```

**Behavior**:
- Orange warning screen displayed
- "Press B to confirm" message
- "Press A to cancel" option
- 10-second timeout (configurable)
- No transmission without explicit confirmation

**Bypass Prevention**:
- Cannot be disabled via UI
- Requires code modification to bypass
- Logged in audit trail

### 2. Frequency Blacklist

**Purpose**: Prevent transmission on prohibited or dangerous frequencies

**Configuration** (config.h):
```cpp
#define FREQ_BLACKLIST_ENABLED true
const float FREQ_BLACKLIST[] = {
    121.5,  // Aviation emergency
    243.0,  // Military emergency
    156.8,  // Marine distress
    // Add more as needed
};
```

**Enforcement**:
- Checked before every transmission
- 100 kHz tolerance window
- Blocked transmissions logged
- No override mechanism in UI

### 3. Rate Limiting

**Purpose**: Prevent excessive transmissions

**Default Settings**:
```cpp
maxTransmitsPerMinute = 10;  // Configurable
```

**Implementation**:
- Rolling 60-second window
- Tracks all transmission attempts
- Automatic cleanup of old entries
- Denial logged with reason

**Use Cases**:
- Prevent DoS attacks
- Limit spectrum pollution
- Enforce reasonable use

### 4. Transmission Duration Limits

**Purpose**: Prevent prolonged transmissions

**Configuration**:
```cpp
#define MAX_TRANSMIT_DURATION 5000  // milliseconds
```

**Enforcement**:
- Checked in policy evaluation
- Applies to single transmission
- Prevents jamming scenarios

### 5. Audit Logging

**Purpose**: Track all transmission activity

**Logged Information**:
- Timestamp
- Frequency
- Duration
- Allowed/Denied status
- Reason for decision
- Signal details

**Storage**:
- In-memory circular buffer (100 entries)
- Can be extended to SPIFFS for persistence
- Accessible via settings menu

**Example Log Entry**:
```
[Safety] Log: 433.92 MHz for 350 ms - ALLOWED (PERMIT_ALLOWED)
```

### 6. Timeout Protection

**Purpose**: Prevent accidental transmissions from stale requests

**Implementation**:
```cpp
#define TRANSMIT_TIMEOUT 10000  // 10 seconds
```

**Behavior**:
- Confirmation dialog auto-closes
- Request automatically denied
- User must re-initiate

## Threat Model

### Threats Addressed

#### 1. Accidental Transmission
**Risk**: User accidentally transmits signal
**Mitigation**: Mandatory confirmation dialog with timeout

#### 2. Malicious Actor
**Risk**: Attacker gains physical access to device
**Mitigation**: 
- No remote access
- Physical confirmation required
- Audit trail of actions

#### 3. Software Bug
**Risk**: Bug causes unintended transmission
**Mitigation**:
- Multiple validation layers
- Safe-by-default code structure
- Transmitter disabled by default

#### 4. Replay Attack
**Risk**: Captured signals replayed maliciously
**Mitigation**:
- User awareness through UI warnings
- Rate limiting prevents rapid replay
- Audit logging tracks activity

#### 5. Jamming/Interference
**Risk**: Device used to jam communications
**Mitigation**:
- Duration limits
- Rate limiting
- Frequency blacklist
- User responsibility education

### Threats Not Addressed

#### 1. Physical Device Modification
**Risk**: Attacker modifies hardware to bypass safety
**Mitigation**: None - physical security is user's responsibility

#### 2. Custom Firmware
**Risk**: Attacker replaces firmware
**Mitigation**: None - assumed user has legitimate reason for custom firmware

#### 3. Reverse Engineering
**Risk**: Attacker analyzes code to find vulnerabilities
**Mitigation**: Open source transparency (feature, not bug)

## Responsible Use Guidelines

### DO ✅

1. **Education and Research**
   - Learn about RF communication
   - Understand protocol vulnerabilities
   - Study security implications

2. **Authorized Testing**
   - Test your own devices
   - Work with permission
   - Document findings properly

3. **Security Research**
   - Responsible disclosure
   - Help improve security
   - Share knowledge ethically

4. **Compliance**
   - Follow local regulations
   - Respect frequency allocations
   - Stay within power limits

### DON'T ❌

1. **Illegal Activities**
   - ❌ Jamming communications
   - ❌ Unauthorized access
   - ❌ Interfering with emergency services
   - ❌ Disrupting critical infrastructure

2. **Unethical Behavior**
   - ❌ Testing without permission
   - ❌ Invading privacy
   - ❌ Malicious transmissions
   - ❌ Harassment via RF

3. **Negligence**
   - ❌ Ignoring regulations
   - ❌ Excessive power levels
   - ❌ Transmitting on restricted frequencies
   - ❌ Operating without understanding risks

## Legal Considerations

### United States (FCC)

**Relevant Regulations**:
- Part 15 (Unlicensed Transmitters)
- Part 97 (Amateur Radio)
- Part 18 (ISM Equipment)

**Key Points**:
- 433 MHz is in ISM band
- Power limits: Typically 1 mW EIRP without license
- Compliance testing may be required for commercial use
- Interference to licensed services is prohibited

**Resources**:
- FCC OET Bulletin 63: https://www.fcc.gov/oet/ea
- Part 15 Rules: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15

### European Union (ETSI)

**Relevant Standards**:
- EN 300 220 (Short Range Devices)
- RED 2014/53/EU (Radio Equipment Directive)

**Key Points**:
- 433.05-434.79 MHz ISM band
- Power limits: 10 mW ERP
- Duty cycle restrictions may apply
- CE marking required for commercial products

### Other Regions

**Consult Local Authorities**:
- Australia: ACMA
- Canada: ISED (formerly Industry Canada)
- Japan: MIC
- China: MIIT

**General Principles**:
- Check frequency allocations
- Verify power limits
- Understand licensing requirements
- Respect interference rules

## Vulnerability Disclosure

### Reporting Security Issues

If you discover a security vulnerability:

1. **Do NOT**:
   - Publicly disclose before fix
   - Exploit vulnerability
   - Share with unauthorized parties

2. **DO**:
   - Email security details privately
   - Provide clear reproduction steps
   - Allow reasonable time for fix
   - Coordinate disclosure timeline

### Contact

**Security Email**: [Create dedicated security contact]

**PGP Key**: [If available]

### Response Timeline

- **Initial Response**: 48 hours
- **Triage**: 7 days
- **Fix Development**: 30-90 days (depending on severity)
- **Public Disclosure**: After fix is released

## Security Best Practices for Users

### Physical Security

1. **Secure Storage**:
   - Store device in secure location
   - Prevent unauthorized physical access
   - Consider lock box if in shared space

2. **Clean Signals**:
   - Delete sensitive captured signals
   - Don't share signal database
   - Be aware of what you're storing

### Operational Security

1. **Awareness**:
   - Know what you're transmitting
   - Understand potential impact
   - Consider unintended consequences

2. **Minimal Exposure**:
   - Only enable features you need
   - Disable transmit when not in use
   - Power off when not needed

3. **Monitoring**:
   - Review audit logs periodically
   - Check for unexpected transmissions
   - Investigate anomalies

### Software Security

1. **Updates**:
   - Keep firmware up to date
   - Review changelogs
   - Test updates before production use

2. **Configuration**:
   - Review config.h settings
   - Understand safety parameters
   - Document any changes

3. **Code Review**:
   - Review code before flashing
   - Understand what you're running
   - Be cautious with third-party modifications

## Security Checklist

Before deploying M5Stack RF Suite:

- [ ] Read and understand this security policy
- [ ] Verify local RF regulations compliance
- [ ] Configure frequency blacklist appropriately
- [ ] Test safety features (confirmation, timeout, rate limit)
- [ ] Review audit logging functionality
- [ ] Secure physical access to device
- [ ] Document authorized use cases
- [ ] Train users on responsible operation
- [ ] Establish incident response procedures
- [ ] Set up monitoring and review process

## Incident Response

### If Misuse Suspected

1. **Immediate Actions**:
   - Power off device
   - Secure physical access
   - Document circumstances

2. **Investigation**:
   - Review audit logs
   - Check configuration
   - Interview operators

3. **Remediation**:
   - Address configuration issues
   - Retrain users
   - Implement additional controls if needed

4. **Reporting**:
   - Report to appropriate authorities if illegal activity
   - Document lessons learned
   - Update procedures

## Code Security

### Security-Critical Files

- `src/safety_module.cpp`: Core safety logic
- `include/safety_module.h`: Safety interfaces
- `include/config.h`: Security configuration
- `src/rf433_module.cpp`: Transmission controls

### Code Review Focus Areas

When reviewing changes:

1. **Transmission Controls**:
   - All transmissions go through safety module
   - No backdoors or bypasses
   - Proper error handling

2. **Input Validation**:
   - Signal parameters validated
   - No buffer overflows
   - Safe string handling

3. **Configuration**:
   - Safe defaults
   - Validated ranges
   - No dangerous options

### Testing Requirements

Before release:

- [ ] All safety features tested
- [ ] Confirmation cannot be bypassed
- [ ] Rate limiting enforced
- [ ] Blacklist respected
- [ ] Audit logging functional
- [ ] Timeout protection working

## Security Improvements (Roadmap)

Future enhancements planned:

1. **Authentication**:
   - PIN code for transmit operations
   - Multi-factor authentication
   - User management

2. **Encryption**:
   - Encrypted signal storage
   - Secure communication for ESP-NOW
   - Protected configuration

3. **Advanced Logging**:
   - Persistent storage on SPIFFS
   - Remote logging capability
   - Real-time alerts

4. **Geofencing**:
   - GPS-based restrictions
   - Location-specific policies
   - Automatic compliance

5. **Time-based Controls**:
   - Schedule-based restrictions
   - Maintenance windows
   - Automatic disable during off-hours

## Acknowledgments

Security best practices inspired by:
- OWASP IoT Security Project
- NIST Cybersecurity Framework
- CWE Top 25 Most Dangerous Software Weaknesses
- Security research community

## Disclaimer

This software is provided for educational and authorized testing purposes only. Users are solely responsible for:
- Compliance with local regulations
- Obtaining necessary permissions
- Consequences of misuse
- Any damages or liabilities

The developers assume no responsibility for:
- Illegal use of this software
- Violations of regulations
- Interference with communications
- Any other misuse or abuse

**Use at your own risk. Know your local laws. Be ethical.**

## Updates to This Policy

This security policy may be updated periodically. Check the repository for the latest version.

**Last Updated**: December 14, 2025
**Version**: 1.0.0
