#include "hw_health.h"
#include <Wire.h>
#include <esp_attr.h>
#include <esp_system.h>
#include "../core/logging.h"
#include "config.h"
#include "fram_layout.h"
#include "fram_controller.h"
#include "rtc_controller.h"

#define HW_CHECK_INTERVAL_MS     60000UL   // periodic check
#define HW_CHECK_SOON_MS          2000UL   // after a failed transfer
#define HW_CHECK_ATTEMPTS            3     // one failed transfer may be a bus collision
#define HW_MAX_RECOVERY_RESTARTS     3     // per latch — no restart loop
#define HW_LATCH_MAGIC      0x48574C54UL   // "HWLT"

// Kept across software restarts, random after power-on (hence the magic)
struct HwLatch {
    uint32_t magic;
    uint8_t  mask;
    uint8_t  restarts;
};
static RTC_NOINIT_ATTR HwLatch g_latch;

static volatile uint8_t g_active    = 0;
static volatile bool    g_checkSoon = false;
static bool     g_ready          = false;
static uint32_t g_lastCheck      = 0;
static uint8_t  g_bootFaultMask  = 0;
static bool     g_recoveryWanted = false;
static uint8_t  g_scratch        = 0;

static bool ack(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

// Magic read + write/read-back of a scratch byte
static bool checkFram() {
    for (int i = 0; i < HW_CHECK_ATTEMPTS; i++) {
        if (i) delay(20);
        uint32_t magic = 0;
        uint8_t  v = ++g_scratch, back = ~v;
        if (framController.readBytes(FRAM_ADDR_HEADER, &magic, sizeof(magic)) &&
            magic == FRAM_MAGIC_NUMBER &&
            framController.writeBytes(FRAM_ADDR_HEALTH_SCRATCH, &v, 1) &&
            framController.readBytes(FRAM_ADDR_HEALTH_SCRATCH, &back, 1) && back == v) {
            return true;
        }
    }
    return false;
}

// DS3231 answering with a plausible time; a lost time is restored from NTP
static bool checkRtc() {
    if (!rtcController.isReady()) {
        if (!ack(RTC_I2C_ADDRESS)) return false;
        rtcController.begin();
    }
    TimeInfo t;
    for (int i = 0; i < HW_CHECK_ATTEMPTS; i++) {
        if (i) delay(20);
        if (rtcController.isTimeValid() && rtcController.readTime(&t)) return true;
    }
    if (!ack(RTC_I2C_ADDRESS)) return false;
    rtcController.recoverTimeFromNtp();
    return rtcController.isTimeValid() && rtcController.readTime(&t);
}

static const char* bitName(uint8_t bit) {
    return bit == HW_FAULT_FRAM ? "FRAM" : "RTC";
}

static void runCheck() {
    uint8_t active = 0;
    if (!checkFram()) active |= HW_FAULT_FRAM;
    if (!checkRtc())  active |= HW_FAULT_RTC;

    for (uint8_t bit : {HW_FAULT_FRAM, HW_FAULT_RTC}) {
        if ((active & bit) && !(g_active & bit)) {
            LOG_ERROR("CRITICAL: %s fault - alarm latched", bitName(bit));
        } else if (!(active & bit) && (g_active & bit)) {
            LOG_WARNING("%s works again - acknowledge to clear the alarm", bitName(bit));
        }
    }
    g_latch.mask |= active;
    g_active = active;

    if (g_bootFaultMask && active == 0) g_recoveryWanted = true;
}

void initHwHealth(uint8_t bootFaultMask) {
    if (esp_reset_reason() == ESP_RST_POWERON || g_latch.magic != HW_LATCH_MAGIC) {
        g_latch = HwLatch{HW_LATCH_MAGIC, 0, 0};
    } else if (g_latch.mask) {
        LOG_WARNING("Hardware alarm kept from before the restart (mask 0x%02X)", g_latch.mask);
    }
    g_bootFaultMask = bootFaultMask;
    g_latch.mask |= bootFaultMask;
    g_ready = true;
    runCheck();
    g_lastCheck = millis();
}

void updateHwHealth() {
    if (!g_ready) return;
    uint32_t now = millis();
    // Check soon only to catch a new fault — with one already active, failed
    // transfers come all the time and the retries would stall loop()
    bool soon = g_checkSoon && !g_active;
    if (now - g_lastCheck < (soon ? HW_CHECK_SOON_MS : HW_CHECK_INTERVAL_MS)) return;
    g_lastCheck = now;
    runCheck();
    g_checkSoon = false;   // after the check — its own failed transfers set it again
}

void noteFramIoError() { g_checkSoon = true; }
void noteRtcIoError()  { g_checkSoon = true; }

void latchHwFault(uint8_t faultBit) {
    LOG_ERROR("CRITICAL: %s fault - alarm latched", bitName(faultBit));
    g_latch.mask |= faultBit;
}

bool    isHwFaultAlarm()    { return g_ready && g_latch.mask != 0; }
uint8_t getHwFaultActive()  { return g_active; }
uint8_t getHwFaultLatched() { return g_ready ? g_latch.mask : 0; }

String getHwFaultText() {
    String s;
    if (!g_ready) return s;
    for (uint8_t bit : {HW_FAULT_FRAM, HW_FAULT_RTC}) {
        if (!(g_latch.mask & bit)) continue;
        if (s.length()) s += " | ";
        s += bitName(bit);
        if (g_active & bit) {
            s += (bit == HW_FAULT_FRAM) ? " not responding or read/write check failed"
                                        : " DS3231 not responding or no valid time";
        } else if (g_bootFaultMask & bit) {
            s += " works again, restart needed";
        } else {
            s += " failed earlier, works now";
        }
    }
    return s;
}

bool ackHwFaults() {
    if (!g_ready || !g_latch.mask) return false;
    // A block missing at boot stays latched: the application never started with it
    uint8_t keep = g_latch.mask & (g_active | g_bootFaultMask);
    if (keep) {
        LOG_WARNING("Hardware alarm stays - still failing (mask 0x%02X)", keep);
    } else {
        LOG_INFO("Hardware alarm acknowledged");
        g_latch.restarts = 0;
    }
    g_latch.mask = keep;
    return true;
}

bool isHwRecoveryRestartWanted() {
    return g_recoveryWanted && g_latch.restarts < HW_MAX_RECOVERY_RESTARTS;
}

void noteHwRecoveryRestart() {
    g_latch.restarts++;
    LOG_WARNING("Block missing at boot answers again - restart %u/%u",
                g_latch.restarts, HW_MAX_RECOVERY_RESTARTS);
}
