#include "node_commands.h"
#include "logging.h"
#include "../config/config.h"
#include "../hardware/safety_manager.h"
#include "../hardware/relay_controller.h"
#include "../hardware/dosing_scheduler.h"

enum : uint8_t { PENDING_NONE = 0, PENDING_STOP, PENDING_RESUME };

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t s_pending = PENDING_NONE;  // last command wins

static void setPending(uint8_t op) {
    portENTER_CRITICAL(&s_mux);
    s_pending = op;
    portEXIT_CRITICAL(&s_mux);
}

const char* cmdResultToString(CmdResult r) {
    switch (r) {
        case CmdResult::OK:     return "OK";
        case CmdResult::HALTED: return "HALTED";
        default:                return "UNKNOWN";
    }
}

static bool resumeBlocked() {
    return systemHalted || safetyManager.isCriticalErrorActive();
}

CmdResult cmdEmergencyStop() {
    // Hard cut first (also covers a stuck pump relay), then every pump relay
    safetyManager.setUserStop(true);
    relayController.allOff();
    setPending(PENDING_STOP);
    LOG_INFO("[CMD] STOP: pumps and master relay off");
    return CmdResult::OK;
}

CmdResult cmdResume() {
    if (resumeBlocked()) {
        LOG_INFO("[CMD] RESUME refused: critical error active");
        return CmdResult::HALTED;
    }
    setPending(PENDING_RESUME);
    LOG_INFO("[CMD] RESUME accepted");
    return CmdResult::OK;
}

bool isDoserStopped() {
    return safetyManager.isUserStopped();
}

void processNodeCommands() {
    portENTER_CRITICAL(&s_mux);
    uint8_t op = s_pending;
    s_pending = PENDING_NONE;
    portEXIT_CRITICAL(&s_mux);

    if (op == PENDING_STOP) {
        // A pump started between the check in turnOn() and allOff() is cut here
        relayController.allOff();
        if (dosingScheduler.isInitialized()) {
            dosingScheduler.stopCurrentDose();
            dosingScheduler.setEnabled(false);
        }
        LOG_INFO("[CMD] STOP applied: scheduler disabled");
    } else if (op == PENDING_RESUME) {
        if (resumeBlocked()) {
            LOG_INFO("[CMD] RESUME dropped: critical error appeared");
            return;
        }
        if (dosingScheduler.isInitialized()) dosingScheduler.setEnabled(true);
        safetyManager.setUserStop(false);
        LOG_INFO("[CMD] RESUME applied: master relay and scheduler on");
    }
}
