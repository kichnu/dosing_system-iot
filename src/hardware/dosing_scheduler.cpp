/**
 * DOZOWNIK - Dosing Scheduler Implementation
 *
 * Harmonogram: parzyste godziny 02,04,...,22.
 * CH0-CH3: w godzinie eventu (:00,:15,:30,:45)
 * CH4-CH7: w godzinie eventu+1 (:00,:15,:30,:45)
 */

#include "dosing_scheduler.h"
#include "../core/logging.h"

DosingScheduler dosingScheduler;

static portMUX_TYPE _schedulerMux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================================
// INITIALIZATION
// ============================================================================

bool DosingScheduler::begin() {
    LOG_INFO("[SCHED] Initializing...");

    _state = SchedulerState::SCHED_DISABLED;
    memset(&_currentEvent, 0, sizeof(_currentEvent));
    _currentEvent.channel = 255;

    _lastCheckTime  = 0;
    _lastUpdateTime = 0;
    _lastDay        = 255;
    _todayEventCount = 0;

    SystemState sysState;
    if (framController.readSystemState(&sysState)) {
        _enabled = (sysState.system_enabled != 0);
        sysState.boot_count++;
        framController.writeSystemState(&sysState);
        LOG_INFO("[SCHED] State from FRAM: %s, boot#%lu",
                      _enabled ? "ENABLED" : "DISABLED", sysState.boot_count);
    } else {
        _enabled = true;
        LOG_INFO("[SCHED] FRAM load failed, defaulting to ENABLED");
    }

    if (rtcController.isReady() && rtcController.isTimeValid()) {
        TimeInfo now = rtcController.getTime();
        _lastDay = now.day;

        uint32_t currentUtcDay = (uint32_t)now.year * 366 + (uint32_t)now.month * 31 + now.day;
        if (framController.readSystemState(&sysState)) {
            if (sysState.last_daily_reset_day != currentUtcDay) {
                LOG_INFO("[SCHED] New day at startup — performing daily reset");
                _performDailyReset();
                sysState.last_daily_reset_day = currentUtcDay;
                framController.writeSystemState(&sysState);
            }
        }
    }

    _state = _enabled ? SchedulerState::IDLE : SchedulerState::SCHED_DISABLED;
    _initialized = true;
    LOG_INFO("[SCHED] Ready (%s)", _enabled ? "ENABLED" : "DISABLED");
    return true;
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void DosingScheduler::update() {
    if (!_initialized) return;

    uint32_t now = millis();

    // Zawsze sprawdzaj postęp dozowania
    if (_state == SchedulerState::DOSING || _state == SchedulerState::WAITING_PUMP) {
        _checkDosingProgress();
    }

    if (now - _lastUpdateTime < 1000) return;
    _lastUpdateTime = now;

    if (!_enabled) { _state = SchedulerState::SCHED_DISABLED; return; }
    if (systemHalted) { _state = SchedulerState::ERROR; return; }
    if (!rtcController.isReady() || !rtcController.isTimeValid()) return;

    switch (_state) {
        case SchedulerState::IDLE:
        case SchedulerState::CHECKING:
            if (_checkDailyReset()) {
                _state = SchedulerState::DAILY_RESET;
                _performDailyReset();
                _state = SchedulerState::IDLE;
            }
            _state = SchedulerState::CHECKING;
            _checkSchedule();
            if (_state != SchedulerState::DOSING && _state != SchedulerState::WAITING_PUMP)
                _state = SchedulerState::IDLE;
            break;

        case SchedulerState::DOSING:
        case SchedulerState::WAITING_PUMP:
            _checkDosingProgress();
            break;

        case SchedulerState::DAILY_RESET:
            _state = SchedulerState::IDLE;
            break;

        case SchedulerState::ERROR:
            if (!systemHalted) _state = SchedulerState::IDLE;
            break;

        case SchedulerState::SCHED_DISABLED:
            break;
    }
}

// ============================================================================
// ENABLE / DISABLE
// ============================================================================

void DosingScheduler::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    _enabled = enabled;

    SystemState sysState;
    if (framController.readSystemState(&sysState)) {
        sysState.system_enabled = enabled ? 1 : 0;
        framController.writeSystemState(&sysState);
    }

    if (enabled) {
        _state = SchedulerState::IDLE;
        if (rtcController.isReady()) {
            TimeInfo now = rtcController.getTime();
            _lastDay = now.day;
        }
        LOG_INFO("[SCHED] Enabled");
    } else {
        if (_state == SchedulerState::DOSING || _state == SchedulerState::WAITING_PUMP)
            stopCurrentDose();
        _state = SchedulerState::SCHED_DISABLED;
        LOG_INFO("[SCHED] Disabled");
    }
}

// ============================================================================
// DAILY RESET
// ============================================================================

bool DosingScheduler::_checkDailyReset() {
    TimeInfo now = rtcController.getTime();
    if (now.hour != DAILY_RESET_HOUR) return false;

    uint32_t currentUtcDay = (uint32_t)now.year * 366 + (uint32_t)now.month * 31 + now.day;
    SystemState sysState;
    if (framController.readSystemState(&sysState)) {
        if (sysState.last_daily_reset_day == currentUtcDay) return false;
    }
    _lastDay = now.day;
    return true;
}

bool DosingScheduler::_performDailyReset() {
    TimeInfo now = rtcController.getTime();
    LOG_INFO("[SCHED] === DAILY RESET at %02d:%02d UTC ===", now.hour, now.minute);

    uint32_t currentUtcDay = (uint32_t)now.year * 366 + (uint32_t)now.month * 31 + now.day;
    SystemState sysState;
    bool isFirstResetToday = true;
    if (framController.readSystemState(&sysState))
        isFirstResetToday = (sysState.last_daily_reset_day != currentUtcDay);

    _lastDay = now.day;
    _todayEventCount = 0;

    if (isFirstResetToday) {
        LOG_INFO("[SCHED] Resetting daily states...");
        channelManager.resetDailyStates();
    }

    if (framController.readSystemState(&sysState)) {
        sysState.last_daily_reset_day = currentUtcDay;
        framController.writeSystemState(&sysState);
    }

    LOG_INFO("[SCHED] Daily reset complete");
    return true;
}

bool DosingScheduler::forceDailyReset() {
    LOG_INFO("[SCHED] Forced daily reset");
    channelManager.resetDailyStates();
    return _performDailyReset();
}

// ============================================================================
// SCHEDULE CHECKING
// ============================================================================

// Oblicz rzeczywistą godzinę i minutę wykonania dla danego kanału i godziny eventu
void DosingScheduler::_getActualTime(uint8_t channel, uint8_t eventHour,
                                     uint8_t* outHour, uint8_t* outMinute) {
    uint16_t totalMinutes = (uint16_t)CHANNEL_SLOT_MAP[channel] * CHANNEL_OFFSET_MINUTES;
    *outHour   = eventHour + totalMinutes / 60;
    *outMinute = totalMinutes % 60;
}

void DosingScheduler::_checkSchedule() {
    TimeInfo now = rtcController.getTime();
    _lastCheckTime = millis();

    // Iteruj po wszystkich parzystych godzinach eventów
    for (uint8_t eventHour = FIRST_EVENT_HOUR;
         eventHour <= LAST_EVENT_HOUR;
         eventHour += 2) {

        for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
            uint8_t actualHour, actualMinute;
            _getActualTime(ch, eventHour, &actualHour, &actualMinute);

            if (now.hour != actualHour) continue;
            if (now.minute < actualMinute || now.minute >= actualMinute + EVENT_WINDOW_SECONDS / 60) continue;

            // Bitmask sprawdzamy wg eventHour (godziny bazowej)
            if (channelManager.shouldExecuteEvent(ch, eventHour, now.dayOfWeek)) {
                LOG_INFO("[SCHED] Event: CH%d eventH=%02d actual=%02d:%02d now=%02d:%02d",
                              ch, eventHour, actualHour, actualMinute,
                              now.hour, now.minute);
                if (_startDosing(ch, eventHour, channelManager.getNextDoseMl(ch))) return;
            }
        }
    }
}

void DosingScheduler::syncTimeState() {
    if (!rtcController.isReady()) return;
    TimeInfo now = rtcController.getTime();
    uint8_t oldDay = _lastDay;
    _lastDay = now.day;
    if (oldDay != _lastDay)
        LOG_INFO("[SCHED] Time synced: day %d -> %d", oldDay, _lastDay);
}

// ============================================================================
// DOSING EXECUTION
// ============================================================================

bool DosingScheduler::_startDosing(uint8_t channel, uint8_t eventHour, float singleDoseMl) {
    if (channel >= CHANNEL_COUNT) return false;
    if (relayController.isAnyOn()) {
        LOG_INFO("[SCHED] Pump busy, skipping");
        return false;
    }

    const ChannelConfig& activeCfg = channelManager.getActiveConfig(channel);
    uint8_t eventCount = activeCfg.getActiveEventsCount();
    if (eventCount == 0 || activeCfg.daily_dose_ml <= 0 || activeCfg.dosing_rate <= 0) {
        LOG_INFO("[SCHED] CH%d active config not ready, skipping", channel);
        return false;
    }
    uint32_t pumpDurMs  = (uint32_t)((singleDoseMl / activeCfg.dosing_rate) * 1000.0f);
    if (singleDoseMl < MIN_SINGLE_DOSE_ML || pumpDurMs == 0 || pumpDurMs > MAX_PUMP_DURATION_MS) {
        LOG_INFO("[SCHED] CH%d active config invalid (%.2f ml, %lu ms), skipping",
                      channel, singleDoseMl, pumpDurMs);
        return false;
    }

    portENTER_CRITICAL(&_schedulerMux);
    _currentEvent.channel          = channel;
    _currentEvent.event_hour       = eventHour;
    _currentEvent.target_ml        = singleDoseMl;
    _currentEvent.target_duration_ms = pumpDurMs;
    _currentEvent.start_time_ms    = millis();
    _currentEvent.completed        = false;
    _currentEvent.failed           = false;
    portEXIT_CRITICAL(&_schedulerMux);

    LOG_INFO("[SCHED] Starting CH%d: %.2f ml, %lu ms",
                  channel, _currentEvent.target_ml, _currentEvent.target_duration_ms);

    RelayResult res = relayController.turnOn(channel, _currentEvent.target_duration_ms);
    if (res != RelayResult::OK) {
        LOG_INFO("[SCHED] Pump start failed: %s", RelayController::resultToString(res));
        portENTER_CRITICAL(&_schedulerMux);
        _currentEvent.failed   = true;
        _currentEvent.channel  = 255;
        portEXIT_CRITICAL(&_schedulerMux);
        return false;
    }

    _state = SchedulerState::DOSING;
    return true;
}

void DosingScheduler::_checkDosingProgress() {
    if (_currentEvent.channel >= CHANNEL_COUNT) {
        _state = SchedulerState::IDLE;
        return;
    }

    if (!relayController.isChannelOn(_currentEvent.channel)) {
        _completeDosing(true);
        return;
    }

    _state = SchedulerState::WAITING_PUMP;
}

void DosingScheduler::_completeDosing(bool success) {
    portENTER_CRITICAL(&_schedulerMux);
    uint8_t channel   = _currentEvent.channel;
    uint8_t eventHour = _currentEvent.event_hour;
    float   targetMl  = _currentEvent.target_ml;
    uint32_t startTime = _currentEvent.start_time_ms;
    portEXIT_CRITICAL(&_schedulerMux);

    uint32_t actualDuration = millis() - startTime;
    LOG_INFO("[SCHED] CH%d done: %s, %lu ms",
                  channel, success ? "OK" : "FAILED", actualDuration);

    if (success) {
        channelManager.markEventCompleted(channel, eventHour, targetMl);
    } else {
        if (!channelManager.isEventFailed(channel, eventHour))
            channelManager.markEventFailed(channel, eventHour);
    }

    portENTER_CRITICAL(&_schedulerMux);
    if (success) { _todayEventCount++; _currentEvent.completed = true; }
    else         { _currentEvent.failed = true; }
    _currentEvent.channel = 255;
    _state = SchedulerState::IDLE;
    portEXIT_CRITICAL(&_schedulerMux);
}

// ============================================================================
// MANUAL CONTROL
// ============================================================================

bool DosingScheduler::triggerManualDose(uint8_t channel) {
    if (channel >= CHANNEL_COUNT || !_enabled) return false;
    if (relayController.isAnyOn()) return false;

    // Dla manualnego dozowania używamy pierwszej aktywnej godziny eventu kanału
    const ChannelConfig& cfg = channelManager.getActiveConfig(channel);
    uint8_t eventHour = FIRST_EVENT_HOUR;
    for (uint8_t h = FIRST_EVENT_HOUR; h <= LAST_EVENT_HOUR; h += 2) {
        if (cfg.isEventEnabled(h)) { eventHour = h; break; }
    }

    LOG_INFO("[SCHED] Manual trigger CH%d (eventH=%d)", channel, eventHour);
    return _startDosing(channel, eventHour, channelManager.getStandardDoseMl(channel));
}

void DosingScheduler::stopCurrentDose() {
    if (_currentEvent.channel < CHANNEL_COUNT) {
        LOG_INFO("[SCHED] Stopping CH%d", _currentEvent.channel);
        relayController.turnOff(_currentEvent.channel);
        _completeDosing(false);
    }
}

// ============================================================================
// QUERIES
// ============================================================================

uint32_t DosingScheduler::getSecondsToNextEvent() const {
    if (!rtcController.isReady()) return 0;
    TimeInfo now = rtcController.getTime();

    uint32_t nowSec = now.hour * 3600UL + now.minute * 60 + now.second;

    for (uint8_t eventHour = FIRST_EVENT_HOUR; eventHour <= LAST_EVENT_HOUR; eventHour += 2) {
        for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
            if (!channelManager.shouldExecuteEvent(ch, eventHour, now.dayOfWeek)) continue;

            uint8_t actualHour, actualMinute;
            DosingScheduler::_getActualTime(ch, eventHour, &actualHour, &actualMinute);

            uint32_t targetSec = actualHour * 3600UL + actualMinute * 60;
            if (targetSec > nowSec) return targetSec - nowSec;
        }
    }
    return 0;
}

// ============================================================================
// DEBUG
// ============================================================================

void DosingScheduler::printStatus() const {
    Serial.println(F("\n[SCHED] Status:"));
    Serial.printf("  State: %s\n", stateToString(_state));
    Serial.printf("  Enabled: %s\n", _enabled ? "YES" : "NO");
    Serial.printf("  Today events: %d\n", _todayEventCount);

    if (_currentEvent.channel < CHANNEL_COUNT) {
        Serial.printf("  Active: CH%d eventH=%d %.2f ml %lu ms\n",
                      _currentEvent.channel, _currentEvent.event_hour,
                      _currentEvent.target_ml, _currentEvent.target_duration_ms);
    }
    uint32_t nextIn = getSecondsToNextEvent();
    if (nextIn > 0) Serial.printf("  Next event in: %lu sec\n", nextIn);
    else            Serial.println(F("  No more events today"));
}

const char* DosingScheduler::stateToString(SchedulerState state) {
    switch (state) {
        case SchedulerState::IDLE:           return "IDLE";
        case SchedulerState::CHECKING:       return "CHECKING";
        case SchedulerState::DOSING:         return "DOSING";
        case SchedulerState::WAITING_PUMP:   return "WAITING_PUMP";
        case SchedulerState::DAILY_RESET:    return "DAILY_RESET";
        case SchedulerState::ERROR:          return "ERROR";
        case SchedulerState::SCHED_DISABLED: return "DISABLED";
        default:                             return "UNKNOWN";
    }
}

DosingEvent DosingScheduler::getEventSnapshot() const {
    DosingEvent snapshot;
    portENTER_CRITICAL(&_schedulerMux);
    snapshot = _currentEvent;
    portEXIT_CRITICAL(&_schedulerMux);
    return snapshot;
}
