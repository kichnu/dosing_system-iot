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
    Serial.println(F("|                       DOZOWNIK - DEBUG CLI                            |"));
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println(F("|  POMPY:                                                               |"));
    Serial.println(F("|    0-7   Toggle pompa CH0-CH7 (mutex)                                 |"));
    Serial.println(F("|    t     Test pompy z timerem (3s)                                    |"));
    Serial.println(F("|    a     Wszystkie ON (mutex powinien blokować)                       |"));
    Serial.println(F("|    o     Wszystkie OFF (emergency stop)                               |"));
    Serial.println(F("|    p     Status pomp                                                  |"));
    Serial.println(F("|    z     Pomiar czasu GPIO (pump timing)                              |"));
    Serial.println(F("+-----------------------------------------------------------------------+"));
    Serial.println(F("|  SYSTEM:                                                              |"));
    Serial.println(F("|    i     Skan I2C                                                     |"));
    Serial.println(F("|    s     Informacje systemowe                                         |"));
    Serial.println(F("|    c     Rozmiary struktur konfiguracji                               |"));
    Serial.println(F("|    h     Ten ekran pomocy                                             |"));
    Serial.println(F("|    e     Toggle emergency halt                                        |"));
    Serial.println(F("|    x     Reboot                                                       |"));
    Serial.println(F("|    f     Test FRAM (odczyt sekcji)                                    |"));
    Serial.println(F("|    r     Factory reset FRAM                                           |"));
    Serial.println(F("|    m     Menu Channel Manager                                         |"));
    Serial.println(F("|    w     Menu RTC (czas/data)                                         |"));
    Serial.println(F("|    d     Menu schedulera dozowania                                    |"));
    Serial.println(F("|    n     Apply pending + reset stanów dziennych (= północ)            |"));
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
