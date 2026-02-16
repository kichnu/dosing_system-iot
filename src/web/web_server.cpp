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
#include "../hardware/fram_controller.h"

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
    Serial.printf("[WEB] ROOT request from: %s\n", clientIP.toString().c_str());
    if (!isAuthenticated(request)) {
        request->redirect("login");
        return;
    }
    request->send(200, "text/html", getDashboardHTML());
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
    request->send(200, "text/html", getLoginHTML());
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

        Serial.printf("[WEB] Login OK from %s\n", clientIP.toString().c_str());
    } else {
        recordFailedLogin(clientIP);
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Invalid password\"}");
        Serial.printf("[WEB] Login FAILED from %s\n", clientIP.toString().c_str());
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
    
    Serial.println(F("[WEB] Logout"));
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
    doc["systemOk"] = !systemHalted;
    doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["schedulerEnabled"] = dosingScheduler.isEnabled();
    
    // Active dosing info
    if (relayController.isAnyOn()) {
        doc["activeChannel"] = relayController.getActiveChannel();
        doc["activeEventHour"] = dosingScheduler.getCurrentEvent().hour;
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
        const ChannelConfig& active = channelManager.getActiveConfig(i);
        const ChannelConfig& pending = channelManager.getPendingConfig(i);
        const ChannelDailyState& daily = channelManager.getDailyState(i);
        const ChannelCalculated& calc = channelManager.getCalculated(i);
        
        // Use pending config if has changes, otherwise active
        const ChannelConfig& cfg = pending.has_pending ? pending : active;
        
        JsonObject ch = channels.add<JsonObject>();
        
        ch["id"] = i;
        ch["events"] = cfg.events_bitmask;
        ch["days"] = cfg.days_bitmask;
        ch["dailyDose"] = cfg.daily_dose_ml;
        ch["dosingRate"] = cfg.dosing_rate;
        ch["enabled"] = cfg.enabled ? true : false;
        
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
        
        ch["hasPending"] = pending.has_pending ? true : false;
        
        // State string for GUI
        const char* stateStr;
        switch (calc.state) {
            case CH_STATE_INACTIVE:   stateStr = "inactive"; break;
            case CH_STATE_INCOMPLETE: stateStr = "incomplete"; break;
            case CH_STATE_INVALID:    stateStr = "invalid"; break;
            case CH_STATE_CONFIGURED: stateStr = "configured"; break;
            case CH_STATE_PENDING:    stateStr = "pending"; break;
            default:                  stateStr = "unknown"; break;
        }
        ch["state"] = stateStr;

                // Container volume
        const ContainerVolume& vol = channelManager.getContainerVolume(i);
        ch["containerMl"] = vol.getContainerMl();
        ch["remainingMl"] = vol.getRemainingMl();
        ch["remainingPct"] = vol.getRemainingPercent();
        ch["lowVolume"] = vol.isLowVolume();
        ch["daysRemaining"] = channelManager.getDaysRemaining(i);

        // Dosed tracker (total dosed since last reset)
        ch["totalDosedMl"] = channelManager.getTotalDosed(i);
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
        Serial.printf("[WEB] JSON parse error: %s\n", err.c_str());
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
    
    Serial.printf("[WEB] Config update CH%d\n", channel);

    // === Input validation before applying ===
    // Events bitmask: only bits 1-23 valid (hour 0 reserved for daily reset)
    if (doc["events"].is<uint32_t>()) {
        uint32_t events = doc["events"].as<uint32_t>();
        if (events & ~0x00FFFFFE) {  // bits 24+ or bit 0
            request->send(400, "application/json",
                "{\"success\":false,\"error\":\"Invalid events bitmask (valid: bits 1-23)\"}");
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

    // Daily dose: MIN_SINGLE_DOSE_ML - MAX_DAILY_DOSE_ML
    if (doc["dailyDose"].is<float>()) {
        float dose = doc["dailyDose"].as<float>();
        if (dose < MIN_SINGLE_DOSE_ML || dose > MAX_DAILY_DOSE_ML) {
            request->send(400, "application/json",
                "{\"success\":false,\"error\":\"Invalid dailyDose (valid: 1.0-500.0 ml)\"}");
            return;
        }
    }

    // Dosing rate: MIN_DOSING_RATE - MAX_DOSING_RATE
    if (doc["dosingRate"].is<float>()) {
        float rate = doc["dosingRate"].as<float>();
        if (rate < MIN_DOSING_RATE || rate > MAX_DOSING_RATE) {
            request->send(400, "application/json",
                "{\"success\":false,\"error\":\"Invalid dosingRate (valid: 0.1-5.0 ml/s)\"}");
            return;
        }
    }

    // Build atomic config update (prevents partial writes race condition)
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

    // Apply all changes atomically
    bool success = channelManager.updatePendingConfigBatch(channel, update);
    
    // Validate config
    ValidationError valErr;
    bool valid = channelManager.validateConfig(channel, &valErr);
    
    if (!valid) {
        Serial.printf("[WEB] Validation: %s\n", valErr.message);
    }
    
    // Build response
    JsonDocument resp;
    resp["success"] = success;
    resp["valid"] = valid;
    resp["hasPending"] = channelManager.hasPendingChanges(channel);
    
    if (!valid && valErr.has_error) {
        resp["validationError"] = valErr.message;
    }
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
    
    Serial.printf("[WEB] Config saved: success=%d valid=%d pending=%d\n", 
                  success, valid, channelManager.hasPendingChanges(channel));
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
    
    Serial.printf("[WEB] Calibration request CH%d\n", channel);
    
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
    
    Serial.printf("[WEB] Calibration started CH%d for %lu ms\n", channel, CALIB_DURATION_MS);
    
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
    
    Serial.printf("[WEB] Scheduler %s\n", enabled ? "ENABLED" : "DISABLED");
    
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
    
    Serial.printf("[WEB] Manual dose request CH%d\n", channel);
    
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
    
    Serial.printf("[WEB] Manual dose started CH%d: %.2f ml\n", channel, calc.single_dose_ml);
}

// ============================================================================
// API: DAILY RESET (POST) - Force daily reset
// ============================================================================

void handleApiDailyReset(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        return;
    }
    
    Serial.println(F("[WEB] Forcing daily reset..."));
    
    bool success = dosingScheduler.forceDailyReset();
    
    JsonDocument resp;
    resp["success"] = success;
    resp["message"] = success ? "Daily reset complete" : "Reset failed";
    resp["pendingApplied"] = !channelManager.hasAnyPendingChanges();
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
    
    Serial.printf("[WEB] Daily reset: %s\n", success ? "OK" : "FAILED");
}

// ============================================================================
// API: CONTAINER VOLUME - Get container status
// ============================================================================

void handleApiContainerVolumeGet(AsyncWebServerRequest* request) {
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    if (framBusy) {
        request->send(503, "application/json", "{\"error\":\"FRAM busy, retry\"}");
        return;
    }
    
    // Get channel from query param: /api/container-volume?channel=0
    if (!request->hasParam("channel")) {
        request->send(400, "application/json", "{\"error\":\"Missing channel\"}");
        return;
    }
    
    uint8_t channel = request->getParam("channel")->value().toInt();
    
    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"error\":\"Invalid channel\"}");
        return;
    }
    
    const ContainerVolume& vol = channelManager.getContainerVolume(channel);
    
    JsonDocument doc;
    doc["channel"] = channel;
    doc["container_ml"] = vol.getContainerMl();
    doc["remaining_ml"] = vol.getRemainingMl();
    doc["remaining_pct"] = vol.getRemainingPercent();
    doc["low_warning"] = vol.isLowVolume();
    doc["days_remaining"] = channelManager.getDaysRemaining(channel);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// ============================================================================
// API: CONTAINER VOLUME - Set container capacity (POST with JSON body)
// ============================================================================

void handleApiContainerVolumeSet(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    static String bodyBuffer;
    
    if (index == 0) {
        bodyBuffer = "";
    }
    
    bodyBuffer += String((char*)data).substring(0, len);
    
    if (index + len < total) {
        return;
    }
    
    if (!isAuthenticated(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
        bodyBuffer = "";
        return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, bodyBuffer);
    bodyBuffer = "";
    
    if (err) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("channel") || !doc.containsKey("container_ml")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing channel or container_ml\"}");
        return;
    }
    
    uint8_t channel = doc["channel"].as<uint8_t>();
    float container_ml = doc["container_ml"].as<float>();
    
    if (channel >= CHANNEL_COUNT) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid channel\"}");
        return;
    }
    
    if (container_ml < CONTAINER_MIN_ML || container_ml > CONTAINER_MAX_ML) {
        char errMsg[64];
        snprintf(errMsg, sizeof(errMsg), 
                 "{\"success\":false,\"error\":\"Container must be %d-%d ml\"}", 
                 CONTAINER_MIN_ML, CONTAINER_MAX_ML);
        request->send(400, "application/json", errMsg);
        return;
    }
    
    Serial.printf("[WEB] Setting container CH%d to %.1f ml\n", channel, container_ml);
    
    bool success = channelManager.setContainerCapacity(channel, container_ml);
    
    const ContainerVolume& vol = channelManager.getContainerVolume(channel);
    
    JsonDocument resp;
    resp["success"] = success;
    resp["channel"] = channel;
    resp["container_ml"] = vol.getContainerMl();
    resp["remaining_ml"] = vol.getRemainingMl();
    resp["remaining_pct"] = vol.getRemainingPercent();
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
}

// ============================================================================
// API: REFILL - Reset remaining to container capacity
// ============================================================================

void handleApiRefill(AsyncWebServerRequest* request) {
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
    
    Serial.printf("[WEB] Refill request CH%d\n", channel);
    
    bool success = channelManager.refillContainer(channel);
    
    const ContainerVolume& vol = channelManager.getContainerVolume(channel);
    
    JsonDocument resp;
    resp["success"] = success;
    resp["channel"] = channel;
    resp["remaining_ml"] = vol.getRemainingMl();
    resp["container_ml"] = vol.getContainerMl();
    resp["message"] = success ? "Container refilled" : "Refill failed";
    
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
    
    Serial.printf("[WEB] Refill CH%d: %s (%.1f ml)\n",
                  channel, success ? "OK" : "FAILED", vol.getRemainingMl());
}

// ============================================================================
// API: RESET DOSED TRACKER - Reset total dosed since reset
// ============================================================================

void handleApiResetDosed(AsyncWebServerRequest* request) {
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

    Serial.printf("[WEB] Reset dosed tracker request CH%d\n", channel);

    bool success = channelManager.resetDosedTracker(channel);

    JsonDocument resp;
    resp["success"] = success;
    resp["channel"] = channel;
    resp["totalDosedMl"] = 0;
    resp["message"] = success ? "Dosed tracker reset" : "Reset failed";

    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);

    Serial.printf("[WEB] Reset dosed CH%d: %s\n", channel, success ? "OK" : "FAILED");
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
    json += "\"uptime\":" + String(millis());
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
// INITIALIZATION
// ============================================================================

void initWebServer() {
    Serial.println(F("[WEB] Initializing web server..."));
    
    // Init dependencies
    initSessionManager();
    initRateLimiter();
    initAuthManager();
    
    // === PAGE ROUTES ===
    server.on("/", HTTP_GET, handleRoot);
    server.on("/login", HTTP_GET, handleLogin);
    
    // === API ROUTES ===
    server.on("/api/health", HTTP_GET, handleHealth);
    server.on("/api/login", HTTP_POST, handleApiLogin);
    server.on("/api/logout", HTTP_POST, handleApiLogout);
    server.on("/api/dosing-status", HTTP_GET, handleApiDosingStatus); 
    server.on("/api/dosing-config", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, handleApiDosingConfig);
    server.on("/api/calibrate", HTTP_POST, handleApiCalibrate);
    server.on("/api/scheduler", HTTP_POST, handleApiScheduler);
    server.on("/api/manual-dose", HTTP_POST, handleApiManualDose);
    server.on("/api/daily-reset", HTTP_POST, handleApiDailyReset);

    // === CONTAINER VOLUME API ===
    server.on("/api/container-volume", HTTP_GET, handleApiContainerVolumeGet);
    server.on("/api/container-volume", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL, handleApiContainerVolumeSet);
    server.on("/api/refill", HTTP_POST, handleApiRefill);
    server.on("/api/reset-dosed", HTTP_POST, handleApiResetDosed);

    // === 404 HANDLER ===
    server.onNotFound(handleNotFound);
    
    // === START SERVER ===
    server.begin();
    serverRunning = true;
    
    Serial.println(F("[WEB] Server started on port 80"));
    Serial.printf("[WEB] Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
}

bool isWebServerRunning() {
    return serverRunning;
}