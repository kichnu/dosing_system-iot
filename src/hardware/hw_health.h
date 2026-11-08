#ifndef HW_HEALTH_H
#define HW_HEALTH_H

#include <Arduino.h>

// Critical hardware blocks (FRAM, DS3231 RTC). A failure at boot or during
// operation latches a critical alarm: dosing stops, the buzzer sounds and the
// alarm shows in the GUI and in /api/dosing-status and /api/health. It stays
// until the block works again and the user acknowledges it. The latch survives
// software restarts (midnight, OTA) and is cleared by a power-on.

enum HwFaultBit : uint8_t {
    HW_FAULT_FRAM = 0x01,
    HW_FAULT_RTC  = 0x02,
};

// End of setup(). bootFaultMask: blocks the application could not start with
void initHwHealth(uint8_t bootFaultMask);
void updateHwHealth();        // every loop() — periodic check
void noteFramIoError();       // failed FRAM transfer (any task) — check soon
void noteRtcIoError();        // failed DS3231 read — check soon
void latchHwFault(uint8_t faultBit);  // fault found outside the periodic check

bool    isHwFaultAlarm();     // latched — stops dosing, buzzer, GUI
uint8_t getHwFaultActive();   // HwFaultBit mask, failing now
uint8_t getHwFaultLatched();  // HwFaultBit mask, latched
String  getHwFaultText();
bool    ackHwFaults();        // clears what works again; true if anything was latched

// A block that failed at boot answers again — restart to initialize it (limited count)
bool isHwRecoveryRestartWanted();
void noteHwRecoveryRestart();

#endif // HW_HEALTH_H
