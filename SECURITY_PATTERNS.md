# Security Patterns Reference

Zestawienie mechanizmow bezpieczenstwa zastosowanych w projekcie ESP32-C3 Water Top-Off.
Referencja na potrzeby implementacji w innych projektach embedded/IoT.

---

## 1. Kontrola dostepu sieciowego (IP whitelist)

| Element | Implementacja |
|---|---|
| Whitelist | Tablica `ALLOWED_IPS[]` w `config.cpp` |
| Sprawdzenie | `isIPAllowed(IPAddress ip)` w `auth_manager.cpp` - iteracja po tablicy |
| Egzekwowanie | `checkAuthentication()` w `web_server.cpp` - odrzucenie przed jakimkolwiek przetwarzaniem |
| Zakres | Wszystkie endpointy wlacznie z `/login`, 404 handler |
| Trusted proxy | Oddzielny hardcoded IP (WireGuard), omija cala autentykacje |

```
Zapytanie -> TrustedProxy? -> Whitelist? -> RateLimit? -> Sesja? -> Dostep
              bypass all       403           429          401       200
```

## 2. Autentykacja haslem (SHA-256)

| Element | Implementacja |
|---|---|
| Hashing | `hashPassword()` w `auth_manager.cpp` - mbedtls SHA-256 |
| Weryfikacja | `verifyPassword()` - porownanie hex string hasha z wartoscia z FRAM |
| Przechowywanie | Hash w FRAM (szyfrowany AES-256), nigdy plaintext |
| Blokada bez FRAM | `areCredentialsLoaded()` = false -> wszystkie loginy odrzucone (503) |

## 3. Sesje (token + IP binding)

| Element | Implementacja |
|---|---|
| Token | 32 znaki hex, `random(0,16)` w petli, `session_manager.cpp` |
| Powiazanie z IP | `session.ip == ip` sprawdzane przy kazdej walidacji |
| Cookie | `session_token=<val>; Path=/; HttpOnly; Max-Age=1800` |
| Timeout | 30 min nieaktywnosci (`SESSION_TIMEOUT_MS`), `lastActivity` aktualizowane |
| Limity | Max 10 sesji globalnie, max 3 per IP |
| Czyszczenie | `updateSessionManager()` w loop - usuwa wygasle sesje |
| Prealokacja | `vector.reserve(MAX_TOTAL_SESSIONS)` - zapobiega fragmentacji heap |

## 4. Rate limiting i blokowanie IP

| Element | Implementacja |
|---|---|
| Struktura | `RateLimitData` per IP: timestamps, failedAttempts, blockUntil |
| Limit zapytan | 5 req/s per IP (`MAX_REQUESTS_PER_SECOND`) w oknie 1s |
| Blokada | 10 nieudanych prob -> blokada na 60s (`BLOCK_DURATION_MS`) |
| Bypass | Whitelisted IP omijaja rate limit i blokade |
| Cleanup | Co 5 min usuwanie wpisow starszych niz 5 min |
| Hard limit | Max 20 timestampow per IP w wektorze (zapobiega OOM) |
| Cache | `ipStringCache` max 10 wpisow - unika powtarzanych `ip.toString()` |

## 5. Szyfrowanie FRAM (AES-256-CBC)

| Element | Implementacja |
|---|---|
| Algorytm | AES-256-CBC, wlasna implementacja w `crypto/aes.cpp` |
| Klucz | SHA-256 z `device_name + SALT + SEED` (`fram_encryption.cpp`) |
| IV | 8 bajtow losowych rozszerzonych do 16 (`random() ^ micros()`) |
| Padding | PKCS7 |
| Zerowanie pamieci | Volatile pointer pattern po uzyciu (`secureZeroMemory`) |
| Weryfikacja | `verifyCredentialsInFRAM()` - decrypt + sprawdzenie pol po zapisie |

## 6. Walidacja danych wejsciowych

| Pole | Reguly | Plik |
|---|---|---|
| Device name | 3-32 zn., alfanumeryczne + `_-` | `credentials_validator.cpp` |
| WiFi SSID | 1-32 zn., niepuste | `credentials_validator.cpp` |
| WiFi password | 8-64 zn. (wymog WPA2) | `credentials_validator.cpp` |
| Admin password | 8-64 zn. | `credentials_validator.cpp` |
| VPS token | 1-64 zn. | `credentials_validator.cpp` |
| VPS URL | 0-100 zn., prefix `http(s)://`, min 3 slashe | `fram_encryption.cpp` |
| volume_per_second | 0.1-20.0 float | `web_handlers.cpp` |
| available volume | 100-10000 int | `web_handlers.cpp` |
| fill water max | 100-10000 int | `web_handlers.cpp` |

## 7. Bezpieczenstwo fizyczne

| Element | Implementacja |
|---|---|
| Provisioning | GPIO 8 przytrzymany 5s podczas boot -> Captive Portal |
| Feedback | Beep na GPIO 2 (10ms HIGH / 100ms LOW) potwierdzajacy detekcje |
| AP | SSID `ESP32-WATER-SETUP`, haslo `setup12345`, max 4 klientow |
| Sesja AP | Timeout 10 min nieaktywnosci |
| Reset z bledu | Krotkie nacisniecie przycisku GPIO 8 w stanie ERROR |
| Brak auto-restart | Po zapisie credentiali - wymaga fizycznego power cycle |

## 8. Mechanizmy auto-recovery

| Mechanizm | Parametry | Plik |
|---|---|---|
| System auto-enable | 30 min po disable | `config.cpp` |
| 24h auto-restart | `millis() > 86400000` -> `ESP.restart()` | `main.cpp` |
| WiFi reconnect | Co 30s jesli rozlaczony | `wifi_manager.cpp` |
| NTP retry | Co godzine, 3 serwery po IP (bez DNS) | `rtc_controller.cpp` |
| Session cleanup | W loop, usuwa wygasle (>30min) | `session_manager.cpp` |
| Rate limit cleanup | Co 5 min, usuwa stare wpisy | `rate_limiter.cpp` |
| Safe pump stop | Przed restart/disable - `stopPump()` | `main.cpp`, `water_algorithm.cpp` |

## 9. Ochrona pamieci (ESP32 heap)

| Element | Implementacja |
|---|---|
| Sesje | `vector.reserve(10)`, hard limit 10 globalnie |
| Rate limit timestamps | Hard limit 20 per IP, eraze 10 najstarszych |
| IP cache | Max 10 wpisow, FIFO |
| Cykle pompy (RAM) | Max 50 w `todayCycles`, FIFO |
| FRAM ring buffer | Max ~200 cyklow, nadpisywanie najstarszych |
| Alokacja z `nothrow` | `new(std::nothrow)` w `fram_encryption.cpp` |
| `framBusy` flag | Blokuje HTTP read FRAM podczas zapisu |

## 10. Logowanie zdarzen bezpieczenstwa

| Zdarzenie | Poziom |
|---|---|
| Udane logowanie | LOG_INFO |
| Nieudane logowanie | LOG_WARNING |
| IP spoza whitelist | LOG_WARNING |
| Blokada IP (10 prob) | LOG_WARNING |
| Tworzenie/usuwanie sesji | LOG_INFO |
| Ladowanie credentiali z FRAM | LOG_INFO |
| Brak credentiali w FRAM | LOG_ERROR |
| Operacje szyfrowania | LOG_INFO |
| NTP sync | LOG_INFO |
| WiFi connect/disconnect | LOG_INFO |

Logi wychodza na Serial (115200 baud). Brak persystencji logow do pamieci nieulotnej.

---

## Znane luki i ograniczenia

| Luka | Opis |
|---|---|
| Brak TLS | HTTP only - ruch widoczny w sieci WiFi |
| Slaby PRNG tokenow | `random()` z `millis() ^ micros()` - niska entropia |
| Brak CSRF | POST endpointy nie weryfikuja origin/tokenu |
| Brak SameSite cookie | Cookie podatne na cross-site request |
| Timing attack na haslo | `String ==` nie jest constant-time |
| Hardcoded whitelist | Zmiana wymaga rekompilacji firmware |
| Hardcoded AP password | `setup12345` w firmware - kazdy moze wejsc w provisioning |
| IV z `random()` | Nie jest kryptograficznie bezpieczne |
| Klucz z device name | Znajomosc nazwy urzadzenia + stalych = mozliwosc odtworzenia klucza |
| Brak persystencji logow | Audit trail istnieje tylko w RAM do restartu |





● Checklist: SECURITY_PATTERNS.md vs DOZOWNIK                                                                                                                                
                                                                                                                                                                             
  1. Kontrola dostępu sieciowego (IP whitelist)                                                                                                                              
  ┌─────────────────────────────────────┬─────────┬─────────────────────────────────────────────┐                                                                            
  │               Element               │ Status  │                    Uwagi                    │                                                                            
  ├─────────────────────────────────────┼─────────┼─────────────────────────────────────────────┤                                                                            
  │ Tablica ALLOWED_IPS[] w config      │ BRAK    │ Tylko 1 hardcoded trusted IP                │                                                                            
  ├─────────────────────────────────────┼─────────┼─────────────────────────────────────────────┤                                                                            
  │ isIPAllowed() iteracja po tablicy   │ PARTIAL │ Istnieje, ale bez prawdziwej whitelisty LAN │                                                                            
  ├─────────────────────────────────────┼─────────┼─────────────────────────────────────────────┤                                                                            
  │ Odrzucenie 403 przed przetwarzaniem │ BRAK    │ Nie-whitelisted IP dostaje auth, nie 403    │                                                                            
  ├─────────────────────────────────────┼─────────┼─────────────────────────────────────────────┤                                                                            
  │ Zakres: wszystkie endpointy + 404   │ BRAK    │ Tylko auth-protected endpointy              │                                                                            
  ├─────────────────────────────────────┼─────────┼─────────────────────────────────────────────┤                                                                            
  │ Trusted proxy bypass                │ TAK     │ 10.99.0.1 WireGuard                         │                                                                            
  └─────────────────────────────────────┴─────────┴─────────────────────────────────────────────┘                                                                            
  2. Autentykacja hasłem (SHA-256)                                                                                                                                           
  ┌──────────────────────────────────┬────────┬─────────────────────────┐                                                                                                    
  │             Element              │ Status │          Uwagi          │                                                                                                    
  ├──────────────────────────────────┼────────┼─────────────────────────┤                                                                                                    
  │ hashPassword() mbedtls SHA-256   │ TAK    │                         │                                                                                                    
  ├──────────────────────────────────┼────────┼─────────────────────────┤                                                                                                    
  │ verifyPassword() porównanie hash │ TAK    │                         │                                                                                                    
  ├──────────────────────────────────┼────────┼─────────────────────────┤                                                                                                    
  │ Hash w FRAM szyfrowany AES-256   │ TAK    │ via credentials_manager │                                                                                                    
  ├──────────────────────────────────┼────────┼─────────────────────────┤                                                                                                    
  │ Blokada bez FRAM → 503           │ TAK    │ Właśnie wdrożone        │                                                                                                    
  └──────────────────────────────────┴────────┴─────────────────────────┘                                                                                                    
  3. Sesje (token + IP binding)                                                                                                                                              
  ┌───────────────────────────────┬────────┬───────────────────────────────────────────┐                                                                                     
  │            Element            │ Status │                   Uwagi                   │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Token 32 znaki hex            │ TAK    │                                           │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Powiązanie z IP               │ TAK    │                                           │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Cookie HttpOnly               │ TAK    │                                           │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Cookie Max-Age=1800           │ BRAK   │ Brak Max-Age na login cookie              │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Cookie SameSite               │ TAK    │ Właśnie dodane (wykracza poza referencję) │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Timeout 30 min                │ TAK    │ Właśnie zmienione z 1h                    │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ Max 10 globalnie, 3 per IP    │ TAK    │                                           │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ updateSessionManager() w loop │ TAK    │ Właśnie dodane                            │                                                                                     
  ├───────────────────────────────┼────────┼───────────────────────────────────────────┤                                                                                     
  │ vector.reserve()              │ TAK    │                                           │                                                                                     
  └───────────────────────────────┴────────┴───────────────────────────────────────────┘                                                                                     
  4. Rate limiting i blokowanie IP                                                                                                                                           
  ┌───────────────────────────────────┬────────┬──────────────────────────────────────────┐                                                                                  
  │              Element              │ Status │                  Uwagi                   │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ RateLimitData per IP              │ TAK    │ Właśnie stworzone                        │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ Limit requestów per IP            │ INNE   │ Ref: 5 req/1s → Dozownik: 100 req/60s    │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ Blokada po nieudanych logowaniach │ INNE   │ Ref: 10 prób/60s → Dozownik: 5 prób/300s │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ Bypass whitelisted IP             │ TAK    │                                          │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ Cleanup co 5 min                  │ TAK    │                                          │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ Hard limit 20 timestamps          │ TAK    │                                          │                                                                                  
  ├───────────────────────────────────┼────────┼──────────────────────────────────────────┤                                                                                  
  │ ipStringCache max 10              │ TAK    │                                          │                                                                                  
  └───────────────────────────────────┴────────┴──────────────────────────────────────────┘                                                                                  
  5. Szyfrowanie FRAM (AES-256-CBC)                                                                                                                                          
  ┌───────────────────────────────────────┬────────┬──────────────┐                                                                                                          
  │                Element                │ Status │    Uwagi     │                                                                                                          
  ├───────────────────────────────────────┼────────┼──────────────┤                                                                                                          
  │ AES-256-CBC                           │ TAK    │ Już istniało │                                                                                                          
  ├───────────────────────────────────────┼────────┼──────────────┤                                                                                                          
  │ Klucz SHA-256 z device_name+SALT+SEED │ TAK    │              │                                                                                                          
  ├───────────────────────────────────────┼────────┼──────────────┤                                                                                                          
  │ IV 8B → 16B                           │ TAK    │              │                                                                                                          
  ├───────────────────────────────────────┼────────┼──────────────┤                                                                                                          
  │ PKCS7 padding                         │ TAK    │              │                                                                                                          
  ├───────────────────────────────────────┼────────┼──────────────┤                                                                                                          
  │ secureZeroMemory                      │ TAK    │              │                                                                                                          
  ├───────────────────────────────────────┼────────┼──────────────┤                                                                                                          
  │ verifyCredentialsInFRAM()             │ TAK    │              │                                                                                                          
  └───────────────────────────────────────┴────────┴──────────────┘                                                                                                          
  6. Walidacja danych wejściowych                                                                                                                                            
  ┌─────────────────────────────────────┬─────────┬──────────────────────────────────────────────────────────────────┐                                                       
  │               Element               │ Status  │                              Uwagi                               │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ Device name 3-32 zn.                │ TAK     │ credentials_validator.cpp                                        │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ WiFi SSID 1-32                      │ TAK     │                                                                  │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ WiFi password 8-64                  │ TAK     │                                                                  │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ Admin password 8-64                 │ TAK     │                                                                  │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ VPS token 1-64                      │ N/A     │ DOZOWNIK nie ma VPS                                              │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ VPS URL walidacja                   │ N/A     │                                                                  │                                                       
  ├─────────────────────────────────────┼─────────┼──────────────────────────────────────────────────────────────────┤                                                       
  │ Parametry API (dose, rate, bitmask) │ PARTIAL │ Channel i container walidowane, brak walidacji dose/rate/bitmask │                                                       
  └─────────────────────────────────────┴─────────┴──────────────────────────────────────────────────────────────────┘                                                       
  7. Bezpieczeństwo fizyczne                                                                                                                                                 
  ┌───────────────────────────────┬────────┬─────────────────────────┐                                                                                                       
  │            Element            │ Status │          Uwagi          │                                                                                                       
  ├───────────────────────────────┼────────┼─────────────────────────┤                                                                                                       
  │ Provisioning GPIO hold 5s     │ TAK    │ GPIO40 (vs GPIO8 w ref) │                                                                                                       
  ├───────────────────────────────┼────────┼─────────────────────────┤                                                                                                       
  │ Feedback buzzer               │ TAK    │ GPIO39 (vs GPIO2)       │                                                                                                       
  ├───────────────────────────────┼────────┼─────────────────────────┤                                                                                                       
  │ AP SSID/hasło, max 4 klientów │ TAK    │ DOZOWNIK-SETUP          │                                                                                                       
  ├───────────────────────────────┼────────┼─────────────────────────┤                                                                                                       
  │ Timeout 10 min nieaktywności  │ TAK    │                         │                                                                                                       
  ├───────────────────────────────┼────────┼─────────────────────────┤                                                                                                       
  │ Reset z błędu przyciskiem     │ TAK    │                         │                                                                                                       
  ├───────────────────────────────┼────────┼─────────────────────────┤                                                                                                       
  │ Brak auto-restart po zapisie  │ TAK    │                         │                                                                                                       
  └───────────────────────────────┴────────┴─────────────────────────┘                                                                                                       
  8. Mechanizmy auto-recovery                                                                                                                                                
  ┌───────────────────────────┬────────┬────────────────────────────────┐                                                                                                    
  │          Element          │ Status │             Uwagi              │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ System auto-enable 30 min │ BRAK   │                                │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ 24h auto-restart          │ BRAK   │                                │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ WiFi reconnect co 30s     │ TAK    │                                │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ NTP retry                 │ TAK    │ Co 60s check (ref: co godzinę) │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ Session cleanup w loop    │ TAK    │ Właśnie dodane                 │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ Rate limit cleanup        │ TAK    │ Właśnie dodane                 │                                                                                                    
  ├───────────────────────────┼────────┼────────────────────────────────┤                                                                                                    
  │ Safe pump stop            │ TAK    │ relay_controller timeout       │                                                                                                    
  └───────────────────────────┴────────┴────────────────────────────────┘                                                                                                    
  9. Ochrona pamięci (ESP32 heap)                                                                                                                                            
  ┌─────────────────────────────────────┬────────┬──────────────────────────────────────┐                                                                                    
  │               Element               │ Status │                Uwagi                 │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ Sesje vector.reserve(10)            │ TAK    │                                      │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ Rate limit 20 timestamps hard limit │ TAK    │ Właśnie stworzone                    │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ IP cache max 10 FIFO                │ TAK    │ Właśnie stworzone                    │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ Cykle pompy RAM max 50              │ N/A    │ Inna architektura (FRAM daily state) │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ FRAM ring buffer                    │ TAK    │ Daily log ring buffer                │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ new(std::nothrow)                   │ TAK    │ fram_encryption.cpp                  │                                                                                    
  ├─────────────────────────────────────┼────────┼──────────────────────────────────────┤                                                                                    
  │ framBusy flag                       │ BRAK   │                                      │                                                                                    
  └─────────────────────────────────────┴────────┴──────────────────────────────────────┘                                                                                    
  10. Logowanie zdarzeń bezpieczeństwa                                                                                                                                       
  ┌───────────────────────────────────┬─────────┬──────────────────────────────────┐                                                                                         
  │              Element              │ Status  │              Uwagi               │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Udane logowanie LOG_INFO          │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Nieudane logowanie LOG_WARNING    │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ IP spoza whitelist LOG_WARNING    │ BRAK    │ Brak whitelisty = brak logowania │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Blokada IP LOG_WARNING            │ TAK     │ W rate_limiter                   │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Tworzenie/usuwanie sesji LOG_INFO │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Ładowanie credentiali LOG_INFO    │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Brak credentiali LOG_ERROR        │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ Operacje szyfrowania LOG_INFO     │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ NTP sync LOG_INFO                 │ TAK     │                                  │                                                                                         
  ├───────────────────────────────────┼─────────┼──────────────────────────────────┤                                                                                         
  │ WiFi connect/disconnect LOG_INFO  │ PARTIAL │ Podstawowe logi                  │                                                                                         
  └───────────────────────────────────┴─────────┴──────────────────────────────────┘                                                                                         
  ---                                                                                                                                                                        
  Elementy DOZOWNIK wykraczające poza referencję                                                                                                                             
  ┌─────────────────────────────┬───────────────────────────────────────────────────────────────┐                                                                            
  │           Element           │                             Uwagi                             │                                                                            
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────┤                                                                            
  │ Cookie SameSite=Strict      │ Referencja wymienia to jako "znaną lukę", ale go nie naprawia │                                                                            
  ├─────────────────────────────┼───────────────────────────────────────────────────────────────┤                                                                            
  │ 503 z JSON instrukcją setup │ Bardziej szczegółowy niż w referencji                         │                                                                            
  └─────────────────────────────┴───────────────────────────────────────────────────────────────┘                                                                            
  ---                                                                                                                                                                        
  Podsumowanie braków                                                                                                                                                        
  ┌─────┬──────────────────────────────────────────┬────────────┐                                                                                                            
  │  #  │                   Brak                   │ Priorytet  │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 1   │ IP whitelist (tablica ALLOWED_IPS + 403) │ Średni     │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 2   │ Cookie Max-Age=1800                      │ Niski      │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 3   │ 24h auto-restart                         │ Niski      │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 4   │ System auto-enable po 30 min             │ Niski      │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 5   │ framBusy flag                            │ Średni     │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 6   │ Walidacja dose/rate/bitmask w API        │ Średni     │                                                                                                            
  ├─────┼──────────────────────────────────────────┼────────────┤                                                                                                            
  │ 7   │ Parametry rate limiter inne niż ref      │ Do decyzji │                                                                                                            
  └─────┴──────────────────────────────────────────┴────────────┘ 