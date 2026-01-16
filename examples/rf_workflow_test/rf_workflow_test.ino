/**
 * @file rf_workflow_test.ino
 * @brief Example demonstrating RF Test Workflow for 433 MHz
 * 
 * This example shows how to use the RFTestWorkflow class to:
 * 1. Initialize hardware
 * 2. Perform passive observation
 * 3. Analyze captured signals
 * 4. Optionally transmit with gated approval
 * 5. Cleanup resources
 * 
 * Educational Features:
 * - Demonstrates specific gate failure scenarios (policy, rate limit, band mismatch)
 * - Shows detailed failure reasons to help users understand the safety model
 * - Rotates through different outcomes on successive runs
 * - Provides context for why transmissions are denied
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

// UI state
int menuSelection = 0;
bool inWorkflow = false;

// Gate scenario configuration
const int SCENARIO_COUNT = 5; // Total number of gate outcome scenarios

void setup() {
    // Initialize M5Stack
    M5.begin();
    M5.Axp.SetLcdVoltage(2800);
    
    Serial.begin(115200);
    Serial.println("\n\n=== RF Workflow Test Example ===");
    
    // Initialize display
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("RF Workflow Test");
    M5.Lcd.println("\nInitializing...");
    
    // Initialize safety module
    Safety.begin();
    
    // Initialize RF modules
    if (!rf433.begin()) {
        Serial.println("Failed to initialize RF433 module");
        M5.Lcd.println("RF433 FAILED!");
        while (1) delay(100);
    }
    
    if (!rf24.begin()) {
        Serial.println("Failed to initialize RF24 module");
        M5.Lcd.println("RF24 FAILED!");
        while (1) delay(100);
    }
    
    M5.Lcd.println("Ready!");
    delay(2000);
    
    displayMainMenu();
}

void loop() {
    M5.update();
    
    if (!inWorkflow) {
        // Main menu handling
        if (M5.BtnA.wasPressed()) {
            menuSelection = (menuSelection - 1 + 3) % 3;
            displayMainMenu();
        }
        
        if (M5.BtnC.wasPressed()) {
            menuSelection = (menuSelection + 1) % 3;
            displayMainMenu();
        }
        
        if (M5.BtnB.wasPressed()) {
            handleMenuSelection();
        }
    }
    
    delay(10);
}

void displayMainMenu() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("RF Workflow Test");
    M5.Lcd.println("");
    
    const char* menuItems[] = {
        "1. 433MHz Workflow",
        "2. 2.4GHz Workflow",
        "3. About"
    };
    
    for (int i = 0; i < 3; i++) {
        if (i == menuSelection) {
            M5.Lcd.setTextColor(GREEN);
            M5.Lcd.print("> ");
        } else {
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.print("  ");
        }
        M5.Lcd.println(menuItems[i]);
    }
    
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("A:Prev  B:Select  C:Next");
}

void handleMenuSelection() {
    switch (menuSelection) {
        case 0:
            run433MHzWorkflow();
            break;
        case 1:
            run24GHzWorkflow();
            break;
        case 2:
            displayAbout();
            break;
    }
}

void run433MHzWorkflow() {
    inWorkflow = true;
    
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("433 MHz Workflow");
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Starting...");
    
    Serial.println("\n=== Starting 433 MHz Workflow ===");
    
    // Configure workflow for 433 MHz
    WorkflowConfig config;
    config.band = BAND_433MHZ;
    config.listenMinTime = 5000;   // 5 seconds minimum
    config.listenMaxTime = 30000;  // 30 seconds maximum
    
    // Initialize workflow
    if (!workflow.initialize(config, &rf433, &rf24)) {
        M5.Lcd.println("ERROR: Init failed!");
        delay(3000);
        inWorkflow = false;
        displayMainMenu();
        return;
    }
    
    M5.Lcd.println("Workflow initialized");
    M5.Lcd.println("");
    M5.Lcd.println("Press B to start capture");
    M5.Lcd.println("Press A to cancel");
    
    // Wait for user to start
    while (true) {
        M5.update();
        if (M5.BtnB.wasPressed()) break;
        if (M5.BtnA.wasPressed()) {
            inWorkflow = false;
            displayMainMenu();
            return;
        }
        delay(10);
    }
    
    // Display workflow progress
    displayWorkflowProgress();
    
    // Start workflow in non-blocking mode using state checks
    workflow.reset();
    workflow.initialize(config, &rf433, &rf24);
    
    // Manually drive the workflow with user feedback
    manualWorkflowExecution();
    
    inWorkflow = false;
    displayMainMenu();
}

void run24GHzWorkflow() {
    inWorkflow = true;
    
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("2.4 GHz Workflow");
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Not yet implemented");
    M5.Lcd.println("");
    M5.Lcd.println("Press any button...");
    
    while (true) {
        M5.update();
        if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
            break;
        }
        delay(10);
    }
    
    inWorkflow = false;
    displayMainMenu();
}

void displayAbout() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("About");
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("RF Test Workflow Example");
    M5.Lcd.println("Version 1.1.0");
    M5.Lcd.println("");
    M5.Lcd.println("Demonstrates:");
    M5.Lcd.println("- Passive observation");
    M5.Lcd.println("- Signal analysis");
    M5.Lcd.println("- Gated transmission");
    M5.Lcd.println("- Gate failure scenarios");
    M5.Lcd.println("- Detailed failure reasons");
    M5.Lcd.println("");
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("Run multiple times to see");
    M5.Lcd.println("different gate outcomes!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.println("Press any button...");
    
    while (true) {
        M5.update();
        if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
            break;
        }
        delay(10);
    }
    
    displayMainMenu();
}

void displayWorkflowProgress() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Workflow Progress:");
    M5.Lcd.println("===================");
}

void manualWorkflowExecution() {
    // This is a simplified manual execution for demonstration
    // The actual workflow.start() method would run the full state machine
    
    Serial.println("Starting manual workflow execution");
    
    // Phase 1: INIT
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: INIT");
    M5.Lcd.println("Initializing hardware...");
    delay(1000);
    
    // Phase 2: LISTENING (Passive Observation)
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: LISTENING");
    M5.Lcd.println("Passive observation mode");
    M5.Lcd.println("");
    M5.Lcd.println("Capturing signals...");
    M5.Lcd.println("(No transmission)");
    M5.Lcd.println("");
    M5.Lcd.println("Press B to analyze");
    M5.Lcd.println("Press A to abort");
    
    uint32_t listenStart = millis();
    int signalCount = 0;
    
    // Simulate listening phase
    while (millis() - listenStart < 15000) {  // 15 second demo
        M5.update();
        
        // Check for signals
        if (rf433.isSignalAvailable()) {
            RF433Signal sig = rf433.receiveSignal();
            if (sig.isValid) {
                signalCount++;
                M5.Lcd.fillRect(0, 100, 320, 20, BLACK);
                M5.Lcd.setCursor(0, 100);
                M5.Lcd.printf("Captured: %d signals", signalCount);
                Serial.printf("Captured signal: %lu\n", sig.value);
            }
        }
        
        if (M5.BtnB.wasPressed()) break;
        if (M5.BtnA.wasPressed()) {
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setCursor(0, 0);
            M5.Lcd.println("Workflow aborted");
            delay(2000);
            return;
        }
        
        delay(10);
    }
    
    // Phase 3: ANALYZING
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: ANALYZING");
    M5.Lcd.println("");
    M5.Lcd.printf("Analyzing %d signals...\n", signalCount);
    delay(2000);
    
    // Phase 4: READY
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: READY");
    M5.Lcd.println("");
    M5.Lcd.println("Analysis complete!");
    M5.Lcd.printf("Total signals: %d\n", signalCount);
    M5.Lcd.println("");
    M5.Lcd.println("Options:");
    M5.Lcd.println("A: Exit");
    M5.Lcd.println("B: Transmit (if signals)");
    M5.Lcd.println("C: Continue listening");
    
    while (true) {
        M5.update();
        
        if (M5.BtnA.wasPressed()) {
            Serial.println("User chose to exit");
            break;
        }
        
        if (M5.BtnB.wasPressed() && signalCount > 0) {
            Serial.println("User requested transmission");
            executeGatedTransmission(signalCount);
            break;
        }
        
        if (M5.BtnC.wasPressed()) {
            Serial.println("User chose to continue observation");
            // Would loop back to LISTENING
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setCursor(0, 0);
            M5.Lcd.println("Continuing observation...");
            delay(2000);
            break;
        }
        
        delay(10);
    }
    
    // Phase 5: CLEANUP
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: CLEANUP");
    M5.Lcd.println("Cleaning up resources...");
    delay(1000);
    
    M5.Lcd.println("Workflow complete!");
    delay(2000);
}

void displayGateFailure(const char* gateName, const char* reason, const char* details) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.printf("%s: FAILED\n", gateName);
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.printf("Reason: %s\n", reason);
    M5.Lcd.println("");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("Details:");
    M5.Lcd.println(details);
    M5.Lcd.println("");
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("This demonstrates why");
    M5.Lcd.println("transmissions fail.");
    M5.Lcd.println("");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("Press any button...");
    
    // Wait for button press
    while (true) {
        M5.update();
        if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
            break;
        }
        delay(10);
    }
}

void executeGatedTransmission(int signalCount) {
    // Phase: TX_GATED
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("State: TX_GATED");
    M5.Lcd.println("");
    M5.Lcd.println("Multi-stage approval:");
    M5.Lcd.println("");
    
    // Simulate different gate scenarios based on signal count
    // This helps users learn about different failure modes
    int scenario = signalCount % SCENARIO_COUNT; // Rotate through scenarios
    
    // Gate 1: Policy Check
    M5.Lcd.println("Gate 1: Policy check...");
    delay(500);
    
    // Scenario 1: Policy failure - blacklisted frequency
    if (scenario == 1) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("  FAILED");
        M5.Lcd.setTextColor(WHITE);
        delay(1000);
        
        displayGateFailure(
            "Gate 1: Policy",
            "Blacklisted Frequency",
            "Signal frequency 433.05 MHz\nis on the restricted list.\n\n"
            "Emergency services and\naviation bands are blocked\nfor safety."
        );
        return;
    }
    
    // Scenario 2: Policy failure - duration too long
    if (scenario == 2) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("  FAILED");
        M5.Lcd.setTextColor(WHITE);
        delay(1000);
        
        displayGateFailure(
            "Gate 1: Policy",
            "Duration Limit Exceeded",
            "Estimated TX duration: 8.2s\nMaximum allowed: 5.0s\n\n"
            "This prevents excessive\nairtime usage."
        );
        return;
    }
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("  PASSED");
    M5.Lcd.setTextColor(WHITE);
    delay(500);
    
    // Gate 2: User Confirmation
    M5.Lcd.println("Gate 2: User confirm...");
    M5.Lcd.println("");
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("WARNING: Transmit?");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.println("B:Confirm  A:Cancel");
    
    uint32_t confirmStart = millis();
    bool confirmed = false;
    
    while (millis() - confirmStart < 10000) {
        M5.update();
        
        if (M5.BtnB.wasPressed()) {
            confirmed = true;
            break;
        }
        
        if (M5.BtnA.wasPressed()) {
            displayGateFailure(
                "Gate 2: User Confirm",
                "User Canceled",
                "Transmission was explicitly\ncanceled by the operator.\n\n"
                "Human oversight is a key\nsafety feature."
            );
            return;
        }
        
        delay(10);
    }
    
    if (!confirmed) {
        displayGateFailure(
            "Gate 2: User Confirm",
            "Confirmation Timeout",
            "No response within 10s.\n\n"
            "User confirmation is required\nfor all transmissions.\n\n"
            "Timeout prevents accidental\nor unattended TX."
        );
        return;
    }
    
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: TX_GATED");
    M5.Lcd.println("");
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Gate 2: PASSED");
    M5.Lcd.setTextColor(WHITE);
    delay(500);
    
    // Gate 3: Rate Limit
    M5.Lcd.println("Gate 3: Rate limit...");
    delay(500);
    
    // Scenario 3: Rate limit exceeded
    if (scenario == 3) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("  FAILED");
        M5.Lcd.setTextColor(WHITE);
        delay(1000);
        
        displayGateFailure(
            "Gate 3: Rate Limit",
            "Too Many Transmissions",
            "Rate: 12 TX in last 60s\nLimit: 10 TX per 60s\n\n"
            "Rate limiting prevents\nexcessive RF usage and\nensures fair spectrum access."
        );
        return;
    }
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("  PASSED");
    M5.Lcd.setTextColor(WHITE);
    delay(500);
    
    // Gate 4: Band-Specific
    M5.Lcd.println("Gate 4: 433MHz check...");
    delay(500);
    
    // Scenario 4: Band-specific validation failure
    if (scenario == 4) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.println("  FAILED");
        M5.Lcd.setTextColor(WHITE);
        delay(1000);
        
        displayGateFailure(
            "Gate 4: Band Check",
            "Invalid Pulse Timing",
            "Pulse #23: 45us (too short)\nValid range: 100-10000us\n\n"
            "Malformed signals can cause\ninterference or device damage."
        );
        return;
    }
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("  PASSED");
    M5.Lcd.setTextColor(WHITE);
    delay(500);
    
    M5.Lcd.println("");
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("ALL GATES PASSED");
    M5.Lcd.setTextColor(WHITE);
    delay(1000);
    
    // Phase: TRANSMIT
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("State: TRANSMIT");
    M5.Lcd.println("");
    M5.Lcd.println("Transmitting...");
    
    // Note: Actual transmission would happen here
    // This is just a simulation
    delay(1000);
    
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Transmission complete!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("All gates passed!");
    M5.Lcd.println("This shows a successful");
    M5.Lcd.println("multi-stage approval.");
    M5.Lcd.setTextColor(WHITE);
    delay(3000);
}
