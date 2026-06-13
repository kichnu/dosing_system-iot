#include "ap_handlers.h"
#include "credentials_validator.h"
#include "../core/logging.h"
#include "../crypto/fram_encryption.h"
#include "../hardware/fram_controller.h"
#include "../config/credentials_manager.h"

void handleConfigureSubmit(AsyncWebServerRequest *request, JsonVariant &json) {
    LOG_INFO("Configuration submission from: %s", request->client()->remoteIP().toString().c_str());
    
    // Parse JSON body
    JsonObject jsonObj = json.as<JsonObject>();
    
    // Extract fields
    String deviceName = jsonObj["device_name"] | "";
    String wifiSSID = jsonObj["wifi_ssid"] | "";
    String wifiPassword = jsonObj["wifi_password"] | "";
    String adminPassword = jsonObj["admin_password"] | "";
    String lockPin = jsonObj["lock_pin"] | "1234";

    LOG_INFO("Received credentials:");
    LOG_INFO("  Device Name: %s", deviceName.c_str());
    LOG_INFO("  WiFi SSID: %s", wifiSSID.c_str());
    // LOG_INFO("  VPS URL: %s", vpsURL.c_str());
    
    // Validate all credentials
    ValidationResult validation = prov_validateAllCredentials(
        deviceName, wifiSSID, wifiPassword
    );
    
    if (!validation.valid) {
        LOG_ERROR("Validation failed: %s - %s", 
                  validation.errorField.c_str(), 
                  validation.errorMessage.c_str());
        
        // Send error response
        DynamicJsonDocument responseDoc(512);
        responseDoc["success"] = false;
        responseDoc["error"] = validation.errorMessage;
        responseDoc["field"] = validation.errorField;
        
        String response;
        serializeJson(responseDoc, response);
        request->send(400, "application/json", response);
        return;
    }
    
    LOG_INFO("✓ Validation passed");
    
    // Build DeviceCredentials structure
    DeviceCredentials creds;
    creds.device_name = deviceName;
    creds.wifi_ssid = wifiSSID;
    creds.wifi_password = wifiPassword;
    creds.admin_password = adminPassword;

    // Encrypt credentials
    FRAMCredentials framCreds;
    if (!encryptCredentials(creds, framCreds)) {
        LOG_ERROR("Failed to encrypt credentials!");
        
        DynamicJsonDocument responseDoc(512);
        responseDoc["success"] = false;
        responseDoc["error"] = "Failed to encrypt credentials";
        
        String response;
        serializeJson(responseDoc, response);
        request->send(500, "application/json", response);
        return;
    }
    
    LOG_INFO("✓ Credentials encrypted");
    
    // Write to FRAM
    if (!writeCredentialsToFRAM(framCreds)) {
        LOG_ERROR("Failed to write credentials to FRAM!");
        
        DynamicJsonDocument responseDoc(512);
        responseDoc["success"] = false;
        responseDoc["error"] = "Failed to save credentials to FRAM";
        
        String response;
        serializeJson(responseDoc, response);
        request->send(500, "application/json", response);
        return;
    }
    
    LOG_INFO("✓ Credentials saved to FRAM");
    
    // Verify write
    if (!verifyCredentialsInFRAM()) {
        LOG_ERROR("FRAM verification failed after write!");
        
        DynamicJsonDocument responseDoc(512);
        responseDoc["success"] = false;
        responseDoc["error"] = "FRAM verification failed";
        
        String response;
        serializeJson(responseDoc, response);
        request->send(500, "application/json", response);
        return;
    }
    
    LOG_INFO("✓ FRAM verification passed");

    // Zapisz Lock PIN (walidacja: 4-8 cyfr, domyślnie "1234")
    {
        bool pinValid = lockPin.length() >= 4 && lockPin.length() <= 8;
        for (char c : lockPin) { if (!isdigit(c)) { pinValid = false; break; } }
        if (!pinValid) lockPin = "1234";
        LockPin lp;
        memset(&lp, 0, sizeof(lp));
        strncpy(lp.pin, lockPin.c_str(), sizeof(lp.pin) - 1);
        framController.writeLockPin(&lp);
        LOG_INFO("✓ Lock PIN saved: %s", lockPin.c_str());
    }

    // SUCCESS!
    LOG_INFO("=== CONFIGURATION SAVED SUCCESSFULLY ===");
    LOG_INFO("Device configured as: %s", deviceName.c_str());
    LOG_INFO("Manual restart required to boot in production mode");
    
    // Send success response
    DynamicJsonDocument responseDoc(512);
    responseDoc["success"] = true;
    responseDoc["message"] = "Configuration saved successfully! Please restart the device manually.";
    responseDoc["device_name"] = deviceName;
    
    String response;
    serializeJson(responseDoc, response);
    request->send(200, "application/json", response);
    
    // TODO ETAP 4: Connection tests will be added here before save
}