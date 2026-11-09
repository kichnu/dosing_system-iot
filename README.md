# DOZOWNIK

Fertilizer and nutrient dosing system (aquarium or hydroponics) on ESP32-S3, with up to 8 peristaltic pump channels, an hourly schedule and web-based configuration. Persistent data lives in FRAM (MB85RC256V), time comes from a DS3231, and safety relies on a master relay and a latched critical error.

Architecture details, the FRAM map and code conventions are in [CLAUDE.md](CLAUDE.md), which is the source of truth and is not duplicated here.

## Build

```bash
pio run -e production        # no CLI, minimal logs, for the target device
pio run -e debug             # full CLI and logs, for development and testing
pio run -e production -t upload
pio device monitor -b 115200
```

### OTA (upload over WiFi)

OTA requires a prior USB upload of firmware that contains `ArduinoOTA.begin()`; without it, OTA does not exist on the device. `OTA_PASSWORD_192_168_10_3` must be set in the environment where `pio run` runs. An export in a separate process or session does not reach the `pio` process (symptom: `Authentication Failed`). The variable is named after the device's local IP rather than the project, because several IoT projects share one `~/.secrets/iot.env` and a common `OTA_PASSWORD` could flash the wrong device's password.

**Set the password once, permanently**, not ad hoc before each command. `-DOTA_PASSWORD` (a firmware macro) goes into the global `build_flags`, so changing the source environment variable between runs changes the build signature for all files at once (PlatformIO/SCons). That forces a full rebuild on every OTA instead of an incremental one. Keep the password in `~/.secrets/iot.env` (`chmod 700` on the directory, `chmod 600` on the file, one line `export OTA_PASSWORD_192_168_10_3='...'`), sourced from `~/.zshrc`:

```bash
[ -f ~/.secrets/iot.env ] && source ~/.secrets/iot.env
```

```bash
# First upload: always over USB, burns the password in via build_flags
pio run -e production -t upload

# Later updates: over WiFi (the device must be on the same network)
pio run -e production_ota -t upload
pio run -e debug_ota -t upload

# If mDNS (dozownik.local) does not work (e.g. VLAN or IoT isolation),
# override --upload-port with the IP address
pio run -e production_ota -t upload --upload-port 192.168.10.3
```

Changing the partition table (`board_build.partitions`) always requires a full reflash over USB. It cannot be done over OTA.

### Remote log viewing (no USB)

The firmware mirrors logs (`logInfo`/`logWarning`/`logError`) to a TCP socket on port 8880 (`src/core/logging.cpp`, `startLogServer()`/`updateLogServer()`), which is useful after OTA when USB is not available. It serves one client at a time (LAN only, developer tool, no authentication). **It does not show the cause of a crash or panic**, since those go straight to UART and bypass the logger. Diagnosing a crash always needs a physical USB monitor.

```bash
pio device monitor --port socket://<device-ip>:8880
```

**Scope:** the log socket only mirrors what goes through `LOG_INFO`/`LOG_WARNING`/`LOG_ERROR` (`src/core/logging.h`), meaning live runtime events: pumps (`[PUMP]`), web requests (`[WEB]`), the scheduler (`[SCHED]`) and the heartbeat (`[HEARTBEAT]`). Other output (boot initialization, CLI/debug menu, `printStatus()` dumps, libraries) uses raw `Serial.print`/`printf` and is visible **only over USB** (`pio device monitor -b 115200`).

## Status

The v4.0 rebuild is in progress (Active LOW relays, 8 channels, preparation for Edge Impulse pump monitoring). See `docs/sessions/` and the commit history.

### Pump control: relays (Active LOW)

The firmware drives `PUMPS_PINS` as Active LOW (LOW = pump on, HIGH = off), in preparation for replacing the ULN2003AN drivers with relays (candidate: `docs/G3VM-61G1_NOTES_1.0.md`). The logic change is ready in code, but the electrical change (physically replacing the part on the board) has not been done yet.

## Documentation (docs/)

| File | Contents |
|---|---|
| `FRAM_MAP.txt` | Current FRAM memory map (addresses, sections, layout version) |
| `ZGODNOSC_REGUL.md` | Compliance status with the secrets and critical hardware rules (in Polish) |
| `G3VM-61G1_NOTES_1.0.md` | Notes on the G3VM-61G1 MOS FET relay (in Polish) |
| `SESSION_MANAGER_GPIO1_NOTES.md` | Notes on session manager issues shared with another project (in Polish) |
| `ESP32-S3-Pico.png` | Board pinout |
| `sessions/` | Working session logs, in chronological order |
