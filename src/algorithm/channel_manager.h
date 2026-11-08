/**
 * DOZOWNIK - Channel Manager
 * 
 * Zarządzanie konfiguracją kanałów dozowania.
 * Walidacja, obliczenia, zapis/odczyt z FRAM.
 */

#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "dosing_types.h"
#include "fram_controller.h"

struct TimeInfo;

// ============================================================================
// VALIDATION RESULT
// ============================================================================

struct ValidationError {
    bool     has_error;
    uint8_t  channel;
    char     message[64];
};

// ============================================================================
// CHANNEL MANAGER CLASS
// ============================================================================

class ChannelManager {
public:
    /**
     * Inicjalizacja - ładuje konfigurację z FRAM
     */
    bool begin();
    
    /**
     * Czy manager zainicjalizowany
     */
    bool isReady() const { return _initialized; }
    
    // --- Gettery konfiguracji ---
    
    /**
     * Pobierz aktywną konfigurację kanału (tylko do odczytu)
     */
    const ChannelConfig& getActiveConfig(uint8_t channel) const;
    
    /**
     * Pobierz stan dzienny kanału
     */
    const ChannelDailyState& getDailyState(uint8_t channel) const;
    
    /**
     * Pobierz obliczone wartości kanału
     */
    const ChannelCalculated& getCalculated(uint8_t channel) const;
    
    /**
     * Pobierz stan kanału (do GUI)
     */
    ChannelState getChannelState(uint8_t channel) const;
    
    // --- Configuration ---

    struct ConfigUpdate {
        bool has_events = false;
        uint32_t events = 0;
        bool has_days = false;
        uint8_t days = 0;
        bool has_dose = false;
        float dose = 0;
        bool has_rate = false;
        float rate = 0;
    };

    struct ConfigSaveResult {
        bool  valid = true;               // false: rejected, nothing was stored
        bool  today_recalculated = false; // The plan for the rest of today changed
        uint8_t today_slots = 0;          // Slots still to run today
        float today_dose_ml = 0;          // Dose per slot; 0 = nothing left to dose
    };

    /**
     * Validate and store a config change; it takes effect immediately.
     * Only fields with has_* = true are changed.
     * If the dose, the slots or today's activity changed, the rest of today is
     * planned once: (daily dose - dosed today) / slots left. Missed or failed
     * slots after that point do not raise later doses.
     * @param now current time, nullptr to skip planning (e.g. no valid RTC)
     * @return true if the config was stored
     */
    bool updateConfig(uint8_t channel, const ConfigUpdate& update, const TimeInfo* now,
                      ConfigSaveResult* result = nullptr, ValidationError* error = nullptr);

    /**
     * Standard dose per event: daily dose / active events
     */
    float getStandardDoseMl(uint8_t channel) const;

    /**
     * Dose for the next scheduled event, including today's plan
     * @return ml, 0 when nothing is left to dose today
     */
    float getNextDoseMl(uint8_t channel) const;

    // --- Validation ---

    /**
     * Validate the active config of a channel
     */
    bool validateConfig(uint8_t channel, ValidationError* error = nullptr);

    /**
     * Validate a candidate config for a channel
     */
    bool validateConfig(uint8_t channel, const ChannelConfig& cfg, ValidationError* error = nullptr);

    /**
     * Validate all channels
     */
    bool validateAll(ValidationError* firstError = nullptr);

    // --- Daily State ---

    /**
     * Oznacz event jako wykonany
     */
    bool markEventCompleted(uint8_t channel, uint8_t hour, float dosed_ml);

    /**
     * Oznacz event jako nieudany (failed)
     */
    bool markEventFailed(uint8_t channel, uint8_t hour);
    
    /**
     * Czy event się nie powiódł
     */
    bool isEventFailed(uint8_t channel, uint8_t hour) const;
    
    /**
     * Reset stanów dziennych (o północy)
     */
    bool resetDailyStates();

    /**
     * Czy event został wykonany
     */
    bool isEventCompleted(uint8_t channel, uint8_t hour) const;
    
    /**
     * Ile dozowano dziś na kanale
     */
    float getTodayDosed(uint8_t channel) const;
    
    // --- Queries ---
    
    /**
     * Czy kanał jest aktywny dzisiaj (dzień tygodnia)
     */
    bool isActiveToday(uint8_t channel, uint8_t dayOfWeek) const;
    
    /**
     * Czy event powinien być wykonany teraz
     */
    bool shouldExecuteEvent(uint8_t channel, uint8_t hour, uint8_t dayOfWeek) const;
    
    /**
     * Pobierz następny zaplanowany event dla kanału
     * @return godzina (1-23) lub 255 jeśli brak
     */
    uint8_t getNextEventHour(uint8_t channel, uint8_t currentHour) const;
    
    // --- Utility ---
    
    /**
     * Przeładuj wszystko z FRAM
     */
    bool reloadFromFRAM();
    
    /**
     * Zapisz wszystko do FRAM
     */
    bool saveToFRAM();
    
    /**
     * Przelicz wartości dla kanału
     */
    void recalculate(uint8_t channel);
    
    /**
     * Przelicz wszystkie kanały
     */
    void recalculateAll();
    
    /**
     * Debug print konfiguracji kanału
     */
    void printChannelInfo(uint8_t channel) const;
    
    /**
     * Debug print wszystkich kanałów
     */
    void printAllChannels() const;

private:
    bool _initialized;
    
    // Cached data (RAM)
    ChannelConfig     _activeConfig[CHANNEL_COUNT];
    ChannelDailyState _dailyState[CHANNEL_COUNT];
    ChannelCalculated _calculated[CHANNEL_COUNT];

    // Empty config for invalid channel access
    static ChannelConfig _emptyConfig;
    static ChannelDailyState _emptyDailyState;
    static ChannelCalculated _emptyCalculated;
    
    /**
     * Plan the rest of today for a candidate config (updates *state)
     */
    void _planToday(uint8_t channel, const ChannelConfig& cfg, const TimeInfo& now,
                    ChannelDailyState* state, ConfigSaveResult* result) const;

    /**
     * Aktualizuj CRC w strukturze
     */
    void _updateConfigCRC(ChannelConfig* cfg);
    void _updateDailyStateCRC(ChannelDailyState* state);
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

extern ChannelManager channelManager;

#endif // CHANNEL_MANAGER_H