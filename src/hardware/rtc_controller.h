
/**
 * DOZOWNIK - RTC Controller
 * 
 * Obsługa zegara czasu rzeczywistego DS3231M.
 * Synchronizacja NTP gdy dostępne WiFi.
 */

#ifndef RTC_CONTROLLER_H
#define RTC_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// ============================================================================
// TIME STRUCTURE (prostsza niż DateTime z RTClib)
// ============================================================================

struct TimeInfo {
    uint16_t year;      // 2000-2099
    uint8_t  month;     // 1-12
    uint8_t  day;       // 1-31
    uint8_t  hour;      // 0-23
    uint8_t  minute;    // 0-59
    uint8_t  second;    // 0-59
    uint8_t  dayOfWeek; // 0=Monday, 6=Sunday (ISO 8601)
    
    /**
     * Konwersja do Unix timestamp
     */
    uint32_t toUnixTime() const;
    
    /**
     * Ustawienie z Unix timestamp
     */
    void fromUnixTime(uint32_t timestamp);
    
    /**
     * Format do stringa "YYYY-MM-DD HH:MM:SS"
     */
    void toString(char* buffer, size_t size) const;
    
    /**
     * Format do stringa "HH:MM"
     */
    void toTimeString(char* buffer, size_t size) const;
};

// ============================================================================
// RTC CONTROLLER CLASS
// ============================================================================

class RtcController {
public:
    /**
     * Inicjalizacja - sprawdza RTC, odczytuje czas
     */
    bool begin();
    
    /**
     * Czy RTC działa poprawnie
     */
    bool isReady() const { return _initialized; }
    
    /**
     * Czy czas jest ustawiony (nie jest domyślny)
     */
    bool isTimeValid() const { return _timeValid; }
    
    // --- Odczyt czasu ---
    
    /**
     * Pobierz aktualny czas UTC
     */
    TimeInfo getTime();

    /**
     * Read the DS3231 and check the result is a plausible time
     * @return false on a bus error or implausible values (*out is then undefined)
     */
    bool readTime(TimeInfo* out);

    /**
     * Non-blocking NTP recovery of a lost time: starts SNTP once, then uses its
     * result as soon as it arrives. For the hardware check, which runs in loop().
     */
    bool recoverTimeFromNtp();
    
    /**
     * Pobierz Unix timestamp UTC
     */
    uint32_t getUnixTime();
    
    /**
     * Pobierz aktualną godzinę UTC (0-23)
     */
    uint8_t getHour();
    
    /**
     * Pobierz aktualną minutę (0-59)
     */
    uint8_t getMinute();
    
    /**
     * Pobierz dzień tygodnia (0=Mon, 6=Sun)
     */
    uint8_t getDayOfWeek();
    
    /**
     * Pobierz UTC day (dni od 1970-01-01)
     */
    uint32_t getUTCDay();
    
    // --- Ustawianie czasu ---
    
    /**
     * Ustaw czas z TimeInfo
     */
    bool setTime(const TimeInfo& time);
    
    /**
     * Ustaw czas z Unix timestamp
     */
    bool setUnixTime(uint32_t timestamp);
    
    /**
     * Synchronizuj z NTP (wymaga WiFi)
     */
    bool syncNTP(const char* ntpServer = "pool.ntp.org", long gmtOffset = 0);
    
    // --- Utility ---

        /**
     * Synchronizuj z NTP z retry i wieloma serwerami
     * @return true jeśli synchronizacja udana
     */
    bool syncNTPWithRetry();
    
    /**
     * Czy potrzebna resynchronizacja (minął interwał)
     */
    bool needsResync() const;
    
    /**
     * Czy NTP było zsynchronizowane od startu
     */
    bool isNtpSynced() const { return _ntpSynced; }
    
    /**
     * Pobierz czas ostatniej synchronizacji NTP (millis)
     */
    uint32_t getLastNtpSyncTime() const { return _lastNtpSyncTime; }
    
    /**
     * Odczytaj temperaturę z DS3231 (ma wbudowany sensor)
     */
    float getTemperature();
    
    /**
     * Sprawdź czy minęła północ od ostatniego sprawdzenia
     */
    bool hasMidnightPassed();
    
    /**
     * Debug print
     */
    void printTime();

private:
    bool _initialized;
    bool _timeValid;
    uint8_t _lastDay;  // Midnight detection
    TimeInfo _lastGoodTime;  // Returned by getTime() when a read fails
    bool _ntpRecoveryStarted;
     // NTP sync tracking
    bool _ntpSynced;                // Czy udana sync od startu
    uint32_t _lastNtpSyncTime;      // millis() ostatniej sync
    uint32_t _lastNtpSyncTimestamp; // Unix timestamp ostatniej sync
    
    /**
     * Odczyt rejestru RTC
     */
    uint8_t _readRegister(uint8_t reg);
    
    /**
     * Zapis rejestru RTC
     */
    void _writeRegister(uint8_t reg, uint8_t value);
    
    /**
     * BCD to decimal
     */
    static uint8_t _bcd2dec(uint8_t bcd);
    
    /**
     * Decimal to BCD
     */
    static uint8_t _dec2bcd(uint8_t dec);
    
    /**
     * Oblicz dzień tygodnia
     */
    static uint8_t _calcDayOfWeek(uint16_t year, uint8_t month, uint8_t day);
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

extern RtcController rtcController;

#endif // RTC_CONTROLLER_H