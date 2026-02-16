#include "auth_manager.h"
#include "../core/logging.h"
#include "../config/credentials_manager.h"
#include <mbedtls/md.h>
#include <WiFi.h>

void initAuthManager() {
    LOG_INFO("Authentication manager initialized");
    if (areCredentialsLoaded()) {
        LOG_INFO("Using FRAM admin credentials");
    } else {
        LOG_WARNING("NO FRAM CREDENTIALS - Web login will return 503!");
        LOG_WARNING("Use Captive Portal to configure credentials");
    }
}

String hashPassword(const String& password) {
    byte hash[32];
    
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)password.c_str(), password.length());
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    
    String hashString = "";
    for (int i = 0; i < 32; i++) {
        char str[3];
        sprintf(str, "%02x", (int)hash[i]);
        hashString += str;
    }
    
    return hashString;
}

bool areCredentialsAvailable() {
    return areCredentialsLoaded();
}

bool verifyPassword(const String& password) {
    // SECURITY: Block authentication when no FRAM credentials loaded
    if (!areCredentialsLoaded()) {
        LOG_ERROR("Authentication BLOCKED - No FRAM credentials loaded!");
        return false;
    }
    
    // Get stored hash from FRAM
    String expectedHash = String(getAdminPasswordHash());
    
    if (expectedHash.length() == 0 || expectedHash == "NO_AUTH_REQUIRES_FRAM_PROGRAMMING") {
        LOG_ERROR("Invalid admin hash from FRAM");
        return false;
    }
    
    // Hash input password and compare
    String inputHash = hashPassword(password);
    
    bool valid = (inputHash == expectedHash);
    
    if (valid) {
        LOG_INFO("Password verification OK (FRAM credentials)");
    } else {
        LOG_WARNING("Password verification FAILED");
    }
    
    return valid;
}

// ============================================================================
// IP WHITELIST - tylko te IP mają w ogóle dostęp do serwera
// Proxy IP NIE jest w ALLOWED_IPS[] — ma oddzielną ścieżkę (auto-auth)
// ============================================================================
static const IPAddress ALLOWED_IPS[] = {
    IPAddress(192, 168, 2, 10),    // Placeholder - zmień na swoje IP z LAN
    IPAddress(192, 168, 2, 20),    // Placeholder - zmień na swoje IP z LAN
};
static const size_t ALLOWED_IPS_COUNT = sizeof(ALLOWED_IPS) / sizeof(ALLOWED_IPS[0]);

// Trusted proxy - omija CAŁĄ autentykację ESP32 (VPS już uwierzytelnił)
const IPAddress TRUSTED_PROXY_IP(10, 99, 0, 1);  // VPS przez WireGuard tunnel

bool isIPWhitelisted(IPAddress ip) {
    // Trusted proxy jest zawsze na whiteliście
    if (ip == TRUSTED_PROXY_IP) {
        return true;
    }

    for (size_t i = 0; i < ALLOWED_IPS_COUNT; i++) {
        if (ip == ALLOWED_IPS[i]) {
            return true;
        }
    }

    LOG_WARNING("IP %s REJECTED - not on whitelist", ip.toString().c_str());
    return false;
}

bool isTrustedProxy(IPAddress ip) {
    return ip == TRUSTED_PROXY_IP;
}

IPAddress resolveClientIP(AsyncWebServerRequest* request) {
    IPAddress sourceIP = request->client()->remoteIP();
    if (isTrustedProxy(sourceIP) && request->hasHeader("X-Forwarded-For")) {
        String xff = request->getHeader("X-Forwarded-For")->value();
        int comma = xff.indexOf(',');
        String clientStr = (comma > 0) ? xff.substring(0, comma) : xff;
        clientStr.trim();
        IPAddress realIP;
        if (realIP.fromString(clientStr)) {
            LOG_INFO("Proxy request: %s via %s", clientStr.c_str(), sourceIP.toString().c_str());
            return realIP;
        }
        LOG_WARNING("Failed to parse X-Forwarded-For: %s", xff.c_str());
    }
    return sourceIP;
}

bool isIPAllowed(IPAddress ip) {
    // Trusted proxy - bypass authentication (reverse proxy)
    if (isTrustedProxy(ip)) {
        return true;
    }
    return false;
}