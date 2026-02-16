/**
 * DOZOWNIK - System Configuration
 * 
 * Główny plik konfiguracyjny systemu dozowania nawozów.
 * Zawiera stałe systemowe, pinout GPIO, limity i parametry czasowe.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================
#define DEVICE_ID           "DOZOWNIK"
#define FIRMWARE_VERSION    "2.0.0"
#define DEVICE_TYPE         "dosing_system"

// ============================================================================
// CHANNEL CONFIGURATION
// Zmień CHANNEL_COUNT dla różnych wersji sprzętowych (1-6)
// ============================================================================
#define CHANNEL_COUNT       4

// ============================================================================
// GPIO PINOUT - RELAY OUTPUTS (Active HIGH)
// ============================================================================
#define RELAY_PIN_CH0       4
#define RELAY_PIN_CH1       5
#define RELAY_PIN_CH2       6
#define RELAY_PIN_CH3       7
#define RELAY_PIN_CH4       8
#define RELAY_PIN_CH5       9

// Tablica pinów relay (inicjalizowana w relay_controller.cpp)
// static const uint8_t RELAY_PINS[6] = {
//     RELAY_PIN_CH0, RELAY_PIN_CH1, RELAY_PIN_CH2,
//     RELAY_PIN_CH3, RELAY_PIN_CH4, RELAY_PIN_CH5
// };

static const uint8_t RELAY_PINS[4] = {
    RELAY_PIN_CH0, RELAY_PIN_CH1, RELAY_PIN_CH2,
    RELAY_PIN_CH3
};

// ============================================================================
// GPIO PINOUT - VALIDATION INPUTS (Pump feedback)
// ============================================================================
#define VALIDATE_PIN_CH0    13
#define VALIDATE_PIN_CH1    14
#define VALIDATE_PIN_CH2    15
#define VALIDATE_PIN_CH3    16
#define VALIDATE_PIN_CH4    17
#define VALIDATE_PIN_CH5    18

// static const uint8_t VALIDATE_PINS[6] = {
//     VALIDATE_PIN_CH0, VALIDATE_PIN_CH1, VALIDATE_PIN_CH2,
//     VALIDATE_PIN_CH3, VALIDATE_PIN_CH4, VALIDATE_PIN_CH5
// };

static const uint8_t VALIDATE_PINS[4] = {
    VALIDATE_PIN_CH0, VALIDATE_PIN_CH1, VALIDATE_PIN_CH2,
    VALIDATE_PIN_CH3
};

// ============================================================================
// I2C CONFIGURATION (FRAM + RTC)
// ============================================================================
#define I2C_SDA_PIN         11
#define I2C_SCL_PIN         12
#define I2C_FREQUENCY       400000  // 400 kHz

#define FRAM_I2C_ADDRESS    0x50
#define RTC_I2C_ADDRESS     0x68

// ============================================================================
// TIMING CONSTANTS
// ============================================================================
// Harmonogram eventów
#define EVENTS_PER_DAY              23      // Godziny 01:00-23:00
#define FIRST_EVENT_HOUR            1       // Pierwsza godzina eventów
#define LAST_EVENT_HOUR             23      // Ostatnia godzina eventów
#define RESERVED_HOUR               0       // 00:xx zarezerwowane 
#define WDT_TIMEOUT_SECONDS         30

// Offsety czasowe kanałów (w minutach)
#define CHANNEL_OFFSET_MINUTES      15      // CH0=:00, CH1=:10, CH2=:20...

// Okno czasowe eventu
#define EVENT_WINDOW_SECONDS        300     // 5 minut na wykonanie eventu
#define EVENT_CHECK_INTERVAL_MS     10000   // Sprawdzanie co 10 sekund

// ============================================================================
// PUMP TIMING
// ============================================================================
#define MAX_PUMP_DURATION_SECONDS   180     // Maksymalny czas pracy pompy (3 min)
#define MAX_PUMP_DURATION_MS        (MAX_PUMP_DURATION_SECONDS * 1000UL)

#define CALIBRATION_DURATION_SEC    30      // Czas kalibracji pompy
#define CALIBRATION_DURATION_MS     (CALIBRATION_DURATION_SEC * 1000UL)

// // ============================================================================
// // GPIO VALIDATION
// // ============================================================================

// #define GPIO_VALIDATION_DEFAULT     true    // Wartość startowa
// extern bool gpioValidationEnabled;  

// // #define GPIO_VALIDATION_ENABLED     false    // Globalne włączenie walidacji
// #define GPIO_CHECK_DELAY_MS         200    // Opóźnienie przed sprawdzeniem (2s)
// #define GPIO_DEBOUNCE_MS            100    // Czas debounce (1s)
// #define GPIO_EXPECTED_STATE         HIGH    // Oczekiwany stan przy działającej pompie

// ============================================================================
// DOSING LIMITS
// ============================================================================
#define MIN_SINGLE_DOSE_ML          1.0f    // Minimalna pojedyncza dawka
#define MAX_SINGLE_DOSE_ML          50.0f   // Maksymalna pojedyncza dawka
#define MAX_DAILY_DOSE_ML           500.0f  // Maksymalna dawka dzienna
#define MAX_WEEKLY_DOSE_ML          3500.0f // Maksymalna dawka tygodniowa

#define DEFAULT_DOSING_RATE         0.33f   // Domyślna wydajność pompy (ml/s)
#define MIN_DOSING_RATE             0.1f    // Minimalna wydajność
#define MAX_DOSING_RATE             5.0f    // Maksymalna wydajność

// ============================================================================
// CONTAINER VOLUME TRACKING
// ============================================================================
#define CONTAINER_MIN_ML            100     // Minimalna pojemność pojemnika (ml)
#define CONTAINER_MAX_ML            5000    // Maksymalna pojemność pojemnika (ml)
#define CONTAINER_DEFAULT_ML        1000    // Domyślna pojemność (ml)
#define LOW_VOLUME_THRESHOLD_PCT    10      // Próg ostrzeżenia o niskim stanie (%)

// ============================================================================
// CONTAINER VOLUME TRACKING
// ============================================================================
#define CONTAINER_MIN_ML            100     // Minimalna pojemność pojemnika (ml)
#define CONTAINER_MAX_ML            5000    // Maksymalna pojemność pojemnika (ml)
#define CONTAINER_DEFAULT_ML        1000    // Domyślna pojemność (ml)
#define LOW_VOLUME_THRESHOLD_PCT    10      // Próg ostrzeżenia o niskim stanie (%)

// ============================================================================
// DAILY RESET
// ============================================================================
#define DAILY_RESET_HOUR            0       // Godzina resetu (00:xx UTC)
#define DAILY_RESET_MARGIN_SEC      60      // Margines bezpieczeństwa

// ============================================================================
// SESSION & SECURITY (z bazowego projektu)
// ============================================================================
#define SESSION_TIMEOUT_MS          1800000 // 30 minut
#define MAX_LOGIN_ATTEMPTS          5
#define LOGIN_LOCKOUT_MS            300000  // 5 minut blokady
#define RATE_LIMIT_REQUESTS         100
#define RATE_LIMIT_WINDOW_MS        60000   // 1 minuta

// ================= TRUSTED PROXY (WireGuard VPS) =================
extern const IPAddress TRUSTED_PROXY_IP;

// ============================================================================
// SERIAL DEBUG
// ============================================================================
#define SERIAL_BAUD_RATE            115200
#define DEBUG_ENABLED               true

// ============================================================================
// WIFI (do nadpisania przez credentials_manager)
// ============================================================================
#define WIFI_CONNECT_TIMEOUT_MS     25000   // Timeout połączenia WiFi
#define WIFI_RECONNECT_INTERVAL_MS  30000   // Interwał reconnect

// ============================================================================
// SYSTEM FLAGS
// ============================================================================
extern volatile bool systemHalted;  // Flaga błędu krytycznego (volatile: multi-context access)
extern bool pumpGlobalEnabled;      // Globalne włączenie pomp

// ============================================================================
// HELPER MACROS
// ============================================================================
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define MS_TO_SEC(ms)       ((ms) / 1000)
#define SEC_TO_MS(sec)      ((sec) * 1000UL)
#define MIN_TO_SEC(min)     ((min) * 60)
#define MIN_TO_MS(min)      ((min) * 60000UL)

// Bitmask helpers
#define BIT_SET(mask, bit)      ((mask) |= (1UL << (bit)))
#define BIT_CLEAR(mask, bit)    ((mask) &= ~(1UL << (bit)))
#define BIT_CHECK(mask, bit)    (((mask) >> (bit)) & 1)
#define BIT_TOGGLE(mask, bit)   ((mask) ^= (1UL << (bit)))

// ============================================================================
// NTP CONFIGURATION
// ============================================================================

// NTP servers (IP addresses - no DNS required)
#define NTP_SERVER_1            "216.239.35.0"      // time.google.com
#define NTP_SERVER_2            "216.239.35.4"      // time2.google.com
#define NTP_SERVER_3            "162.159.200.1"     // time.cloudflare.com

// Timezone (UTC for RTC storage, conversion in GUI if needed)
#define NTP_GMT_OFFSET_SEC      0                   // RTC stores UTC
#define NTP_DAYLIGHT_OFFSET_SEC 0                   // No DST for UTC

// Sync settings
#define NTP_SYNC_TIMEOUT_MS     20000               // 20 seconds max wait
#define NTP_SYNC_RETRY_DELAY_MS 500                 // Delay between retries
#define NTP_RESYNC_INTERVAL_MS  3600000             // Resync every 1 hour
#define NTP_MIN_VALID_YEAR      2024                // Timestamp sanity check

// Popcount (liczba ustawionych bitów)
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
// SAFETY SYSTEM CONFIGURATION
// ============================================================================

// --- Master Relay (główne odcięcie zasilania pomp) ---
#define MASTER_RELAY_PIN              38
#define MASTER_RELAY_ACTIVE           HIGH    // Stan aktywny (pompy zasilone)
#define MASTER_RELAY_INACTIVE         LOW     // Stan bezpieczny (pompy odcięte)

// --- Buzzer (sygnalizacja błędu) ---
#define BUZZER_PIN                    39      // Z prov_config.h
#define BUZZER_ACTIVE                 HIGH
#define BUZZER_INACTIVE               LOW
#define BUZZER_ERROR_ON_MS            100     // HIGH przez 0.5s
#define BUZZER_ERROR_OFF_MS           500    // LOW przez 1.0s

// --- Reset Button ---
#define RESET_BUTTON_PIN              40      // Z prov_config.h
#define RESET_BUTTON_ACTIVE           LOW     // Aktywny stan (pull-up)
#define RESET_BUTTON_HOLD_MS          5000    // 5 sekund przytrzymania

// --- GPIO Validation (rozszerzone) ---
#define GPIO_VALIDATION_DEFAULT       true
#define GPIO_DEBOUNCE_MS              5       // Debounce dla każdego odczytu
#define GPIO_CHECK_DELAY_MS           200     // Po włączeniu, przed sprawdzeniem HIGH
#define GPIO_POST_CHECK_DELAY_MS      200     // Po wyłączeniu, przed sprawdzeniem LOW
#define GPIO_STATE_IDLE               LOW     // Stan spoczynkowy (przekaźnik OFF)
#define GPIO_STATE_ACTIVE             HIGH    // Stan aktywny (przekaźnik ON)

// ============================================================================
// INITIALIZATION STATUS
// ============================================================================

/**
 * Status inicjalizacji komponentów systemu
 */
struct InitStatus {
    // Hardware
    bool i2c_ok;
    bool fram_ok;
    bool rtc_ok;
    bool relays_ok;
    
    // Network
    bool wifi_ok;
    bool webserver_ok;
    
    // Application
    bool channel_manager_ok;
    bool scheduler_ok;
    
    // Overall
    bool critical_ok;       // Czy komponenty krytyczne działają
    bool system_ready;      // Czy system gotowy do pracy
    
    /**
     * Sprawdź czy hardware krytyczny OK
     */
    bool isHardwareOk() const {
        return i2c_ok && fram_ok && rtc_ok && relays_ok;
    }
    
    /**
     * Sprawdź czy aplikacja OK
     */
    bool isApplicationOk() const {
        return channel_manager_ok && scheduler_ok;
    }
    
    /**
     * Reset do wartości domyślnych
     */
    void reset() {
        i2c_ok = false;
        fram_ok = false;
        rtc_ok = false;
        relays_ok = false;
        wifi_ok = false;
        webserver_ok = false;
        channel_manager_ok = false;
        scheduler_ok = false;
        critical_ok = false;
        system_ready = false;
    }
};

/**
 * Globalny status inicjalizacji
 */
extern InitStatus initStatus;

#endif // CONFIG_H