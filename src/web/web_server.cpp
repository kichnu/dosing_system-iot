/**
 * DOZOWNIK - Web Server Implementation
 */

#include <ArduinoJson.h>
#include "web_server.h"
#include "html_pages.h"
#include "../config/config.h"
#include "../security/session_manager.h"
#include "../security/auth_manager.h"
#include "../security/rate_limiter.h"
#include "../core/logging.h"
#include "../algorithm/channel_manager.h"
#include "../hardware/dosing_scheduler.h"
#include "../hardware/rtc_controller.h"
#include "../hardware/relay_controller.h"
#include "../hardware/safety_manager.h"
#include "../hardware/fram_controller.h"
#include "../core/user_data.h"

// ============================================================================
// SERVER INSTANCE
// ============================================================================
AsyncWebServer server(80);
bool serverRunning = false;

// ============================================================================
// SESSION HELPERS
// ============================================================================

String getSessionToken(AsyncWebServerRequest* request) {
    if (request->hasHeader("Cookie")) {
        String cookie = request->header("Cookie");
        int idx = cookie.indexOf("session_token=");
        if (idx >= 0) {
            int start = idx + 14;
            int end = cookie.indexOf(";", start);
            if (end < 0) end = cookie.length();
            return cookie.substring(start, end);
        }
    }
    return "";
}

// Sprawdzenie whitelisty IP - 403 przed jakimkolwiek przetwarzaniem
bool checkWhitelist(AsyncWebServerRequest* request) {
    if (!isIPWhitelisted(request->client()->remoteIP())) {
        request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
        return false;
    }
    return true;
}

bool isAuthenticated(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();

    // VPS proxy auto-auth — VPS już uwierzytelnił użytkownika
    if (isTrustedProxy(sourceIP)) {
        return true;
    }

    // Dostęp LAN — pełny łańcuch: whitelist -> rate limit -> sesja
    if (!isIPWhitelisted(sourceIP)) {
        return false;
    }

    if (isRateLimited(sourceIP) || isIPBlocked(sourceIP)) {
        return false;
    }

    recordRequest(sourceIP);

    // Check session cookie
    String token = getSessionToken(request);
    if (token.length() == 0) return false;
    return validateSession(token, sourceIP);
}

// ============================================================================
// ROUTE HANDLERS
// ============================================================================

void handleRoot(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (!isIPWhitelisted(sourceIP) && !isTrustedProxy(sourceIP)) {
        request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
        return;
    }

    IPAddress clientIP = resolveClientIP(request);
    LOG_INFO("[WEB] ROOT request from: %s", clientIP.toString().c_str());
    if (!isAuthenticated(request)) {
        request->redirect("login");
        return;
    }
    const char* html = getDashboardHTML();
    request->send(request->beginResponse(200, "text/html", (const uint8_t*)html, strlen(html)));
}

void handleLogin(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (!isIPWhitelisted(sourceIP) && !isTrustedProxy(sourceIP)) {
        request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
        return;
    }

    if (isAuthenticated(request)) {
        request->redirect("./");
        return;
    }
    const char* html = getLoginHTML();
    request->send(request->beginResponse(200, "text/html", (const uint8_t*)html, strlen(html)));
}

void handleApiLogin(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (!isIPWhitelisted(sourceIP) && !isTrustedProxy(sourceIP)) {
        request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
        return;
    }

    IPAddress clientIP = resolveClientIP(request);

    // Rate limit / block check
    if (isRateLimited(clientIP) || isIPBlocked(clientIP)) {
        request->send(429, "application/json",
            "{\"success\":false,\"error\":\"Too many requests. Try again later.\"}");
        return;
    }

    if (!request->hasParam("password", true)) {
        recordFailedLogin(clientIP);
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing password\"}");
        return;
    }

    // SECURITY: Check if FRAM credentials are configured
    if (!areCredentialsAvailable()) {
        LOG_WARNING("Login attempt with no FRAM credentials from %s",
                    clientIP.toString().c_str());
        request->send(503, "application/json",
            "{\"success\":false,"
            "\"error\":\"System not configured\","
            "\"message\":\"FRAM credentials required. Use Captive Portal to configure.\","
            "\"setup\":\"Hold button 5s at boot -> Connect to DOZOWNIK-SETUP -> Configure credentials\"}");
        return;
    }

    String password = request->getParam("password", true)->value();

    if (verifyPassword(password)) {
        String token = createSession(clientIP);

        AsyncWebServerResponse* response = request->beginResponse(200, "application/json",
            "{\"success\":true}");
        response->addHeader("Set-Cookie", "session_token=" + token + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=1800");
        request->send(response);

        LOG_INFO("[WEB] Login OK from %s", clientIP.toString().c_str());
    } else {
        recordFailedLogin(clientIP);
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Invalid password\"}");
        LOG_INFO("[WEB] Login FAILED from %s", clientIP.toString().c_str());
    }
}

void handleApiVerifyPin(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (!isIPWhitelisted(sourceIP) && !isTrustedProxy(sourceIP)) {
        request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
        return;
    }
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    // Brak danych = sprawdzenie czy PIN jest skonfigurowany (lazy init zwróci domyślny)
    if (len == 0) {
        request->send(200, "application/json", "{\"success\":false}");
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, data, len)) {
        request->send(400, "application/json", "{\"success\":false}");
        return;
    }
    String pin = doc["pin"] | "";
    if (pin.isEmpty()) {
        // Pusty pin = probe: zawsze mamy PIN (lazy init), więc brak "no_password"
        request->send(200, "application/json", "{\"success\":false,\"has_pin\":true}");
        return;
    }
    LockPin lp;
    if (!framController.readLockPin(&lp)) {
        request->send(200, "application/json", "{\"success\":false}");
        return;
    }
    if (strncmp(pin.c_str(), lp.pin, sizeof(lp.pin)) == 0) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(200, "application/json", "{\"success\":false}");
    }
}

void handleApiLogout(AsyncWebServerRequest* request) {
    String token = getSessionToken(request);
    if (token.length() > 0) {
        destroySession(token);
    }
    
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", 
        "{\"success\":true}");
    response->addHeader("Set-Cookie", "session_token=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    request->send(response);
    
    LOG_INFO("[WEB] Logout");
}

// ============================================================================
// API: DOSING STATUS
// ============================================================================

void handleApiDosingStatus(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    if (framBusy) {
        request->send(503, "application/json", "{\"error\":\"FRAM busy, retry\"}");
        return;
    }
    
    JsonDocument doc;
    
    // System status
    doc["systemOk"] = !systemHalted && !safetyManager.isCriticalErrorActive();
    doc["alarm"] = safetyManager.isCriticalErrorActive();
    doc["alarmText"] = safetyManager.getAlarmText();
    doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["schedulerEnabled"] = dosingScheduler.isEnabled();
    doc["notesRev"] = getNotesRev();
    doc["paramsRev"] = getParamsRev();
    
    // Active dosing info
    if (relayController.isAnyOn()) {
        doc["activeChannel"] = relayController.getActiveChannel();
        doc["activeEventHour"] = dosingScheduler.getCurrentEvent().event_hour;
        doc["activeRemainingMs"] = relayController.getRemainingTime();
    } else {
        doc["activeChannel"] = -1;
        doc["activeEventHour"] = -1;
        doc["activeRemainingMs"] = 0;
    }
    
    // Time
    if (rtcController.isReady()) {
        TimeInfo now = rtcController.getTime();
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", now.hour, now.minute);
        doc["time"] = timeStr;
        doc["dayOfWeek"] = now.dayOfWeek;
    }
    
    // Channels array
    JsonArray channels = doc["channels"].to<JsonArray>();
    
for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        const ChannelConfig& cfg = channelManager.getActiveConfig(i);
        const ChannelDailyState& daily = channelManager.getDailyState(i);
        const ChannelCalculated& calc = channelManager.getCalculated(i);
        
        JsonObject ch = channels.add<JsonObject>();
        
        ch["id"] = i;
        ch["events"] = cfg.events_bitmask;
        ch["days"] = cfg.days_bitmask;
        ch["dailyDose"] = cfg.daily_dose_ml;
        ch["dosingRate"] = cfg.dosing_rate;
        ch["enabled"] = (bool)cfg.enabled;

        // Channel label (name)
        ChannelLabel label;
        if (framController.readChannelLabel(i, &label) && label.name[0] != '\0') {
            ch["name"] = label.name;
        } else {
            char defName[8];
            snprintf(defName, sizeof(defName), "CH%d", i);
            ch["name"] = defName;
        }

        // Channel params (min/max doses)
        ChannelParams params;
        if (framController.readChannelParams(i, &params)) {
            ch["minSingleDose"] = params.min_single_dose_ml;
            ch["maxSingleDose"] = params.max_single_dose_ml;
            ch["maxDailyDose"]  = params.max_daily_dose_ml;
        } else {
            ch["minSingleDose"] = MIN_SINGLE_DOSE_ML;
            ch["maxSingleDose"] = MAX_SINGLE_DOSE_ML;
            ch["maxDailyDose"]  = MAX_DAILY_DOSE_ML;
        }
        
        ch["eventsCompleted"] = daily.events_completed;
        ch["eventsFailed"] = daily.events_failed;
        ch["failedToday"] = daily.failed_count;
        ch["todayDosed"] = daily.today_added_ml;
        
        ch["singleDose"] = calc.single_dose_ml;
        ch["pumpDurationMs"] = calc.pump_duration_ms;
        ch["weeklyDose"] = calc.weekly_dose_ml;
        ch["activeEvents"] = calc.active_events_count;
        ch["activeDays"] = calc.active_days_count;
        ch["completedToday"] = calc.completed_today_count;
        ch["isValid"] = calc.is_valid;
        
        // State string for GUI
        const char* stateStr;
        switch (calc.state) {
            case CH_STATE_INACTIVE:   stateStr = "inactive"; break;
            case CH_STATE_INCOMPLETE: stateStr = "incomplete"; break;
            case CH_STATE_INVALID:    stateStr = "invalid"; break;
            case CH_STATE_CONFIGURED: stateStr = "configured"; break;
            default:                  stateStr = "unknown"; break;
        }
        ch["state"] = stateStr;
    }
    
    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// ============================================================================
// API: DOSING CONFIG (POST) - Save channel configuration
// ============================================================================

void handleApiDosingConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    // Accumulate body data
    static String bodyBuffer;
    
    if (index == 0) {
        bodyBuffer = "";
    }
    
    bodyBuffer += String((char*)data).substring(0, len);
    
    // Wait for complete body
    if (index + len < total) {
        return;
    }
    
    // Auth check
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        bodyBuffer = "";
        return;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, bodyBuffer);
    bodyBuffer = "";
    
    if (err) {
        LOG_INFO("[WEB] JSON parse error: %s", err.c_str());
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }
    
    // Validate channel
    if (!doc.containsKey("channel")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing channel\"}");
        return;
    }
    
    uint8_t channel = doc["channel"].as<uint8_t>();
    
    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid channel\"}");
        return;
    }
    
    LOG_INFO("[WEB] Config update CH%d", channel);

    // === Input validation before applying ===
    // Events bitmask: tylko parzyste godziny 2,4,...,22 (maska 0x00555554)
    if (doc["events"].is<uint32_t>()) {
        uint32_t events = doc["events"].as<uint32_t>();
        if (events & ~EVENT_VALID_HOURS_MASK) {
            request->send(400, "application/json",
                "{\"success\":false,\"error\":\"Invalid events bitmask (valid: even hours 2-22)\"}");
            return;
        }
    }

    // Days bitmask: only bits 0-6 valid (Mon-Sun)
    if (doc["days"].is<uint8_t>()) {
        uint8_t days = doc["days"].as<uint8_t>();
        if (days & ~0x7F) {  // bits 7+
            request->send(400, "application/json",
                "{\"success\":false,\"error\":\"Invalid days bitmask (valid: bits 0-6)\"}");
            return;
        }
    }

    // Daily dose: MIN_SINGLE_DOSE_ML - MAX_DAILY_DOSE_ML (air pump: 1-MAX_AIR_PUMP_DURATION_SECONDS s)
    if (doc["dailyDose"].is<float>()) {
        float dose = doc["dailyDose"].as<float>();
        float maxDose = (channel == AIR_PUMP_CHANNEL) ? (float)MAX_AIR_PUMP_DURATION_SECONDS : MAX_DAILY_DOSE_ML;
        if (dose < MIN_SINGLE_DOSE_ML || dose > maxDose) {
            char errMsg[80];
            snprintf(errMsg, sizeof(errMsg),
                "{\"success\":false,\"error\":\"Invalid dailyDose (valid: %.1f-%.0f %s)\"}",
                (float)MIN_SINGLE_DOSE_ML, maxDose,
                (channel == AIR_PUMP_CHANNEL) ? "s" : "ml");
            request->send(400, "application/json", errMsg);
            return;
        }
    }

    // Dosing rate: MIN_DOSING_RATE - MAX_DOSING_RATE
    if (doc["dosingRate"].is<float>()) {
        float rate = doc["dosingRate"].as<float>();
        if (rate < MIN_DOSING_RATE || rate > MAX_DOSING_RATE) {
            char errMsg[80];
            snprintf(errMsg, sizeof(errMsg),
                "{\"success\":false,\"error\":\"Invalid dosingRate (valid: %.4f-%.1f ml/s)\"}",
                (float)MIN_DOSING_RATE, (float)MAX_DOSING_RATE);
            request->send(400, "application/json", errMsg);
            return;
        }
    }

    // Today's total is only updated when the dose ends, so plan with the pump stopped
    if (relayController.isChannelOn(channel)) {
        request->send(409, "application/json", "{\"success\":false,\"error\":\"Pump is running, try again when it stops\"}");
        return;
    }

    // Build atomic config update
    ChannelManager::ConfigUpdate update;

    if (doc["events"].is<uint32_t>()) {
        update.has_events = true;
        update.events = doc["events"].as<uint32_t>();
        Serial.printf("  Events: 0x%06X\n", update.events);
    }

    if (doc["days"].is<uint8_t>()) {
        update.has_days = true;
        update.days = doc["days"].as<uint8_t>();
        Serial.printf("  Days: 0x%02X\n", update.days);
    }

    if (doc["dailyDose"].is<float>()) {
        update.has_dose = true;
        update.dose = doc["dailyDose"].as<float>();
        Serial.printf("  Dose: %.2f ml\n", update.dose);
    }

    if (doc["dosingRate"].is<float>()) {
        update.has_rate = true;
        update.rate = doc["dosingRate"].as<float>();
        Serial.printf("  Rate: %.3f ml/s\n", update.rate);
    }

    // Takes effect immediately and plans the rest of today
    TimeInfo now;
    bool hasTime = rtcController.isReady();
    if (hasTime) now = rtcController.getTime();

    ChannelManager::ConfigSaveResult saveResult;
    ValidationError valErr;
    valErr.has_error = false;
    bool success = channelManager.updateConfig(channel, update, hasTime ? &now : nullptr, &saveResult, &valErr);

    // Save channel name if provided
    if (success && doc["name"].is<const char*>()) {
        const char* name = doc["name"].as<const char*>();
        ChannelLabel label;
        memset(&label, 0, sizeof(label));
        strncpy(label.name, name, sizeof(label.name) - 1);
        framController.writeChannelLabel(channel, &label);
        Serial.printf("  Name: %s\n", label.name);
    }

    if (!saveResult.valid) {
        LOG_INFO("[WEB] Validation: %s", valErr.message);
    }

    // Build response
    JsonDocument resp;
    resp["success"] = success;
    resp["valid"] = saveResult.valid;
    resp["todayRecalculated"] = saveResult.today_recalculated;
    resp["todaySlots"] = saveResult.today_slots;
    resp["todayDose"] = saveResult.today_dose_ml;

    if (!success) {
        resp["error"] = saveResult.valid ? "FRAM write failed" : valErr.message;
    }

    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);

    LOG_INFO("[WEB] Config saved: success=%d valid=%d recalculated=%d slots=%d dose=%.2f",
                  success, saveResult.valid, saveResult.today_recalculated,
                  saveResult.today_slots, saveResult.today_dose_ml);
}

// ============================================================================
// API: CALIBRATE (POST) - Run pump for calibration
// ============================================================================

void handleApiCalibrate(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }
    
    // Get channel from query param: /api/calibrate?channel=0
    if (!request->hasParam("channel")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing channel\"}");
        return;
    }
    
    uint8_t channel = request->getParam("channel")->value().toInt();
    
    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid channel\"}");
        return;
    }
    
    LOG_INFO("[WEB] Calibration request CH%d", channel);
    
    // Check if any pump already running
    if (relayController.isAnyOn()) {
        request->send(409, "application/json", "{\"success\":false,\"error\":\"Pump busy\"}");
        return;
    }
    
    // Run pump for 30 seconds
    const uint32_t CALIB_DURATION_MS = 30000;
    
    RelayResult res = relayController.turnOn(channel, CALIB_DURATION_MS);
    
    if (res != RelayResult::OK) {
        String errJson = "{\"success\":false,\"error\":\"";
        errJson += RelayController::resultToString(res);
        errJson += "\"}";
        request->send(500, "application/json", errJson);
        return;
    }
    
    LOG_INFO("[WEB] Calibration started CH%d for %lu ms", channel, CALIB_DURATION_MS);
    
    // Success response
    JsonDocument resp;
    resp["success"] = true;
    resp["channel"] = channel;
    resp["durationMs"] = CALIB_DURATION_MS;
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
}

// ============================================================================
// API: PUMP HOLD (POST) - Momentary "press and hold" manual pump run.
// No client-visible timer: MAX_PUMP_DURATION_MS (relay_controller) is the
// only backstop if the stop request is never received.
// ============================================================================

void handleApiPumpStart(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }

    if (!request->hasParam("channel")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing channel\"}");
        return;
    }

    uint8_t channel = request->getParam("channel")->value().toInt();

    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid channel\"}");
        return;
    }

    if (relayController.isAnyOn()) {
        request->send(409, "application/json", "{\"success\":false,\"error\":\"Pump busy\"}");
        return;
    }

    const uint32_t PUMP_HOLD_MAX_DURATION_MS = 60000; // backstop if stop request is never received
    RelayResult res = relayController.turnOn(channel, PUMP_HOLD_MAX_DURATION_MS);

    if (res != RelayResult::OK) {
        String errJson = "{\"success\":false,\"error\":\"";
        errJson += RelayController::resultToString(res);
        errJson += "\"}";
        request->send(500, "application/json", errJson);
        return;
    }

    LOG_INFO("[WEB] Pump hold START CH%d", channel);
    request->send(200, "application/json", "{\"success\":true}");
}

void handleApiPumpStop(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }

    if (!request->hasParam("channel")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing channel\"}");
        return;
    }

    uint8_t channel = request->getParam("channel")->value().toInt();

    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid channel\"}");
        return;
    }

    RelayResult res = relayController.turnOff(channel);

    // ALREADY_OFF is fine here — the button may send stop more than once
    // (e.g. mouseup + mouseleave) or after the backstop already fired.
    bool success = (res == RelayResult::OK || res == RelayResult::ERROR_ALREADY_OFF);

    LOG_INFO("[WEB] Pump hold STOP CH%d: %s", channel, success ? "OK" : "FAILED");

    if (!success) {
        String errJson = "{\"success\":false,\"error\":\"";
        errJson += RelayController::resultToString(res);
        errJson += "\"}";
        request->send(500, "application/json", errJson);
        return;
    }

    request->send(200, "application/json", "{\"success\":true}");
}

// ============================================================================
// API: SCHEDULER (POST) - Enable/disable scheduler
// ============================================================================

void handleApiScheduler(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }
    
    // Check for 'enabled' param
    if (!request->hasParam("enabled", true)) {
        // No param = GET current state
        JsonDocument resp;
        resp["enabled"] = dosingScheduler.isEnabled();
        resp["state"] = DosingScheduler::stateToString(dosingScheduler.getState());
        
        String response;
        serializeJson(resp, response);
        request->send(200, "application/json", response);
        return;
    }
    
    // Set enabled state
    String val = request->getParam("enabled", true)->value();
    bool enabled = (val == "true" || val == "1");
    
    dosingScheduler.setEnabled(enabled);
    
    LOG_INFO("[WEB] Scheduler %s", enabled ? "ENABLED" : "DISABLED");
    
    JsonDocument resp;
    resp["success"] = true;
    resp["enabled"] = dosingScheduler.isEnabled();
    resp["state"] = DosingScheduler::stateToString(dosingScheduler.getState());
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
}

// ============================================================================
// API: MANUAL DOSE (POST) - Trigger manual dosing
// ============================================================================

void handleApiManualDose(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }
    
    // Get channel
    if (!request->hasParam("channel", true)) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing channel\"}");
        return;
    }
    
    uint8_t channel = request->getParam("channel", true)->value().toInt();
    
    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid channel\"}");
        return;
    }
    
    LOG_INFO("[WEB] Manual dose request CH%d", channel);
    
    // Check scheduler
    if (!dosingScheduler.isEnabled()) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Scheduler disabled\"}");
        return;
    }
    
    // Check if pump busy
    if (relayController.isAnyOn()) {
        request->send(409, "application/json", "{\"success\":false,\"error\":\"Pump busy\"}");
        return;
    }
    
    // Trigger dose
    if (!dosingScheduler.triggerManualDose(channel)) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to start\"}");
        return;
    }
    
    const ChannelCalculated& calc = channelManager.getCalculated(channel);
    
    JsonDocument resp;
    resp["success"] = true;
    resp["channel"] = channel;
    resp["doseMl"] = calc.single_dose_ml;
    resp["durationMs"] = calc.pump_duration_ms;
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
    
    LOG_INFO("[WEB] Manual dose started CH%d: %.2f ml", channel, calc.single_dose_ml);
}

// ============================================================================
// API: DAILY RESET (POST) - Force daily reset
// ============================================================================

void handleApiDailyReset(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }
    
    LOG_INFO("[WEB] Forcing daily reset...");
    
    bool success = dosingScheduler.forceDailyReset();
    
    JsonDocument resp;
    resp["success"] = success;
    resp["message"] = success ? "Daily reset complete" : "Reset failed";
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
    
    LOG_INFO("[WEB] Daily reset: %s", success ? "OK" : "FAILED");
}

// ============================================================================
// API: ALARM ACKNOWLEDGE
// ============================================================================

void handleApiAlarmAck(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }

    safetyManager.requestAlarmAck();
    request->send(200, "application/json", "{\"success\":true}");
}

void handleHealth(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (!isIPWhitelisted(sourceIP) && !isTrustedProxy(sourceIP)) {
        request->send(403, "application/json", "{\"error\":\"Forbidden\"}");
        return;
    }

    String json = "{";
    json += "\"status\":\"ok\",";
    json += "\"device_name\":\"" + String(DEVICE_ID) + "\",";
    json += "\"uptime\":" + String(millis()) + ",";
    // status stays "ok" (availability contract with the VPS); faults go here
    json += "\"alarm\":" + String(safetyManager.isCriticalErrorActive() ? "true" : "false") + ",";
    json += "\"alarm_text\":\"" + safetyManager.getAlarmText() + "\"";
    json += "}";

    request->send(200, "application/json", json);
}

void handleNotFound(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (!isIPWhitelisted(sourceIP) && !isTrustedProxy(sourceIP)) {
        request->send(403, "text/plain", "Forbidden");
        return;
    }
    request->send(404, "text/plain", "Not Found");
}

// ============================================================================
// API: SHARED NOTES / PARAM LOG
// GET returns the whole structure plus its revision. Changes are single-item
// POSTs (form-encoded) through core/user_data — the same operations the touch
// panel uses, so no client can overwrite another client's changes.
// ============================================================================

static void sendUserDataResult(AsyncWebServerRequest* request, UserDataResult res, uint16_t rev) {
    JsonDocument doc;
    doc["success"] = (res == UserDataResult::OK);
    if (res != UserDataResult::OK) doc["error"] = userDataResultToString(res);
    doc["rev"] = rev;
    String response;
    serializeJson(doc, response);
    int code = 200;
    if (res == UserDataResult::FRAM_ERROR || res == UserDataResult::BUSY) code = 503;
    else if (res != UserDataResult::OK) code = 400;
    request->send(code, "application/json", response);
}

// Integer form parameter; false when missing or not a number in [0, maxVal].
static bool getUintParam(AsyncWebServerRequest* request, const char* name, long maxVal, uint8_t* out) {
    if (!request->hasParam(name, true)) return false;
    String v = request->getParam(name, true)->value();
    if (v.length() == 0) return false;
    for (size_t i = 0; i < v.length(); i++) if (!isDigit(v[i])) return false;
    long n = v.toInt();
    if (n < 0 || n > maxVal) return false;
    *out = (uint8_t)n;
    return true;
}

static bool getFloatParam(AsyncWebServerRequest* request, const char* name, float* out) {
    if (!request->hasParam(name, true)) return false;
    String v = request->getParam(name, true)->value();
    v.replace(',', '.');
    v.trim();
    if (v.length() == 0) return false;
    char* end = nullptr;
    float f = strtof(v.c_str(), &end);
    if (!end || *end != '\0') return false;
    *out = f;
    return true;
}

static void sendBadRequest(AsyncWebServerRequest* request) {
    request->send(400, "application/json", "{\"success\":false,\"error\":\"BAD_REQUEST\"}");
}

void handleApiNotesGet(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    SharedNotes notes;
    UserDataResult res = notesRead(&notes);
    if (res != UserDataResult::OK) {
        sendUserDataResult(request, res, getNotesRev());
        return;
    }

    JsonDocument doc;
    doc["rev"] = getNotesRev();
    JsonArray notesArr = doc["notes"].to<JsonArray>();
    for (int i = 0; i < NOTE_COUNT; i++) {
        notesArr.add((notes.notes[i].flags & 1) ? notes.notes[i].text : "");
    }
    JsonArray idxArr = doc["ch_note_idx"].to<JsonArray>();
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        idxArr.add(notes.ch_note_idx[i]);   // NOTE_NONE (255) = no note
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// POST /api/note-set  slot, text (empty = clear)
void handleApiNoteSet(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t slot;
    if (!getUintParam(request, "slot", NOTE_COUNT - 1, &slot) || !request->hasParam("text", true)) { sendBadRequest(request); return; }
    String text = request->getParam("text", true)->value();
    UserDataResult res = noteSet(slot, text.c_str());
    if (res == UserDataResult::OK) LOG_INFO("[NOTES] Slot %d set (%d B)", slot + 1, text.length());
    sendUserDataResult(request, res, getNotesRev());
}

// POST /api/note-select  channel, slot (255 = no note)
void handleApiNoteSelect(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t ch, slot;
    if (!getUintParam(request, "channel", CHANNEL_COUNT - 1, &ch) || !getUintParam(request, "slot", 255, &slot)) { sendBadRequest(request); return; }
    sendUserDataResult(request, noteSelect(ch, slot), getNotesRev());
}

void handleApiParamLogGet(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    ParamLog* log = (ParamLog*)malloc(sizeof(ParamLog));
    if (!log) {
        request->send(503, "application/json", "{\"error\":\"Out of memory\"}");
        return;
    }
    UserDataResult res = paramLogRead(log);
    if (res != UserDataResult::OK) {
        free(log);
        sendUserDataResult(request, res, getParamsRev());
        return;
    }

    JsonDocument doc;
    doc["rev"]        = getParamsRev();
    doc["head"]       = log->head;
    doc["count"]      = log->count;
    doc["tmpl_count"] = log->tmpl_count;

    JsonArray tArr = doc["templates"].to<JsonArray>();
    for (int i = 0; i < PARAM_TMPL_COUNT; i++) {
        JsonObject t = tArr.add<JsonObject>();
        t["name"]  = log->templates[i].name;
        t["unit"]  = log->templates[i].unit;
        t["flags"] = log->templates[i].flags;
        t["channel_mask"] = log->templates[i].channel_mask;
    }

    JsonArray rArr = doc["ring"].to<JsonArray>();
    for (int i = 0; i < PARAM_RING_SIZE; i++) {
        JsonObject r = rArr.add<JsonObject>();
        r["tmpl_idx"]  = log->ring[i].tmpl_idx;
        r["channel"]   = log->ring[i].channel;
        r["value"]     = log->ring[i].value;
        r["timestamp"] = log->ring[i].timestamp;
        r["flags"]     = log->ring[i].flags;
    }

    free(log);

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// POST /api/param-add  tmpl, channel (255 = template's first channel), value
void handleApiParamAdd(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t tmpl, ch;
    float value;
    if (!getUintParam(request, "tmpl", PARAM_TMPL_COUNT - 1, &tmpl) || !getUintParam(request, "channel", 255, &ch)
        || !getFloatParam(request, "value", &value)) { sendBadRequest(request); return; }
    sendUserDataResult(request, paramAdd(tmpl, ch, value), getParamsRev());
}

// POST /api/param-edit  rec, value
void handleApiParamEdit(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t rec;
    float value;
    if (!getUintParam(request, "rec", PARAM_RING_SIZE - 1, &rec) || !getFloatParam(request, "value", &value)) { sendBadRequest(request); return; }
    sendUserDataResult(request, paramEdit(rec, value), getParamsRev());
}

// POST /api/param-delete  rec
void handleApiParamDelete(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t rec;
    if (!getUintParam(request, "rec", PARAM_RING_SIZE - 1, &rec)) { sendBadRequest(request); return; }
    sendUserDataResult(request, paramDelete(rec), getParamsRev());
}

// POST /api/param-template-create  name, unit, channel -> slot
void handleApiParamTemplateCreate(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t ch;
    if (!getUintParam(request, "channel", CHANNEL_COUNT - 1, &ch) || !request->hasParam("name", true)) { sendBadRequest(request); return; }
    String name = request->getParam("name", true)->value();
    String unit = request->hasParam("unit", true) ? request->getParam("unit", true)->value() : String("");
    name.trim();
    unit.trim();
    uint8_t slot = 0;
    UserDataResult res = paramTemplateCreate(name.c_str(), unit.c_str(), ch, &slot);
    if (res != UserDataResult::OK) { sendUserDataResult(request, res, getParamsRev()); return; }
    JsonDocument doc;
    doc["success"] = true;
    doc["rev"] = getParamsRev();
    doc["slot"] = slot;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// POST /api/param-template-assign  tmpl, channel, assigned (0|1)
void handleApiParamTemplateAssign(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) { request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}"); return; }
    uint8_t tmpl, ch, on;
    if (!getUintParam(request, "tmpl", PARAM_TMPL_COUNT - 1, &tmpl) || !getUintParam(request, "channel", CHANNEL_COUNT - 1, &ch)
        || !getUintParam(request, "assigned", 1, &on)) { sendBadRequest(request); return; }
    sendUserDataResult(request, paramTemplateAssign(tmpl, ch, on == 1), getParamsRev());
}

// ============================================================================
// API: EVENT LOG — GET
// Zwraca pełny stan EventLog: 75 slotów ringa notatnika zdarzeń
// ============================================================================

void handleApiEventLogGet(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }
    if (framBusy) {
        request->send(503, "application/json", "{\"error\":\"FRAM busy\"}");
        return;
    }

    EventLog* log = (EventLog*)malloc(sizeof(EventLog));
    if (!log) {
        request->send(503, "application/json", "{\"error\":\"Out of memory\"}");
        return;
    }
    framController.readEventLog(log);  // zerowy blok jeśli CRC fail — OK

    JsonDocument doc;
    doc["head"]  = log->head;
    doc["count"] = log->count;

    JsonArray eArr = doc["entries"].to<JsonArray>();
    for (int i = 0; i < 75; i++) {
        JsonObject e = eArr.add<JsonObject>();
        e["timestamp"] = log->entries[i].timestamp;
        e["text"]      = log->entries[i].text;
        e["flags"]     = log->entries[i].flags;
    }

    free(log);

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// ============================================================================
// API: EVENT LOG — POST
// Przyjmuje pełny stan EventLog i zapisuje do FRAM
// ============================================================================

void handleApiEventLogPost(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    static String elBodyBuffer;
    if (index == 0) elBodyBuffer = "";
    elBodyBuffer += String((char*)data).substring(0, len);
    if (index + len < total) return;

    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        elBodyBuffer = "";
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, elBodyBuffer) != DeserializationError::Ok) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        elBodyBuffer = "";
        return;
    }
    elBodyBuffer = "";

    EventLog* log = (EventLog*)malloc(sizeof(EventLog));
    if (!log) {
        request->send(503, "application/json", "{\"success\":false,\"error\":\"Out of memory\"}");
        return;
    }
    memset(log, 0, sizeof(EventLog));

    log->head  = (uint8_t)(doc["head"]  | 0);
    log->count = (uint8_t)(doc["count"] | 0);
    if (log->head >= 75)  log->head = 0;
    if (log->count > 75)  log->count = 75;

    JsonArray eArr = doc["entries"].as<JsonArray>();
    for (int i = 0; i < 75 && i < (int)eArr.size(); i++) {
        JsonObject e = eArr[i];
        const char* text = e["text"] | "";
        strncpy(log->entries[i].text, text, 160);
        log->entries[i].text[160] = '\0';
        log->entries[i].timestamp = (uint32_t)(e["timestamp"] | 0);
        log->entries[i].flags     = (uint8_t)(e["flags"] | 0);
    }

    bool ok = framController.writeEventLog(log);
    free(log);

    request->send(200, "application/json",
        ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"FRAM write failed\"}");
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void initWebServer() {
    LOG_INFO("[WEB] Initializing web server...");
    
    // Init dependencies
    initSessionManager();
    initRateLimiter();
    initAuthManager();
    
    // === PAGE ROUTES ===
    server.on("/", HTTP_GET, handleRoot);
    server.on("/login", HTTP_GET, handleLogin);
    
    // === API ROUTES ===
    server.on("/api/health", HTTP_GET, handleHealth);
    server.on("/api/alarm-ack", HTTP_POST, handleApiAlarmAck);
    server.on("/api/login", HTTP_POST, handleApiLogin);
    server.on("/api/verify-pin", HTTP_POST, [](AsyncWebServerRequest* r){}, NULL, handleApiVerifyPin);
    server.on("/api/logout", HTTP_POST, handleApiLogout);
    server.on("/api/dosing-status", HTTP_GET, handleApiDosingStatus); 
    server.on("/api/dosing-config", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, handleApiDosingConfig);
    server.on("/api/calibrate", HTTP_POST, handleApiCalibrate);
    server.on("/api/pump-start", HTTP_POST, handleApiPumpStart);
    server.on("/api/pump-stop", HTTP_POST, handleApiPumpStop);
    server.on("/api/scheduler", HTTP_POST, handleApiScheduler);
    server.on("/api/manual-dose", HTTP_POST, handleApiManualDose);
    server.on("/api/daily-reset", HTTP_POST, handleApiDailyReset);

    // === SHARED NOTES API ===
    server.on("/api/notes", HTTP_GET, handleApiNotesGet);
    server.on("/api/note-set", HTTP_POST, handleApiNoteSet);
    server.on("/api/note-select", HTTP_POST, handleApiNoteSelect);

    // === PARAM LOG API ===
    server.on("/api/paramlog", HTTP_GET, handleApiParamLogGet);
    server.on("/api/param-add", HTTP_POST, handleApiParamAdd);
    server.on("/api/param-edit", HTTP_POST, handleApiParamEdit);
    server.on("/api/param-delete", HTTP_POST, handleApiParamDelete);
    server.on("/api/param-template-create", HTTP_POST, handleApiParamTemplateCreate);
    server.on("/api/param-template-assign", HTTP_POST, handleApiParamTemplateAssign);

    // === EVENT LOG API ===
    server.on("/api/eventlog", HTTP_GET, handleApiEventLogGet);
    server.on("/api/eventlog", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, handleApiEventLogPost);

    // === PUMP MONITOR (Edge Impulse — stub, future implementation) ===
    server.on("/api/pump-monitor-status", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isAuthenticated(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        // Placeholder — przyszła integracja z Edge Impulse przez UART2
        request->send(200, "application/json",
            "{\"monitor_active\":false,\"channels\":[]}");
    });

    // === 404 HANDLER ===
    server.onNotFound(handleNotFound);
    
    // === START SERVER ===
    server.begin();
    serverRunning = true;
    
    LOG_INFO("[WEB] Server started on port 80");
    LOG_INFO("[WEB] Dashboard: http://%s/", WiFi.localIP().toString().c_str());
}

bool isWebServerRunning() {
    return serverRunning;
}