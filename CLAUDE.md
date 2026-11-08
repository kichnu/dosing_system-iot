# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DOZOWNIK is an ESP32-based fertilizer/nutrient dosing system for aquariums or hydroponics. It controls up to 4-6 peristaltic pumps on a configurable schedule, with persistent storage in FRAM, web-based configuration, and comprehensive safety features.

**Hardware:** Seeed Studio XIAO ESP32-S3 with:
- MB85RC256V FRAM (32KB) for persistent storage
- DS3231 RTC for timekeeping
- 4-6 relay-controlled peristaltic pumps
- GPIO validation inputs for pump feedback
- Master relay (GPIO38) for safety cutoff
- Buzzer (GPIO39) and reset button (GPIO40)

## Build Commands

```bash
# Build for production (no CLI, minimal logs)
pio run -e production

# Build for debug (full CLI and logging)
pio run -e debug

# Upload firmware
pio run -e production -t upload
pio run -e debug -t upload

# Monitor serial output
pio device monitor -b 115200

# Clean build
pio run -t clean
```

## Architecture

### Initialization Flow (main.cpp)
1. **Provisioning check** - Hold button at boot for WiFi setup AP mode
2. **initHardware()** - I2C, FRAM, RTC, Relays
3. **initNetwork()** - WiFi connection, WebServer
4. **initApplication()** - ChannelManager, DosingScheduler, NTP sync
5. **SafetyManager** - Check for persisted critical errors

### Core Components

**ChannelManager** (`src/algorithm/channel_manager.h`)
- Manages dosing configuration for all channels
- A saved config takes effect immediately; the rest of the day is planned once at save: (daily dose - dosed today) / slots left
- Validates dosing parameters, tracks daily state
- Caches FRAM data in RAM for fast access

**DosingScheduler** (`src/hardware/dosing_scheduler.h`)
- Time-based event scheduling (hours 1-23, channels offset by 15 min)
- Daily reset at midnight
- State machine: IDLE → CHECKING → DOSING → WAITING_PUMP

**RelayController** (`src/hardware/relay_controller.h`)
- Single-pump mutex (only one pump runs at a time)
- 3-phase GPIO validation: PRE (wire check), RUN (relay confirm), POST (stuck detection)
- Timeout protection (max 180 seconds per pump)

**SafetyManager** (`src/hardware/safety_manager.h`)
- Master relay for global power cutoff
- Persists critical errors to FRAM (survive power loss)
- Buzzer alarm pattern, 5-second button hold to reset

**FramController** (`src/hardware/fram_controller.h`)
- Low-level FRAM access with CRC32 validation
- See `fram_layout.h` for complete memory map

### FRAM Memory Layout

Key sections (see `src/config/fram_layout.h`):
- 0x0000: Header (magic, version)
- 0x0020: Encrypted WiFi credentials (1KB, compatible with DOLEWKA project)
- 0x0440: Channel configs (8 × 32B)
- 0x0640: Daily states (8 × 24B)
- 0x0700: Critical error state
- 0x17CC: Event log ring buffer (75 entries)

### Data Structures

All packed to specific sizes with CRC32 (see `src/config/dosing_types.h`):
- `ChannelConfig` (32B): events bitmask, days bitmask, daily dose, dosing rate
- `ChannelDailyState` (24B): completed/failed events, today's total, dose planned for the rest of today

### Build Environments

- **production**: `ENABLE_CLI=0`, `CORE_DEBUG_LEVEL=0`, watchdog enabled
- **debug**: `ENABLE_CLI=1`, `CORE_DEBUG_LEVEL=3`, serial menu for testing

### Provisioning Mode

Hold GPIO40 button for 5 seconds at boot to enter AP mode:
- SSID: `DOZOWNIK-SETUP`, Password: `setup12345`
- Captive portal at 192.168.4.1 for WiFi configuration

## Key Constants (src/config/config.h)

- `CHANNEL_COUNT`: 4 (adjustable 1-6)
- Events: Hours 1-23 (hour 0 reserved for daily reset)
- Channel time offsets: 15 minutes apart
- Max pump duration: 180 seconds
- Dosing rate: 0.1-5.0 ml/s
- Single dose: 1-50 ml, Daily max: 500 ml

## Code Conventions

- Polish comments throughout the codebase
- Global instances declared as `extern` in headers (e.g., `framController`, `channelManager`, `dosingScheduler`)
- Structures use `#pragma pack(push, 1)` for FRAM compatibility
- Bitmasks for scheduling: bit 1-23 for hours, bits 0-6 for weekdays (Mon-Sun)

## Session Logging

**At the start of each session, ask the user if they want to save conversation notes.**

Session logs are stored in `docs/sessions/` with format: `YYYY-MM-DD_HHMM_topic.md`

When logging is enabled:
- Create/update session file for significant topics
- Write notes in English, concise style
- Include: context, decisions made, files changed
- Update regularly without asking, but keep entries brief
