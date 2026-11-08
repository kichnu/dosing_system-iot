

/**
 * DOZOWNIK - RTC Controller Implementation
 */

#include "rtc_controller.h"
#include "hw_health.h"
#include <WiFi.h>
#include <time.h>

// Global instance
RtcController rtcController;

// DS3231 Register addresses
#define DS3231_REG_SECONDS    0x00
#define DS3231_REG_MINUTES    0x01
#define DS3231_REG_HOURS      0x02
#define DS3231_REG_DAY        0x03
#define DS3231_REG_DATE       0x04
#define DS3231_REG_MONTH      0x05
#define DS3231_REG_YEAR       0x06
#define DS3231_REG_CONTROL    0x0E
#define DS3231_REG_STATUS     0x0F
#define DS3231_REG_TEMP_MSB   0x11
#define DS3231_REG_TEMP_LSB   0x12

// Days in month (non-leap year)
static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// ============================================================================
// TIME INFO METHODS
// ============================================================================

uint32_t TimeInfo::toUnixTime() const {
    uint32_t days = 0;
    
    // Years since 1970
    for (uint16_t y = 1970; y < year; y++) {
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    
    // Months this year
    for (uint8_t m = 1; m < month; m++) {
        days += daysInMonth[m - 1];
        if (m == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
            days++; // Leap year February
        }
    }
    
    // Days this month
    days += day - 1;
    
    return days * 86400UL + hour * 3600UL + minute * 60UL + second;
}

void TimeInfo::fromUnixTime(uint32_t timestamp) {
    second = timestamp % 60;
    timestamp /= 60;
    minute = timestamp % 60;
    timestamp /= 60;
    hour = timestamp % 24;
    timestamp /= 24;
    
    // timestamp is now days since 1970-01-01
    uint32_t days = timestamp;
    
    // Calculate day of week (1970-01-01 was Thursday = 3)
    dayOfWeek = (days + 3) % 7;
    
    // Find year
    year = 1970;
    while (true) {
        uint16_t daysInYear = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
        if (days < daysInYear) break;
        days -= daysInYear;
        year++;
    }
    
    // Find month
    month = 1;
    while (true) {
        uint8_t dim = daysInMonth[month - 1];
        if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
            dim = 29;
        }
        if (days < dim) break;
        days -= dim;
        month++;
    }
    
    day = days + 1;
}

void TimeInfo::toString(char* buffer, size_t size) const {
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);
}

void TimeInfo::toTimeString(char* buffer, size_t size) const {
    snprintf(buffer, size, "%02d:%02d", hour, minute);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool RtcController::begin() {
    Serial.println(F("[RTC] Initializing..."));
    
    // Check if RTC is present
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    if (Wire.endTransmission() != 0) {
        Serial.println(F("[RTC] ERROR: DS3231 not found!"));
        _initialized = false;
        _timeValid = false;
        return false;
    }
    
    // Check oscillator stop flag
    uint8_t status = _readRegister(DS3231_REG_STATUS);
    if (status & 0x80) {
        Serial.println(F("[RTC] WARNING: Oscillator was stopped, time invalid"));
        // OSF stays set until a real time is written (setTime), so the next
        // boot cannot take this time for a valid one
        _timeValid = false;
    } else {
        _timeValid = true;
    }
    
    // Read current time for midnight detection
    TimeInfo now = getTime();
    _lastDay = now.day;

    // Initialize NTP tracking
    _ntpSynced = false;
    _lastNtpSyncTime = 0;
    _lastNtpSyncTimestamp = 0;
    
    _initialized = true;
    
    char timeStr[32];
    now.toString(timeStr, sizeof(timeStr));
    Serial.printf("[RTC] Time: %s UTC (valid: %s)\n", 
                  timeStr, _timeValid ? "YES" : "NO");
    
    return true;
}

// ============================================================================
// READ TIME
// ============================================================================

bool RtcController::readTime(TimeInfo* out) {
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(DS3231_REG_SECONDS);
    if (Wire.endTransmission() != 0) return false;

    if (Wire.requestFrom((uint8_t)RTC_I2C_ADDRESS, (uint8_t)7) != 7) return false;

    TimeInfo t;
    t.second = _bcd2dec(Wire.read() & 0x7F);
    t.minute = _bcd2dec(Wire.read());
    t.hour = _bcd2dec(Wire.read() & 0x3F);  // 24h format
    Wire.read();  // Skip day of week from RTC (we calculate it)
    t.day = _bcd2dec(Wire.read());
    t.month = _bcd2dec(Wire.read() & 0x1F);
    t.year = 2000 + _bcd2dec(Wire.read());

    if (t.year < NTP_MIN_VALID_YEAR || t.month < 1 || t.month > 12 || t.day < 1 || t.day > 31 ||
        t.hour > 23 || t.minute > 59 || t.second > 59) {
        return false;
    }

    // Calculate day of week (0=Mon, 6=Sun)
    t.dayOfWeek = _calcDayOfWeek(t.year, t.month, t.day);

    *out = t;
    return true;
}

TimeInfo RtcController::getTime() {
    if (!_initialized) {
        TimeInfo t;
        memset(&t, 0, sizeof(t));
        return t;
    }

    TimeInfo t;
    if (readTime(&t)) {
        _lastGoodTime = t;
        return t;
    }

    // Keep the last good time: a garbage value could look like a new day or a due slot
    noteRtcIoError();
    return _lastGoodTime;
}

bool RtcController::recoverTimeFromNtp() {
    if (!_initialized || WiFi.status() != WL_CONNECTED) return false;

    if (!_ntpRecoveryStarted) {
        configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC,
                   NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
        _ntpRecoveryStarted = true;
    }

    time_t now = 0;
    time(&now);
    if (now <= 1704067200) return false;  // No NTP answer yet (2024-01-01 UTC)
    return setUnixTime((uint32_t)now);
}

uint32_t RtcController::getUnixTime() {
    return getTime().toUnixTime();
}

uint8_t RtcController::getHour() {
    if (!_initialized) return 0;
    return _bcd2dec(_readRegister(DS3231_REG_HOURS) & 0x3F);
}

uint8_t RtcController::getMinute() {
    if (!_initialized) return 0;
    return _bcd2dec(_readRegister(DS3231_REG_MINUTES));
}

uint8_t RtcController::getDayOfWeek() {
    TimeInfo t = getTime();
    return t.dayOfWeek;
}

uint32_t RtcController::getUTCDay() {
    return getUnixTime() / 86400UL;
}

// ============================================================================
// SET TIME
// ============================================================================

bool RtcController::setTime(const TimeInfo& time) {
    if (!_initialized) return false;
    
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(DS3231_REG_SECONDS);
    Wire.write(_dec2bcd(time.second));
    Wire.write(_dec2bcd(time.minute));
    Wire.write(_dec2bcd(time.hour));
    Wire.write(_dec2bcd(time.dayOfWeek + 1));  // DS3231 uses 1-7
    Wire.write(_dec2bcd(time.day));
    Wire.write(_dec2bcd(time.month));
    Wire.write(_dec2bcd(time.year - 2000));
    
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    // The stored time is real again
    _writeRegister(DS3231_REG_STATUS, _readRegister(DS3231_REG_STATUS) & ~0x80);

    _timeValid = true;
    _lastDay = time.day;
    
    char timeStr[32];
    time.toString(timeStr, sizeof(timeStr));
    Serial.printf("[RTC] Time set to: %s\n", timeStr);
    
    return true;
}

bool RtcController::setUnixTime(uint32_t timestamp) {
    TimeInfo t;
    t.fromUnixTime(timestamp);
    return setTime(t);
}

bool RtcController::syncNTP(const char* ntpServer, long gmtOffset) {
    Serial.printf("[RTC] NTP sync from %s...\n", ntpServer);
    
    // Configure NTP
    configTime(gmtOffset, 0, ntpServer);
    
    // Wait for time (max 10 seconds)
    uint32_t start = millis();
    struct tm timeinfo;
    
    while (!getLocalTime(&timeinfo)) {
        if (millis() - start > 10000) {
            Serial.println(F("[RTC] NTP sync timeout!"));
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    // Convert to TimeInfo
    TimeInfo t;
    t.year = timeinfo.tm_year + 1900;
    t.month = timeinfo.tm_mon + 1;
    t.day = timeinfo.tm_mday;
    t.hour = timeinfo.tm_hour;
    t.minute = timeinfo.tm_min;
    t.second = timeinfo.tm_sec;
    t.dayOfWeek = (timeinfo.tm_wday + 6) % 7;  // Convert Sun=0 to Mon=0
    
    return setTime(t);
}

// ============================================================================
// UTILITY
// ============================================================================

float RtcController::getTemperature() {
    if (!_initialized) return 0.0f;
    
    int8_t msb = (int8_t)_readRegister(DS3231_REG_TEMP_MSB);
    uint8_t lsb = _readRegister(DS3231_REG_TEMP_LSB);
    
    return (float)msb + ((lsb >> 6) * 0.25f);
}

bool RtcController::hasMidnightPassed() {
    if (!_initialized || !_timeValid) return false;
    
    TimeInfo now = getTime();
    
    if (now.day != _lastDay) {
        _lastDay = now.day;
        return true;
    }
    
    return false;
}

void RtcController::printTime() {
    TimeInfo t = getTime();
    
    const char* dayNames[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    
    char timeStr[32];
    t.toString(timeStr, sizeof(timeStr));
    
    Serial.printf("[RTC] %s %s UTC\n", dayNames[t.dayOfWeek], timeStr);
    Serial.printf("[RTC] Unix: %lu, UTCDay: %lu\n", t.toUnixTime(), getUTCDay());
    Serial.printf("[RTC] Temperature: %.2f C\n", getTemperature());
    Serial.printf("[RTC] Time valid: %s\n", _timeValid ? "YES" : "NO");
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

uint8_t RtcController::_readRegister(uint8_t reg) {
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission();
    
    Wire.requestFrom((uint8_t)RTC_I2C_ADDRESS, (uint8_t)1);
    return Wire.read();
}

void RtcController::_writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t RtcController::_bcd2dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

uint8_t RtcController::_dec2bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

uint8_t RtcController::_calcDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
    // Zeller's congruence (modified for Monday=0)
    if (month < 3) {
        month += 12;
        year--;
    }
    
    int k = year % 100;
    int j = year / 100;
    
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    
    // Convert from Zeller (Sat=0) to ISO (Mon=0)
    return (h + 5) % 7;
}

// ============================================================================
// NTP SYNC WITH RETRY
// ============================================================================

bool RtcController::syncNTPWithRetry() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[RTC] NTP sync skipped - WiFi not connected"));
        return false;
    }
    
    Serial.println(F("[RTC] Starting NTP synchronization..."));
    Serial.printf("[RTC] Servers: %s, %s, %s\n", NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    // Configure NTP with multiple servers
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, 
               NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    // Wait for valid time with timeout
    uint32_t startMs = millis();
    time_t now = 0;
    struct tm timeinfo;
    int attempts = 0;
    int maxAttempts = NTP_SYNC_TIMEOUT_MS / NTP_SYNC_RETRY_DELAY_MS;
    
    while (attempts < maxAttempts) {
        time(&now);
        
        // Check if timestamp is valid (after 2024)
        if (now > 1704067200) {  // 2024-01-01 00:00:00 UTC
            localtime_r(&now, &timeinfo);
            
            // Additional sanity check
            if (timeinfo.tm_year + 1900 >= NTP_MIN_VALID_YEAR) {
                // Success! Set RTC
                if (setUnixTime((uint32_t)now)) {
                    _ntpSynced = true;
                    _lastNtpSyncTime = millis();
                    _lastNtpSyncTimestamp = (uint32_t)now;
                    
                    Serial.printf("[RTC] NTP sync OK in %lu ms\n", millis() - startMs);
                    Serial.printf("[RTC] UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

                    return true;
                }
            }
        }
        
        delay(NTP_SYNC_RETRY_DELAY_MS);
        attempts++;
        
        // Progress log every 5 seconds
        if (attempts % 10 == 0) {
            Serial.printf("[RTC] NTP waiting... (%d/%d)\n", attempts, maxAttempts);
        }
    }
    
    Serial.printf("[RTC] NTP sync FAILED after %lu ms\n", millis() - startMs);
    return false;
}

bool RtcController::needsResync() const {
    // Never synced - needs sync
    if (!_ntpSynced) {
        return true;
    }
    
    // Check if interval passed
    if (millis() - _lastNtpSyncTime > NTP_RESYNC_INTERVAL_MS) {
        return true;
    }
    
    return false;
}