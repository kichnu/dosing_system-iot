# Zgodność z regułami globalnymi

Reguły: `~/.claude/CLAUDE.md` („Sekrety w kodzie”, „Krytyczne bloki sprzętowe”). Wzorce i stan wszystkich
projektów: `~/dev-knowledge/iot-security/secrets-in-firmware.md`, `~/dev-knowledge/esp32/critical-hardware-alarm.md`.
Punkt naprawiony → oznaczyć tu i w tabeli w dev-knowledge. Audyt 2026-10-29, kod niezmieniany.

## Sekrety — ❌ do naprawy

- Prawdziwe SSID/hasło Wi-Fi w `src/main.cpp`, repo publiczne (kichnu/dosing_system-iot; fallback w credentials_manager.cpp to już placeholder).
- Naprawa: rotacja hasła Wi-Fi + provisioning (Krzysztof, stopniowo), potem fallback z `~/.secrets/iot.env` przez
  pre-script → wygenerowany nagłówek (jak hasło OTA), w repo żadnej wartości. Użycie fallbacku = stan awaryjny.

## Alarm krytyczny FRAM/RTC — ❌ do naprawy (najpilniejsze)

- FRAM/RTC niedostępne przy starcie → `systemHalted` (`src/main.cpp` ~310), dozowanie stoi — to jest dobre.
- Ale: `SafetyManager::_loadErrorFromFRAM()` przy nieudanym odczycie traktuje stan jako „brak błędu”, więc
  `enableIfSafe()` włącza master relay, a `setup()` gra `buzzerOK()` (~417). Zatrzymany dozownik daje sygnał „OK”.
- `systemOk` jest w API (`src/web/web_server.cpp` ~234), ale GUI go nie pokazuje. Błędy FRAM/RTC w trakcie pracy nie są wykrywane.
- Wymagane: zatrzaśnięty alarm z buzzerem, widoczny w GUI i na dashboardzie VPS, master relay wyłączony,
  wykrywanie także w trakcie pracy.
