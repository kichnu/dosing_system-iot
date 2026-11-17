#ifndef USER_DATA_H
#define USER_DATA_H

#include <Arduino.h>
#include "../config/dosing_types.h"

// Shared notes and the parameter tracker — single-item operations.
//
// Every change is a read-modify-write of the FRAM structure under one mutex,
// so concurrent writers (web GUI tabs, the touch panel over ESP-NOW) cannot
// overwrite each other's changes the way whole-structure POSTs did.
// Each successful change bumps a revision counter; clients reload their copy
// when the revision in /api/dosing-status differs from the one they hold.
// Revisions start at a random value on every boot, so a cached revision from
// before a restart never matches by accident.

static constexpr uint8_t NOTE_COUNT      = 12;
static constexpr uint8_t NOTE_NONE       = 0xFF;   // ch_note_idx: no note for the channel
static constexpr uint8_t NOTE_MAX_BYTES  = 29;     // UTF-8 bytes, without the terminator
static constexpr uint8_t PARAM_TMPL_COUNT = 20;
static constexpr uint8_t PARAM_RING_SIZE  = 100;
static constexpr uint8_t PARAM_ANY_CHANNEL = 0xFF; // paramAdd: use the template's first channel

enum class UserDataResult : uint8_t {
    OK = 0,
    BAD_SLOT,       // note slot / channel / template / record index out of range or unused
    BAD_TEXT,       // too long, invalid UTF-8 or control characters
    BAD_VALUE,      // NaN / inf
    FULL,           // no free template slot
    FRAM_ERROR,
    BUSY,           // mutex timeout
};

const char* userDataResultToString(UserDataResult r);

void initUserData();

uint16_t getNotesRev();
uint16_t getParamsRev();

// --- Notes ---
// Reads the pool; ch_note_idx is normalized: out of range or an empty slot -> NOTE_NONE.
UserDataResult notesRead(SharedNotes* out);
// Empty text clears the slot; channels pointing at a cleared slot get NOTE_NONE.
UserDataResult noteSet(uint8_t slot, const char* text);
// slot = NOTE_NONE removes the channel's note; an empty slot is rejected.
UserDataResult noteSelect(uint8_t channel, uint8_t slot);

// --- Parameter tracker ---
UserDataResult paramLogRead(ParamLog* out);
// Timestamp from the RTC. channel = PARAM_ANY_CHANNEL -> first channel of the template's mask (0 if none).
UserDataResult paramAdd(uint8_t tmpl, uint8_t channel, float value);
// Keeps the existing web GUI behavior: an edited record gets the current time.
UserDataResult paramEdit(uint8_t rec, float value);
UserDataResult paramDelete(uint8_t rec);
UserDataResult paramTemplateCreate(const char* name, const char* unit, uint8_t channel, uint8_t* outSlot);
UserDataResult paramTemplateAssign(uint8_t tmpl, uint8_t channel, bool assigned);

#endif // USER_DATA_H
