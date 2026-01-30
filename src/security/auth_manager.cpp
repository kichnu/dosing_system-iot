#include "auth_manager.h"
#include "../core/logging.h"
#include "../config/credentials_manager.h"
#include <mbedtls/md.h>
#include <WiFi.h>

void initAuthManager() {
    LOG_INFO("Authentication manager initialized");
    LOG_INFO("Using %s credentials", areCredentialsLoaded() ? "FRAM" : "fallback");
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

bool verifyPassword(const String& password) {
    // Check if FRAM credentials loaded
    if (!areCredentialsLoaded()) {
        LOG_WARNING("No FRAM credentials - using fallback password 'admin123'");
        // Fallback for testing without FRAM
        String testHash = hashPassword("admin123");
        String inputHash = hashPassword(password);
        return (inputHash == testHash);
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

bool isIPAllowed(IPAddress ip) {
    // Trusted IPs - bypass authentication (reverse proxy)
    static const IPAddress trustedIPs[] = {
        IPAddress(10, 99, 0, 1)   // VPS przez WireGuard tunnel
    };

    for (const auto& trusted : trustedIPs) {
        if (ip == trusted) {
            Serial.printf("[AUTH] Trusted IP: %s\n", ip.toString().c_str());
            return true;
        }
    }
    return false;
}