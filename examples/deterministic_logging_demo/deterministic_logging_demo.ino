/**
 * @file deterministic_logging_demo.ino
 * @brief Demonstration of Deterministic State Transition Logging
 * 
 * This example demonstrates the deterministic logging feature which records:
 * - Every state entry and exit
 * - Transition causes
 * - User actions
 * - Timeouts and errors
 * - All in machine-readable format (JSON/CSV)
 * 
 * The deterministic logs enable:
 * - Replay of workflow execution
 * - Debugging and troubleshooting
 * - Compliance review and audit
 * - Test automation and validation
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

// Demo state
enum DemoPhase {
    DEMO_INTRO,
    DEMO_WORKFLOW,
    DEMO_LOGS,
    DEMO_EXPORT
};

DemoPhase currentPhase = DEMO_INTRO;

void setup() {
    // Initialize M5Stack
    M5.begin();
    M5.Axp.SetLcdVoltage(2800);
    
    Serial.begin(115200);
    Serial.println("\n\n=== Deterministic Logging Demo ===");
    
    // Initialize display
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Deterministic");
    M5.Lcd.println("Logging Demo");
    M5.Lcd.println("\nInitializing...");
    
    // Initialize safety module
    Safety.begin();
    
    // Initialize RF modules
    if (!rf433.begin()) {
        Serial.println("Failed to initialize RF433 module");
        M5.Lcd.println("RF433 FAILED!");
        delay(5000);
    }
    
    if (!rf24.begin()) {
        Serial.println("Failed to initialize RF24 module");
        M5.Lcd.println("RF24 FAILED!");
        delay(5000);
    }
    
    M5.Lcd.println("Ready!");
    delay(2000);
    
    showIntro();
}

void loop() {
    M5.update();
    
    switch (currentPhase) {
        case DEMO_INTRO:
            handleIntroPhase();
            break;
        case DEMO_WORKFLOW:
            handleWorkflowPhase();
            break;
        case DEMO_LOGS:
            handleLogsPhase();
            break;
        case DEMO_EXPORT:
            handleExportPhase();
            break;
    }
    
    delay(10);
}

void showIntro() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.println("Deterministic");
    M5.Lcd.println("Logging Demo");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("This demo shows:");
    M5.Lcd.println("- State entry/exit logs");
    M5.Lcd.println("- Transition causes");
    M5.Lcd.println("- User actions");
    M5.Lcd.println("- Machine-readable format");
    M5.Lcd.println("");
    M5.Lcd.println("Press B to start demo");
    M5.Lcd.println("Press C to skip to logs");
    
    Serial.println("\n=== Deterministic Logging Demo ===");
    Serial.println("This demo demonstrates structured state transition logging");
    Serial.println("Press B to start the workflow demo");
}

void handleIntroPhase() {
    if (M5.BtnB.wasPressed()) {
        currentPhase = DEMO_WORKFLOW;
        runDemoWorkflow();
    }
    
    if (M5.BtnC.wasPressed()) {
        currentPhase = DEMO_LOGS;
        showLogs();
    }
}

void runDemoWorkflow() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Running Workflow");
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Watch serial output for");
    M5.Lcd.println("deterministic logs!");
    M5.Lcd.println("");
    M5.Lcd.println("Format:");
    M5.Lcd.println("[DET_LOG] seq=N ts_ms=T");
    M5.Lcd.println("  type=TYPE state=STATE");
    M5.Lcd.println("  event=EVENT reason=REASON");
    
    Serial.println("\n=== Starting Demo Workflow ===");
    Serial.println("Watch for [DET_LOG] entries in structured format");
    
    delay(3000);
    
    // Configure workflow for 433 MHz
    WorkflowConfig config;
    config.band = BAND_433MHZ;
    config.listenMinTime = 3000;   // 3 seconds for demo
    config.listenMaxTime = 10000;  // 10 seconds for demo
    
    // Enable deterministic logging
    workflow.enableDeterministicLogging(true);
    
    // Initialize workflow
    if (!workflow.initialize(config, &rf433, &rf24)) {
        M5.Lcd.println("\nERROR: Init failed!");
        Serial.println("ERROR: Workflow initialization failed");
        delay(3000);
        currentPhase = DEMO_INTRO;
        showIntro();
        return;
    }
    
    Serial.println("\n--- Workflow initialized ---");
    
    // Run a simulated workflow sequence
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Workflow Progress:");
    M5.Lcd.println("==================");
    M5.Lcd.println("");
    
    // Simulate state transitions with user actions
    simulateWorkflowExecution();
    
    Serial.println("\n--- Workflow complete ---");
    Serial.printf("Generated %d deterministic log entries\n", workflow.getDeterministicLogCount());
    
    delay(2000);
    
    currentPhase = DEMO_LOGS;
    showLogs();
}

void simulateWorkflowExecution() {
    // This simulates a workflow execution with various events
    
    M5.Lcd.println("Phase: IDLE -> INIT");
    delay(1000);
    
    M5.Lcd.println("Phase: INIT -> LISTENING");
    delay(1000);
    
    M5.Lcd.println("Phase: LISTENING (passive)");
    M5.Lcd.println("  Capturing signals...");
    
    // Simulate some signal captures
    for (int i = 0; i < 3; i++) {
        delay(1000);
        M5.Lcd.printf("  Signal %d captured\n", i + 1);
    }
    
    // Simulate user triggering analysis
    M5.Lcd.println("\nUser: Trigger analysis");
    workflow.triggerAnalysis();
    delay(500);
    
    M5.Lcd.println("Phase: LISTENING -> ANALYZING");
    delay(1000);
    
    M5.Lcd.println("Phase: ANALYZING -> READY");
    delay(1000);
    
    // Simulate user selecting signal
    M5.Lcd.println("\nUser: Select signal 0");
    workflow.selectSignalForTransmission(0);
    delay(500);
    
    M5.Lcd.println("Phase: READY -> TX_GATED");
    delay(1000);
    
    // Simulate user confirming transmission
    M5.Lcd.println("\nUser: Confirm transmission");
    workflow.confirmTransmission();
    delay(500);
    
    M5.Lcd.println("\nPhase: TX_GATED -> TRANSMIT");
    delay(1000);
    
    M5.Lcd.println("Phase: TRANSMIT -> CLEANUP");
    delay(1000);
    
    M5.Lcd.println("Phase: CLEANUP -> IDLE");
    delay(1000);
    
    M5.Lcd.println("\nWorkflow complete!");
}

void handleWorkflowPhase() {
    // Workflow phase is handled by runDemoWorkflow()
    // This is just a placeholder
}

void showLogs() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Log Summary");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    
    int logCount = workflow.getDeterministicLogCount();
    M5.Lcd.printf("Total entries: %d\n", logCount);
    M5.Lcd.println("");
    M5.Lcd.println("Recent entries:");
    M5.Lcd.println("---------------");
    
    // Show last 5 entries
    int startIdx = max(0, logCount - 5);
    for (int i = startIdx; i < logCount; i++) {
        const DeterministicLogEntry* entry = workflow.getDeterministicLog(i);
        if (entry) {
            M5.Lcd.printf("%lu: %s\n", entry->sequenceNumber, entry->event);
        }
    }
    
    M5.Lcd.println("");
    M5.Lcd.println("B:Export  C:Next");
    
    Serial.println("\n=== Deterministic Log Summary ===");
    Serial.printf("Total log entries: %d\n", logCount);
    Serial.println("\nAll entries:");
    for (int i = 0; i < logCount; i++) {
        const DeterministicLogEntry* entry = workflow.getDeterministicLog(i);
        if (entry) {
            Serial.printf("  [%lu] %s: %s (%s)\n", 
                         entry->sequenceNumber,
                         entry->event,
                         entry->reason,
                         entry->data);
        }
    }
}

void handleLogsPhase() {
    if (M5.BtnB.wasPressed()) {
        currentPhase = DEMO_EXPORT;
        exportLogs();
    }
    
    if (M5.BtnC.wasPressed()) {
        currentPhase = DEMO_INTRO;
        workflow.clearLogs();
        showIntro();
    }
}

void exportLogs() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("Export Logs");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("");
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Exporting to Serial...");
    M5.Lcd.println("");
    
    Serial.println("\n=== EXPORT: JSON Format ===");
    String jsonExport = workflow.exportDeterministicLogsJSON();
    Serial.println(jsonExport);
    
    M5.Lcd.println("JSON export: Done");
    delay(1000);
    
    Serial.println("\n=== EXPORT: CSV Format ===");
    String csvExport = workflow.exportDeterministicLogsCSV();
    Serial.println(csvExport);
    
    M5.Lcd.println("CSV export: Done");
    M5.Lcd.println("");
    M5.Lcd.println("Check Serial output!");
    M5.Lcd.println("");
    M5.Lcd.println("These formats enable:");
    M5.Lcd.println("- Replay of execution");
    M5.Lcd.println("- Automated testing");
    M5.Lcd.println("- Compliance audit");
    M5.Lcd.println("- Debugging analysis");
    M5.Lcd.println("");
    M5.Lcd.println("Press any button...");
}

void handleExportPhase() {
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
        currentPhase = DEMO_INTRO;
        workflow.clearLogs();
        showIntro();
    }
}
