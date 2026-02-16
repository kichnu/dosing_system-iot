#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

void initAuthManager();
bool isIPAllowed(IPAddress ip);
bool isIPWhitelisted(IPAddress ip);
bool isTrustedProxy(IPAddress ip);
IPAddress resolveClientIP(AsyncWebServerRequest* request);
bool verifyPassword(const String& password);
String hashPassword(const String& password);
bool areCredentialsAvailable();

#endif
