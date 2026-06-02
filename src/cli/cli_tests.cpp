/**
 * CLI Tests - Implementation
 */

#include "cli_tests.h"
#include "../config/config.h"
#include "../config/dosing_types.h"
#include "../config/fram_layout.h"
#include "../hardware/relay_controller.h"
#include "../hardware/fram_controller.h"
#include "../hardware/rtc_controller.h"
#include "../algorithm/channel_manager.h"
#include "../hardware/dosing_scheduler.h"
#include <Wire.h>

// External references from main
extern RelayController relayController;
extern FramController framController;
extern RtcController rtcController;
extern ChannelManager channelManager;
extern DosingScheduler dosingScheduler;

// ============================================================================
// TIMED PUMP TEST
// ============================================================================
void testTimedPump(uint8_t channel, uint32_t duration_ms) {
    Serial.printf("[TEST] Running CH%d for %lu ms\n", channel, duration_ms);

    RelayResult res = relayController.turnOn(channel, duration_ms);
    if (res != RelayResult::OK) {
        Serial.printf("[TEST] Failed to start: %s\n",
                      RelayController::resultToString(res));
        return;
    }

    // Wait for completion
    while (relayController.isChannelOn(channel)) {
        relayController.update();

        // Print progress every 500ms
        static uint32_t lastPrint = 0;
        if (millis() - lastPrint > 500) {
            lastPrint = millis();
            Serial.printf("[TEST] CH%d: %lu ms / %lu ms\n",
                          channel,
                          relayController.getActiveRuntime(),
                          duration_ms);
        }

        delay(50);
    }

    Serial.println(F("[TEST] Complete"));
}

// ============================================================================
// I2C SCAN
// ============================================================================
void i2cScan() {
    Serial.println(F("[I2C] Scanning bus..."));
    Serial.println(F("      ┌──────────────────────────────────────┐"));

    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            found++;
            Serial.printf("      │  0x%02X - ", addr);

            if (addr == FRAM_I2C_ADDRESS) {
                Serial.print(F("FRAM MB85RC256V ✓"));
            } else if (addr == RTC_I2C_ADDRESS) {
                Serial.print(F("RTC DS3231M ✓"));
            } else {
                Serial.print(F("Unknown device"));
            }
            Serial.println(F("             │"));
        }
    }

    Serial.println(F("      └──────────────────────────────────────┘"));
    Serial.printf("[I2C] Found %d device(s)\n\n", found);
}

// ============================================================================
// FRAM TESTS
// ============================================================================
void testFRAM() {
    Serial.println(F("\n[FRAM TEST] Starting FRAM tests...\n"));

    if (!framController.isReady()) {
        Serial.println(F("[FRAM TEST] FRAM not ready!"));
        return;
    }

    // Test 1: Read header
    Serial.println(F("--- Test 1: Read Header ---"));
    FramHeader header;
    if (framController.readHeader(&header)) {
        Serial.printf("  Magic:   0x%08X %s\n", header.magic,
                      header.magic == FRAM_MAGIC_NUMBER ? "(OK)" : "(INVALID)");
        Serial.printf("  Version: %d\n", header.layout_version);
        Serial.printf("  Channels: %d\n", header.channel_count);
    } else {
        Serial.println(F("  Failed to read header!"));
    }

    // Test 2: Read channel configs
    Serial.println(F("\n--- Test 2: Channel Configs ---"));
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        ChannelConfig cfg;
        if (framController.readActiveConfig(i, &cfg)) {
            Serial.printf("  CH%d: events=0x%06X days=0x%02X dose=%.1f rate=%.2f\n",
                          i, cfg.events_bitmask, cfg.days_bitmask,
                          cfg.daily_dose_ml, cfg.dosing_rate);
        }
    }

    // Test 3: Read system state
    Serial.println(F("\n--- Test 3: System State ---"));
    SystemState sys;
    if (framController.readSystemState(&sys)) {
        Serial.printf("  Enabled: %d\n", sys.system_enabled);
        Serial.printf("  Halted:  %d\n", sys.system_halted);
        Serial.printf("  Active:  %d\n", sys.active_channel);
        Serial.printf("  Boots:   %lu\n", sys.boot_count);
    }

    // Test 4: Write/Read test
    Serial.println(F("\n--- Test 4: Write/Read Test ---"));
    uint8_t testData[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78};
    uint8_t readBack[8] = {0};

    // Write to reserved area (safe for testing)
    uint16_t testAddr = FRAM_ADDR_RESERVED;

    if (framController.writeBytes(testAddr, testData, sizeof(testData))) {
        Serial.println(F("  Write OK"));

        if (framController.readBytes(testAddr, readBack, sizeof(readBack))) {
            bool match = memcmp(testData, readBack, sizeof(testData)) == 0;
            Serial.printf("  Read OK, data %s\n", match ? "MATCHES" : "MISMATCH!");

            if (!match) {
                Serial.print(F("  Expected: "));
                for (int i = 0; i < 8; i++) Serial.printf("%02X ", testData[i]);
                Serial.print(F("\n  Got:      "));
                for (int i = 0; i < 8; i++) Serial.printf("%02X ", readBack[i]);
                Serial.println();
            }
        }
    } else {
        Serial.println(F("  Write FAILED!"));
    }

    // Test 5: Dump header section
    Serial.println(F("\n--- Test 5: Header Dump ---"));
    framController.dumpSection(FRAM_ADDR_HEADER, 32);

    Serial.println(F("\n[FRAM TEST] Complete\n"));

    Serial.println(F("\n--- Container Volume CH0 ---"));
    framController.dumpSection(0x0730, 8);
}

// ============================================================================
// CHANNEL MANAGER TESTS
// ============================================================================
void testChannelManager() {
    Serial.println(F("\n[CH TEST] Channel Manager Test Menu"));
    Serial.println(F("  1 - Print all channels"));
    Serial.println(F("  2 - Print single channel detail"));
    Serial.println(F("  3 - Set test config on CH0"));
    Serial.println(F("  4 - Apply pending changes"));
    Serial.println(F("  5 - Simulate dosing event"));
    Serial.println(F("  6 - Reset daily states"));
    Serial.println(F("  0 - Exit"));

    while (true) {
        Serial.print(F("\n[CH TEST] > "));
        while (!Serial.available()) delay(10);
        char cmd = Serial.read();
        while (Serial.available()) Serial.read();
        Serial.println(cmd);

        switch (cmd) {
            case '1':
                channelManager.printAllChannels();
                break;

            case '2': {
                Serial.print(F("Channel (0-5): "));
                while (!Serial.available()) delay(10);
                uint8_t ch = Serial.read() - '0';
                while (Serial.available()) Serial.read();
                Serial.println(ch);

                if (ch < CHANNEL_COUNT) {
                    channelManager.printChannelInfo(ch);
                }
                break;
            }

            case '3': {
                Serial.println(F("\nSetting test config on CH0:"));
                Serial.println(F("  Events: hours 8,12,18 (0x00044100)"));
                Serial.println(F("  Days: Mon-Fri (0x1F)"));
                Serial.println(F("  Daily dose: 6.0 ml"));
                Serial.println(F("  Rate: 0.5 ml/s"));

                // Set events: bits 8, 12, 18
                uint32_t events = (1 << 8) | (1 << 12) | (1 << 18);
                channelManager.setEventsBitmask(0, events);

                // Set days: Mon-Fri (bits 0-4)
                channelManager.setDaysBitmask(0, 0x1F);

                // Set dose
                channelManager.setDailyDose(0, 6.0);

                // Set rate
                channelManager.setDosingRate(0, 0.5);

                // Enable
                channelManager.setEnabled(0, true);

                Serial.println(F("Done! Config saved as PENDING."));
                Serial.println(F("Use '4' to apply or check with '2'."));

                channelManager.printChannelInfo(0);
                break;
            }

            case '4': {
                Serial.println(F("\nApplying all pending changes..."));
                if (channelManager.applyAllPendingChanges()) {
                    Serial.println(F("Success!"));
                } else {
                    Serial.println(F("Failed!"));
                }
                channelManager.printAllChannels();
                break;
            }

            case '5': {
                Serial.print(F("Channel (0-5): "));
                while (!Serial.available()) delay(10);
                uint8_t ch = Serial.read() - '0';
                while (Serial.available()) Serial.read();
                Serial.println(ch);

                if (ch < CHANNEL_COUNT) {
                    const ChannelCalculated& calc = channelManager.getCalculated(ch);
                    float dose = calc.single_dose_ml;

                    Serial.printf("Simulating dose of %.2f ml on CH%d at hour 12\n", dose, ch);

                    if (channelManager.markEventCompleted(ch, 12, dose)) {
                        Serial.println(F("Event marked complete!"));
                        channelManager.printChannelInfo(ch);
                    } else {
                        Serial.println(F("Failed!"));
                    }
                }
                break;
            }

            case '6':
                Serial.println(F("\nResetting daily states..."));
                if (channelManager.resetDailyStates()) {
                    Serial.println(F("Success!"));
                } else {
                    Serial.println(F("Failed!"));
                }
                break;

            case '0':
                Serial.println(F("Exiting test menu"));
                return;

            default:
                Serial.println(F("Unknown option"));
                break;
        }
    }
}

// ============================================================================
// RTC TESTS
// ============================================================================
void testRTC() {
    Serial.println(F("\n[RTC TEST] RTC Test Menu"));
    Serial.println(F("  1 - Print current time"));
    Serial.println(F("  2 - Set time manually"));
    Serial.println(F("  3 - Test midnight detection"));
    Serial.println(F("  0 - Exit"));

    while (true) {
        Serial.print(F("\n[RTC TEST] > "));
        while (!Serial.available()) delay(10);
        char cmd = Serial.read();
        while (Serial.available()) Serial.read();
        Serial.println(cmd);

        switch (cmd) {
            case '1':
                rtcController.printTime();
                break;

            case '2': {
                Serial.println(F("\nSet time (UTC) - enter each value:"));

                Serial.print(F("Year (2025): "));
                while (!Serial.available()) delay(10);
                int year = Serial.parseInt();
                while (Serial.available()) Serial.read();
                Serial.println(year);

                Serial.print(F("Month (1-12): "));
                while (!Serial.available()) delay(10);
                int month = Serial.parseInt();
                while (Serial.available()) Serial.read();
                Serial.println(month);

                Serial.print(F("Day (1-31): "));
                while (!Serial.available()) delay(10);
                int day = Serial.parseInt();
                while (Serial.available()) Serial.read();
                Serial.println(day);

                Serial.print(F("Hour (0-23): "));
                while (!Serial.available()) delay(10);
                int hour = Serial.parseInt();
                while (Serial.available()) Serial.read();
                Serial.println(hour);

                Serial.print(F("Minute (0-59): "));
                while (!Serial.available()) delay(10);
                int minute = Serial.parseInt();
                while (Serial.available()) Serial.read();
                Serial.println(minute);

                TimeInfo t;
                t.year = year;
                t.month = month;
                t.day = day;
                t.hour = hour;
                t.minute = minute;
                t.second = 0;
                t.dayOfWeek = 0;

                Serial.printf("\nSetting: %04d-%02d-%02d %02d:%02d:00\n",
                              year, month, day, hour, minute);

                if (rtcController.setTime(t)) {
                    Serial.println(F("Time set OK!"));
                    rtcController.printTime();
                } else {
                    Serial.println(F("Failed!"));
                }
                break;
            }

            case '3': {
                Serial.println(F("\nMidnight detection:"));
                Serial.printf("  hasMidnightPassed(): %s\n",
                              rtcController.hasMidnightPassed() ? "YES" : "NO");
                break;
            }

            case '0':
                Serial.println(F("Exiting"));
                return;

            default:
                Serial.println(F("Unknown option"));
                break;
        }
    }
}

// ============================================================================
// SCHEDULER TESTS
// ============================================================================
void testScheduler() {
    Serial.println(F("\n[SCHED TEST] Scheduler Test Menu"));
    Serial.println(F("  1 - Print scheduler status"));
    Serial.println(F("  2 - Enable scheduler"));
    Serial.println(F("  3 - Disable scheduler"));
    Serial.println(F("  4 - Trigger manual dose"));
    Serial.println(F("  5 - Force daily reset"));
    Serial.println(F("  6 - Setup quick test (CH0, 1 event now)"));
    Serial.println(F("  0 - Exit"));

    while (true) {
        Serial.print(F("\n[SCHED TEST] > "));
        while (!Serial.available()) delay(10);
        char cmd = Serial.read();
        while (Serial.available()) Serial.read();
        Serial.println(cmd);

        switch (cmd) {
            case '1':
                dosingScheduler.printStatus();
                channelManager.printAllChannels();
                rtcController.printTime();
                break;

            case '2':
                dosingScheduler.setEnabled(true);
                Serial.println(F("Scheduler ENABLED"));
                break;

            case '3':
                dosingScheduler.setEnabled(false);
                Serial.println(F("Scheduler DISABLED"));
                break;

            case '4': {
                Serial.print(F("Channel (0-5): "));
                while (!Serial.available()) delay(10);
                uint8_t ch = Serial.read() - '0';
                while (Serial.available()) Serial.read();
                Serial.println(ch);

                if (ch < CHANNEL_COUNT) {
                    if (dosingScheduler.triggerManualDose(ch)) {
                        Serial.println(F("Manual dose started!"));

                        // Wait and show progress
                        SchedulerState state;
                        while ((state = dosingScheduler.getState()) == SchedulerState::DOSING ||
                               state == SchedulerState::WAITING_PUMP) {
                            dosingScheduler.update();
                            relayController.update();

                            if (relayController.isAnyOn()) {
                                Serial.printf("  Pump running: %lu ms\n",
                                              relayController.getActiveRuntime());
                            }
                            delay(100);  // Faster update for validation
                        }

                        if (dosingScheduler.getCurrentEvent().failed) {
                            Serial.println(F("Dose FAILED!"));
                        } else {
                            Serial.println(F("Dose complete!"));
                        }
                    } else {
                        Serial.println(F("Failed to start dose"));
                    }
                }
                break;
            }

            case '5':
                Serial.println(F("Forcing daily reset..."));
                dosingScheduler.forceDailyReset();
                Serial.println(F("Done!"));
                break;

            case '6': {
                Serial.println(F("\nQuick test setup for CH0:"));

                // Get current time
                TimeInfo now = rtcController.getTime();
                uint8_t nextHour = now.hour;
                if (now.minute > 50) {
                    nextHour = (now.hour + 1) % 24;
                }

                // Skip hour 0
                if (nextHour == 0) nextHour = 1;

                Serial.printf("  Setting event at hour %d\n", nextHour);
                Serial.printf("  Current day of week: %d\n", now.dayOfWeek);

                // Set single event at next hour
                uint32_t events = (1UL << nextHour);
                channelManager.setEventsBitmask(0, events);

                // Set current day only
                uint8_t days = (1 << now.dayOfWeek);
                channelManager.setDaysBitmask(0, days);

                // Set small dose for testing
                channelManager.setDailyDose(0, 2.0);  // 2ml
                channelManager.setDosingRate(0, 0.5); // 0.5 ml/s = 4 sec pump
                channelManager.setEnabled(0, true);

                // Apply immediately (bypass pending)
                channelManager.applyPendingChanges(0);

                Serial.println(F("\nConfig applied:"));
                channelManager.printChannelInfo(0);

                Serial.println(F("\nEnable scheduler with '2' to activate!"));
                break;
            }

            case '0':
                Serial.println(F("Exiting"));
                return;

            default:
                Serial.println(F("Unknown option"));
                break;
        }
    }
}

// ============================================================================
// GPIO TIMING MEASUREMENT
// ============================================================================
void measureGpioTiming(uint8_t channel) {
    if (channel >= CHANNEL_COUNT) {
        Serial.println(F("[MEASURE] Invalid channel"));
        return;
    }

    Serial.printf("\n[MEASURE] === Pump Timing Test CH%d ===\n", channel);
    Serial.printf("[MEASURE] Pump pin: GPIO%d\n", PUMPS_PINS[channel]);
    Serial.printf("[MEASURE] Initial pump state: %s\n",
                  digitalRead(PUMPS_PINS[channel]) ? "HIGH(ON)" : "LOW(OFF)");

    if (relayController.isAnyOn()) {
        Serial.println(F("[MEASURE] ERROR: Another pump running!"));
        return;
    }

    Serial.println(F("[MEASURE] Starting pump..."));
    uint32_t startTime = micros();
    RelayResult res = relayController.turnOn(channel, 5000);

    if (res != RelayResult::OK) {
        Serial.printf("[MEASURE] Failed to start: %s\n", RelayController::resultToString(res));
        return;
    }

    Serial.printf("[MEASURE] Pump ON, GPIO%d = %s\n",
                  PUMPS_PINS[channel], digitalRead(PUMPS_PINS[channel]) ? "HIGH" : "LOW");

    delay(2000);
    relayController.turnOff(channel);

    uint32_t duration = (micros() - startTime) / 1000;
    Serial.printf("[MEASURE] Done. Total time: %lu ms\n", duration);
    Serial.printf("[MEASURE] Final pump state: %s\n",
                  digitalRead(PUMPS_PINS[channel]) ? "HIGH(ON!)" : "LOW(OFF)");
    Serial.println();
}

// ============================================================================
// CONFIG STRUCT SIZES
// ============================================================================
void printChannelConfigSize() {
    Serial.println(F("[CONFIG] Structure sizes:"));
    Serial.println(F("+-----------------------------------------+"));
    Serial.printf ("|  ChannelConfig:       %3d bytes         |\n", sizeof(ChannelConfig));
    Serial.printf ("|  ChannelDailyState:   %3d bytes         |\n", sizeof(ChannelDailyState));
    Serial.printf ("|  SystemState:         %3d bytes         |\n", sizeof(SystemState));
    Serial.printf ("|  CriticalErrorState:  %3d bytes         |\n", sizeof(CriticalErrorState));
    Serial.printf ("|  FramHeader:          %3d bytes         |\n", sizeof(FramHeader));
    Serial.printf ("|  AuthData:            %3d bytes         |\n", sizeof(AuthData));
    Serial.println(F("+-----------------------------------------+"));
    Serial.printf ("|  PumpChannelState:    %3d bytes         |\n", sizeof(PumpChannelState));
    Serial.println(F("+-----------------------------------------+"));
    Serial.println();
}
