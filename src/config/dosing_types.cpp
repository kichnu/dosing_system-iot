/**
 * DOZOWNIK - Data Types Implementation
 */

#include "dosing_types.h"

// ============================================================================
// STRING CONVERSIONS
// ============================================================================

const char* channelStateToString(ChannelState state) {
    switch (state) {
        case CH_STATE_INACTIVE:   return "INACTIVE";
        case CH_STATE_INCOMPLETE: return "INCOMPLETE";
        case CH_STATE_INVALID:    return "INVALID";
        case CH_STATE_CONFIGURED: return "CONFIGURED";
        case CH_STATE_PENDING:    return "PENDING";
        default:                  return "UNKNOWN";
    }
}

const char* errorTypeToString(CriticalErrorType error) {
    switch (error) {
        case ERROR_NONE:         return "NONE";
        case ERROR_PUMP_TIMEOUT: return "PUMP_TIMEOUT";
        case ERROR_FRAM_FAILURE: return "FRAM_FAILURE";
        case ERROR_RTC_FAILURE:  return "RTC_FAILURE";
        case ERROR_UNKNOWN:      return "UNKNOWN";
        default:                 return "UNDEFINED";
    }
}

// ============================================================================
// TIME UTILITIES
// ============================================================================

uint8_t getDayOfWeek(uint32_t unixTimestamp) {
    // Unix epoch (1970-01-01) was Thursday
    // 0 = Monday, 6 = Sunday (ISO 8601)
    uint32_t days = unixTimestamp / 86400UL;
    return (days + 3) % 7;
}

uint8_t getHourUTC(uint32_t unixTimestamp) {
    return (unixTimestamp % 86400UL) / 3600UL;
}

uint32_t getUTCDay(uint32_t unixTimestamp) {
    return unixTimestamp / 86400UL;
}