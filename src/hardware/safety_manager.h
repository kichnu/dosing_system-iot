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
#include "hw_health.h"

class SafetyManager {
public:
    void begin();
    bool enableIfSafe();
    void update();

    void triggerCriticalError(CriticalErrorType type,
                               uint8_t channel = 255,
                               uint32_t errorData = 0);

    bool resetCriticalError();

    // True while dosing must stay stopped: persisted error or latched hardware fault
    bool isCriticalErrorActive() const { return _errorActive || isHwFaultAlarm(); }

    // Acknowledge from the button or the main loop; clears only what works again
    bool acknowledgeAlarm();
    // Safe from any task: the acknowledgement runs in update()
    void requestAlarmAck() { _ackRequested = true; }
    String getAlarmText() const;
    bool isMasterRelayEnabled() const { return _masterRelayEnabled; }

    // User stop (STOP / RESUME): keeps the master relay off until resumed.
    // Not an alarm: no buzzer, no FRAM error record. Persisted by the scheduler
    // (system_enabled = 0); main applies it at boot before enableIfSafe().
    void setUserStop(bool stopped);
    bool isUserStopped() const { return _userStop; }
    CriticalErrorType getErrorType() const { return _currentError.error_type; }
    uint8_t getErrorChannel() const { return _currentError.channel; }
    const CriticalErrorState& getErrorState() const { return _currentError; }

    void printStatus() const;

private:
    bool _initialized = false;
    bool _errorActive = false;
    bool _masterRelayEnabled = false;
    volatile bool _userStop = false;
    bool _hwAlarmOn = false;
    volatile bool _ackRequested = false;
    CriticalErrorState _currentError;

    unsigned long _buttonPressStart = 0;
    bool _buttonWasPressed = false;
    bool _resetInProgress = false;

    void _setMasterRelay(bool enabled);
    // Master relay on only without a user stop, critical error or hardware fault
    void _enableMasterIfAllowed();
    void _handleResetButton();
    void _takePumpSnapshot();
    void _saveErrorToFRAM();
    bool _loadErrorFromFRAM();
    void _syncHardwareAlarm();
    void _clearErrorInFRAM();
};

extern SafetyManager safetyManager;

#endif // SAFETY_MANAGER_H
