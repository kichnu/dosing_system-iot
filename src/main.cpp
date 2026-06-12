/**
 * DOZOWNIK - Main Entry Point
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "dosing_types.h"
#include "fram_layout.h"
#include "relay_controller.h"
#include "fram_controller.h"
#include "algorithm/channel_manager.h"
#include "rtc_controller.h"
#include "dosing_scheduler.h"
#include "esp_system.h"

#include "web_server.h"
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "provisioning/prov_detector.h"
#include "provisioning/ap_core.h"
#include "provisioning/ap_server.h"
#include "config/credentials_manager.h"
#include "hardware/safety_manager.h"
#include "security/session_manager.h"
#include "security/rate_limiter.h"

// CLI modules (debug only)
#if ENABLE_CLI
#include "cli/cli_menu.h"
#include "cli/cli_commands.h"
#include "cli/cli_tests.h"
#endif

// ============================================================================
// GLOBAL STATE
// ============================================================================
volatile bool systemHalted = false;  // volatile: accessed from multiple contexts (main loop, web handlers, SafetyManager)
bool pumpGlobalEnabled = true;
InitStatus initStatus;

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

/**
 * Inicjalizacja hardware: I2C, FRAM, RTC, Relay
 */
void initHardware() {
    Serial.println(F("\n[INIT] === HARDWARE INIT ==="));
    
    // --- I2C ---
    Serial.print(F("[INIT] I2C... "));
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
    initStatus.i2c_ok = true;  // Wire.begin() nie zwraca błędu
    Serial.printf("OK (SDA=%d, SCL=%d)\n", I2C_SDA_PIN, I2C_SCL_PIN);
    
    // --- FRAM ---
    Serial.print(F("[INIT] FRAM... "));
    if (framController.begin()) {
        initStatus.fram_ok = true;
        Serial.println(F("OK"));
    } else {
        initStatus.fram_ok = false;
        Serial.println(F("FAILED!"));
    }
    
        // --- Credentials ---
    Serial.print(F("[INIT] Credentials... "));
    if (initStatus.fram_ok) {
        if (initCredentialsManager()) {
            Serial.println(F("OK (from FRAM)"));
        } else {
            Serial.println(F("using fallback"));
        }
    } else {
        Serial.println(F("SKIPPED (no FRAM)"));
    }
// --- RTC ---
    Serial.print(F("[INIT] RTC... "));
    if (rtcController.begin()) {
        delay(50);  // Daj RTC chwilę na stabilizację
        if (rtcController.isTimeValid()) {
            initStatus.rtc_ok = true;
            TimeInfo now = rtcController.getTime();
            Serial.printf("OK (%04d-%02d-%02d %02d:%02d)\n", 
                          now.year, now.month, now.day, now.hour, now.minute);
        } else {
            initStatus.rtc_ok = false;
            Serial.println(F("INVALID TIME!"));
        }
    } else {
        initStatus.rtc_ok = false;
        Serial.println(F("FAILED!"));
    }
    
    // --- Pump Controller (ULN2003AN) ---
    Serial.print(F("[INIT] Pumps (ULN2003AN)... "));
    relayController.begin();
    initStatus.pumps_ok = true;
    Serial.println(F("OK"));
    
    // // --- GPIO Validator ---
    // Serial.print(F("[INIT] GPIO Validator... "));
    // gpioValidator.begin();
    // Serial.println(F("OK"));
    
    // --- Hardware summary ---
    initStatus.critical_ok = initStatus.fram_ok && initStatus.rtc_ok;
    
    if (initStatus.isHardwareOk()) {
        Serial.println(F("[INIT] Hardware: ALL OK"));
    } else {
        Serial.println(F("[INIT] Hardware: ERRORS DETECTED!"));
        if (!initStatus.fram_ok) Serial.println(F("  - FRAM: CRITICAL!"));
        if (!initStatus.rtc_ok)  Serial.println(F("  - RTC: CRITICAL!"));
    }
}

/**
 * Inicjalizacja sieci: WiFi, WebServer
 */
void initNetwork() {
    Serial.println(F("\n[INIT] === NETWORK INIT ==="));
    
    // --- WiFi ---
    Serial.print(F("[INIT] WiFi... "));
    
    // TODO: Docelowo credentials z FRAM/Captive Portal
    // WiFi.begin("KiG_2.4_IOT", "*qY4I@5&*%0lK1Q$U6UV7^S");

    // Use credentials from FRAM or fallback
    const char* ssid = getWiFiSSID();
    const char* password = getWiFiPassword();
    
    Serial.printf("connecting to %s (%s)... ", 
                  ssid, 
                  areCredentialsLoaded() ? "FRAM" : "fallback");

    // WiFi event handler for disconnect/reconnect
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            Serial.println(F("[WIFI] Disconnected, attempting reconnect..."));
            WiFi.reconnect();
        } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
            Serial.println(F("[WIFI] Reconnected!"));
        }
    });
    
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid, password);
    
    uint32_t wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiStart > WIFI_CONNECT_TIMEOUT_MS) {
            break;
        }
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        initStatus.wifi_ok = true;
        Serial.printf(" OK (%s)\n", WiFi.localIP().toString().c_str());
    } else {
        initStatus.wifi_ok = false;
        Serial.println(F(" FAILED!"));
    }
    
    // --- WebServer (only if WiFi OK) ---
    if (initStatus.wifi_ok) {
        Serial.print(F("[INIT] WebServer... "));
        initWebServer();
        initStatus.webserver_ok = true;
        Serial.println(F("OK"));
    } else {
        initStatus.webserver_ok = false;
        Serial.println(F("[INIT] WebServer: SKIPPED (no WiFi)"));
    }
    
    // --- Network summary ---
    if (initStatus.wifi_ok && initStatus.webserver_ok) {
        Serial.println(F("[INIT] Network: ALL OK"));
        Serial.printf("[INIT] Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(F("[INIT] Network: DEGRADED MODE"));
        Serial.println(F("  - Dosing will work, but no remote access"));
    }
}

/**
 * Inicjalizacja aplikacji: ChannelManager, Scheduler
 */
void initApplication() {
    Serial.println(F("\n[INIT] === APPLICATION INIT ==="));
    
    // --- Channel Manager (wymaga FRAM) ---
    Serial.print(F("[INIT] Channel Manager... "));
    if (initStatus.fram_ok) {
        if (channelManager.begin()) {
            initStatus.channel_manager_ok = true;
            Serial.println(F("OK"));
        } else {
            initStatus.channel_manager_ok = false;
            Serial.println(F("FAILED!"));
        }
    } else {
        initStatus.channel_manager_ok = false;
        Serial.println(F("SKIPPED (no FRAM)"));
    }
    
    // --- Dosing Scheduler (wymaga RTC + ChannelManager) ---
    Serial.print(F("[INIT] Scheduler... "));
    if (initStatus.rtc_ok && initStatus.channel_manager_ok) {
        if (dosingScheduler.begin()) {
            initStatus.scheduler_ok = true;
            Serial.println(F("OK"));
        } else {
            initStatus.scheduler_ok = false;
            Serial.println(F("FAILED!"));
        }
    } else {
        initStatus.scheduler_ok = false;
        Serial.println(F("SKIPPED (missing dependencies)"));
        if (!initStatus.rtc_ok) Serial.println(F("  - Requires: RTC"));
        if (!initStatus.channel_manager_ok) Serial.println(F("  - Requires: Channel Manager"));
    }

    // --- NTP Sync (wymaga WiFi + RTC) ---
    if (initStatus.wifi_ok && initStatus.rtc_ok) {
        Serial.print(F("[INIT] NTP Sync... "));
        if (rtcController.syncNTPWithRetry()) {
                // Synchronizuj stan schedulera z nowym czasem
        dosingScheduler.syncTimeState();
            Serial.println(F("OK"));
        } else {
            Serial.println(F("FAILED (will retry later)"));
        }
    } else {
        Serial.println(F("[INIT] NTP Sync... SKIPPED (missing WiFi or RTC)"));
    }
    
    // --- Application summary ---
    if (initStatus.isApplicationOk()) {
        Serial.println(F("[INIT] Application: ALL OK"));
    } else {
        Serial.println(F("[INIT] Application: ERRORS!"));
        Serial.println(F("  - Dosing may not work correctly"));
    }
    
    // --- Final system status ---
    initStatus.system_ready = initStatus.isHardwareOk() && initStatus.isApplicationOk();
    
    Serial.println(F("\n[INIT] =============================="));
    if (initStatus.system_ready) {
        Serial.println(F("[INIT] SYSTEM READY"));
    } else {
        Serial.println(F("[INIT] SYSTEM DEGRADED"));
        systemHalted = !initStatus.critical_ok;
        if (systemHalted) {
            Serial.println(F("[INIT] CRITICAL ERROR - SYSTEM HALTED!"));
        }
    }
    Serial.println(F("[INIT] ==============================\n"));
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_BAUD_RATE);

    // Wait for Serial (USB CDC)
    uint32_t startWait = millis();
    while (!Serial && (millis() - startWait < 3000)) {
        delay(10);
    }
    delay(500);

     // === CHECK PROVISIONING BUTTON (before any other init) ===
    if (checkProvisioningButton()) {
        Serial.println();
        Serial.println(F("+=======================================+"));
        Serial.println(F("|     ENTERING PROVISIONING MODE        |"));
        Serial.println(F("+=======================================+"));
        
        // Minimal init for provisioning
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        delay(100);
        framController.begin();
        
        // Start AP + Web Server
        if (startAccessPoint() && startDNSServer() && startWebServer()) {
            Serial.println(F("[PROV] Configuration portal ready!"));
            Serial.println(F("[PROV] Connect to WiFi: DOZOWNIK-SETUP"));
            Serial.println(F("[PROV] Password: setup12345"));
            Serial.println(F("[PROV] Open: http://192.168.4.1"));
            
            // Blocking loop - never returns
            runProvisioningLoop();
        } else {
            Serial.println(F("[PROV] Failed to start provisioning!"));
            Serial.println(F("[PROV] Rebooting in 5 seconds..."));
            delay(5000);
            ESP.restart();
        }
    }

    // Log restart reason
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[BOOT] Reset reason: %d ", reason);
    switch(reason) {
        case ESP_RST_POWERON: Serial.println(F("(Power-on)")); break;
        case ESP_RST_SW: Serial.println(F("(Software)")); break;
        case ESP_RST_PANIC: Serial.println(F("(Panic/Exception)")); break;
        case ESP_RST_INT_WDT: Serial.println(F("(Interrupt WDT)")); break;
        case ESP_RST_TASK_WDT: Serial.println(F("(Task WDT)")); break;
        case ESP_RST_WDT: Serial.println(F("(Other WDT)")); break;
        case ESP_RST_BROWNOUT: Serial.println(F("(Brownout)")); break;
        default: Serial.println(F("(Unknown)")); break;
    }

    // Reset init status
    initStatus.reset();
    
    #if ENABLE_CLI
    printBanner();
    #endif

    Serial.println(F("\n"));
    Serial.println(F("╔══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║              DOZOWNIK v2.0 - Starting...                 ║"));
    Serial.println(F("╚══════════════════════════════════════════════════════════╝"));
    
    // === INITIALIZATION SEQUENCE ===
    initHardware();
    initNetwork();
    initApplication();

    safetyManager.begin();

    if (!safetyManager.enableIfSafe()) {
        // Błąd krytyczny aktywny - system zablokowany
        // Wejdź w pętlę oczekiwania na reset
        Serial.println(F("[MAIN] System locked due to critical error"));
        Serial.println(F("[MAIN] Only reset button will be handled"));
        
        while (safetyManager.isCriticalErrorActive()) {
            safetyManager.update();  // Obsługa buzzera i przycisku
            delay(10);
        }
        
        // Po resecie - restart systemu
        Serial.println(F("[MAIN] Error cleared - restarting..."));
        delay(1000);
        ESP.restart();
    }

    // === POST-INIT ===
    #if ENABLE_CLI
    i2cScan();
    printMenu();
    #endif

// === WATCHDOG TIMER (production only) ===
    #if !ENABLE_CLI
    Serial.print(F("[INIT] Watchdog Timer... "));
    if (esp_task_wdt_add(NULL) == ESP_OK) {
        Serial.println(F("OK (subscribed to default WDT)"));
    } else {
        Serial.println(F("SKIPPED"));
    }
    #else
    Serial.println(F("[INIT] Watchdog Timer... DISABLED (debug mode)"));
    #endif
    
    // Final status
    if (initStatus.system_ready) {
        Serial.println(F("[MAIN] Entering main loop..."));
    } else if (systemHalted) {
        Serial.println(F("[MAIN] System halted - check errors above"));
    } else {
        Serial.println(F("[MAIN] Running in degraded mode"));
    }

    // === TEST: Zapisz testową wartość do Container Volume ===
Serial.println(F("\n[TEST] Writing test data to Container Volume..."));
ContainerVolume testVol;
testVol.setContainerMl(1000.0f);  // 1000ml pojemność
testVol.setRemainingMl(750.0f);   // 750ml pozostało
testVol.crc32 = framController.calculateCRC32(&testVol, sizeof(testVol) - sizeof(testVol.crc32));

if (framController.writeContainerVolume(0, &testVol)) {
    Serial.printf("[TEST] Written to CH0: container=%.1fml, remaining=%.1fml\n",
                  testVol.getContainerMl(), testVol.getRemainingMl());
    
    // Odczytaj z powrotem
    ContainerVolume readBack;
    if (framController.readContainerVolume(0, &readBack)) {
        Serial.printf("[TEST] Read back: container=%.1fml, remaining=%.1fml\n",
                      readBack.getContainerMl(), readBack.getRemainingMl());
        
        if (readBack.getContainerMl() == 1000.0f && readBack.getRemainingMl() == 750.0f) {
            Serial.println(F("[TEST] ✓ Container Volume: PASS"));
        } else {
            Serial.println(F("[TEST] ✗ Container Volume: FAIL (corrupted!)"));
        }
    }
} else {
    Serial.println(F("[TEST] ✗ Failed to write Container Volume!"));
}
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    // === CRITICAL: Always update relay (safety) ===
    safetyManager.update();
    relayController.update();

    if (safetyManager.isCriticalErrorActive()) {
        // Zatrzymaj wszelkie operacje
        // (relay_controller powinien już być zatrzymany przez triggerCriticalError)
        return;
    }


 
    // === FEED WATCHDOG (if subscribed) ===
    static bool wdtSubscribed = false;
    static bool wdtChecked = false;
    if (!wdtChecked) {
        wdtSubscribed = (esp_task_wdt_status(NULL) == ESP_OK);
        wdtChecked = true;
    }
    if (wdtSubscribed) {
        // === FEED WATCHDOG (production only) ===
        #if !ENABLE_CLI
            esp_task_wdt_reset();
        #endif
    }
    
    // === Skip processing if system halted ===
    if (systemHalted) {
        static uint32_t lastHaltMsg = 0;
        if (millis() - lastHaltMsg > 30000) {  // Co 30s przypomnienie
            lastHaltMsg = millis();
            Serial.println(F("[MAIN] System HALTED - restart required"));
        }
        delay(100);
        return;
    }
    
    // === Normal operation ===
    
    // Update GPIO validator
        // Update GPIO validator
    // gpioValidator.update();
    
    // === NTP Resync (hourly) ===
    static uint32_t lastNtpCheck = 0;
    if (millis() - lastNtpCheck > 60000) {  // Check every minute
        lastNtpCheck = millis();

        if (initStatus.wifi_ok && initStatus.rtc_ok && rtcController.needsResync()) {
            Serial.println(F("[MAIN] NTP resync due..."));
            if (rtcController.syncNTPWithRetry()) {
                // CRITICAL: Sync scheduler state BEFORE update() to prevent
                // false daily reset detection due to time jump
                dosingScheduler.syncTimeState();
            }
        }
    }

    // === SESSION & RATE LIMITER CLEANUP (every 30s) ===
    static uint32_t lastSecurityCleanup = 0;
    if (millis() - lastSecurityCleanup > 30000) {
        lastSecurityCleanup = millis();
        updateSessionManager();
        updateRateLimiter();
    }

    // Update scheduler (main dosing logic)
    if (initStatus.scheduler_ok) {
        dosingScheduler.update();
    }
    
    // === CLI (debug only) ===
    #if ENABLE_CLI
    if (Serial.available()) {
        processSerialCommand();
    }
    
    // Show pump status when running
    static uint32_t lastStatusPrint = 0;
    if (relayController.isAnyOn() && (millis() - lastStatusPrint > 1000)) {
        lastStatusPrint = millis();
        uint8_t ch = relayController.getActiveChannel();
        uint32_t runtime = relayController.getActiveRuntime();
        uint32_t remaining = relayController.getRemainingTime();
        Serial.printf("[STATUS] CH%d running: %lu ms (remaining: %lu ms)\n",
                      ch, runtime, remaining);
    }
    #endif

    // === Heartbeat (production) ===
    #if !ENABLE_CLI
    static uint32_t lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 60000) {  // Co 1 minutę
        lastHeartbeat = millis();
        Serial.printf("[HEARTBEAT] Uptime: %lu min, Scheduler: %s\n",
                      millis() / 60000,
                      dosingScheduler.isEnabled() ? "ON" : "OFF");
    }
    #endif

    // === 24H AUTO-RESTART (ochrona przed fragmentacją pamięci) ===
    // Raz dziennie w oknie 00:15-00:45 UTC (po daily reset, przed eventami o 01:00)
    // Flaga statyczna zapobiega wielokrotnym próbom w tej samej godzinie 0
    {
        static bool autoRestartDone = false;

        if (initStatus.rtc_ok) {
            TimeInfo now = rtcController.getTime();

            // Reset flagi poza godziną 0 — gotowy na następne okno
            if (now.hour != 0) {
                autoRestartDone = false;
            }

            // Warunki restartu: okno czasowe + scheduler IDLE + pompy wyłączone
            if (!autoRestartDone &&
                now.hour == 0 && now.minute >= 15 && now.minute <= 45 &&
                dosingScheduler.getState() == SchedulerState::IDLE &&
                !relayController.isAnyOn() &&
                !safetyManager.isCriticalErrorActive())
            {
                autoRestartDone = true;
                Serial.println(F("[MAIN] === DAILY AUTO-RESTART ==="));
                Serial.printf("[MAIN] Uptime: %lu min, Time: %02d:%02d UTC\n",
                              millis() / 60000, now.hour, now.minute);
                Serial.println(F("[MAIN] Restarting for memory health..."));
                delay(500);
                ESP.restart();
            }
        }
    }

    delay(10);
}

// ============================================================================
// BANNER & MENU
// ============================================================================
