# Zgodność z regułami globalnymi

Reguły: `~/.claude/CLAUDE.md` („Sekrety w kodzie”, „Krytyczne bloki sprzętowe”). Wzorce i stan wszystkich
projektów: `~/dev-knowledge/iot-security/secrets-in-firmware.md`, `~/dev-knowledge/esp32/critical-hardware-alarm.md`.
Punkt naprawiony → oznaczyć tu i w tabeli w dev-knowledge. Audyt 2026-10-29, kod niezmieniany.

## Sekrety — ✅ w kodzie, rotacja do zrobienia

- Zakomentowane `WiFi.begin` z prawdziwym SSID/hasłem usunięte z `src/main.cpp` (2026-11-08). Fallback w
  `credentials_manager.cpp` to placeholder, więc bez provisioningu urządzenie po prostu nie łączy się z Wi-Fi.
- Dane zostają w historii publicznego repo (kichnu/dosing_system-iot). Jedyna skuteczna naprawa to rotacja hasła
  Wi-Fi + provisioning (Krzysztof, stopniowo); czyszczenie historii potem opcjonalne.
- `PROV_AP_PASSWORD` w `src/provisioning/prov_config.h` to jawne, udokumentowane hasło lokalnego AP konfiguracyjnego.

## Alarm krytyczny FRAM/RTC — ✅ w kodzie, czeka na wgranie i test na sprzęcie

- `src/hardware/hw_health.*` (wzór: top_off_water_new-iot): kontrola FRAM (magic + zapis/odczyt bajtu testowego) i
  RTC co 60 s oraz 2 s po nieudanym transferze, 3 próby. Zatrzask w `RTC_NOINIT_ATTR` przeżywa restart programowy.
- Awaria = dozowanie stoi, pompy i master relay wyłączone, buzzer w trybie ALARM, czerwony baner w GUI, pola
  `alarm`/`alarmText` w `/api/dosing-status`, `alarm`/`alarm_text` w `/api/health` (`status` zostaje `ok`).
- Potwierdzenie: Reset w GUI lub przycisk 5 s; kasuje tylko to, co znów działa, dozowanie wraca dopiero wtedy.
- Blok brakujący przy starcie: po powrocie restart (maks. 3 na zatrzask), bo aplikacja z nim nie wystartowała.
- `buzzerOK()` tylko przy sprawnym starcie. Nieczytelny stan błędu z FRAM to awaria, nie „brak błędu”.
- Do zrobienia: dashboard VPS nie pokazuje jeszcze pola `alarm`.
- Do zrobienia: tryb provisioning jest bez sygnału buzzera. `runProvisioningLoop()` (`src/provisioning/ap_core.cpp`)
  powinien ustawiać `setBuzzerMode(BuzzerMode::WARNING)` jak w Dolewce, a błąd FRAM/AP w tej ścieżce wywoływać
  `buzzerCritical()`. Stan sprawdzony 2026-11-09.
