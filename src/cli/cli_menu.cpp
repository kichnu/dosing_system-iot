/**
 * CLI Menu - Implementation
 */

#include "cli_menu.h"
#include "../config/config.h"
#include <esp_system.h>

extern volatile bool systemHalted;

void printBanner() {
    Serial.println();
    Serial.println(F("+=======================================================================+"));
    Serial.println(F("|          DOZOWNIK - Automatic Fertilizer Dosing System                |"));
    Serial.println(F("|                    PHASE 2 - Controller Test                          |"));
    Serial.println(F("+=======================================================================+"));
    Serial.printf("|  Firmware: %-20s  Channels: %-14d  |\n", FIRMWARE_VERSION, CHANNEL_COUNT);
    Serial.printf("|  Device:   %-20s  Build: %s %-5s  |\n", DEVICE_ID, __DATE__, "");
    Serial.println(F("+=======================================================================+"));
    Serial.println();
}

void printMenu() {
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println(F("|                    PHASE 2 - CONTROLLER TEST                          |"));
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println(F("|  RELAY CONTROLLER:                                                    |"));
    Serial.println(F("|    0-5   Toggle relay CH0-CH5 (with mutex)                            |"));
    Serial.println(F("|    t     Timed pump test (3s on selected channel)                     |"));
    Serial.println(F("|    a     All relays ON (blocked by mutex!)                            |"));
    Serial.println(F("|    o     All relays OFF (emergency stop)                              |"));
    Serial.println(F("|    p     Print relay status                                           |"));
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println(F("|  GPIO VALIDATOR:                                                      |"));
    Serial.println(F("|    g     Read all GPIO states                                         |"));
    Serial.println(F("|    v     Run validation on active channel                             |"));
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println(F("|  SYSTEM:                                                              |"));
    Serial.println(F("|    i     I2C scan                                                     |"));
    Serial.println(F("|    s     System info                                                  |"));
    Serial.println(F("|    c     Config struct sizes                                          |"));
    Serial.println(F("|    h     This help menu                                               |"));
    Serial.println(F("|    e     Toggle emergency halt                                        |"));
    Serial.println(F("|    x     Reboot                                                       |"));
    Serial.println(F("|    f     FRAM test (read all sections)                                |"));
    Serial.println(F("|    r     Factory reset FRAM                                           |"));
    Serial.println(F("|    m     Channel Manager test menu                                    |"));
    Serial.println(F("|    w     RTC test menu (time/date)                                    |"));
    Serial.println(F("|    d     Dosing scheduler test menu                                   |"));
    Serial.println(F("|    y     Toggle GPIO validation                                       |"));
    Serial.println(F("|    z     Measure GPIO relay->validate timing                          |"));
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println();
}

void printSystemInfo() {
    Serial.println(F("[SYSTEM] Device Information:"));
    Serial.println(F("+----------------------------------------------------------+"));
    Serial.printf ("|  Device ID:       %-40s |\n", DEVICE_ID);
    Serial.printf ("|  Firmware:        %-40s |\n", FIRMWARE_VERSION);
    Serial.printf ("|  Channels:        %-40d |\n", CHANNEL_COUNT);
    Serial.printf ("|  System Halted:   %-40s |\n", systemHalted ? "YES" : "NO");
    Serial.printf ("|  Pump monitor: %-43s |\n", "UART2 (reserved, Edge Impulse)");
    Serial.println(F("+----------------------------------------------------------+"));
    Serial.printf ("|  Chip Model:      %-40s |\n", ESP.getChipModel());
    Serial.printf ("|  CPU Freq:        %-37d MHz |\n", ESP.getCpuFreqMHz());
    Serial.printf ("|  Free Heap:       %-37d B   |\n", ESP.getFreeHeap());
    Serial.printf ("|  Uptime:          %-34lu ms |\n", millis());
    Serial.println(F("+----------------------------------------------------------+"));
    Serial.println();
}
