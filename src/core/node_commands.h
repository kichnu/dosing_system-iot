#ifndef NODE_COMMANDS_H
#define NODE_COMMANDS_H

// Node commands — one gate for the web handlers and the touch panel (ESP-NOW DS_CMD).
// Every rule is checked here, in the node, not in a GUI.
//
// STOP is an emergency stop for any pump source (scheduled dose, manual dose,
// calibration, hold, air pump): the master relay and all pump relays go off at
// once, in the calling task. The scheduler part (interrupted dose recorded as
// failed, scheduler disabled in FRAM so the stop survives a restart) runs in the
// main loop. STOP is accepted in every state, also with a critical error.
// No buzzer and no error record: it is a user action, not a fault.
//
// RESUME turns the master relay and the scheduler back on; refused (HALTED)
// while a critical error or a hardware fault is active. Doses missed while
// stopped follow the scheduler's usual behavior after re-enabling.

#include <Arduino.h>

enum class CmdResult : uint8_t {
    OK = 0,
    HALTED,     // critical error / hardware fault: clear it on the node first
};

const char* cmdResultToString(CmdResult r);

CmdResult cmdEmergencyStop();
CmdResult cmdResume();

// True from STOP until RESUME (also after a restart)
bool isDoserStopped();

// Main loop, every iteration, before the critical-error early return
void processNodeCommands();

#endif // NODE_COMMANDS_H
