/**
 * CLI Commands - Implementation
 */

#include "cli_commands.h"
#include "cli_menu.h"
#include "cli_tests.h"
#include "../config/config.h"
#include "../config/dosing_types.h"
#include "../hardware/relay_controller.h"
#include "../hardware/fram_controller.h"
#include "../hardware/rtc_controller.h"
#include "../algorithm/channel_manager.h"
#include "../hardware/dosing_scheduler.h"
#include "../config/fram_layout.h"
#include <esp_system.h>

// External references from main
extern RelayController relayController;
extern FramController framController;
extern RtcController rtcController;
extern ChannelManager channelManager;
extern DosingScheduler dosingScheduler;
extern volatile bool systemHalted;

// ============================================================================
// SERIAL COMMAND PROCESSOR
// ============================================================================
void processSerialCommand() {
    char cmd = Serial.read();

    // Flush remaining input
    while (Serial.available()) Serial.read();

    Serial.println();

    switch (cmd) {
        // --- Relay commands ---
        case '0': case '1': case '2': case '3': case '4': case '5': {
            uint8_t ch = cmd - '0';
            if (ch < CHANNEL_COUNT) {
                if (relayController.isChannelOn(ch)) {
                    RelayResult res = relayController.turnOff(ch);
                    Serial.printf("[CMD] CH%d OFF -> %s\n", ch,
                                  RelayController::resultToString(res));
                } else {
                    RelayResult res = relayController.turnOn(ch, 3000); // 30s max
                    Serial.printf("[CMD] CH%d ON (30s max) -> %s\n", ch,
                                  RelayController::resultToString(res));
                }
            }
            break;
        }

        case 't':
        case 'T': {
            Serial.println(F("[CMD] Timed pump test"));
            Serial.print(F("      Enter channel (0-5): "));

            // Wait for input
            while (!Serial.available()) delay(10);
            char chChar = Serial.read();
            while (Serial.available()) Serial.read();

            uint8_t ch = chChar - '0';
            if (ch < CHANNEL_COUNT) {
                Serial.printf("%d\n", ch);
                testTimedPump(ch, 3000); // 3 second test
            } else {
                Serial.println(F("Invalid channel"));
            }
            break;
        }

        case 'a':
        case 'A':
            Serial.println(F("[CMD] Trying to turn ALL ON (mutex should block)"));
            for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
                RelayResult res = relayController.turnOn(i, 5000);
                Serial.printf("       CH%d -> %s\n", i,
                              RelayController::resultToString(res));
            }
            break;

        case 'o':
        case 'O':
            Serial.println(F("[CMD] All OFF"));
            relayController.allOff();
            break;

        case 'p':
        case 'P':
            relayController.printStatus();
            break;

        case 'f':
        case 'F':
            testFRAM();
            break;

        case 'r':
        case 'R': {
            Serial.println(F("[CMD] Factory reset FRAM? (y/n): "));
            while (!Serial.available()) delay(10);
            char confirm = Serial.read();
            while (Serial.available()) Serial.read();

            if (confirm == 'y' || confirm == 'Y') {
                Serial.println(F("[CMD] Resetting FRAM..."));
                if (framController.factoryReset()) {
                    Serial.println(F("[CMD] Factory reset complete"));
                } else {
                    Serial.println(F("[CMD] Factory reset FAILED!"));
                }
            } else {
                Serial.println(F("[CMD] Cancelled"));
            }
            break;
        }

        // --- System commands ---
        case 'i':
        case 'I':
            i2cScan();
            break;

        case 's':
        case 'S':
            printSystemInfo();
            break;

        case 'c':
        case 'C':
            printChannelConfigSize();
            break;

        case 'e':
        case 'E':
            systemHalted = !systemHalted;
            Serial.printf("[CMD] System halt: %s\n",
                          systemHalted ? "ENABLED (pumps blocked)" : "DISABLED");
            if (systemHalted) {
                relayController.allOff();
            }
            break;

        case 'h':
        case 'H':
        case '?':
            printMenu();
            break;

        case 'x':
        case 'X':
            Serial.println(F("[REBOOT] Restarting in 2 seconds..."));
            relayController.allOff();
            delay(2000);
            ESP.restart();
            break;

        case '\n':
        case '\r':
            break;

        case 'm':
        case 'M':
            testChannelManager();
            printMenu();
            break;

        case 'w':
        case 'W':
            testRTC();
            printMenu();
            break;

        case 'd':
        case 'D':
            testScheduler();
            printMenu();
            break;

        case 'y':
        case 'Y':
            Serial.println(F("[CMD] GPIO validation removed (ULN2003AN mode)"));
            break;

        case 'z':
        case 'Z': {
            Serial.println(F("[CMD] GPIO Timing Measurement"));
            Serial.print(F("      Enter channel (0-5): "));

            while (!Serial.available()) delay(10);
            char chChar = Serial.read();
            while (Serial.available()) Serial.read();

            uint8_t ch = chChar - '0';
            if (ch < CHANNEL_COUNT) {
                Serial.println(ch);
                measureGpioTiming(ch);
            } else {
                Serial.println(F("Invalid channel"));
            }
            break;
        }

        default:
            Serial.printf("[?] Unknown command: '%c' (0x%02X). Press 'h' for help.\n",
                          cmd, cmd);
            break;
    }
}
