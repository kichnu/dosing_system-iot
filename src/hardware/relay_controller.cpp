/**
 * DOZOWNIK - Pump Controller Implementation
 *
 * ULN2003AN: Active HIGH — HIGH = pompa ON, LOW = pompa OFF.
 * Mutex gwarantuje że tylko jedna pompa pracuje jednocześnie.
 */

#include "relay_controller.h"
#include "safety_manager.h"

RelayController relayController;

static portMUX_TYPE _pumpMutex = portMUX_INITIALIZER_UNLOCKED;

// ============================================================================
// INITIALIZATION
// ============================================================================

void RelayController::begin() {
    Serial.println(F("[PUMP] Initializing pump controller (ULN2003AN)..."));

    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        pinMode(PUMPS_PINS[i], OUTPUT);
        digitalWrite(PUMPS_PINS[i], LOW);  // OFF (active HIGH — LOW = off)

        _channels[i].is_on = false;
        _channels[i].on_since_ms = 0;
        _channels[i].total_on_time_ms = 0;
        _channels[i].activation_count = 0;

        Serial.printf("  CH%d -> GPIO%d\n", i, PUMPS_PINS[i]);
    }

    _activeChannel    = 255;
    _activeMaxDuration = 0;
    _pumpStartTime    = 0;
    _initialized      = true;

    Serial.println(F("[PUMP] Controller ready"));
}

// ============================================================================
// UPDATE (wywołuj w loop)
// ============================================================================

void RelayController::update() {
    if (!_initialized) return;
    if (_activeChannel < CHANNEL_COUNT) {
        _checkTimeout();
    }
}

void RelayController::_checkTimeout() {
    if (_activeMaxDuration == 0) return;
    uint32_t runtime = millis() - _pumpStartTime;
    if (runtime >= _activeMaxDuration) {
        Serial.printf("[PUMP] CH%d TIMEOUT after %lu ms\n", _activeChannel, runtime);
        turnOff(_activeChannel);
    }
}

// ============================================================================
// TURN ON
// ============================================================================

RelayResult RelayController::turnOn(uint8_t channel, uint32_t max_duration_ms) {
    if (channel >= CHANNEL_COUNT) {
        Serial.printf("[PUMP] ERROR: Invalid channel %d\n", channel);
        return RelayResult::ERROR_INVALID_CHANNEL;
    }

    if (systemHalted) {
        Serial.println(F("[PUMP] ERROR: System halted"));
        return RelayResult::ERROR_SYSTEM_HALTED;
    }

    if (safetyManager.isCriticalErrorActive()) {
        Serial.println(F("[PUMP] ERROR: Critical error active"));
        return RelayResult::ERROR_SYSTEM_HALTED;
    }

    portENTER_CRITICAL(&_pumpMutex);

    if (_activeChannel < CHANNEL_COUNT && _activeChannel != channel) {
        portEXIT_CRITICAL(&_pumpMutex);
        Serial.printf("[PUMP] ERROR: CH%d blocked, CH%d active\n", channel, _activeChannel);
        return RelayResult::ERROR_MUTEX_LOCKED;
    }

    if (_channels[channel].is_on) {
        portEXIT_CRITICAL(&_pumpMutex);
        return RelayResult::ERROR_ALREADY_ON;
    }

    _activeMaxDuration = (max_duration_ms > 0) ? max_duration_ms : MAX_PUMP_DURATION_MS;
    _activeChannel     = channel;

    portEXIT_CRITICAL(&_pumpMutex);

    _setPump(channel, true);
    _channels[channel].is_on            = true;
    _channels[channel].on_since_ms      = millis();
    _channels[channel].activation_count++;
    _pumpStartTime = millis();

    Serial.printf("[PUMP] CH%d ON (max %lu ms)\n", channel, _activeMaxDuration);
    return RelayResult::OK;
}

// ============================================================================
// TURN OFF
// ============================================================================

RelayResult RelayController::turnOff(uint8_t channel) {
    uint32_t duration;
    return turnOffWithDuration(channel, &duration);
}

RelayResult RelayController::turnOffWithDuration(uint8_t channel, uint32_t* actual_duration_ms) {
    if (channel >= CHANNEL_COUNT) {
        if (actual_duration_ms) *actual_duration_ms = 0;
        return RelayResult::ERROR_INVALID_CHANNEL;
    }

    if (!_channels[channel].is_on) {
        if (actual_duration_ms) *actual_duration_ms = 0;
        return RelayResult::ERROR_ALREADY_OFF;
    }

    uint32_t duration = (_pumpStartTime > 0) ? (millis() - _pumpStartTime) : 0;
    if (actual_duration_ms) *actual_duration_ms = duration;

    _setPump(channel, false);
    _channels[channel].total_on_time_ms += duration;
    _channels[channel].is_on = false;

    portENTER_CRITICAL(&_pumpMutex);
    _activeChannel     = 255;
    _activeMaxDuration = 0;
    _pumpStartTime     = 0;
    portEXIT_CRITICAL(&_pumpMutex);

    Serial.printf("[PUMP] CH%d OFF (ran %lu ms)\n", channel, duration);
    return RelayResult::OK;
}

// ============================================================================
// FORCE OFF (bez sprawdzeń — dla błędów)
// ============================================================================

void RelayController::forceOffImmediate(uint8_t channel) {
    if (channel >= CHANNEL_COUNT) return;

    Serial.printf("[PUMP] CH%d FORCE OFF\n", channel);
    _setPump(channel, false);

    portENTER_CRITICAL(&_pumpMutex);
    if (_channels[channel].is_on) {
        uint32_t duration = millis() - _channels[channel].on_since_ms;
        _channels[channel].total_on_time_ms += duration;
    }
    _channels[channel].is_on = false;
    if (_activeChannel == channel) {
        _activeChannel     = 255;
        _activeMaxDuration = 0;
        _pumpStartTime     = 0;
    }
    portEXIT_CRITICAL(&_pumpMutex);
}

// ============================================================================
// EMERGENCY
// ============================================================================

void RelayController::allOff() {
    Serial.println(F("[PUMP] ALL OFF"));
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        _setPump(i, false);
        if (_channels[i].is_on) {
            _channels[i].total_on_time_ms += millis() - _channels[i].on_since_ms;
            _channels[i].is_on = false;
        }
    }
    portENTER_CRITICAL(&_pumpMutex);
    _activeChannel     = 255;
    _activeMaxDuration = 0;
    _pumpStartTime     = 0;
    portEXIT_CRITICAL(&_pumpMutex);
}

void RelayController::emergencyStop() {
    Serial.println(F("[PUMP] !!! EMERGENCY STOP !!!"));
    allOff();
    systemHalted = true;
}

// ============================================================================
// HELPERS
// ============================================================================

void RelayController::_setPump(uint8_t channel, bool state) {
    if (channel >= CHANNEL_COUNT) return;
    // ULN2003AN: HIGH = pompa ON, LOW = pompa OFF
    digitalWrite(PUMPS_PINS[channel], state ? HIGH : LOW);
}

// ============================================================================
// STATUS
// ============================================================================

bool RelayController::isAnyOn() const {
    return _activeChannel < CHANNEL_COUNT;
}

uint8_t RelayController::getActiveChannel() const {
    return _activeChannel;
}

bool RelayController::isChannelOn(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) return false;
    return _channels[channel].is_on;
}

uint32_t RelayController::getActiveRuntime() const {
    if (_activeChannel >= CHANNEL_COUNT || _pumpStartTime == 0) return 0;
    return millis() - _pumpStartTime;
}

uint32_t RelayController::getRemainingTime() const {
    if (_activeChannel >= CHANNEL_COUNT || _activeMaxDuration == 0) return 0;
    if (_pumpStartTime == 0) return _activeMaxDuration;
    uint32_t runtime = millis() - _pumpStartTime;
    return (runtime >= _activeMaxDuration) ? 0 : _activeMaxDuration - runtime;
}

const PumpChannelState& RelayController::getChannelState(uint8_t channel) const {
    static PumpChannelState empty = {false, 0, 0, 0};
    if (channel >= CHANNEL_COUNT) return empty;
    return _channels[channel];
}

uint32_t RelayController::getTotalRuntime() const {
    uint32_t total = 0;
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        total += _channels[i].total_on_time_ms;
        if (_channels[i].is_on && _pumpStartTime > 0)
            total += millis() - _pumpStartTime;
    }
    return total;
}

// ============================================================================
// DEBUG
// ============================================================================

void RelayController::printStatus() const {
    Serial.println(F("[PUMP] Status:"));
    Serial.printf("  Active: %s\n",
                  _activeChannel < CHANNEL_COUNT ? String(_activeChannel).c_str() : "none");
    if (_activeChannel < CHANNEL_COUNT) {
        Serial.printf("  Runtime: %lu / %lu ms\n", getActiveRuntime(), _activeMaxDuration);
    }
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        Serial.printf("  CH%d: %s total=%lu ms count=%lu\n",
                      i, _channels[i].is_on ? "ON" : "OFF",
                      _channels[i].total_on_time_ms, _channels[i].activation_count);
    }
}

const char* RelayController::resultToString(RelayResult result) {
    switch (result) {
        case RelayResult::OK:                    return "OK";
        case RelayResult::ERROR_INVALID_CHANNEL: return "INVALID_CHANNEL";
        case RelayResult::ERROR_MUTEX_LOCKED:    return "MUTEX_LOCKED";
        case RelayResult::ERROR_SYSTEM_HALTED:   return "SYSTEM_HALTED";
        case RelayResult::ERROR_ALREADY_ON:      return "ALREADY_ON";
        case RelayResult::ERROR_ALREADY_OFF:     return "ALREADY_OFF";
        case RelayResult::ERROR_TIMEOUT:         return "TIMEOUT";
        default:                                 return "UNKNOWN";
    }
}
