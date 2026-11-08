/**
 * DOZOWNIK - Safety Manager Implementation
 */

#include "safety_manager.h"
#include "buzzer_controller.h"
#include "fram_layout.h"
#include "rtc_controller.h"
#include "fram_controller.h"

// Global instance
SafetyManager safetyManager;

// ============================================================================
// INITIALIZATION
// ============================================================================

void SafetyManager::begin() {
    Serial.println(F("[SAFETY] Initializing..."));
    
    // --- GPIO Setup (NAJWCZEŚNIEJ!) ---
    
    // Master relay - domyślnie OFF (bezpieczny stan)
    pinMode(MASTER_RELAY_PIN, OUTPUT);
    digitalWrite(MASTER_RELAY_PIN, MASTER_RELAY_INACTIVE);
    _masterRelayEnabled = false;
    Serial.println(F("[SAFETY] Master relay: INACTIVE (safe default)"));
    
    // Reset button - input z pull-up
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    
    // Inicjalizuj strukturę błędu
    memset(&_currentError, 0, sizeof(_currentError));
    
    _initialized = true;
    Serial.println(F("[SAFETY] GPIO initialized"));
}

bool SafetyManager::enableIfSafe() {
    if (!_initialized) {
        Serial.println(F("[SAFETY] ERROR: Not initialized!"));
        return false;
    }
    
    // An unreadable error state must not count as "no error"
    if (!isHwFaultAlarm() && !_loadErrorFromFRAM()) {
        latchHwFault(HW_FAULT_FRAM);
    }

    if (isHwFaultAlarm()) {
        _syncHardwareAlarm();
        return false;
    }

    if (_currentError.active_flag) {
        // Błąd aktywny w FRAM - pozostań w trybie bezpiecznym
        _errorActive = true;
        _masterRelayEnabled = false;
        
        Serial.println(F("[SAFETY] *** CRITICAL ERROR ACTIVE FROM FRAM ***"));
        Serial.printf("[SAFETY] Type: %s, Channel: %d\n", 
                      errorTypeToString(_currentError.error_type),
                      _currentError.channel);
        Serial.printf("[SAFETY] Timestamp: %lu\n", _currentError.timestamp);
        Serial.println(F("[SAFETY] Master relay: REMAINS INACTIVE"));
        setBuzzerMode(BuzzerMode::ALARM);
        Serial.println(F("[SAFETY] Buzzer: ACTIVE"));
        Serial.println(F("[SAFETY] Press RESET button for 5s to clear"));
        
        return false;
    }
    
    // Brak błędu - włącz master relay
    _setMasterRelay(true);
    Serial.println(F("[SAFETY] No error in FRAM - Master relay ENABLED"));
    
    return true;
}

// ============================================================================
// MAIN UPDATE LOOP
// ============================================================================

void SafetyManager::update() {
    if (!_initialized) return;
    
    _handleResetButton();

    if (_ackRequested) {
        _ackRequested = false;
        acknowledgeAlarm();
    }

    _syncHardwareAlarm();
}

void SafetyManager::_syncHardwareAlarm() {
    bool hwAlarm = isHwFaultAlarm();
    if (hwAlarm == _hwAlarmOn) return;
    _hwAlarmOn = hwAlarm;

    if (hwAlarm) {
        _setMasterRelay(false);
        setBuzzerMode(BuzzerMode::ALARM);
        Serial.printf("[SAFETY] *** HARDWARE ALARM: %s ***\n", getHwFaultText().c_str());
    } else if (!_errorActive) {
        setBuzzerMode(BuzzerMode::OFF);
        _setMasterRelay(true);
        buzzerOK();
        Serial.println(F("[SAFETY] Hardware alarm cleared - normal operation resumed"));
    }
}

bool SafetyManager::acknowledgeAlarm() {
    bool anything = false;
    if (_errorActive) {
        resetCriticalError();
        anything = true;
    }
    if (ackHwFaults()) anything = true;
    _syncHardwareAlarm();
    return anything;
}

String SafetyManager::getAlarmText() const {
    String text = getHwFaultText();
    if (_errorActive) {
        if (text.length()) text += " | ";
        text += errorTypeToString(_currentError.error_type);
        if (_currentError.channel < CHANNEL_COUNT) {
            text += " CH";
            text += String(_currentError.channel + 1);
        }
    }
    return text;
}

// ============================================================================
// CRITICAL ERROR HANDLING
// ============================================================================

void SafetyManager::triggerCriticalError(CriticalErrorType type,
                                          uint8_t channel,
                                          uint32_t errorData) {
    Serial.println();
    Serial.println(F("+==========================================================+"));
    Serial.println(F("|            *** CRITICAL ERROR TRIGGERED ***              |"));
    Serial.println(F("+==========================================================+"));

    _setMasterRelay(false);
    Serial.println(F("[CRITICAL] Master relay DISABLED immediately!"));

    _takePumpSnapshot();

    _currentError.active_flag  = 1;
    _currentError.error_type   = type;
    _currentError.channel      = channel;
    _currentError.timestamp    = rtcController.getUnixTime();
    _currentError.error_data   = errorData;
    _currentError.total_critical_errors++;
    _currentError.write_count++;

    _saveErrorToFRAM();
    Serial.println(F("[CRITICAL] Error saved to FRAM"));

    _errorActive = true;
    setBuzzerMode(BuzzerMode::ALARM);

    Serial.printf("[CRITICAL] Type: %s (%d)\n", errorTypeToString(type), type);
    Serial.printf("[CRITICAL] Channel: %d\n", channel);
    Serial.printf("[CRITICAL] Pump was running: %d\n", _currentError.pump_was_running);
    Serial.printf("[CRITICAL] Total errors: %d\n", _currentError.total_critical_errors);
    Serial.println(F("[CRITICAL] >>> SYSTEM LOCKED - PRESS RESET FOR 5s <<<"));
    Serial.println();
}

bool SafetyManager::resetCriticalError() {
    if (!_errorActive) {
        Serial.println(F("[SAFETY] No active error to reset"));
        return false;
    }
    
    Serial.println(F(""));
    Serial.println(F("[SAFETY] === CRITICAL ERROR RESET ==="));
    
    // 1. Buzzer stays on while a hardware fault is latched
    if (!isHwFaultAlarm()) setBuzzerMode(BuzzerMode::OFF);
    
    // 2. Aktualizuj strukturę
    _currentError.active_flag = 0;
    _currentError.reset_count++;
    _currentError.last_reset_timestamp = rtcController.getUnixTime();
    _currentError.write_count++;
    
    // 3. Zapisz do FRAM (wyczyść flagę aktywnego błędu)
    _clearErrorInFRAM();
    Serial.println(F("[SAFETY] Error cleared in FRAM"));
    
    // 4. Zresetuj stan
    _errorActive = false;
    
    // 5. Enable master relay and confirm, unless a hardware fault is still latched
    if (!isHwFaultAlarm()) {
        _setMasterRelay(true);
        Serial.println(F("[SAFETY] Master relay RE-ENABLED"));
        buzzerOK();
    }
    
    Serial.println(F("[SAFETY] System unlocked - normal operation resumed"));
    Serial.println(F(""));
    
    return true;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void SafetyManager::_setMasterRelay(bool enabled) {
    digitalWrite(MASTER_RELAY_PIN, enabled ? MASTER_RELAY_ACTIVE : MASTER_RELAY_INACTIVE);
    _masterRelayEnabled = enabled;
    Serial.printf("[SAFETY] Master relay: %s\n", enabled ? "ENABLED" : "DISABLED");
}

void SafetyManager::_handleResetButton() {
    bool buttonPressed = (digitalRead(RESET_BUTTON_PIN) == RESET_BUTTON_ACTIVE);
    unsigned long now = millis();
    
    if (buttonPressed) {
        if (!_buttonWasPressed) {
            // Początek naciśnięcia
            _buttonPressStart = now;
            _buttonWasPressed = true;
            Serial.println(F("[SAFETY] Reset button pressed..."));
        } else {
            // Sprawdź czy przytrzymano wystarczająco długo
            if (!_resetInProgress && (now - _buttonPressStart >= RESET_BUTTON_HOLD_MS)) {
                _resetInProgress = true;
                
                if (isCriticalErrorActive()) {
                    acknowledgeAlarm();
                } else {
                    // Brak błędu - może być użyte do provisioning
                    Serial.println(F("[SAFETY] No error active - button may trigger provisioning"));
                    // Tu można dodać callback do provisioning
                }
            }
        }
    } else {
        // Przycisk puszczony
        if (_buttonWasPressed) {
            if (!_resetInProgress) {
                unsigned long holdTime = now - _buttonPressStart;
                Serial.printf("[SAFETY] Button released after %lu ms (need %d ms)\n",
                              holdTime, RESET_BUTTON_HOLD_MS);
            }
            _buttonWasPressed = false;
            _resetInProgress = false;
        }
    }
}

void SafetyManager::_takePumpSnapshot() {
    // Snapshot stanu wyjść pompy (przekaźniki — LOW = ON)
    uint8_t pumpSnapshot = 0;
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        if (digitalRead(PUMPS_PINS[i]) == LOW) {
            pumpSnapshot |= (1 << i);
        }
    }
    _currentError.pump_was_running = (pumpSnapshot != 0) ? 1 : 0;
}

void SafetyManager::_saveErrorToFRAM() {
    // Oblicz CRC
    _currentError.crc32 = FramController::calculateCRC32(&_currentError, 
                                          sizeof(CriticalErrorState) - sizeof(uint32_t));
    
    // Zapisz do FRAM
    framController.writeBytes(FRAM_ADDR_CRITICAL_ERROR, &_currentError, sizeof(CriticalErrorState));
}

bool SafetyManager::_loadErrorFromFRAM() {
    CriticalErrorState loaded;
    
    if (!framController.readBytes(FRAM_ADDR_CRITICAL_ERROR, &loaded, sizeof(CriticalErrorState))) {
        Serial.println(F("[SAFETY] Failed to read error state from FRAM"));
        memset(&_currentError, 0, sizeof(_currentError));
        return false;
    }
    
    // Walidacja CRC
    uint32_t expectedCrc = FramController::calculateCRC32(&loaded, sizeof(CriticalErrorState) - sizeof(uint32_t));
    if (loaded.crc32 != expectedCrc) {
        // Expected on a freshly formatted FRAM, which holds no error record yet
        Serial.println(F("[SAFETY] Error state CRC mismatch - treating as no error"));
        memset(&_currentError, 0, sizeof(_currentError));
        return true;
    }
    
    _currentError = loaded;
    return true;
}

void SafetyManager::_clearErrorInFRAM() {
    // Zachowaj statystyki, wyczyść tylko flagę aktywnego błędu
    _currentError.active_flag = 0;
    _currentError.crc32 = FramController::calculateCRC32(&_currentError,
                                          sizeof(CriticalErrorState) - sizeof(uint32_t));
    
    framController.writeBytes(FRAM_ADDR_CRITICAL_ERROR, &_currentError, sizeof(CriticalErrorState));
}

// ============================================================================
// STATUS / DEBUG
// ============================================================================

void SafetyManager::printStatus() const {
    Serial.println(F(""));
    Serial.println(F("=== SAFETY SYSTEM STATUS ==="));
    Serial.printf("Master relay: %s\n", _masterRelayEnabled ? "ENABLED" : "DISABLED");
    Serial.printf("Critical error: %s\n", _errorActive ? "ACTIVE" : "None");
    
    if (_errorActive) {
        Serial.printf("  Type: %s (%d)\n",
                      errorTypeToString(_currentError.error_type),
                      _currentError.error_type);
        Serial.printf("  Channel: %d\n", _currentError.channel);
        Serial.printf("  Timestamp: %lu\n", _currentError.timestamp);
        Serial.printf("  Pump was running: %d\n", _currentError.pump_was_running);
    }
    
    Serial.printf("Total errors (history): %d\n", _currentError.total_critical_errors);
    Serial.printf("Reset count: %d\n", _currentError.reset_count);
    Serial.println(F("============================"));
    Serial.println(F(""));
}