/**
 * DOZOWNIK - Channel Manager Implementation
 */

#include "channel_manager.h"
#include "rtc_controller.h"

// Global instance
ChannelManager channelManager;

// Recursive mutex for RAM cache access (race condition fix)
// Recursive so that a locked method can safely call another locked method
static SemaphoreHandle_t _channelMutex = nullptr;
static bool _mutexInitialized = false;

// Helper to initialize mutex once
static void _initMutex() {
    if (!_mutexInitialized) {
        _channelMutex = xSemaphoreCreateRecursiveMutex();
        _mutexInitialized = (_channelMutex != nullptr);
    }
}

// RAII-style lock guard for cleaner code (recursive mutex version)
class ChannelLock {
public:
    ChannelLock() : _locked(false) {
        if (_channelMutex && xSemaphoreTakeRecursive(_channelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            _locked = true;
        }
    }
    ~ChannelLock() {
        if (_locked) {
            xSemaphoreGiveRecursive(_channelMutex);
        }
    }
    bool isLocked() const { return _locked; }
private:
    bool _locked;
};

// Static empty instances for invalid access
ChannelConfig ChannelManager::_emptyConfig = {};
ChannelDailyState ChannelManager::_emptyDailyState = {};
ChannelCalculated ChannelManager::_emptyCalculated = {};

// ============================================================================
// INITIALIZATION
// ============================================================================

bool ChannelManager::begin() {
    Serial.println(F("[CH_MGR] Initializing..."));

    // Initialize mutex for thread-safe access
    _initMutex();
    if (!_mutexInitialized) {
        Serial.println(F("[CH_MGR] WARNING: Failed to create mutex"));
    }

    if (!framController.isReady()) {
        Serial.println(F("[CH_MGR] ERROR: FRAM not ready!"));
        _initialized = false;
        return false;
    }
    
    // Load all data from FRAM
    if (!reloadFromFRAM()) {
        Serial.println(F("[CH_MGR] ERROR: Failed to load from FRAM!"));
        _initialized = false;
        return false;
    }
    
    // Recalculate all channels
    recalculateAll();

    _initialized = true;
    Serial.println(F("[CH_MGR] Ready"));
    return true;
}

// ============================================================================
// GETTERS
// ============================================================================

const ChannelConfig& ChannelManager::getActiveConfig(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return _emptyConfig;
    return _activeConfig[channel];
}

const ChannelDailyState& ChannelManager::getDailyState(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return _emptyDailyState;
    return _dailyState[channel];
}

const ChannelCalculated& ChannelManager::getCalculated(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return _emptyCalculated;
    return _calculated[channel];
}

ChannelState ChannelManager::getChannelState(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return CH_STATE_INACTIVE;
    
    const ChannelConfig& active = _activeConfig[channel];
    const ChannelCalculated& calc = _calculated[channel];
    
    // Check if inactive (no events)
    if (active.events_bitmask == 0) {
        return CH_STATE_INACTIVE;
    }
    
    // Check if incomplete (no days or no dose)
    if (active.days_bitmask == 0 || active.daily_dose_ml <= 0) {
        return CH_STATE_INCOMPLETE;
    }
    
    // Check if invalid (validation failed)
    if (!calc.is_valid) {
        return CH_STATE_INVALID;
    }
    
    // All good
    return CH_STATE_CONFIGURED;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

bool ChannelManager::updateConfig(uint8_t channel, const ConfigUpdate& update, const TimeInfo* now,
                                  ConfigSaveResult* result, ValidationError* error) {
    ConfigSaveResult localResult;
    if (!result) result = &localResult;
    *result = ConfigSaveResult();

    if (channel >= CHANNEL_COUNT) {
        result->valid = false;
        return false;
    }

    // Lock so the scheduler never reads a partial update
    ChannelLock lock;
    if (!lock.isLocked()) {
        Serial.println(F("[CH_MGR] WARNING: updateConfig failed to acquire lock"));
    }

    const ChannelConfig& active = _activeConfig[channel];
    ChannelConfig candidate = active;

    if (update.has_events) {
        // Only even hours 2,4,...,22 are valid
        candidate.events_bitmask = update.events & EVENT_VALID_HOURS_MASK;
    }

    if (update.has_days) {
        candidate.days_bitmask = update.days & 0x7F;
    }

    if (update.has_dose) {
        float dose = update.dose;
        if (dose < 0) dose = 0;
        float maxDose = (channel == AIR_PUMP_CHANNEL) ? (float)MAX_AIR_PUMP_DURATION_SECONDS : MAX_DAILY_DOSE_ML;
        if (dose > maxDose) dose = maxDose;
        candidate.daily_dose_ml = dose;
    }

    if (update.has_rate) {
        float rate = update.rate;
        if (rate < MIN_DOSING_RATE) rate = MIN_DOSING_RATE;
        if (rate > MAX_DOSING_RATE) rate = MAX_DOSING_RATE;
        candidate.dosing_rate = rate;
    }

    // enabled always follows events_bitmask (no GUI toggle for it)
    candidate.enabled = (candidate.events_bitmask > 0) ? 1 : 0;

    if (!validateConfig(channel, candidate, error)) {
        result->valid = false;
        return false;
    }

    ChannelDailyState newState = _dailyState[channel];
    bool planChanged = false;
    if (now) {
        planChanged = candidate.daily_dose_ml != active.daily_dose_ml ||
                      candidate.events_bitmask != active.events_bitmask ||
                      candidate.isDayEnabled(now->dayOfWeek) != active.isDayEnabled(now->dayOfWeek);
        if (planChanged) {
            _planToday(channel, candidate, *now, &newState, result);
            result->today_recalculated = true;
            _updateDailyStateCRC(&newState);
        }
    }

    _updateConfigCRC(&candidate);

    // RAM follows FRAM: a failed write leaves the running config unchanged
    if (!framController.writeActiveConfig(channel, &candidate)) {
        return false;
    }
    _activeConfig[channel] = candidate;

    bool stateOk = true;
    if (planChanged) {
        stateOk = framController.writeDailyState(channel, &newState);
        _dailyState[channel] = newState;
    }

    recalculate(channel);
    return stateOk;
}

void ChannelManager::_planToday(uint8_t channel, const ChannelConfig& cfg, const TimeInfo& now,
                                ChannelDailyState* state, ConfigSaveResult* result) const {
    state->today_dose_cml = TODAY_DOSE_STANDARD;

    // Slots that have not run and whose window is still open or ahead
    uint8_t slots = 0;
    if (cfg.isDayEnabled(now.dayOfWeek)) {
        uint16_t nowMin = (uint16_t)now.hour * 60 + now.minute;
        uint16_t offsetMin = (uint16_t)CHANNEL_SLOT_MAP[channel] * CHANNEL_OFFSET_MINUTES;
        for (uint8_t h = FIRST_EVENT_HOUR; h <= LAST_EVENT_HOUR; h += 2) {
            if (!cfg.isEventEnabled(h)) continue;
            if (state->isEventCompleted(h) || state->isEventFailed(h)) continue;
            uint16_t windowEnd = (uint16_t)h * 60 + offsetMin + EVENT_WINDOW_SECONDS / 60;
            if (nowMin < windowEnd) slots++;
        }
    }
    result->today_slots = slots;
    if (slots == 0) return;

    float standard = cfg.getSingleDose();

    // Air pump runs a fixed time per event; there is no dose to rebalance
    if (channel == AIR_PUMP_CHANNEL) {
        result->today_dose_ml = standard;
        return;
    }

    float remaining = cfg.daily_dose_ml - state->today_added_ml;
    if (remaining < MIN_SINGLE_DOSE_ML) {
        state->today_dose_cml = TODAY_DOSE_NONE;
        return;
    }

    float dose = remaining / (float)slots;
    if (fabsf(dose - standard) < 0.005f) {
        result->today_dose_ml = standard;
        return;
    }

    if (dose < MIN_SINGLE_DOSE_ML) dose = MIN_SINGLE_DOSE_ML;

    // Cap at the limits; whatever does not fit is dropped rather than overdosed
    float maxDose = MAX_SINGLE_DOSE_ML;
    ChannelParams params;
    if (framController.readChannelParams(channel, &params) &&
        params.max_single_dose_ml > 0 && params.max_single_dose_ml < maxDose) {
        maxDose = params.max_single_dose_ml;
    }
    float maxByPump = (float)(MAX_PUMP_DURATION_SECONDS - 1) * cfg.dosing_rate;
    if (maxByPump < maxDose) maxDose = maxByPump;
    if (dose > maxDose) dose = maxDose;

    long cml = lroundf(dose * 100.0f);
    if (cml < 1) cml = 1;
    if (cml >= TODAY_DOSE_NONE) cml = TODAY_DOSE_NONE - 1;
    state->today_dose_cml = (uint16_t)cml;
    result->today_dose_ml = cml / 100.0f;
}

float ChannelManager::getStandardDoseMl(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return 0;
    return _activeConfig[channel].getSingleDose();
}

float ChannelManager::getNextDoseMl(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return 0;

    const ChannelConfig& cfg = _activeConfig[channel];
    const ChannelDailyState& state = _dailyState[channel];

    if (channel == AIR_PUMP_CHANNEL || state.today_dose_cml == TODAY_DOSE_STANDARD) {
        return cfg.getSingleDose();
    }
    if (state.today_dose_cml == TODAY_DOSE_NONE) return 0;

    float remaining = cfg.daily_dose_ml - state.today_added_ml;
    if (remaining < MIN_SINGLE_DOSE_ML) return 0;

    float dose = state.today_dose_cml / 100.0f;
    if (dose > remaining) dose = remaining;

    // The rate may have been recalibrated after today's plan was made
    float maxByPump = (float)(MAX_PUMP_DURATION_SECONDS - 1) * cfg.dosing_rate;
    if (dose > maxByPump) dose = maxByPump;
    return dose;
}

// ============================================================================
// VALIDATION
// ============================================================================

bool ChannelManager::validateConfig(uint8_t channel, ValidationError* error) {
    return validateConfig(channel, getActiveConfig(channel), error);
}

bool ChannelManager::validateConfig(uint8_t channel, const ChannelConfig& cfg, ValidationError* error) {
    if (channel >= CHANNEL_COUNT) {
        if (error) {
            error->has_error = true;
            error->channel = channel;
            snprintf(error->message, sizeof(error->message), "Invalid channel");
        }
        return false;
    }
    
    // Skip validation if no events (inactive channel)
    if (cfg.events_bitmask == 0) {
        if (error) error->has_error = false;
        return true;
    }

    // Channel 7: air pump — duration mode, dailyDose stores seconds not ml
    if (channel == AIR_PUMP_CHANNEL) {
        if (cfg.days_bitmask == 0) {
            if (error) {
                error->has_error = true;
                error->channel = channel;
                snprintf(error->message, sizeof(error->message), "No days selected");
            }
            return false;
        }
        if (cfg.daily_dose_ml <= 0 || cfg.daily_dose_ml > MAX_AIR_PUMP_DURATION_SECONDS) {
            if (error) {
                error->has_error = true;
                error->channel = channel;
                snprintf(error->message, sizeof(error->message),
                         "Duration must be 1-%ds", MAX_AIR_PUMP_DURATION_SECONDS);
            }
            return false;
        }
        if (error) error->has_error = false;
        return true;
    }

    // Check days
    if (cfg.days_bitmask == 0) {
        if (error) {
            error->has_error = true;
            error->channel = channel;
            snprintf(error->message, sizeof(error->message), "No days selected");
        }
        return false;
    }
    
    // Check daily dose
    if (cfg.daily_dose_ml <= 0) {
        if (error) {
            error->has_error = true;
            error->channel = channel;
            snprintf(error->message, sizeof(error->message), "Daily dose not set");
        }
        return false;
    }
    
    // Check dosing rate
    if (cfg.dosing_rate <= 0) {
        if (error) {
            error->has_error = true;
            error->channel = channel;
            snprintf(error->message, sizeof(error->message), "Dosing rate not calibrated");
        }
        return false;
    }
    
    // Calculate single dose
    uint8_t eventCount = cfg.getActiveEventsCount();
    float singleDose = cfg.daily_dose_ml / (float)eventCount;
    
    // Validate single dose minimum
    if (singleDose < MIN_SINGLE_DOSE_ML) {
        if (error) {
            error->has_error = true;
            error->channel = channel;
            snprintf(error->message, sizeof(error->message), 
                     "Single dose %.2f < %.1f ml min", singleDose, MIN_SINGLE_DOSE_ML);
        }
        return false;
    }
    
    // Validate pump duration
    uint32_t pumpMs = (uint32_t)((singleDose / cfg.dosing_rate) * 1000.0f);
    if (pumpMs > MAX_PUMP_DURATION_MS) {
        if (error) {
            error->has_error = true;
            error->channel = channel;
            snprintf(error->message, sizeof(error->message), 
                     "Pump time %lus > %ds max", pumpMs / 1000, MAX_PUMP_DURATION_SECONDS);
        }
        return false;
    }
    
    if (error) error->has_error = false;
    return true;
}

bool ChannelManager::validateAll(ValidationError* firstError) {
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!validateConfig(i, firstError)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// DAILY STATE
// ============================================================================

bool ChannelManager::markEventCompleted(uint8_t channel, uint8_t hour, float dosed_ml) {
    if (channel >= CHANNEL_COUNT) return false;
    if (hour < FIRST_EVENT_HOUR || hour > LAST_EVENT_HOUR) return false;

    // Lock for atomic daily state update
    ChannelLock lock;
    if (!lock.isLocked()) {
        Serial.println(F("[CH_MGR] WARNING: markEventCompleted failed to acquire lock"));
    }

    _dailyState[channel].markEventCompleted(hour);
    _dailyState[channel].today_added_ml += dosed_ml;

    _updateDailyStateCRC(&_dailyState[channel]);

    return framController.writeDailyState(channel, &_dailyState[channel]);
}

bool ChannelManager::markEventFailed(uint8_t channel, uint8_t hour) {
    if (channel >= CHANNEL_COUNT) return false;
    if (hour < FIRST_EVENT_HOUR || hour > LAST_EVENT_HOUR) return false;

    // Lock for atomic daily state update
    ChannelLock lock;
    if (!lock.isLocked()) {
        Serial.println(F("[CH_MGR] WARNING: markEventFailed failed to acquire lock"));
    }

    _dailyState[channel].markEventFailed(hour);

    _updateDailyStateCRC(&_dailyState[channel]);

    Serial.printf("[CH_MGR] CH%d hour %d marked as FAILED (total failed today: %d)\n",
                  channel, hour, _dailyState[channel].failed_count);

    return framController.writeDailyState(channel, &_dailyState[channel]);
}

bool ChannelManager::isEventFailed(uint8_t channel, uint8_t hour) const {
    if (channel >= CHANNEL_COUNT) return false;
    return _dailyState[channel].isEventFailed(hour);
}

bool ChannelManager::resetDailyStates() {
    Serial.println(F("[CH_MGR] Resetting daily states"));

    // Lock for atomic reset of ALL channels (prevents web reading inconsistent state)
    ChannelLock lock;
    if (!lock.isLocked()) {
        Serial.println(F("[CH_MGR] WARNING: resetDailyStates failed to acquire lock"));
    }

    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        _dailyState[i].reset();
        _updateDailyStateCRC(&_dailyState[i]);

        if (!framController.writeDailyState(i, &_dailyState[i])) {
            return false;
        }
    }

    recalculateAll();
    return true;
}

bool ChannelManager::isEventCompleted(uint8_t channel, uint8_t hour) const {
    if (channel >= CHANNEL_COUNT) return false;
    return _dailyState[channel].isEventCompleted(hour);
}

float ChannelManager::getTodayDosed(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return 0;
    return _dailyState[channel].today_added_ml;
}

// ============================================================================
// QUERIES
// ============================================================================

bool ChannelManager::isActiveToday(uint8_t channel, uint8_t dayOfWeek) const {
    if (channel >= CHANNEL_COUNT) return false;
    if (dayOfWeek > 6) return false;
    
    return _activeConfig[channel].isDayEnabled(dayOfWeek);
}

bool ChannelManager::shouldExecuteEvent(uint8_t channel, uint8_t hour, uint8_t dayOfWeek) const {
    if (channel >= CHANNEL_COUNT) return false;
    
    const ChannelConfig& cfg = _activeConfig[channel];
    const ChannelDailyState& state = _dailyState[channel];
    
    if (!cfg.enabled) return false;
    if (cfg.daily_dose_ml <= 0 || cfg.dosing_rate <= 0) return false;
    
    // Check if day is active
    if (!cfg.isDayEnabled(dayOfWeek)) return false;
    
    // Check if hour is scheduled
    if (!cfg.isEventEnabled(hour)) return false;
    
    // Check if already completed
    if (state.isEventCompleted(hour)) return false;

    // Check if failed (no retry after critical error)
    if (state.isEventFailed(hour)) return false;
    
    // Air pump runs per event; a daily total would cut off runs after a slot change
    if (channel != AIR_PUMP_CHANNEL) {
        if (state.today_added_ml >= cfg.daily_dose_ml) return false;
        if (getNextDoseMl(channel) <= 0) return false;
    }
    
    return true;
}

uint8_t ChannelManager::getNextEventHour(uint8_t channel, uint8_t currentHour) const {
    if (channel >= CHANNEL_COUNT) return 255;
    
    const ChannelConfig& cfg = _activeConfig[channel];
    
    // Szukaj tylko parzystych godzin eventów
    uint8_t startH = (currentHour % 2 == 0) ? currentHour : currentHour + 1;
    for (uint8_t h = startH; h <= LAST_EVENT_HOUR; h += 2) {
        if (cfg.isEventEnabled(h)) {
            return h;
        }
    }
    
    return 255; // No more events today
}

// ============================================================================
// FRAM OPERATIONS
// ============================================================================

bool ChannelManager::reloadFromFRAM() {
    Serial.println(F("[CH_MGR] Loading from FRAM..."));
    
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!framController.readActiveConfig(i, &_activeConfig[i])) {
            Serial.printf("[CH_MGR] Failed to read active CH%d\n", i);
            return false;
        }
        
        if (!framController.readDailyState(i, &_dailyState[i])) {
            Serial.printf("[CH_MGR] Failed to read daily CH%d\n", i);
            return false;
        }
    }
    
    return true;
}

bool ChannelManager::saveToFRAM() {
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        _updateConfigCRC(&_activeConfig[i]);
        _updateDailyStateCRC(&_dailyState[i]);
        
        if (!framController.writeActiveConfig(i, &_activeConfig[i])) return false;
        if (!framController.writeDailyState(i, &_dailyState[i])) return false;
    }
    return true;
}

// ============================================================================
// CALCULATIONS
// ============================================================================

void ChannelManager::recalculate(uint8_t channel) {
    if (channel >= CHANNEL_COUNT) return;
    
    const ChannelConfig& cfg = _activeConfig[channel];
    const ChannelDailyState& state = _dailyState[channel];
    ChannelCalculated& calc = _calculated[channel];
    
    // Count events and days
    calc.active_events_count = cfg.getActiveEventsCount();
    calc.active_days_count = cfg.getActiveDaysCount();
    calc.completed_today_count = state.getCompletedCount();
    
    // Calculate doses
    if (calc.active_events_count > 0 && cfg.daily_dose_ml > 0) {
        calc.single_dose_ml = cfg.daily_dose_ml / (float)calc.active_events_count;
        calc.weekly_dose_ml = cfg.daily_dose_ml * (float)calc.active_days_count;
    } else {
        calc.single_dose_ml = 0;
        calc.weekly_dose_ml = 0;
    }
    
    // Calculate remaining
    calc.today_remaining_ml = cfg.daily_dose_ml - state.today_added_ml;
    if (calc.today_remaining_ml < 0) calc.today_remaining_ml = 0;
    
    // Calculate pump duration
    if (cfg.dosing_rate > 0 && calc.single_dose_ml > 0) {
        calc.pump_duration_ms = (uint32_t)((calc.single_dose_ml / cfg.dosing_rate) * 1000.0f);
    } else {
        calc.pump_duration_ms = 0;
    }
    
    // Validate
    ValidationError err;
    calc.is_valid = validateConfig(channel, &err);
    
    // Determine state
    calc.state = getChannelState(channel);
    
    // Next event (simplified - would need current hour)
    calc.next_event_hour = 255;
}

void ChannelManager::recalculateAll() {
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        recalculate(i);
    }
}

// ============================================================================
// CRC HELPERS
// ============================================================================

void ChannelManager::_updateConfigCRC(ChannelConfig* cfg) {
    cfg->crc32 = FramController::calculateCRC32(cfg, sizeof(ChannelConfig) - sizeof(uint32_t));
}

void ChannelManager::_updateDailyStateCRC(ChannelDailyState* state) {
    state->crc32 = FramController::calculateCRC32(state, sizeof(ChannelDailyState) - sizeof(uint32_t));
}

// ============================================================================
// DEBUG
// ============================================================================

void ChannelManager::printChannelInfo(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) {
        Serial.println(F("[CH_MGR] Invalid channel"));
        return;
    }
    
    const ChannelConfig& cfg = _activeConfig[channel];
    const ChannelDailyState& state = _dailyState[channel];
    const ChannelCalculated& calc = _calculated[channel];
    
    const char* stateNames[] = {"INACTIVE", "INCOMPLETE", "INVALID", "CONFIGURED"};
    
    Serial.printf("\n=== Channel %d ===\n", channel);
    Serial.printf("State: %s\n", stateNames[calc.state]);
    Serial.printf("Enabled: %s\n", cfg.enabled ? "YES" : "NO");
    
    Serial.println(F("\nActive Config:"));
    Serial.printf("  Events bitmask: 0x%06X (%d events)\n", cfg.events_bitmask, calc.active_events_count);
    Serial.printf("  Days bitmask:   0x%02X (%d days)\n", cfg.days_bitmask, calc.active_days_count);
    Serial.printf("  Daily dose:     %.2f ml\n", cfg.daily_dose_ml);
    Serial.printf("  Dosing rate:    %.3f ml/s\n", cfg.dosing_rate);
    
    Serial.println(F("\nCalculated:"));
    Serial.printf("  Single dose:    %.2f ml\n", calc.single_dose_ml);
    Serial.printf("  Weekly dose:    %.2f ml\n", calc.weekly_dose_ml);
    Serial.printf("  Pump duration:  %lu ms\n", calc.pump_duration_ms);
    Serial.printf("  Valid:          %s\n", calc.is_valid ? "YES" : "NO");
    
    Serial.println(F("\nToday:"));
    Serial.printf("  Completed:      %d events\n", calc.completed_today_count);
    Serial.printf("  Dosed:          %.2f ml\n", state.today_added_ml);
    Serial.printf("  Remaining:      %.2f ml\n", calc.today_remaining_ml);
    
    Serial.println();
}

void ChannelManager::printAllChannels() const {
    Serial.println(F("\n[CH_MGR] All Channels Summary:"));
    Serial.println(F("┌────┬──────────────┬────────┬────────┬──────────┬────────┐"));
    Serial.println(F("│ CH │    State     │ Events │  Days  │ Dose/day │  Rate  │"));
    Serial.println(F("├────┼──────────────┼────────┼────────┼──────────┼────────┤"));
    
    const char* stateNames[] = {"INACTIVE", "INCOMPLETE", "INVALID", "CONFIGURED"};
    
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        const ChannelConfig& cfg = _activeConfig[i];
        const ChannelCalculated& calc = _calculated[i];
        
        Serial.printf("│ %2d │ %-12s │   %2d   │   %2d   │ %6.1f ml│ %5.2f  │\n",
                      i, 
                      stateNames[calc.state],
                      calc.active_events_count,
                      calc.active_days_count,
                      cfg.daily_dose_ml,
                      cfg.dosing_rate);
    }
}