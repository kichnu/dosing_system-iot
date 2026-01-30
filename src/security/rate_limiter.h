#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <Arduino.h>
#include <WiFi.h>

// Inicjalizacja rate limiter
void initRateLimiter();

// Czyszczenie starych danych (wywoływać z loop co 30s)
void updateRateLimiter();

// Czy IP przekroczył limit requestów
bool isRateLimited(IPAddress ip);

// Rejestracja requestu (wywoływać dla każdego requestu)
void recordRequest(IPAddress ip);

// Rejestracja nieudanej próby logowania
void recordFailedLogin(IPAddress ip);

// Czy IP jest zablokowany (za dużo nieudanych logowań)
bool isIPBlocked(IPAddress ip);

#endif // RATE_LIMITER_H
