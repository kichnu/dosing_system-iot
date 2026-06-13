/**
 * DOZOWNIK - System Configuration
 *
 * Główny plik konfiguracyjny systemu dozowania nawozów.
 * Zawiera stałe systemowe, pinout GPIO, limity i parametry czasowe.
 */

// ============================================================
// Waveshare ESP32-S3-Pico — Pin Assignment v2.0
// ============================================================
//
//  Top edge: GND ─ USB_D- ─ USB_D+ ─ GPIO 0 (BOOT strapping) ─ GPIO 3 (strapping) ─ GPIO 21 (FREE)
//
//                                                      ┌───────────────────   ──┐
//                                                      │       USB-C            │
//                                    FREE  GPIO 11 ────┤                        ├──── VBUS (5V)
//                                    FREE  GPIO 12 ────┤                        ├──── VSYS (5V)
//                                            GND   ────┤                        ├──── GND
//                                 PUMP_0   GPIO 13 ────┤  ULN2003AN             ├──── EN   (3V3_EN)
//                                 PUMP_1   GPIO 14 ────┤  ULN2003AN             ├──── 3V3  (OUT)
//                                 PUMP_2   GPIO 15 ────┤  ULN2003AN             ├──── GPIO 10  FREE
//                                 PUMP_3   GPIO 16 ────┤  ULN2003AN             ├──── GPIO  9  FREE
//                                            GND   ────┤                        ├──── GND
//                                 PUMP_4   GPIO 17 ────┤  ULN2003AN             ├──── GPIO  8  [RGB LED!]
//                                 PUMP_5   GPIO 18 ────┤  ULN2003AN             ├──── GPIO  7  I2C_SCL
//                                 PUMP_6   GPIO 33 ────┤  ULN2003AN             ├──── RUN  (hw reset)
//                                 PUMP_7   GPIO 34 ────┤  ULN2003AN             ├──── GPIO  6  I2C_SDA
//                                            GND   ────┤                        ├──── GND
//                         PUMP_MONITOR_RX  GPIO 35 ────┤  UART2 ← Edge Impulse  ├──── GPIO  5  BUZZER
//                         PUMP_MONITOR_TX  GPIO 36 ────┤  UART2 → Edge Impulse  ├──── GPIO  4  RESET_BUTTON
//                                     FREE GPIO 37 ────┤                        ├──── GPIO  2  FREE
//                             MASTER_RELAY GPIO 38 ────┤  safety cutoff         ├──── GPIO  1  FREE
//                                            GND   ────┤                        ├──── GND
//                              JTAG(MTCK)  GPIO 39 ────┤                        ├──── GPIO 41  JTAG(MTDI)
//                              JTAG(MTDO)  GPIO 40 ────┤                        ├──── GPIO 42  JTAG(MTMS)
//                                                      └───────────────   ──────┘
//
// Etykiety kanałów konfigurowane w GUI (domyślne):
//   CH0=Ca  CH1=kH  CH2=Jod  CH3=phyto  CH4=Food_1  CH5=Food_2  CH6=--  CH7=--
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================
#define DEVICE_ID           "DOZOWNIK"
#define FIRMWARE_VERSION    "4.0.0"
#define DEVICE_TYPE         "dosing_system"

// ============================================================================
// CHANNEL CONFIGURATION
// ============================================================================
#define CHANNEL_COUNT       8

// ============================================================================
// GPIO PINOUT - PUMP OUTPUTS (ULN2003AN — Active HIGH)
// ============================================================================
#define PUMP_PIN_CH0       13
#define PUMP_PIN_CH1       14
#define PUMP_PIN_CH2       15
#define PUMP_PIN_CH3       16
#define PUMP_PIN_CH4       17
#define PUMP_PIN_CH5       18
#define PUMP_PIN_CH6       33
#define PUMP_PIN_CH7       34

static const uint8_t PUMPS_PINS[CHANNEL_COUNT] = {
    PUMP_PIN_CH0, PUMP_PIN_CH1, PUMP_PIN_CH2, PUMP_PIN_CH3,
    PUMP_PIN_CH4, PUMP_PIN_CH5, PUMP_PIN_CH6, PUMP_PIN_CH7
};

// ============================================================================
// I2C CONFIGURATION (FRAM + RTC)
// ============================================================================
#define I2C_SDA_PIN         6
#define I2C_SCL_PIN         7
#define I2C_FREQUENCY       400000

#define FRAM_I2C_ADDRESS    0x50
#define RTC_I2C_ADDRESS     0x68

// ============================================================================
// PUMP MONITOR UART (Edge Impulse ESP32 — rezerwacja interfejsu)
// ============================================================================
#define PUMP_MONITOR_RX_PIN     35
#define PUMP_MONITOR_TX_PIN     36
#define PUMP_MONITOR_BAUD       115200

// ============================================================================
// TIMING CONSTANTS
// ============================================================================
// Harmonogram eventów — tylko parzyste godziny 02, 04, 06 ... 22
#define EVENTS_PER_DAY              11      // 11 parzystych godzin
#define FIRST_EVENT_HOUR            2       // Pierwsza godzina eventu
#define LAST_EVENT_HOUR             22      // Ostatnia godzina eventu
#define RESERVED_HOUR               0       // 00:xx zarezerwowane (daily reset)
#define EVENT_VALID_HOURS_MASK      0x00555554UL  // bity 2,4,6,...,22

// Offset kanałów od godziny eventu (minuty):
//   CH0=:00  CH1=:15  CH2=:30  CH3=:45
//   CH4=(+1h):00  CH5=(+1h):15  CH6=(+1h):30  CH7=(+1h):45
#define CHANNEL_OFFSET_MINUTES      15

#define EVENT_WINDOW_SECONDS        300
#define EVENT_CHECK_INTERVAL_MS     10000
#define WDT_TIMEOUT_SECONDS         30

// ============================================================================
// PUMP TIMING
// ============================================================================
#define MAX_PUMP_DURATION_SECONDS   180
#define MAX_PUMP_DURATION_MS        (MAX_PUMP_DURATION_SECONDS * 1000UL)

#define CALIBRATION_DURATION_SEC    30
#define CALIBRATION_DURATION_MS     (CALIBRATION_DURATION_SEC * 1000UL)

// ============================================================================
// DOSING LIMITS
// ============================================================================
#define MIN_SINGLE_DOSE_ML          0.1f
#define MAX_SINGLE_DOSE_ML          50.0f
#define MAX_DAILY_DOSE_ML           500.0f
#define MAX_WEEKLY_DOSE_ML          3500.0f
#define DEFAULT_DOSING_RATE         0.33f
#define MIN_DOSING_RATE             (1.0f / CALIBRATION_DURATION_SEC)  // 1 ml/30s ≈ 0.0333 ml/s
#define MAX_DOSING_RATE             5.0f

// ============================================================================
// CONTAINER VOLUME TRACKING
// ============================================================================
#define CONTAINER_MIN_ML            100
#define CONTAINER_MAX_ML            5000
#define CONTAINER_DEFAULT_ML        1000
#define LOW_VOLUME_THRESHOLD_PCT    10

// ============================================================================
// DAILY RESET
// ============================================================================
#define DAILY_RESET_HOUR            0
#define DAILY_RESET_MARGIN_SEC      60

// ============================================================================
// SESSION & SECURITY
// ============================================================================
#define SESSION_TIMEOUT_MS          1800000
#define MAX_LOGIN_ATTEMPTS          5
#define LOGIN_LOCKOUT_MS            300000
#define RATE_LIMIT_REQUESTS         100
#define RATE_LIMIT_WINDOW_MS        60000

extern const IPAddress TRUSTED_PROXY_IP;

// ============================================================================
// SERIAL DEBUG
// ============================================================================
#define SERIAL_BAUD_RATE            115200
#define DEBUG_ENABLED               true

// ============================================================================
// WIFI
// ============================================================================
#define WIFI_CONNECT_TIMEOUT_MS     25000
#define WIFI_RECONNECT_INTERVAL_MS  30000

// ============================================================================
// SYSTEM FLAGS
// ============================================================================
extern volatile bool systemHalted;
extern bool pumpGlobalEnabled;

// ============================================================================
// HELPER MACROS
// ============================================================================
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define MS_TO_SEC(ms)       ((ms) / 1000)
#define SEC_TO_MS(sec)      ((sec) * 1000UL)
#define MIN_TO_SEC(min)     ((min) * 60)
#define MIN_TO_MS(min)      ((min) * 60000UL)

#define BIT_SET(mask, bit)      ((mask) |= (1UL << (bit)))
#define BIT_CLEAR(mask, bit)    ((mask) &= ~(1UL << (bit)))
#define BIT_CHECK(mask, bit)    (((mask) >> (bit)) & 1)
#define BIT_TOGGLE(mask, bit)   ((mask) ^= (1UL << (bit)))

// ============================================================================
// NTP CONFIGURATION
// ============================================================================
#define NTP_SERVER_1            "216.239.35.0"
#define NTP_SERVER_2            "216.239.35.4"
#define NTP_SERVER_3            "162.159.200.1"

#define NTP_GMT_OFFSET_SEC      0
#define NTP_DAYLIGHT_OFFSET_SEC 0

#define NTP_SYNC_TIMEOUT_MS     20000
#define NTP_SYNC_RETRY_DELAY_MS 500
#define NTP_RESYNC_INTERVAL_MS  3600000
#define NTP_MIN_VALID_YEAR      2024

inline uint8_t popcount8(uint8_t n) {
    uint8_t count = 0;
    while (n) { count += n & 1; n >>= 1; }
    return count;
}

inline uint8_t popcount32(uint32_t n) {
    uint8_t count = 0;
    while (n) { count += n & 1; n >>= 1; }
    return count;
}

#define ENABLE_SERIAL_DEBUG     true
#define ENABLE_FULL_LOGGING     true

// ============================================================================
// SAFETY SYSTEM
// ============================================================================

// Master relay (opcjonalny external safety cutoff)
#define MASTER_RELAY_PIN              38
#define MASTER_RELAY_ACTIVE           HIGH
#define MASTER_RELAY_INACTIVE         LOW

// Buzzer
#define BUZZER_PIN                    5
#define BUZZER_ACTIVE                 HIGH
#define BUZZER_INACTIVE               LOW
#define BUZZER_ERROR_ON_MS            100
#define BUZZER_ERROR_OFF_MS           500

// Reset button
#define RESET_BUTTON_PIN              4
#define RESET_BUTTON_ACTIVE           LOW
#define RESET_BUTTON_HOLD_MS          5000

// ============================================================================
// INITIALIZATION STATUS
// ============================================================================

struct InitStatus {
    bool i2c_ok;
    bool fram_ok;
    bool rtc_ok;
    bool pumps_ok;
    bool wifi_ok;
    bool webserver_ok;
    bool channel_manager_ok;
    bool scheduler_ok;
    bool critical_ok;
    bool system_ready;

    bool isHardwareOk() const {
        return i2c_ok && fram_ok && rtc_ok && pumps_ok;
    }
    bool isApplicationOk() const {
        return channel_manager_ok && scheduler_ok;
    }
    void reset() {
        i2c_ok = false; fram_ok = false; rtc_ok = false; pumps_ok = false;
        wifi_ok = false; webserver_ok = false;
        channel_manager_ok = false; scheduler_ok = false;
        critical_ok = false; system_ready = false;
    }
};

extern InitStatus initStatus;

#endif // CONFIG_H
