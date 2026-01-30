#include "rate_limiter.h"
#include "../config/config.h"
#include "../security/auth_manager.h"
#include "../core/logging.h"
#include <map>
#include <vector>
#include <algorithm>

// ============================================================================
// STAŁE KONFIGURACYJNE (z config.h)
// ============================================================================
static const unsigned long RL_WINDOW_MS         = RATE_LIMIT_WINDOW_MS;   // 60000 ms (1 min)
static const size_t        RL_MAX_REQUESTS      = RATE_LIMIT_REQUESTS;    // 100 per window
static const int           RL_MAX_FAILED_LOGINS = MAX_LOGIN_ATTEMPTS;     // 5
static const unsigned long RL_LOCKOUT_MS        = LOGIN_LOCKOUT_MS;       // 300000 ms (5 min)
static const unsigned long RL_CLEANUP_INTERVAL  = 300000;                 // 5 min cykl czyszczenia
static const size_t        RL_MAX_TIMESTAMPS    = 20;                     // Hard limit per IP
static const size_t        RL_MAX_IPS           = 10;                     // Max śledzonych IP

// ============================================================================
// STRUKTURY DANYCH
// ============================================================================
struct RateLimitData {
    std::vector<unsigned long> requestTimes;
    int failedLogins;
    unsigned long blockUntil;
    unsigned long lastRequest;
};

static std::map<String, RateLimitData> rateLimitMap;
static unsigned long lastCleanupTime = 0;

// ============================================================================
// IP STRING CACHE (unika powtarzanych IPAddress::toString())
// ============================================================================
static std::map<uint32_t, String> ipCache;

static String getIPString(IPAddress ip) {
    uint32_t ipUint = (uint32_t)ip;
    auto it = ipCache.find(ipUint);
    if (it != ipCache.end()) {
        return it->second;
    }

    String ipStr = ip.toString();
    ipCache[ipUint] = ipStr;

    // Evict oldest jeśli cache za duży
    if (ipCache.size() > RL_MAX_IPS) {
        ipCache.erase(ipCache.begin());
    }

    return ipStr;
}

// ============================================================================
// INICJALIZACJA
// ============================================================================
void initRateLimiter() {
    rateLimitMap.clear();
    ipCache.clear();
    lastCleanupTime = millis();
    LOG_INFO("Rate limiter initialized (max %zu req/%lus, lockout %lus after %d fails)",
             RL_MAX_REQUESTS, RL_WINDOW_MS / 1000, RL_LOCKOUT_MS / 1000, RL_MAX_FAILED_LOGINS);
}

// ============================================================================
// CZYSZCZENIE OKRESOWE (wywoływać z loop co 30s)
// ============================================================================
void updateRateLimiter() {
    unsigned long now = millis();

    if (now - lastCleanupTime < RL_CLEANUP_INTERVAL) {
        return;
    }
    lastCleanupTime = now;

    size_t removed = 0;
    for (auto it = rateLimitMap.begin(); it != rateLimitMap.end();) {
        // Usuwanie wpisów nieaktywnych dłużej niż interwał czyszczenia
        if (now - it->second.lastRequest > RL_CLEANUP_INTERVAL) {
            it = rateLimitMap.erase(it);
            removed++;
        } else {
            ++it;
        }
    }

    if (removed > 0) {
        LOG_INFO("Rate limiter cleanup: removed %zu stale entries, %zu remaining",
                 removed, rateLimitMap.size());
    }
}

// ============================================================================
// SPRAWDZENIE RATE LIMIT
// ============================================================================
bool isRateLimited(IPAddress ip) {
    // Trusted IP omijają rate limiting
    if (isIPAllowed(ip)) {
        return false;
    }

    String ipStr = getIPString(ip);
    auto it = rateLimitMap.find(ipStr);
    if (it == rateLimitMap.end()) {
        return false;
    }

    RateLimitData& data = it->second;
    unsigned long now = millis();

    // Sprawdzenie blokady IP (lockout po nieudanych logowaniach)
    if (data.blockUntil > now) {
        return true;
    }

    // Czyszczenie wygasłych timestampów z okna
    data.requestTimes.erase(
        std::remove_if(data.requestTimes.begin(), data.requestTimes.end(),
                       [now](unsigned long time) { return now - time > RL_WINDOW_MS; }),
        data.requestTimes.end());

    return data.requestTimes.size() >= RL_MAX_REQUESTS;
}

// ============================================================================
// REJESTRACJA REQUESTU
// ============================================================================
void recordRequest(IPAddress ip) {
    String ipStr = getIPString(ip);
    unsigned long now = millis();

    // Limit śledzonych IP
    if (rateLimitMap.find(ipStr) == rateLimitMap.end() && rateLimitMap.size() >= RL_MAX_IPS) {
        return; // Nie dodawaj nowych IP gdy limit osiągnięty
    }

    RateLimitData& data = rateLimitMap[ipStr];

    // Czyszczenie wygasłych timestampów
    data.requestTimes.erase(
        std::remove_if(data.requestTimes.begin(), data.requestTimes.end(),
                       [now](unsigned long time) { return now - time > RL_WINDOW_MS; }),
        data.requestTimes.end());

    // Hard limit timestampów per IP
    if (data.requestTimes.size() >= RL_MAX_TIMESTAMPS) {
        data.requestTimes.erase(data.requestTimes.begin(),
                                data.requestTimes.begin() + (RL_MAX_TIMESTAMPS / 2));
    }

    data.requestTimes.push_back(now);
    data.lastRequest = now;
}

// ============================================================================
// REJESTRACJA NIEUDANEGO LOGOWANIA
// ============================================================================
void recordFailedLogin(IPAddress ip) {
    // Trusted IP nigdy nie są blokowane
    if (isIPAllowed(ip)) {
        return;
    }

    String ipStr = getIPString(ip);
    unsigned long now = millis();

    RateLimitData& data = rateLimitMap[ipStr];
    data.failedLogins++;
    data.lastRequest = now;

    if (data.failedLogins >= RL_MAX_FAILED_LOGINS) {
        data.blockUntil = now + RL_LOCKOUT_MS;
        data.failedLogins = 0;
        LOG_WARNING("IP %s BLOCKED for %lu seconds after %d failed login attempts",
                    ipStr.c_str(), RL_LOCKOUT_MS / 1000, RL_MAX_FAILED_LOGINS);
    }
}

// ============================================================================
// SPRAWDZENIE BLOKADY IP
// ============================================================================
bool isIPBlocked(IPAddress ip) {
    if (isIPAllowed(ip)) {
        return false;
    }

    String ipStr = getIPString(ip);
    auto it = rateLimitMap.find(ipStr);
    if (it == rateLimitMap.end()) {
        return false;
    }

    return it->second.blockUntil > millis();
}
