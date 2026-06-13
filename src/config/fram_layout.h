/**
 * DOZOWNIK - FRAM Memory Layout v8
 *
 * Mapa adresów pamięci FRAM MB85RC256V (32 kB).
 * UWAGA: Niezgodność wersji powoduje automatyczny factory reset.
 *
 * Sekcja CREDENTIALS kompatybilna z projektem DOLEWKA (1024 B) — NIE RUSZAĆ.
 */

#ifndef FRAM_LAYOUT_H
#define FRAM_LAYOUT_H

#include <Arduino.h>
#include "config.h"
#include "dosing_types.h"

// ============================================================================
// FRAM CHIP
// ============================================================================
#define FRAM_SIZE_BYTES         32768   // 32 kB (0x8000)
#define FRAM_PAGE_SIZE          16

// ============================================================================
// MAGIC & VERSION
// ============================================================================
#define FRAM_MAGIC_NUMBER       0x444F5A41  // "DOZA"
#define FRAM_LAYOUT_VERSION     8           // v8: 8 channels, no GPIO validation, new sections

// ============================================================================
// MEMORY MAP v8
// ============================================================================
// Section            | Address  | Size      | Description
// -------------------|----------|-----------|----------------------------------
// HEADER             | 0x0000   | 32 B      | Magic, version
// CREDENTIALS        | 0x0020   | 1024 B    | Encrypted WiFi (DOLEWKA compat)
// SYSTEM_STATE       | 0x0420   | 32 B      | Global system state
// ACTIVE_CONFIG      | 0x0440   | 256 B     | Active config  (8 × 32B)
// PENDING_CONFIG     | 0x0540   | 256 B     | Pending config (8 × 32B)
// DAILY_STATE        | 0x0640   | 192 B     | Daily state    (8 × 24B)
// CRITICAL_ERROR     | 0x0700   | 32 B      | Critical error state
// AUTH_DATA          | 0x0720   | 64 B      | Admin password hash
// SESSION_DATA       | 0x0760   | 128 B     | Session persistence
// CONTAINER_VOLUME   | 0x07E0   | 64 B      | Container vol  (8 × 8B)
// DOSED_TRACKER      | 0x0820   | 64 B      | Dosed tracker  (8 × 8B)
// CHANNEL_LABELS     | 0x0860   | 256 B     | Channel names  (8 × 32B)
// CHANNEL_PARAMS     | 0x0960   | 256 B     | Channel limits (8 × 32B)
// PUMP_MON_CONFIG    | 0x0A60   | 32 B      | Edge Impulse config reserved
// FREE               | 0x0A80   | 128 B     | Rezerva
// SHARED_NOTES       | 0x0B00   | 400 B     | Notes pool + ch_note_idx (12 × 32B + meta)
// PARAM_LOG          | 0x0C90   | 1852 B    | ParamLog: 20 tmpl × 32B + ring 100 × 12B
// RESERVED           | 0x13CC   | ~26.7 KB  | Przyszłe użycie
// ============================================================================

// ----------------------------------------------------------------------------
// HEADER (0x0000 - 0x001F)
// ----------------------------------------------------------------------------
#define FRAM_ADDR_HEADER            0x0000
#define FRAM_SIZE_HEADER            32

#pragma pack(push, 1)
struct FramHeader {
    uint32_t magic;
    uint16_t layout_version;
    uint16_t channel_count;
    uint32_t init_timestamp;
    uint32_t last_write;
    uint8_t  flags;
    uint8_t  _reserved[11];
    uint32_t header_crc;
};
#pragma pack(pop)

static_assert(sizeof(FramHeader) == FRAM_SIZE_HEADER, "FramHeader size mismatch");

// ----------------------------------------------------------------------------
// CREDENTIALS (0x0020 - 0x041F) — NIE ZMIENIAĆ, kompatybilność DOLEWKA
// ----------------------------------------------------------------------------
#define FRAM_ADDR_CREDENTIALS       0x0020
#define FRAM_SIZE_CREDENTIALS       1024

// ----------------------------------------------------------------------------
// SYSTEM STATE (0x0420 - 0x043F)
// ----------------------------------------------------------------------------
#define FRAM_ADDR_SYSTEM_STATE      0x0420
#define FRAM_SIZE_SYSTEM_STATE      32

// ----------------------------------------------------------------------------
// ACTIVE CONFIG (0x0440 - 0x053F)  8 × 32B = 256B
// ----------------------------------------------------------------------------
#define FRAM_ADDR_ACTIVE_CONFIG     0x0440
#define FRAM_SIZE_ACTIVE_CONFIG     (8 * sizeof(ChannelConfig))   // 256B
#define FRAM_ADDR_ACTIVE_CH(n)      (FRAM_ADDR_ACTIVE_CONFIG + ((n) * sizeof(ChannelConfig)))

// ----------------------------------------------------------------------------
// PENDING CONFIG (0x0540 - 0x063F)  8 × 32B = 256B
// ----------------------------------------------------------------------------
#define FRAM_ADDR_PENDING_CONFIG    0x0540
#define FRAM_SIZE_PENDING_CONFIG    (8 * sizeof(ChannelConfig))   // 256B
#define FRAM_ADDR_PENDING_CH(n)     (FRAM_ADDR_PENDING_CONFIG + ((n) * sizeof(ChannelConfig)))

// ----------------------------------------------------------------------------
// DAILY STATE (0x0640 - 0x06FF)  8 × 24B = 192B
// ----------------------------------------------------------------------------
#define FRAM_ADDR_DAILY_STATE       0x0640
#define FRAM_SIZE_DAILY_STATE       (8 * sizeof(ChannelDailyState))  // 192B
#define FRAM_ADDR_DAILY_CH(n)       (FRAM_ADDR_DAILY_STATE + ((n) * sizeof(ChannelDailyState)))

// ----------------------------------------------------------------------------
// CRITICAL ERROR (0x0700 - 0x071F)
// ----------------------------------------------------------------------------
#define FRAM_ADDR_CRITICAL_ERROR    0x0700
#define FRAM_SIZE_CRITICAL_ERROR    32

// ----------------------------------------------------------------------------
// AUTH DATA (0x0720 - 0x075F)
// ----------------------------------------------------------------------------
#define FRAM_ADDR_AUTH_DATA         0x0720
#define FRAM_SIZE_AUTH_DATA         64

#pragma pack(push, 1)
struct AuthData {
    uint8_t  password_hash[32];
    uint8_t  salt[16];
    uint8_t  hash_iterations;
    uint8_t  password_set;
    uint8_t  _reserved[10];
    uint32_t crc32;
};
#pragma pack(pop)

static_assert(sizeof(AuthData) == FRAM_SIZE_AUTH_DATA, "AuthData size mismatch");

// ----------------------------------------------------------------------------
// SESSION DATA (0x0760 - 0x07DF)
// ----------------------------------------------------------------------------
#define FRAM_ADDR_SESSION_DATA      0x0760
#define FRAM_SIZE_SESSION_DATA      128

// ----------------------------------------------------------------------------
// CONTAINER VOLUME (0x07E0 - 0x081F)  8 × 8B = 64B
// ----------------------------------------------------------------------------
#define FRAM_ADDR_CONTAINER_VOLUME  0x07E0
#define FRAM_SIZE_CONTAINER_VOLUME  (8 * sizeof(ContainerVolume))   // 64B
#define FRAM_ADDR_CONTAINER_CH(n)   (FRAM_ADDR_CONTAINER_VOLUME + ((n) * sizeof(ContainerVolume)))

// ----------------------------------------------------------------------------
// DOSED TRACKER (0x0820 - 0x085F)  8 × 8B = 64B
// ----------------------------------------------------------------------------
#define FRAM_ADDR_DOSED_TRACKER     0x0820
#define FRAM_SIZE_DOSED_TRACKER     (8 * sizeof(DosedTracker))      // 64B
#define FRAM_ADDR_DOSED_TRACKER_CH(n) (FRAM_ADDR_DOSED_TRACKER + ((n) * sizeof(DosedTracker)))

// ----------------------------------------------------------------------------
// CHANNEL LABELS (0x0860 - 0x095F)  8 × 32B = 256B  [NOWE]
// Nazwy/przeznaczenie kanałów konfigurowane w GUI
// ----------------------------------------------------------------------------
#define FRAM_ADDR_CHANNEL_LABELS    0x0860
#define FRAM_SIZE_CHANNEL_LABELS    (8 * sizeof(ChannelLabel))      // 256B
#define FRAM_ADDR_LABEL_CH(n)       (FRAM_ADDR_CHANNEL_LABELS + ((n) * sizeof(ChannelLabel)))

// ----------------------------------------------------------------------------
// CHANNEL PARAMS (0x0960 - 0x0A5F)  8 × 32B = 256B  [NOWE]
// Min/max dawki per kanał, konfigurowane w GUI
// ----------------------------------------------------------------------------
#define FRAM_ADDR_CHANNEL_PARAMS    0x0960
#define FRAM_SIZE_CHANNEL_PARAMS    (8 * sizeof(ChannelParams))     // 256B
#define FRAM_ADDR_PARAMS_CH(n)      (FRAM_ADDR_CHANNEL_PARAMS + ((n) * sizeof(ChannelParams)))

// ----------------------------------------------------------------------------
// PUMP MONITOR CONFIG (0x0A60 - 0x0A7F)  32B  [REZERWACJA — Edge Impulse]
// ----------------------------------------------------------------------------
#define FRAM_ADDR_PUMP_MON_CONFIG   0x0A60
#define FRAM_SIZE_PUMP_MON_CONFIG   32

// ----------------------------------------------------------------------------
// FREE (0x0A80 - 0x0AFF)  128B
// ----------------------------------------------------------------------------
#define FRAM_ADDR_FREE_SPACE        0x0A80
#define FRAM_SIZE_FREE_SPACE        128

// ----------------------------------------------------------------------------
// SHARED NOTES (0x0B00 - 0x0C8F)  400B
// Pula 12 notatek shared (12 × 32B) + per-kanał indeks aktywnej notatki
// ----------------------------------------------------------------------------
#define FRAM_ADDR_SHARED_NOTES      0x0B00
#define FRAM_SIZE_SHARED_NOTES      sizeof(SharedNotes)   // 400B

// ----------------------------------------------------------------------------
// PARAM LOG (0x0C90 - 0x134B)  1852B
// Szablony parametrów (20 × 32B) + ring buffer pomiarów (100 × 12B)
// Ring jest SHARED — wspólny dla wszystkich szablonów i kanałów
// ----------------------------------------------------------------------------
#define FRAM_ADDR_PARAM_LOG         0x0C90
#define FRAM_SIZE_PARAM_LOG         sizeof(ParamLog)   // 1852B

// ----------------------------------------------------------------------------
// RESERVED (0x13CC - 0x7FFF)  ~26.7 KB
// ----------------------------------------------------------------------------
#define FRAM_ADDR_RESERVED          0x13CC
#define FRAM_SIZE_RESERVED          (FRAM_SIZE_BYTES - FRAM_ADDR_RESERVED)

// ============================================================================
// COMPILE-TIME VALIDATION
// ============================================================================

static_assert(FRAM_ADDR_RESERVED + FRAM_SIZE_RESERVED == FRAM_SIZE_BYTES,
              "FRAM reserved section calculation error!");

static_assert(FRAM_ADDR_ACTIVE_CONFIG + FRAM_SIZE_ACTIVE_CONFIG <= FRAM_ADDR_PENDING_CONFIG,
              "ACTIVE_CONFIG overlaps PENDING_CONFIG!");

static_assert(FRAM_ADDR_PENDING_CONFIG + FRAM_SIZE_PENDING_CONFIG <= FRAM_ADDR_DAILY_STATE,
              "PENDING_CONFIG overlaps DAILY_STATE!");

static_assert(FRAM_ADDR_SHARED_NOTES + FRAM_SIZE_SHARED_NOTES <= FRAM_ADDR_PARAM_LOG,
              "SHARED_NOTES overlaps PARAM_LOG!");

static_assert(FRAM_ADDR_PARAM_LOG + FRAM_SIZE_PARAM_LOG <= FRAM_ADDR_RESERVED,
              "PARAM_LOG overlaps RESERVED!");

// ============================================================================
// FRAM OPERATIONS (deklaracje)
// ============================================================================

bool framInit();
bool framIsInitialized();
bool framFactoryReset();
bool framReadHeader(FramHeader* header);
bool framWriteHeader(const FramHeader* header);

bool framReadActiveConfig(uint8_t channel, ChannelConfig* config);
bool framWriteActiveConfig(uint8_t channel, const ChannelConfig* config);
bool framReadPendingConfig(uint8_t channel, ChannelConfig* config);
bool framWritePendingConfig(uint8_t channel, const ChannelConfig* config);
bool framApplyPendingConfig(uint8_t channel);
bool framRevertPendingConfig(uint8_t channel);

bool framReadDailyState(uint8_t channel, ChannelDailyState* state);
bool framWriteDailyState(uint8_t channel, const ChannelDailyState* state);
bool framResetAllDailyStates(uint8_t currentDay);

bool framReadDosedTracker(uint8_t channel, DosedTracker* tracker);
bool framWriteDosedTracker(uint8_t channel, const DosedTracker* tracker);
bool framResetDosedTracker(uint8_t channel);

bool framReadSystemState(SystemState* state);
bool framWriteSystemState(const SystemState* state);

// Channel labels/params dostępne przez framController.readChannelLabel/Params()

bool framReadBytes(uint16_t address, void* buffer, size_t length);
bool framWriteBytes(uint16_t address, const void* data, size_t length);
bool framProbe();
void framDumpSection(uint16_t address, size_t length);

#endif // FRAM_LAYOUT_H
