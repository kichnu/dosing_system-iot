/**
 * DOZOWNIK - Safety Manager
 *
 * Zarządzanie bezpieczeństwem systemu:
 * - Master relay (GPIO38) - opcjonalne zewnętrzne odcięcie zasilania pomp
 * - Buzzer (GPIO5) - sygnalizacja błędu
 * - Reset button (GPIO4) - reset błędu krytycznego
 * - Persystencja błędów w FRAM
 */

#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "dosing_types.h"

class SafetyManager {
public:
    void begin();
    bool enableIfSafe();
    void update();

    void triggerCriticalError(CriticalErrorType type,
                               uint8_t channel = 255,
                               uint32_t errorData = 0);

    bool resetCriticalError();

    bool isCriticalErrorActive() const { return _errorActive; }
    bool isMasterRelayEnabled() const { return _masterRelayEnabled; }
    CriticalErrorType getErrorType() const { return _currentError.error_type; }
    uint8_t getErrorChannel() const { return _currentError.channel; }
    const CriticalErrorState& getErrorState() const { return _currentError; }

    void printStatus() const;

private:
    bool _initialized = false;
    bool _errorActive = false;
    bool _masterRelayEnabled = false;
    CriticalErrorState _currentError;

    unsigned long _buzzerLastToggle = 0;
    bool _buzzerState = false;

    unsigned long _buttonPressStart = 0;
    bool _buttonWasPressed = false;
    bool _resetInProgress = false;

    void _setMasterRelay(bool enabled);
    void _setBuzzer(bool on);
    void _updateBuzzerPattern();
    void _handleResetButton();
    void _takePumpSnapshot();
    void _saveErrorToFRAM();
    void _loadErrorFromFRAM();
    void _clearErrorInFRAM();
    void _confirmResetBeep();
};

extern SafetyManager safetyManager;

#endif // SAFETY_MANAGER_H
