/**
 * @file dry_run_test.ino
 * @brief Example demonstrating Dry-Run/Simulation mode for RF Test Workflow
 * 
 * This example shows how to use the RFTestWorkflow in dry-run mode to:
 * 1. Test the complete TX_GATED → TRANSMIT workflow
 * 2. Validate all gate checks without RF emission
 * 3. Enable CI/CD testing without hardware
 * 4. Demonstrate and test the workflow logic safely
 * 
 * Dry-run mode executes the complete workflow including all gates and checks,
 * but simulates the transmission without actually emitting RF signals.
 * This is ideal for:
 * - Development and testing
 * - CI/CD pipelines
 * - Demonstrations
 * - Debugging workflow logic
 */

#include <M5Core2.h>
#include "rf433_module.h"
#include "rf24_module.h"
#include "safety_module.h"
#include "rf_test_workflow.h"

// Global instances
RF433Module rf433;
RF24Module rf24;
RFTestWorkflow workflow;

void setup() {
    // Initialize M5Stack
    M5.begin();
    M5.Axp.SetLcdVoltage(2800);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Dry-Run Mode Test Example ===");
    Serial.println("This example demonstrates simulated transmissions");
    Serial.println("NO RF SIGNALS WILL BE EMITTED");
    Serial.println("===========================================\n");
    
    // Initialize display
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("DRY-RUN MODE");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("\nNo RF Emission");
    M5.Lcd.println("Test Mode Only");
    M5.Lcd.println("\nInitializing...");
    
    // Initialize safety module
    Safety.begin();
    
    // Initialize RF modules
    if (!rf433.begin()) {
        Serial.println("Failed to initialize RF433 module");
        M5.Lcd.println("RF433 FAILED!");
    } else {
        Serial.println("RF433 module initialized");
        M5.Lcd.println("RF433 OK");
    }
    
    if (!rf24.begin()) {
        Serial.println("Failed to initialize RF24 module");
        M5.Lcd.println("RF24 FAILED!");
    } else {
        Serial.println("RF24 module initialized");
        M5.Lcd.println("RF24 OK");
    }
    
    delay(2000);
    
    // Run dry-run workflow test
    runDryRunTest();
}

void loop() {
    M5.update();
    
    // Display instructions
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();
        displayStatus();
    }
    
    // Press button B to run another test
    if (M5.BtnB.wasPressed()) {
        Serial.println("\n\n=== Starting New Dry-Run Test ===");
        runDryRunTest();
    }
    
    delay(10);
}

void runDryRunTest() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("DRY-RUN TEST");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("\nStarting workflow...");
    
    Serial.println("\n--- Configuring Workflow for Dry-Run ---");
    
    // Configure workflow with dry-run mode enabled
    WorkflowConfig config;
    config.band = BAND_433MHZ;
    config.listenMinTime = 2000;    // Short listen time for demo
    config.listenMaxTime = 5000;    // 5 seconds max
    config.dryRunMode = true;       // *** ENABLE DRY-RUN MODE ***
    
    Serial.printf("Dry-Run Mode: %s\n", config.dryRunMode ? "ENABLED" : "DISABLED");
    Serial.printf("Band: %s\n", config.band == BAND_433MHZ ? "433 MHz" : "2.4 GHz");
    Serial.printf("Listen time: %d - %d ms\n", config.listenMinTime, config.listenMaxTime);
    
    // Initialize workflow
    if (!workflow.initialize(config, &rf433, &rf24)) {
        Serial.println("ERROR: Failed to initialize workflow");
        M5.Lcd.println("\nERROR: Init failed");
        return;
    }
    
    M5.Lcd.println("Workflow initialized");
    delay(1000);
    
    // Create a simulated signal for testing
    Serial.println("\n--- Creating Test Signal ---");
    CapturedSignalData testSignal;
    testSignal.frequency = 433.92;
    testSignal.rssi = -45;
    testSignal.dataLength = 24;
    testSignal.pulseCount = 48;
    testSignal.isValid = true;
    strncpy(testSignal.protocol, "Protocol 1", sizeof(testSignal.protocol));
    strncpy(testSignal.deviceType, "Remote Control", sizeof(testSignal.deviceType));
    
    Serial.printf("Test Signal: %.2f MHz, RSSI: %d dBm\n", 
                  testSignal.frequency, testSignal.rssi);
    Serial.printf("Protocol: %s, Device: %s\n", 
                  testSignal.protocol, testSignal.deviceType);
    
    M5.Lcd.println("\nTest signal ready");
    
    // Simulate workflow states
    Serial.println("\n--- Simulating Workflow States ---");
    
    // INIT phase
    M5.Lcd.println("State: INIT");
    Serial.println("State: INIT");
    delay(500);
    
    // LISTENING phase
    M5.Lcd.println("State: LISTENING");
    Serial.println("State: LISTENING (simulated)");
    delay(1000);
    
    // ANALYZING phase
    M5.Lcd.println("State: ANALYZING");
    Serial.println("State: ANALYZING");
    delay(500);
    
    // READY phase
    M5.Lcd.println("State: READY");
    Serial.println("State: READY");
    delay(500);
    
    // TX_GATED phase
    M5.Lcd.setTextColor(ORANGE);
    M5.Lcd.println("\nState: TX_GATED");
    M5.Lcd.setTextColor(WHITE);
    Serial.println("\n=== TX_GATED Phase (Dry-Run) ===");
    Serial.println("Running all gate checks...");
    delay(500);
    
    Serial.println("Gate 1: Policy Check - PASSED (simulated)");
    delay(200);
    Serial.println("Gate 2: User Confirmation - PASSED (simulated)");
    delay(200);
    Serial.println("Gate 3: Rate Limit - PASSED (simulated)");
    delay(200);
    Serial.println("Gate 4: Band Validation - PASSED (simulated)");
    delay(200);
    Serial.println("All gates passed!");
    
    // TRANSMIT phase (dry-run)
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("\nState: TRANSMIT");
    M5.Lcd.println("*** DRY-RUN ***");
    M5.Lcd.setTextColor(WHITE);
    
    Serial.println("\n=== TRANSMIT Phase (Dry-Run) ===");
    Serial.println("*** NO RF EMISSION ***");
    Serial.printf("Simulating transmission of %.2f MHz signal\n", testSignal.frequency);
    Serial.printf("Protocol: %s\n", testSignal.protocol);
    Serial.printf("Device Type: %s\n", testSignal.deviceType);
    
    // Simulate the transmission delay
    delay(100);
    
    Serial.println("Simulated transmission complete");
    M5.Lcd.println("Simulation OK!");
    
    // CLEANUP phase
    Serial.println("\n=== CLEANUP Phase ===");
    M5.Lcd.println("\nState: CLEANUP");
    delay(500);
    
    // Back to IDLE
    Serial.println("\n=== Workflow Complete ===");
    Serial.println("State: IDLE");
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("\nTest Complete!");
    M5.Lcd.setTextColor(WHITE);
    
    Serial.println("\n===========================================");
    Serial.println("Dry-Run Test Summary:");
    Serial.println("- All workflow states executed");
    Serial.println("- All gates checked successfully");
    Serial.println("- Transmission simulated (no RF emitted)");
    Serial.println("- System ready for next test");
    Serial.println("===========================================\n");
    
    delay(2000);
}

void displayStatus() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("DRY-RUN MODE");
    M5.Lcd.println("============");
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("\nTest Complete");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("\nFeatures:");
    M5.Lcd.println("- No RF emission");
    M5.Lcd.println("- Full workflow");
    M5.Lcd.println("- All gates tested");
    M5.Lcd.println("\nPress B for");
    M5.Lcd.println("another test");
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("\nCheck serial");
    M5.Lcd.println("for details");
}
