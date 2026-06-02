/**
 * DOZOWNIK - Data Types & Structures
 *
 * Definicje struktur danych używanych w całym systemie.
 * Struktury przeznaczone do przechowywania w FRAM są packed.
 */

#ifndef DOSING_TYPES_H
#define DOSING_TYPES_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// ENUMERATIONS
// ============================================================================

enum ChannelState : uint8_t {
    CH_STATE_INACTIVE   = 0,
    CH_STATE_INCOMPLETE = 1,
    CH_STATE_INVALID    = 2,
    CH_STATE_CONFIGURED = 3,
    CH_STATE_PENDING    = 4
};

enum CriticalErrorType : uint8_t {
    ERROR_NONE              = 0,
    ERROR_PUMP_TIMEOUT      = 4,
    ERROR_FRAM_FAILURE      = 5,
    ERROR_RTC_FAILURE       = 6,
    ERROR_UNKNOWN           = 255
};

enum EventStatus : uint8_t {
    EVENT_PENDING   = 0,
    EVENT_COMPLETED = 1,
    EVENT_SKIPPED   = 2,
    EVENT_FAILED    = 3
};

enum PumpState : uint8_t {
    PUMP_IDLE    = 0,
    PUMP_RUNNING = 1,
    PUMP_ERROR   = 2
};

// ============================================================================
// CHANNEL CONFIGURATION (32 bajtów, packed — przechowywana w FRAM)
// ============================================================================

#pragma pack(push, 1)

struct ChannelConfig {
    // Parametry użytkownika (12B)
    uint32_t events_bitmask;    // Bity 2,4,6,...,22 = parzyste godziny eventu
    uint8_t  days_bitmask;      // Bit 0-6 = Pon-Ndz
    uint8_t  _reserved1[3];
    float    daily_dose_ml;

    // Kalibracja (4B)
    float    dosing_rate;       // ml/s

    // Flagi (4B)
    uint8_t  enabled;           // 0/1
    uint8_t  has_pending;       // 0/1
    uint8_t  _reserved2[2];

    // Checksum (4B)
    uint32_t crc32;

    // Padding do 32B
    uint8_t  _padding[8];

    inline uint8_t getActiveEventsCount() const { return popcount32(events_bitmask); }
    inline uint8_t getActiveDaysCount() const { return popcount8(days_bitmask); }

    inline float getSingleDose() const {
        uint8_t evCnt = getActiveEventsCount();
        if (evCnt == 0 || daily_dose_ml <= 0) return 0.0f;
        return daily_dose_ml / (float)evCnt;
    }

    inline float getWeeklyDose() const {
        return daily_dose_ml * (float)getActiveDaysCount();
    }

    inline uint32_t getPumpDurationMs() const {
        float single = getSingleDose();
        if (dosing_rate <= 0 || single <= 0) return 0;
        return (uint32_t)((single / dosing_rate) * 1000.0f);
    }

    inline bool isEventEnabled(uint8_t hour) const {
        if (hour < FIRST_EVENT_HOUR || hour > LAST_EVENT_HOUR) return false;
        return BIT_CHECK(events_bitmask, hour);
    }

    inline bool isDayEnabled(uint8_t dayOfWeek) const {
        if (dayOfWeek > 6) return false;
        return BIT_CHECK(days_bitmask, dayOfWeek);
    }
};

#pragma pack(pop)

static_assert(sizeof(ChannelConfig) == 32, "ChannelConfig must be 32 bytes");

// ============================================================================
// DAILY STATE (24 bajtów, packed — resetowana o północy)
// ============================================================================

#pragma pack(push, 1)

struct ChannelDailyState {
    uint32_t events_completed;
    uint32_t events_failed;
    float    today_added_ml;
    uint8_t  last_reset_day;
    uint8_t  failed_count;
    uint8_t  _reserved[2];
    uint32_t crc32;
    uint8_t  _padding[4];

    inline uint8_t getCompletedCount() const { return popcount32(events_completed); }
    inline uint8_t getFailedCount() const { return popcount32(events_failed); }

    inline bool isEventCompleted(uint8_t hour) const {
        if (hour < FIRST_EVENT_HOUR || hour > LAST_EVENT_HOUR) return false;
        return BIT_CHECK(events_completed, hour);
    }

    inline bool isEventFailed(uint8_t hour) const {
        if (hour < FIRST_EVENT_HOUR || hour > LAST_EVENT_HOUR) return false;
        return BIT_CHECK(events_failed, hour);
    }

    inline void markEventCompleted(uint8_t hour) {
        if (hour >= FIRST_EVENT_HOUR && hour <= LAST_EVENT_HOUR)
            BIT_SET(events_completed, hour);
    }

    inline void markEventFailed(uint8_t hour) {
        if (hour >= FIRST_EVENT_HOUR && hour <= LAST_EVENT_HOUR) {
            BIT_SET(events_failed, hour);
            failed_count++;
        }
    }

    inline void reset() {
        events_completed = 0;
        events_failed = 0;
        today_added_ml = 0.0f;
        failed_count = 0;
    }
};

#pragma pack(pop)

static_assert(sizeof(ChannelDailyState) == 24, "ChannelDailyState must be 24 bytes");

// ============================================================================
// SYSTEM STATE (32 bajtów, packed)
// ============================================================================

#pragma pack(push, 1)

struct SystemState {
    uint8_t  system_enabled;
    uint8_t  system_halted;
    uint8_t  active_channel;        // 255 = żaden
    uint8_t  active_pump_state;
    uint32_t last_daily_reset_day;
    uint32_t boot_count;
    uint8_t  pending_changes_mask;
    uint8_t  _reserved[3];
    uint32_t last_event_timestamp;
    uint32_t crc32;
    uint8_t  _padding[8];
};

#pragma pack(pop)

static_assert(sizeof(SystemState) == 32, "SystemState must be 32 bytes");

// ============================================================================
// CRITICAL ERROR STATE (32 bajtów, packed — przetrwa reset/zanik zasilania)
// ============================================================================

#pragma pack(push, 1)

struct CriticalErrorState {
    uint8_t           active_flag;
    CriticalErrorType error_type;
    uint8_t           channel;
    uint8_t           _reserved1;
    uint32_t          timestamp;
    uint32_t          error_data;

    uint8_t           pump_was_running;
    uint8_t           _reserved2[3];

    uint16_t          total_critical_errors;
    uint16_t          reset_count;
    uint32_t          last_reset_timestamp;

    uint32_t          write_count;
    uint32_t          crc32;
};

#pragma pack(pop)

static_assert(sizeof(CriticalErrorState) == 32, "CriticalErrorState must be 32 bytes");

// ============================================================================
// CONTAINER VOLUME (8 bajtów, packed)
// ============================================================================

#pragma pack(push, 1)

struct ContainerVolume {
    uint16_t container_ml;      // × 10 (0.1ml precision)
    uint16_t remaining_ml;
    uint32_t crc32;

    inline float getContainerMl() const { return container_ml / 10.0f; }
    inline float getRemainingMl() const { return remaining_ml / 10.0f; }
    inline void setContainerMl(float ml) { container_ml = (uint16_t)(ml * 10.0f); }
    inline void setRemainingMl(float ml) { remaining_ml = (uint16_t)(ml * 10.0f); }

    inline uint8_t getRemainingPercent() const {
        if (container_ml == 0) return 0;
        return (uint8_t)((remaining_ml * 100UL) / container_ml);
    }

    inline bool isLowVolume(uint8_t threshold_pct = LOW_VOLUME_THRESHOLD_PCT) const {
        return getRemainingPercent() < threshold_pct;
    }

    inline void refill() { remaining_ml = container_ml; }

    inline void deduct(float ml) {
        uint16_t deduct_val = (uint16_t)(ml * 10.0f);
        remaining_ml = (deduct_val >= remaining_ml) ? 0 : remaining_ml - deduct_val;
    }

    inline void reset() {
        container_ml = CONTAINER_DEFAULT_ML * 10;
        remaining_ml = container_ml;
    }
};

#pragma pack(pop)

static_assert(sizeof(ContainerVolume) == 8, "ContainerVolume must be 8 bytes");

// ============================================================================
// DOSED TRACKER (8 bajtów, packed)
// ============================================================================

#pragma pack(push, 1)

struct DosedTracker {
    uint16_t total_dosed_ml;    // × 10
    uint16_t _reserved;
    uint32_t crc32;

    inline float getTotalDosedMl() const { return total_dosed_ml / 10.0f; }

    inline void addDosed(float ml) {
        uint32_t newVal = total_dosed_ml + (uint16_t)(ml * 10.0f);
        total_dosed_ml = (newVal > 65535) ? 65535 : (uint16_t)newVal;
    }

    inline void reset() { total_dosed_ml = 0; _reserved = 0; }

    inline uint8_t getPercentOfWeekly(float weekly_ml) const {
        if (weekly_ml <= 0) return 0;
        float pct = (getTotalDosedMl() / weekly_ml) * 100.0f;
        return (pct > 100.0f) ? 100 : (uint8_t)pct;
    }
};

#pragma pack(pop)

static_assert(sizeof(DosedTracker) == 8, "DosedTracker must be 8 bytes");

// ============================================================================
// CHANNEL LABEL (32 bajtów, packed — nazwa kanału w FRAM)
// ============================================================================

#pragma pack(push, 1)

struct ChannelLabel {
    char     name[20];          // np. "Calcium", "kH", "Jod"
    uint8_t  _reserved[8];
    uint32_t crc32;
};

#pragma pack(pop)

static_assert(sizeof(ChannelLabel) == 32, "ChannelLabel must be 32 bytes");

// ============================================================================
// CHANNEL PARAMS (32 bajtów, packed — limity per kanał w FRAM)
// ============================================================================

#pragma pack(push, 1)

struct ChannelParams {
    float    min_single_dose_ml;    // User-defined minimum single dose (0 = use global)
    float    max_single_dose_ml;    // User-defined maximum single dose (0 = use global)
    float    max_daily_dose_ml;     // User-defined max daily dose (0 = use global)
    float    _reserved_f;
    uint8_t  _reserved[12];
    uint32_t crc32;
};

#pragma pack(pop)

static_assert(sizeof(ChannelParams) == 32, "ChannelParams must be 32 bytes");

// ============================================================================
// PUMP MONITOR STATUS (tylko RAM — dane z Edge Impulse ESP32 przez UART)
// ============================================================================

struct PumpMonitorStatus {
    uint8_t  pump_id;           // Numer kanału (0-7)
    bool     is_running;        // Potwierdzenie pracy
    uint8_t  confidence_pct;    // Pewność klasyfikacji (0-100)
    uint32_t last_update_ms;    // millis() ostatniej aktualizacji
    bool     monitor_active;    // Czy moduł Edge Impulse odpowiada
};

// ============================================================================
// RUNTIME DATA (tylko RAM)
// ============================================================================

struct ChannelCalculated {
    float    single_dose_ml;
    float    weekly_dose_ml;
    float    today_remaining_ml;
    uint32_t pump_duration_ms;

    uint8_t  active_events_count;
    uint8_t  active_days_count;
    uint8_t  completed_today_count;
    uint8_t  next_event_hour;

    ChannelState state;
    bool     is_valid;
    bool     is_active_today;
    uint8_t  _padding;
};

struct DosingContext {
    uint8_t  channel;
    uint8_t  event_hour;        // Godzina eventu (parzysta: 2,4,...,22)
    PumpState pump_state;
    uint8_t  _reserved;

    uint32_t start_time_ms;
    uint32_t target_duration_ms;
    float    target_volume_ml;

    bool     completed;
    uint8_t  _padding[3];
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

uint32_t calculateCRC32(const void* data, size_t length);
bool validateCRC32(const void* data, size_t length, uint32_t expected_crc);

const char* channelStateToString(ChannelState state);
const char* errorTypeToString(CriticalErrorType error);

uint8_t getDayOfWeek(uint32_t unixTimestamp);
uint8_t getHourUTC(uint32_t unixTimestamp);
uint32_t getUTCDay(uint32_t unixTimestamp);

#endif // DOSING_TYPES_H
