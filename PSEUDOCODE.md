# RF Test Workflows - Structured Pseudocode

This document provides structured pseudocode for RF testing scenarios targeting ESP32 with attached RF modules (CC1101 at 433 MHz, nRF24L01+ at 2.4 GHz).

## Overview

The pseudocode follows these RF testing scenarios:
1. **Initialize**: System and hardware setup
2. **Passive Observation**: Signal capture without transmission
3. **Analysis**: Process captured data
4. **Optional Gated Transmission**: Multi-stage approved transmission
5. **Cleanup**: Resource deallocation and state reset

## Core Data Structures

```pseudocode
ENUM WorkflowState:
    IDLE
    INIT
    LISTENING
    ANALYZING
    READY
    TX_GATED
    TRANSMIT
    CLEANUP
END ENUM

ENUM RFBand:
    BAND_433MHZ
    BAND_24GHZ
END ENUM

STRUCTURE WorkflowConfig:
    RFBand band
    INTEGER initTimeout
    INTEGER listenMinTime
    INTEGER listenMaxTime
    INTEGER analyzeTimeout
    INTEGER readyTimeout
    INTEGER txGateTimeout
    INTEGER transmitMaxDuration
    INTEGER cleanupTimeout
    INTEGER bufferSize
END STRUCTURE

STRUCTURE CapturedSignal:
    TIMESTAMP captureTime
    FLOAT frequency
    INTEGER rssi
    ARRAY[BYTE] rawData
    INTEGER dataLength
    ARRAY[INTEGER] pulseTimes  // For 433 MHz
    STRING protocol
    STRING deviceType
    BOOLEAN isValid
END STRUCTURE

STRUCTURE AnalysisResult:
    INTEGER signalCount
    ARRAY[CapturedSignal] signals
    DICTIONARY[STRING] patterns
    ARRAY[FLOAT] frequencies
    STATISTICS statistics
    TIMESTAMP analysisTime
    BOOLEAN complete
END STRUCTURE

STRUCTURE TransmissionRequest:
    CapturedSignal signal
    TIMESTAMP requestTime
    BOOLEAN userConfirmed
    STRING reason
    INTEGER attemptCount
END STRUCTURE

STRUCTURE WorkflowContext:
    WorkflowState currentState
    WorkflowState previousState
    WorkflowConfig config
    TIMESTAMP stateEntryTime
    ARRAY[CapturedSignal] captureBuffer
    AnalysisResult analysisResult
    TransmissionRequest txRequest
    ARRAY[STRING] errorLog
    INTEGER errorCount
    BOOLEAN emergencyStop
END STRUCTURE
```

## Main Workflow Controller

```pseudocode
CLASS RFTestWorkflow:
    PRIVATE:
        WorkflowContext context
        HardwareInterface hardware
        SafetyModule safety
        AuditLogger logger
    
    PUBLIC:
        FUNCTION initialize(config: WorkflowConfig) -> BOOLEAN:
            context.config = config
            context.currentState = IDLE
            context.emergencyStop = FALSE
            context.errorCount = 0
            
            LOG "Initializing RF Test Workflow"
            LOG "Band: " + config.band
            LOG "Configuration validated"
            
            RETURN TRUE
        END FUNCTION
        
        FUNCTION start() -> BOOLEAN:
            IF context.currentState != IDLE THEN
                LOG_ERROR "Cannot start: workflow not in IDLE state"
                RETURN FALSE
            END IF
            
            LOG "Starting workflow"
            CALL transitionToState(INIT)
            
            // Run state machine
            WHILE context.currentState != IDLE DO
                CALL processCurrentState()
                CALL checkTimeouts()
                CALL checkEmergencyStop()
                
                IF context.errorCount > MAX_ERRORS THEN
                    LOG_ERROR "Too many errors, forcing cleanup"
                    CALL transitionToState(CLEANUP)
                END IF
                
                DELAY_MS(10)  // Small delay to prevent busy loop
            END WHILE
            
            LOG "Workflow completed"
            RETURN context.errorCount == 0
        END FUNCTION
        
        FUNCTION abort():
            LOG_WARNING "Workflow abort requested"
            context.emergencyStop = TRUE
            CALL transitionToState(CLEANUP)
        END FUNCTION
        
    PRIVATE:
        FUNCTION processCurrentState():
            SWITCH context.currentState:
                CASE INIT:
                    CALL processInitState()
                CASE LISTENING:
                    CALL processListeningState()
                CASE ANALYZING:
                    CALL processAnalyzingState()
                CASE READY:
                    CALL processReadyState()
                CASE TX_GATED:
                    CALL processTxGatedState()
                CASE TRANSMIT:
                    CALL processTransmitState()
                CASE CLEANUP:
                    CALL processCleanupState()
                DEFAULT:
                    // IDLE state - do nothing
            END SWITCH
        END FUNCTION
        
        FUNCTION transitionToState(newState: WorkflowState):
            LOG "State transition: " + context.currentState + " -> " + newState
            logger.logTransition(context.currentState, newState, CURRENT_TIME())
            
            context.previousState = context.currentState
            context.currentState = newState
            context.stateEntryTime = CURRENT_TIME()
        END FUNCTION
        
        FUNCTION checkTimeouts() -> BOOLEAN:
            elapsed = CURRENT_TIME() - context.stateEntryTime
            timeout = getTimeoutForState(context.currentState)
            
            IF timeout > 0 AND elapsed > timeout THEN
                LOG_WARNING "Timeout in state: " + context.currentState
                CALL handleTimeout()
                RETURN TRUE
            END IF
            
            RETURN FALSE
        END FUNCTION
        
        FUNCTION checkEmergencyStop():
            IF context.emergencyStop THEN
                LOG_ERROR "Emergency stop activated"
                hardware.disableTransmitter()
                CALL transitionToState(CLEANUP)
            END IF
        END FUNCTION
END CLASS
```

## Scenario 1: Initialize

```pseudocode
FUNCTION processInitState():
    LOG "=== INITIALIZATION PHASE ==="
    
    // Step 1: Hardware initialization
    LOG "Step 1: Initialize hardware"
    TRY:
        IF context.config.band == BAND_433MHZ THEN
            success = hardware.init433MHz()
            LOG "433 MHz module: " + (success ? "OK" : "FAILED")
        ELSE IF context.config.band == BAND_24GHZ THEN
            success = hardware.init24GHz()
            LOG "2.4 GHz module: " + (success ? "OK" : "FAILED")
        END IF
        
        IF NOT success THEN
            LOG_ERROR "Hardware initialization failed"
            CALL logError("Hardware init failed")
            CALL transitionToState(CLEANUP)
            RETURN
        END IF
    CATCH error:
        LOG_ERROR "Exception during hardware init: " + error
        CALL logError("Hardware exception: " + error)
        CALL transitionToState(CLEANUP)
        RETURN
    END TRY
    
    // Step 2: Allocate buffers
    LOG "Step 2: Allocate buffers"
    TRY:
        context.captureBuffer = ALLOCATE_ARRAY(context.config.bufferSize)
        IF context.captureBuffer == NULL THEN
            LOG_ERROR "Buffer allocation failed"
            CALL logError("Memory allocation failed")
            CALL transitionToState(CLEANUP)
            RETURN
        END IF
        LOG "Buffer allocated: " + context.config.bufferSize + " slots"
    CATCH error:
        LOG_ERROR "Buffer allocation exception: " + error
        CALL logError("Memory exception: " + error)
        CALL transitionToState(CLEANUP)
        RETURN
    END TRY
    
    // Step 3: Configure receiver
    LOG "Step 3: Configure receiver"
    TRY:
        IF context.config.band == BAND_433MHZ THEN
            hardware.set433MHzFrequency(433.92)  // MHz
            hardware.set433MHzModulation(OOK_ASK)
            hardware.set433MHzBandwidth(250)  // kHz
        ELSE IF context.config.band == BAND_24GHZ THEN
            hardware.set24GHzChannel(40)  // Default channel
            hardware.set24GHzDataRate(1)  // 1 Mbps
            hardware.set24GHzPower(LOW)
        END IF
        LOG "Receiver configured"
    CATCH error:
        LOG_ERROR "Configuration exception: " + error
        CALL logError("Configuration failed: " + error)
        CALL transitionToState(CLEANUP)
        RETURN
    END TRY
    
    // Step 4: Verify receiver operation
    LOG "Step 4: Verify receiver"
    IF NOT hardware.isReceiverReady() THEN
        LOG_ERROR "Receiver not ready after configuration"
        CALL logError("Receiver verification failed")
        CALL transitionToState(CLEANUP)
        RETURN
    END IF
    LOG "Receiver verified and ready"
    
    // Step 5: Ensure transmitter is disabled
    LOG "Step 5: Disable transmitter (safety)"
    hardware.disableTransmitter()
    hardware.setTransmitterLock(TRUE)
    LOG "Transmitter disabled and locked"
    
    // Step 6: Initialize statistics
    CLEAR context.captureBuffer
    context.analysisResult.signalCount = 0
    context.errorCount = 0
    
    LOG "Initialization complete, transitioning to LISTENING"
    CALL transitionToState(LISTENING)
END FUNCTION
```

## Scenario 2: Passive Observation

```pseudocode
FUNCTION processListeningState():
    LOG "=== PASSIVE OBSERVATION PHASE ==="
    
    // Enforce passive-only mode
    ASSERT hardware.isTransmitterDisabled(), "Transmitter must be disabled"
    
    elapsed = CURRENT_TIME() - context.stateEntryTime
    
    // Check minimum observation time
    IF elapsed < context.config.listenMinTime THEN
        // Continue listening
        CALL captureSignals()
        RETURN
    END IF
    
    // Check for transition triggers
    bufferUsage = SIZE(context.captureBuffer) / context.config.bufferSize
    
    IF bufferUsage >= 0.9 THEN
        LOG "Buffer 90% full, triggering analysis"
        CALL transitionToState(ANALYZING)
        RETURN
    END IF
    
    IF elapsed >= context.config.listenMaxTime THEN
        LOG "Maximum observation time reached"
        CALL transitionToState(ANALYZING)
        RETURN
    END IF
    
    IF userTriggeredAnalysis() THEN
        LOG "User manually triggered analysis"
        CALL transitionToState(ANALYZING)
        RETURN
    END IF
    
    // Continue passive observation
    CALL captureSignals()
END FUNCTION

FUNCTION captureSignals():
    // This function performs passive signal capture
    // NO TRANSMISSION OCCURS HERE
    
    IF context.config.band == BAND_433MHZ THEN
        CALL capture433MHzSignals()
    ELSE IF context.config.band == BAND_24GHZ THEN
        CALL capture24GHzPackets()
    END IF
END FUNCTION

FUNCTION capture433MHzSignals():
    // 433 MHz specific capture with pulse timing
    
    WHILE hardware.hasSignal433MHz() DO
        signal = NEW CapturedSignal
        signal.captureTime = CURRENT_TIME_MICROSECONDS()
        signal.frequency = 433.92
        
        // Read signal data
        rawValue = hardware.readSignalValue()
        bitLength = hardware.readSignalBitLength()
        protocol = hardware.readSignalProtocol()
        
        signal.rawData = rawValue
        signal.protocol = "RCSwitch-" + protocol
        
        // Capture pulse timing (critical for 433 MHz)
        pulseCount = hardware.getPulseCount()
        signal.pulseTimes = ALLOCATE_ARRAY(pulseCount)
        
        FOR i = 0 TO pulseCount - 1 DO
            signal.pulseTimes[i] = hardware.getPulseDuration(i)
        END FOR
        
        // Measure RSSI if available
        signal.rssi = hardware.readRSSI()
        
        // Validate signal
        IF validateSignal433MHz(signal) THEN
            signal.isValid = TRUE
            APPEND signal TO context.captureBuffer
            LOG "Captured 433 MHz signal: " + rawValue + " (" + bitLength + " bits)"
        ELSE
            signal.isValid = FALSE
            LOG_WARNING "Invalid 433 MHz signal discarded"
        END IF
        
        // Check buffer overflow
        IF SIZE(context.captureBuffer) >= context.config.bufferSize THEN
            LOG_WARNING "Capture buffer full"
            BREAK
        END IF
    END WHILE
END FUNCTION

FUNCTION capture24GHzPackets():
    // 2.4 GHz specific capture with packet structure
    
    // Scan multiple channels if needed
    FOR channel = 1 TO 80 DO
        IF context.emergencyStop THEN BREAK
        
        hardware.set24GHzChannel(channel)
        DELAY_MS(10)  // Allow channel settling
        
        // Listen for packets on this channel
        startTime = CURRENT_TIME()
        WHILE (CURRENT_TIME() - startTime) < 100 DO  // 100ms per channel
            IF hardware.hasPacket24GHz() THEN
                packet = NEW CapturedSignal
                packet.captureTime = CURRENT_TIME_MICROSECONDS()
                packet.frequency = 2400 + channel  // MHz
                
                // Read packet data
                packetData = hardware.readPacket()
                packet.rawData = packetData.payload
                packet.dataLength = packetData.length
                
                // Extract addressing (for binding analysis)
                address = packetData.address
                packet.protocol = "nRF24-" + FORMAT_HEX(address)
                
                // Measure RSSI
                packet.rssi = hardware.readRSSI()
                
                // Validate packet
                IF validatePacket24GHz(packet) THEN
                    packet.isValid = TRUE
                    APPEND packet TO context.captureBuffer
                    LOG "Captured 2.4 GHz packet on channel " + channel
                ELSE
                    packet.isValid = FALSE
                    LOG_WARNING "Invalid packet discarded"
                END IF
                
                // Check buffer
                IF SIZE(context.captureBuffer) >= context.config.bufferSize THEN
                    LOG_WARNING "Capture buffer full"
                    RETURN
                END IF
            END IF
        END WHILE
    END FOR
END FUNCTION

FUNCTION validateSignal433MHz(signal: CapturedSignal) -> BOOLEAN:
    // Validation rules for 433 MHz signals
    
    // Check pulse count
    IF SIZE(signal.pulseTimes) < WORKFLOW_433_MIN_PULSES THEN
        LOG_DEBUG "Signal has too few pulses: " + SIZE(signal.pulseTimes)
        RETURN FALSE
    END IF
    
    // Check pulse timing consistency
    FOR i = 0 TO SIZE(signal.pulseTimes) - 1 DO
        pulse = signal.pulseTimes[i]
        IF pulse < 100 OR pulse > 10000 THEN  // Microseconds
            LOG_DEBUG "Pulse timing out of range: " + pulse + "us"
            RETURN FALSE
        END IF
    END FOR
    
    // Check RSSI if available
    IF signal.rssi != 0 AND signal.rssi < -100 THEN
        LOG_DEBUG "Signal too weak: " + signal.rssi + " dBm"
        RETURN FALSE
    END IF
    
    RETURN TRUE
END FUNCTION

FUNCTION validatePacket24GHz(packet: CapturedSignal) -> BOOLEAN:
    // Validation rules for 2.4 GHz packets
    
    // Check packet length
    IF packet.dataLength < 1 OR packet.dataLength > 32 THEN
        LOG_DEBUG "Invalid packet length: " + packet.dataLength
        RETURN FALSE
    END IF
    
    // Verify CRC if available
    IF hardware.supports24GHzCRC() THEN
        IF NOT hardware.verify24GHzCRC(packet) THEN
            LOG_DEBUG "CRC verification failed"
            RETURN FALSE
        END IF
    END IF
    
    // Check RSSI
    IF packet.rssi < -90 THEN
        LOG_DEBUG "Packet too weak: " + packet.rssi + " dBm"
        RETURN FALSE
    END IF
    
    RETURN TRUE
END FUNCTION
```

## Scenario 3: Analysis

```pseudocode
FUNCTION processAnalyzingState():
    LOG "=== ANALYSIS PHASE ==="
    
    IF SIZE(context.captureBuffer) == 0 THEN
        LOG_WARNING "No signals captured, returning to LISTENING"
        CALL transitionToState(LISTENING)
        RETURN
    END IF
    
    LOG "Analyzing " + SIZE(context.captureBuffer) + " captured signals"
    
    // Initialize analysis result
    context.analysisResult = NEW AnalysisResult
    context.analysisResult.signalCount = SIZE(context.captureBuffer)
    context.analysisResult.analysisTime = CURRENT_TIME()
    
    IF context.config.band == BAND_433MHZ THEN
        CALL analyze433MHzSignals()
    ELSE IF context.config.band == BAND_24GHZ THEN
        CALL analyze24GHzPackets()
    END IF
    
    // Generate summary statistics
    CALL generateStatistics()
    
    // Mark analysis as complete
    context.analysisResult.complete = TRUE
    
    LOG "Analysis complete"
    LOG "  Valid signals: " + COUNT_VALID(context.captureBuffer)
    LOG "  Unique patterns: " + SIZE(context.analysisResult.patterns)
    LOG "  Frequency range: " + MIN_FREQ() + " - " + MAX_FREQ() + " MHz"
    
    CALL transitionToState(READY)
END FUNCTION

FUNCTION analyze433MHzSignals():
    LOG "Performing 433 MHz pulse analysis"
    
    // Step 1: Protocol classification
    protocolCounts = NEW DICTIONARY[STRING, INTEGER]
    
    FOR EACH signal IN context.captureBuffer DO
        IF signal.isValid THEN
            protocol = signal.protocol
            IF protocolCounts.contains(protocol) THEN
                protocolCounts[protocol] = protocolCounts[protocol] + 1
            ELSE
                protocolCounts[protocol] = 1
            END IF
        END IF
    END FOR
    
    LOG "Protocol distribution:"
    FOR EACH protocol, count IN protocolCounts DO
        LOG "  " + protocol + ": " + count + " signals"
    END FOR
    
    // Step 2: Pulse timing analysis
    allPulses = NEW ARRAY[INTEGER]
    
    FOR EACH signal IN context.captureBuffer DO
        IF signal.isValid THEN
            FOR EACH pulse IN signal.pulseTimes DO
                APPEND pulse TO allPulses
            END FOR
        END IF
    END FOR
    
    avgPulse = AVERAGE(allPulses)
    minPulse = MIN(allPulses)
    maxPulse = MAX(allPulses)
    stdDevPulse = STANDARD_DEVIATION(allPulses)
    
    LOG "Pulse timing statistics:"
    LOG "  Average: " + avgPulse + " us"
    LOG "  Range: " + minPulse + " - " + maxPulse + " us"
    LOG "  Std Dev: " + stdDevPulse + " us"
    
    // Step 3: Device type classification
    FOR EACH signal IN context.captureBuffer DO
        IF signal.isValid THEN
            signal.deviceType = classifyDevice433MHz(signal)
            LOG "Signal " + signal.rawData + " classified as: " + signal.deviceType
        END IF
    END FOR
    
    // Step 4: Pattern recognition
    patterns = detectPatterns433MHz(context.captureBuffer)
    context.analysisResult.patterns = patterns
    
    LOG "Detected " + SIZE(patterns) + " unique patterns"
    
    // Store analysis results
    context.analysisResult.signals = COPY(context.captureBuffer)
END FUNCTION

FUNCTION analyze24GHzPackets():
    LOG "Performing 2.4 GHz packet analysis"
    
    // Step 1: Address analysis for binding detection
    addressCounts = NEW DICTIONARY[STRING, INTEGER]
    
    FOR EACH packet IN context.captureBuffer DO
        IF packet.isValid THEN
            address = EXTRACT_ADDRESS(packet.protocol)
            IF addressCounts.contains(address) THEN
                addressCounts[address] = addressCounts[address] + 1
            ELSE
                addressCounts[address] = 1
            END IF
        END IF
    END FOR
    
    LOG "Address distribution:"
    FOR EACH address, count IN addressCounts DO
        LOG "  " + address + ": " + count + " packets"
        
        // Identify likely binding pairs (addresses seen frequently together)
        IF count >= 5 THEN
            LOG "  ^ Potential binding detected"
        END IF
    END FOR
    
    // Step 2: Channel usage analysis
    channelCounts = NEW DICTIONARY[INTEGER, INTEGER]
    
    FOR EACH packet IN context.captureBuffer DO
        IF packet.isValid THEN
            channel = ROUND(packet.frequency - 2400)
            IF channelCounts.contains(channel) THEN
                channelCounts[channel] = channelCounts[channel] + 1
            ELSE
                channelCounts[channel] = 1
            END IF
        END IF
    END FOR
    
    LOG "Channel usage:"
    FOR EACH channel, count IN channelCounts DO
        LOG "  Channel " + channel + ": " + count + " packets"
    END FOR
    
    // Step 3: Packet structure analysis
    FOR EACH packet IN context.captureBuffer DO
        IF packet.isValid THEN
            CALL analyzePacketStructure(packet)
        END IF
    END FOR
    
    // Step 4: Binding pair extraction
    bindings = detectBindingPairs(context.captureBuffer)
    context.analysisResult.patterns["bindings"] = bindings
    
    LOG "Detected " + SIZE(bindings) + " binding pairs"
    
    // Store analysis results
    context.analysisResult.signals = COPY(context.captureBuffer)
END FUNCTION

FUNCTION classifyDevice433MHz(signal: CapturedSignal) -> STRING:
    // Heuristic classification based on signal characteristics
    
    avgPulse = AVERAGE(signal.pulseTimes)
    pulseCount = SIZE(signal.pulseTimes)
    
    // Garage door remotes: Long pulses, 24-bit codes
    IF avgPulse > 400 AND pulseCount >= 48 THEN
        RETURN "Garage Door Remote"
    END IF
    
    // Doorbells: Short pulses, 12-24 bits
    IF avgPulse < 350 AND pulseCount < 48 THEN
        RETURN "Doorbell"
    END IF
    
    // Car remotes: Variable, typically 64+ bits
    IF pulseCount >= 128 THEN
        RETURN "Car Remote"
    END IF
    
    // Weather stations: Periodic, specific patterns
    IF signal.protocol CONTAINS "Oregon" OR signal.protocol CONTAINS "Lacrosse" THEN
        RETURN "Weather Station"
    END IF
    
    RETURN "Unknown Device"
END FUNCTION

FUNCTION detectBindingPairs(packets: ARRAY[CapturedSignal]) -> ARRAY:
    // Detect pairs of addresses that communicate frequently
    
    bindings = NEW ARRAY
    addressPairs = NEW DICTIONARY[(STRING, STRING), INTEGER]
    
    // Look for request-response patterns
    FOR i = 0 TO SIZE(packets) - 2 DO
        IF packets[i].isValid AND packets[i+1].isValid THEN
            addr1 = EXTRACT_ADDRESS(packets[i].protocol)
            addr2 = EXTRACT_ADDRESS(packets[i+1].protocol)
            
            timeDiff = packets[i+1].captureTime - packets[i].captureTime
            
            // If packets are close in time (< 10ms), likely a pair
            IF timeDiff < 10000 AND addr1 != addr2 THEN  // 10ms in microseconds
                pair = SORT_PAIR(addr1, addr2)
                IF addressPairs.contains(pair) THEN
                    addressPairs[pair] = addressPairs[pair] + 1
                ELSE
                    addressPairs[pair] = 1
                END IF
            END IF
        END IF
    END FOR
    
    // Extract significant pairs (seen at least 3 times)
    FOR EACH pair, count IN addressPairs DO
        IF count >= 3 THEN
            APPEND pair TO bindings
        END IF
    END FOR
    
    RETURN bindings
END FUNCTION

FUNCTION generateStatistics():
    stats = NEW STATISTICS
    
    // Count valid signals
    validCount = 0
    FOR EACH signal IN context.captureBuffer DO
        IF signal.isValid THEN
            validCount = validCount + 1
        END IF
    END FOR
    
    stats["total_signals"] = SIZE(context.captureBuffer)
    stats["valid_signals"] = validCount
    stats["invalid_signals"] = SIZE(context.captureBuffer) - validCount
    
    // RSSI statistics
    rssiValues = NEW ARRAY[FLOAT]
    FOR EACH signal IN context.captureBuffer DO
        IF signal.isValid AND signal.rssi != 0 THEN
            APPEND signal.rssi TO rssiValues
        END IF
    END FOR
    
    IF SIZE(rssiValues) > 0 THEN
        stats["avg_rssi"] = AVERAGE(rssiValues)
        stats["min_rssi"] = MIN(rssiValues)
        stats["max_rssi"] = MAX(rssiValues)
    END IF
    
    // Time span
    IF SIZE(context.captureBuffer) > 0 THEN
        firstTime = context.captureBuffer[0].captureTime
        lastTime = context.captureBuffer[SIZE(context.captureBuffer)-1].captureTime
        stats["capture_duration_ms"] = (lastTime - firstTime) / 1000
    END IF
    
    context.analysisResult.statistics = stats
    
    LOG "Statistics generated"
    FOR EACH key, value IN stats DO
        LOG "  " + key + ": " + value
    END FOR
END FUNCTION
```

## Scenario 4: Optional Gated Transmission

```pseudocode
FUNCTION processReadyState():
    LOG "=== READY PHASE ==="
    LOG "Analysis results available, awaiting user decision"
    
    // Display results to user
    CALL displayAnalysisResults()
    
    // Wait for user input
    userAction = getUserInput()
    
    IF userAction == "TRANSMIT" THEN
        LOG "User requested transmission"
        
        // Get selected signal
        selectedIndex = getUserSignalSelection()
        IF selectedIndex >= 0 AND selectedIndex < SIZE(context.analysisResult.signals) THEN
            selectedSignal = context.analysisResult.signals[selectedIndex]
            
            // Create transmission request
            context.txRequest = NEW TransmissionRequest
            context.txRequest.signal = selectedSignal
            context.txRequest.requestTime = CURRENT_TIME()
            context.txRequest.userConfirmed = FALSE
            context.txRequest.reason = "User initiated from analysis results"
            context.txRequest.attemptCount = 0
            
            CALL transitionToState(TX_GATED)
        ELSE
            LOG_ERROR "Invalid signal selection"
        END IF
        
    ELSE IF userAction == "CANCEL" THEN
        LOG "User canceled, ending workflow"
        CALL transitionToState(CLEANUP)
        
    ELSE IF userAction == "CONTINUE_OBSERVATION" THEN
        LOG "User requested more observation"
        CALL transitionToState(LISTENING)
    END IF
END FUNCTION

FUNCTION processTxGatedState():
    LOG "=== GATED TRANSMISSION PHASE ==="
    LOG "Multi-stage approval process"
    
    // Increment attempt counter
    context.txRequest.attemptCount = context.txRequest.attemptCount + 1
    
    IF context.txRequest.attemptCount > 3 THEN
        LOG_ERROR "Too many transmission attempts, aborting"
        CALL transitionToState(READY)
        RETURN
    END IF
    
    // GATE 1: Policy Check
    LOG "Gate 1: Policy validation"
    IF NOT checkPolicyGate() THEN
        LOG_WARNING "Policy gate FAILED"
        CALL showUserMessage("Transmission denied: Policy violation")
        CALL transitionToState(READY)
        RETURN
    END IF
    LOG "Gate 1: PASSED"
    
    // GATE 2: Safety Confirmation
    LOG "Gate 2: User confirmation"
    IF NOT checkConfirmationGate() THEN
        LOG_WARNING "Confirmation gate FAILED"
        CALL showUserMessage("Transmission canceled: No confirmation")
        CALL transitionToState(READY)
        RETURN
    END IF
    LOG "Gate 2: PASSED"
    
    // GATE 3: Rate Limit Check
    LOG "Gate 3: Rate limiting"
    IF NOT checkRateLimitGate() THEN
        LOG_WARNING "Rate limit gate FAILED"
        CALL showUserMessage("Transmission denied: Rate limit exceeded")
        CALL transitionToState(READY)
        RETURN
    END IF
    LOG "Gate 3: PASSED"
    
    // GATE 4: Band-Specific Validation
    LOG "Gate 4: Band-specific validation"
    IF context.config.band == BAND_433MHZ THEN
        IF NOT check433MHzGate() THEN
            LOG_WARNING "433 MHz specific gate FAILED"
            CALL transitionToState(READY)
            RETURN
        END IF
    ELSE IF context.config.band == BAND_24GHZ THEN
        IF NOT check24GHzGate() THEN
            LOG_WARNING "2.4 GHz specific gate FAILED"
            CALL transitionToState(READY)
            RETURN
        END IF
    END IF
    LOG "Gate 4: PASSED"
    
    // All gates passed
    LOG "ALL GATES PASSED - Transmission approved"
    logger.logTransmissionApproval(context.txRequest)
    
    CALL transitionToState(TRANSMIT)
END FUNCTION

FUNCTION checkPolicyGate() -> BOOLEAN:
    signal = context.txRequest.signal
    
    // Check frequency blacklist
    IF isFrequencyBlacklisted(signal.frequency) THEN
        LOG_WARNING "Frequency " + signal.frequency + " MHz is blacklisted"
        RETURN FALSE
    END IF
    
    // Check duration limit
    estimatedDuration = estimateTransmissionDuration(signal)
    IF estimatedDuration > context.config.transmitMaxDuration THEN
        LOG_WARNING "Estimated duration " + estimatedDuration + " ms exceeds limit"
        RETURN FALSE
    END IF
    
    // Check data integrity
    IF NOT signal.isValid THEN
        LOG_WARNING "Signal data is not valid"
        RETURN FALSE
    END IF
    
    RETURN TRUE
END FUNCTION

FUNCTION checkConfirmationGate() -> BOOLEAN:
    // Display warning to user
    CALL displayTransmissionWarning(context.txRequest.signal)
    
    LOG "Waiting for user confirmation..."
    LOG "Press CONFIRM button within " + context.config.txGateTimeout + " ms"
    
    startTime = CURRENT_TIME()
    
    WHILE (CURRENT_TIME() - startTime) < context.config.txGateTimeout DO
        IF userPressedConfirm() THEN
            LOG "User confirmed transmission"
            context.txRequest.userConfirmed = TRUE
            RETURN TRUE
        END IF
        
        IF userPressedCancel() THEN
            LOG "User canceled transmission"
            RETURN FALSE
        END IF
        
        DELAY_MS(10)
    END WHILE
    
    LOG "Confirmation timeout - transmission denied"
    RETURN FALSE
END FUNCTION

FUNCTION checkRateLimitGate() -> BOOLEAN:
    recentCount = safety.getRecentTransmissionCount()
    rateLimit = safety.getRateLimit()
    
    IF recentCount >= rateLimit THEN
        LOG_WARNING "Rate limit exceeded: " + recentCount + " / " + rateLimit
        RETURN FALSE
    END IF
    
    LOG "Rate limit OK: " + recentCount + " / " + rateLimit
    RETURN TRUE
END FUNCTION

FUNCTION check433MHzGate() -> BOOLEAN:
    // 433 MHz specific validation
    signal = context.txRequest.signal
    
    LOG "Validating 433 MHz pulse parameters"
    
    // Verify pulse timing is within acceptable range
    FOR EACH pulse IN signal.pulseTimes DO
        IF pulse < 100 OR pulse > 10000 THEN
            LOG_WARNING "Pulse timing out of safe range: " + pulse + " us"
            RETURN FALSE
        END IF
    END FOR
    
    // Verify protocol is known/safe
    IF NOT isSafeProtocol(signal.protocol) THEN
        LOG_WARNING "Unknown or unsafe protocol: " + signal.protocol
        RETURN FALSE
    END IF
    
    LOG "433 MHz validation passed"
    RETURN TRUE
END FUNCTION

FUNCTION check24GHzGate() -> BOOLEAN:
    // 2.4 GHz specific validation with binding verification
    signal = context.txRequest.signal
    
    LOG "Validating 2.4 GHz packet and binding"
    
    // Extract destination address
    destAddress = EXTRACT_ADDRESS(signal.protocol)
    
    // Verify address was observed (binding)
    IF NOT wasAddressObserved(destAddress) THEN
        LOG_WARNING "Destination address not in observed bindings"
        RETURN FALSE
    END IF
    
    // Verify channel is safe
    channel = ROUND(signal.frequency - 2400)
    IF channel < 1 OR channel > 80 THEN
        LOG_WARNING "Invalid channel: " + channel
        RETURN FALSE
    END IF
    
    // Verify packet structure
    IF signal.dataLength < 1 OR signal.dataLength > 32 THEN
        LOG_WARNING "Invalid packet length: " + signal.dataLength
        RETURN FALSE
    END IF
    
    LOG "2.4 GHz validation passed (binding verified)"
    RETURN TRUE
END FUNCTION

FUNCTION wasAddressObserved(address: STRING) -> BOOLEAN:
    // Check if address was seen during observation phase
    FOR EACH signal IN context.analysisResult.signals DO
        signalAddress = EXTRACT_ADDRESS(signal.protocol)
        IF signalAddress == address THEN
            RETURN TRUE
        END IF
    END FOR
    RETURN FALSE
END FUNCTION
```

## Scenario 5: Cleanup

```pseudocode
FUNCTION processCleanupState():
    LOG "=== CLEANUP PHASE ==="
    
    // This phase ALWAYS executes, regardless of success/failure
    
    // Step 1: Disable transmitter (critical safety step)
    LOG "Step 1: Disable transmitter"
    TRY:
        hardware.disableTransmitter()
        hardware.setTransmitterLock(TRUE)
        LOG "Transmitter disabled and locked"
    CATCH error:
        LOG_ERROR "Failed to disable transmitter: " + error
        // Try again with different method
        TRY:
            hardware.forceTransmitterOff()
        CATCH error2:
            LOG_CRITICAL "CRITICAL: Cannot disable transmitter!"
            context.emergencyStop = TRUE
        END TRY
    END TRY
    
    // Step 2: Disable receiver
    LOG "Step 2: Disable receiver"
    TRY:
        hardware.disableReceiver()
        LOG "Receiver disabled"
    CATCH error:
        LOG_WARNING "Failed to disable receiver: " + error
        // Non-critical, continue cleanup
    END TRY
    
    // Step 3: Save logs and audit trail
    LOG "Step 3: Save audit trail"
    TRY:
        logger.saveAuditLog()
        logger.saveStateHistory()
        LOG "Audit trail saved"
    CATCH error:
        LOG_ERROR "Failed to save audit trail: " + error
        // Continue cleanup even if logging fails
    END TRY
    
    // Step 4: Free allocated buffers
    LOG "Step 4: Free buffers"
    TRY:
        IF context.captureBuffer != NULL THEN
            FREE(context.captureBuffer)
            context.captureBuffer = NULL
        END IF
        
        IF context.analysisResult.signals != NULL THEN
            FREE(context.analysisResult.signals)
            context.analysisResult.signals = NULL
        END IF
        
        LOG "Buffers freed"
    CATCH error:
        LOG_ERROR "Failed to free buffers: " + error
        // Memory leak, but continue
    END TRY
    
    // Step 5: Clear sensitive data
    LOG "Step 5: Clear sensitive data"
    TRY:
        // Overwrite signal data with zeros
        FOR EACH signal IN context.captureBuffer DO
            CLEAR signal.rawData
            CLEAR signal.pulseTimes
        END FOR
        
        // Clear transmission request
        IF context.txRequest != NULL THEN
            CLEAR context.txRequest.signal
        END IF
        
        LOG "Sensitive data cleared"
    CATCH error:
        LOG_WARNING "Failed to clear data: " + error
    END TRY
    
    // Step 6: Reset hardware to safe state
    LOG "Step 6: Reset hardware"
    TRY:
        hardware.resetToSafeState()
        LOG "Hardware reset complete"
    CATCH error:
        LOG_ERROR "Failed to reset hardware: " + error
    END TRY
    
    // Step 7: Update statistics
    LOG "Step 7: Update statistics"
    TRY:
        IF context.previousState == TRANSMIT THEN
            statistics.incrementTransmitCount()
        END IF
        statistics.updateLastWorkflowTime(CURRENT_TIME())
        statistics.recordWorkflowResult(context.errorCount == 0)
    CATCH error:
        LOG_WARNING "Failed to update statistics: " + error
    END TRY
    
    // Step 8: Final logging
    LOG "Step 8: Final logging"
    IF context.errorCount == 0 THEN
        LOG "Workflow completed successfully"
    ELSE
        LOG "Workflow completed with " + context.errorCount + " errors"
    END IF
    
    LOG "Total signals captured: " + SIZE(context.captureBuffer)
    LOG "Cleanup complete, transitioning to IDLE"
    
    // Always transition to IDLE
    CALL transitionToState(IDLE)
END FUNCTION

FUNCTION processTransmitState():
    LOG "=== TRANSMISSION PHASE ==="
    
    signal = context.txRequest.signal
    
    // Final safety check
    IF NOT hardware.isTransmitterReady() THEN
        LOG_ERROR "Transmitter not ready"
        CALL transitionToState(CLEANUP)
        RETURN
    END IF
    
    LOG "Beginning transmission"
    LOG "  Frequency: " + signal.frequency + " MHz"
    LOG "  Protocol: " + signal.protocol
    LOG "  Data length: " + signal.dataLength + " bytes"
    
    // Start transmission watchdog
    watchdog.start(context.config.transmitMaxDuration)
    
    TRY:
        // Unlock and enable transmitter
        hardware.setTransmitterLock(FALSE)
        hardware.enableTransmitter()
        
        transmitStartTime = CURRENT_TIME_MICROSECONDS()
        
        // Perform band-specific transmission
        IF context.config.band == BAND_433MHZ THEN
            success = transmit433MHz(signal)
        ELSE IF context.config.band == BAND_24GHZ THEN
            success = transmit24GHz(signal)
        ELSE
            LOG_ERROR "Unknown band"
            success = FALSE
        END IF
        
        transmitEndTime = CURRENT_TIME_MICROSECONDS()
        transmitDuration = (transmitEndTime - transmitStartTime) / 1000  // ms
        
        // Stop watchdog
        watchdog.stop()
        
        // Immediately disable transmitter
        hardware.disableTransmitter()
        hardware.setTransmitterLock(TRUE)
        
        IF success THEN
            LOG "Transmission completed successfully"
            LOG "  Duration: " + transmitDuration + " ms"
            
            // Log successful transmission
            logger.logTransmission(signal, TRUE, transmitDuration)
            
            // Update statistics
            safety.recordTransmission()
        ELSE
            LOG_ERROR "Transmission failed"
            logger.logTransmission(signal, FALSE, transmitDuration)
            CALL logError("Transmission failure")
        END IF
        
    CATCH error:
        LOG_ERROR "Exception during transmission: " + error
        
        // Emergency stop
        watchdog.stop()
        hardware.forceTransmitterOff()
        hardware.setTransmitterLock(TRUE)
        
        logger.logTransmission(signal, FALSE, 0)
        CALL logError("Transmission exception: " + error)
    END TRY
    
    // Always proceed to cleanup
    CALL transitionToState(CLEANUP)
END FUNCTION

FUNCTION transmit433MHz(signal: CapturedSignal) -> BOOLEAN:
    LOG "Transmitting 433 MHz signal"
    
    // Configure transmitter parameters
    hardware.set433MHzFrequency(signal.frequency)
    hardware.set433MHzPower(LOW)  // Use minimum power
    
    // Use pulse timing if available
    IF SIZE(signal.pulseTimes) > 0 THEN
        // Transmit with precise pulse timing
        FOR EACH pulse IN signal.pulseTimes DO
            hardware.transmitPulse(pulse)
        END FOR
    ELSE
        // Use decoded value and protocol
        hardware.setProtocol(EXTRACT_PROTOCOL_NUMBER(signal.protocol))
        hardware.transmitValue(signal.rawData, SIZE(signal.rawData) * 8)
    END IF
    
    // Verify transmission
    RETURN hardware.getTransmitStatus() == SUCCESS
END FUNCTION

FUNCTION transmit24GHz(signal: CapturedSignal) -> BOOLEAN:
    LOG "Transmitting 2.4 GHz packet"
    
    // Extract parameters
    channel = ROUND(signal.frequency - 2400)
    address = EXTRACT_ADDRESS(signal.protocol)
    
    // Configure transmitter
    hardware.set24GHzChannel(channel)
    hardware.set24GHzAddress(address)
    hardware.set24GHzPower(LOW)  // Use minimum power
    
    // Transmit packet
    hardware.transmit24GHzPacket(signal.rawData, signal.dataLength)
    
    // Wait for ACK (with timeout)
    ackReceived = hardware.wait24GHzACK(1000)  // 1 second timeout
    
    IF ackReceived THEN
        LOG "ACK received - packet delivered"
        RETURN TRUE
    ELSE
        LOG_WARNING "No ACK received"
        RETURN FALSE
    END IF
END FUNCTION
```

## Helper Functions

```pseudocode
FUNCTION logError(message: STRING):
    context.errorCount = context.errorCount + 1
    APPEND message TO context.errorLog
    logger.logError(message, context.currentState, CURRENT_TIME())
END FUNCTION

FUNCTION getTimeoutForState(state: WorkflowState) -> INTEGER:
    SWITCH state:
        CASE INIT:
            RETURN context.config.initTimeout
        CASE LISTENING:
            RETURN context.config.listenMaxTime
        CASE ANALYZING:
            RETURN context.config.analyzeTimeout
        CASE READY:
            RETURN context.config.readyTimeout
        CASE TX_GATED:
            RETURN context.config.txGateTimeout
        CASE TRANSMIT:
            RETURN context.config.transmitMaxDuration
        CASE CLEANUP:
            RETURN context.config.cleanupTimeout
        DEFAULT:
            RETURN 0  // IDLE has no timeout
    END SWITCH
END FUNCTION

FUNCTION handleTimeout():
    LOG_WARNING "Timeout in state " + context.currentState
    
    SWITCH context.currentState:
        CASE INIT:
            CALL logError("Initialization timeout")
            CALL transitionToState(CLEANUP)
            
        CASE LISTENING:
            LOG "Observation timeout - forcing analysis"
            CALL transitionToState(ANALYZING)
            
        CASE ANALYZING:
            CALL logError("Analysis timeout")
            CALL transitionToState(READY)
            
        CASE READY:
            LOG "User inactivity timeout"
            CALL transitionToState(CLEANUP)
            
        CASE TX_GATED:
            LOG "Transmission approval timeout - denied"
            CALL transitionToState(READY)
            
        CASE TRANSMIT:
            LOG_ERROR "Transmission duration exceeded - emergency stop"
            hardware.forceTransmitterOff()
            CALL transitionToState(CLEANUP)
            
        CASE CLEANUP:
            LOG_ERROR "Cleanup timeout - forcing IDLE"
            CALL transitionToState(IDLE)
    END SWITCH
END FUNCTION

FUNCTION isFrequencyBlacklisted(frequency: FLOAT) -> BOOLEAN:
    FOR EACH blacklistedFreq IN FREQ_BLACKLIST DO
        IF ABS(frequency - blacklistedFreq) < 0.1 THEN  // Within 100 kHz
            RETURN TRUE
        END IF
    END FOR
    RETURN FALSE
END FUNCTION

FUNCTION estimateTransmissionDuration(signal: CapturedSignal) -> INTEGER:
    IF context.config.band == BAND_433MHZ THEN
        // Calculate based on pulse count and timing
        totalPulseDuration = SUM(signal.pulseTimes)
        repeatCount = 10  // Typical repeat count
        RETURN (totalPulseDuration * repeatCount) / 1000  // Convert to ms
    ELSE IF context.config.band == BAND_24GHZ THEN
        // Estimate based on packet size and data rate
        dataRate = 1000000  // 1 Mbps typical
        bitsToSend = signal.dataLength * 8
        transmitTime = (bitsToSend / dataRate) * 1000  // ms
        RETURN transmitTime + 10  // Add overhead
    END IF
    RETURN 0
END FUNCTION
```

## Usage Example

```pseudocode
FUNCTION main():
    LOG "=== RF Test Workflow Example ==="
    
    // Create workflow instance
    workflow = NEW RFTestWorkflow()
    
    // Configure for 433 MHz testing
    config = NEW WorkflowConfig
    config.band = BAND_433MHZ
    config.initTimeout = 5000
    config.listenMinTime = 1000
    config.listenMaxTime = 60000
    config.analyzeTimeout = 10000
    config.readyTimeout = 120000
    config.txGateTimeout = 10000
    config.transmitMaxDuration = 5000
    config.cleanupTimeout = 5000
    config.bufferSize = 100
    
    // Initialize workflow
    IF NOT workflow.initialize(config) THEN
        LOG_ERROR "Failed to initialize workflow"
        RETURN
    END IF
    
    // Start workflow (blocks until completion)
    success = workflow.start()
    
    IF success THEN
        LOG "Workflow completed successfully"
    ELSE
        LOG "Workflow completed with errors"
    END IF
    
    LOG "=== Workflow finished ==="
END FUNCTION
```

---

**Document Version**: 1.0  
**Last Updated**: 2025-12-14  
**Status**: Complete
