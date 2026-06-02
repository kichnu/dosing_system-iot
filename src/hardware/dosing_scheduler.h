/**
 * DOZOWNIK - Dosing Scheduler
 *
 * Harmonogram dozowania: parzyste godziny 02,04,...,22.
 * Kanały offsetowane co 15 minut:
 *   CH0=:00  CH1=:15  CH2=:30  CH3=:45
 *   CH4=(+1h):00  CH5=(+1h):15  CH6=(+1h):30  CH7=(+1h):45
 */

#ifndef DOSING_SCHEDULER_H
#define DOSING_SCHEDULER_H

#include <Arduino.h>
#include "config.h"
#include "dosing_types.h"
#include "channel_manager.h"
#include "relay_controller.h"
#include "rtc_controller.h"
#include "fram_controller.h"

enum class SchedulerState : uint8_t {
    IDLE,
    CHECKING,
    DOSING,
    WAITING_PUMP,
    DAILY_RESET,
    ERROR,
    SCHED_DISABLED
};

struct DosingEvent {
    uint8_t  channel;
    uint8_t  event_hour;        // Godzina bazowa eventu (parzysta: 2,4,...,22)
    float    target_ml;
    uint32_t target_duration_ms;
    uint32_t start_time_ms;
    bool     completed;
    bool     failed;
};

class DosingScheduler {
public:
    bool begin();
    void update();

    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled);
    void syncTimeState();

    SchedulerState getState() const { return _state; }
    const DosingEvent& getCurrentEvent() const { return _currentEvent; }
    DosingEvent getEventSnapshot() const;

    bool triggerManualDose(uint8_t channel);
    void stopCurrentDose();
    bool forceDailyReset();

    uint32_t getSecondsToNextEvent() const;
    uint32_t getLastCheckTime() const { return _lastCheckTime; }
    uint16_t getTodayEventCount() const { return _todayEventCount; }

    void printStatus() const;
    static const char* stateToString(SchedulerState state);

private:
    bool _initialized;
    bool _enabled;
    SchedulerState _state;

    DosingEvent _currentEvent;

    uint32_t _lastCheckTime;
    uint32_t _lastUpdateTime;
    uint8_t  _lastDay;
    uint16_t _todayEventCount;

    bool _checkDailyReset();
    bool _performDailyReset();
    void _checkSchedule();
    bool _startDosing(uint8_t channel, uint8_t eventHour);
    void _checkDosingProgress();
    void _completeDosing(bool success);

    // Oblicz rzeczywistą godzinę i minutę dla danego kanału i godziny eventu
    static void _getActualTime(uint8_t channel, uint8_t eventHour,
                               uint8_t* outHour, uint8_t* outMinute);
};

extern DosingScheduler dosingScheduler;

#endif // DOSING_SCHEDULER_H
