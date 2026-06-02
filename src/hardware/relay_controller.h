/**
 * DOZOWNIK - Pump Controller
 *
 * Bezpieczne sterowanie pompami perystaltycznymi przez ULN2003AN (Active HIGH).
 * Gwarantuje że tylko jedna pompa pracuje w danym momencie (mutex).
 */

#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "dosing_types.h"

// ============================================================================
// PUMP STATE
// ============================================================================

struct PumpChannelState {
    bool     is_on;
    uint32_t on_since_ms;
    uint32_t total_on_time_ms;
    uint32_t activation_count;
};

enum class RelayResult : uint8_t {
    OK = 0,
    ERROR_INVALID_CHANNEL,
    ERROR_MUTEX_LOCKED,
    ERROR_SYSTEM_HALTED,
    ERROR_ALREADY_ON,
    ERROR_ALREADY_OFF,
    ERROR_TIMEOUT
};

// ============================================================================
// PUMP CONTROLLER CLASS
// ============================================================================

class RelayController {
public:
    void begin();
    void update();

    // Sterowanie
    RelayResult turnOn(uint8_t channel, uint32_t max_duration_ms = 0);
    RelayResult turnOff(uint8_t channel);
    RelayResult turnOffWithDuration(uint8_t channel, uint32_t* actual_duration_ms);

    void allOff();
    void emergencyStop();
    void forceOffImmediate(uint8_t channel);

    // Status
    bool    isAnyOn() const;
    uint8_t getActiveChannel() const;
    bool    isChannelOn(uint8_t channel) const;
    uint32_t getActiveRuntime() const;
    uint32_t getRemainingTime() const;
    const PumpChannelState& getChannelState(uint8_t channel) const;
    uint32_t getTotalRuntime() const;

    // Debug
    void printStatus() const;
    static const char* resultToString(RelayResult result);

private:
    PumpChannelState _channels[CHANNEL_COUNT];

    uint8_t  _activeChannel;
    uint32_t _activeMaxDuration;
    uint32_t _pumpStartTime;
    bool     _initialized;

    void _setPump(uint8_t channel, bool state);
    void _checkTimeout();
};

extern RelayController relayController;

#endif // RELAY_CONTROLLER_H
