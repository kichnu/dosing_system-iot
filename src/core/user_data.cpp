#include "user_data.h"

#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "logging.h"
#include "../hardware/fram_controller.h"
#include "../hardware/rtc_controller.h"
#include "../config/fram_layout.h"

static SemaphoreHandle_t s_mutex = nullptr;
static uint16_t s_notesRev  = 0;
static uint16_t s_paramsRev = 0;

namespace {

class Lock {
public:
    Lock() : _ok(s_mutex && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {}
    ~Lock() { if (_ok) xSemaphoreGive(s_mutex); }
    bool ok() const { return _ok; }
private:
    bool _ok;
};

// Well-formed UTF-8, no control characters, at most maxBytes bytes.
bool validText(const char* s, size_t maxBytes) {
    if (!s) return false;
    size_t len = strlen(s);
    if (len > maxBytes) return false;
    const uint8_t* p = (const uint8_t*)s;
    const uint8_t* end = p + len;
    while (p < end) {
        uint8_t c = *p;
        size_t n;
        if (c < 0x20 || c == 0x7F) return false;
        if (c < 0x80)                n = 1;
        else if ((c & 0xE0) == 0xC0) n = 2;
        else if ((c & 0xF0) == 0xE0) n = 3;
        else if ((c & 0xF8) == 0xF0) n = 4;
        else return false;
        if (p + n > end) return false;
        for (size_t i = 1; i < n; i++) {
            if ((p[i] & 0xC0) != 0x80) return false;
        }
        if (n == 2 && c < 0xC2) return false;  // overlong
        p += n;
    }
    return true;
}

bool slotUsed(const SharedNotes& n, uint8_t slot) {
    return slot < NOTE_COUNT && (n.notes[slot].flags & 1) && n.notes[slot].text[0] != '\0';
}

void normalizeNoteIdx(SharedNotes& n) {
    for (uint8_t ch = 0; ch < CHANNEL_COUNT; ch++) {
        if (!slotUsed(n, n.ch_note_idx[ch])) n.ch_note_idx[ch] = NOTE_NONE;
    }
}

// An I/O error is reported; a CRC mismatch (never written, or corrupted) gives an
// empty block — same as the old handlers: an empty pool / log is a valid start.
bool readNotesRaw(SharedNotes& n) {
    if (!framController.readBytes(FRAM_ADDR_SHARED_NOTES, &n, sizeof(SharedNotes))) return false;
    if (n.crc32 != FramController::calculateCRC32(&n, sizeof(SharedNotes) - sizeof(uint32_t))) {
        memset(&n, 0, sizeof(SharedNotes));
    }
    return true;
}

ParamLog* allocParamLog() {
    return (ParamLog*)malloc(sizeof(ParamLog));
}

bool readParamLogRaw(ParamLog* log) {
    if (!framController.readBytes(FRAM_ADDR_PARAM_LOG, log, sizeof(ParamLog))) return false;
    if (log->crc32 != FramController::calculateCRC32(log, sizeof(ParamLog) - sizeof(uint32_t))) {
        memset(log, 0, sizeof(ParamLog));
    }
    return true;
}

UserDataResult writeNotes(SharedNotes& n) {
    if (!framController.writeSharedNotes(&n)) return UserDataResult::FRAM_ERROR;
    s_notesRev++;
    return UserDataResult::OK;
}

UserDataResult writeParamLog(ParamLog* log) {
    if (!framController.writeParamLog(log)) return UserDataResult::FRAM_ERROR;
    s_paramsRev++;
    return UserDataResult::OK;
}

}  // namespace

const char* userDataResultToString(UserDataResult r) {
    switch (r) {
        case UserDataResult::OK:         return "OK";
        case UserDataResult::BAD_SLOT:   return "BAD_SLOT";
        case UserDataResult::BAD_TEXT:   return "BAD_TEXT";
        case UserDataResult::BAD_VALUE:  return "BAD_VALUE";
        case UserDataResult::FULL:       return "FULL";
        case UserDataResult::FRAM_ERROR: return "FRAM_ERROR";
        case UserDataResult::BUSY:       return "BUSY";
    }
    return "UNKNOWN";
}

void initUserData() {
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
    s_notesRev  = (uint16_t)esp_random();
    s_paramsRev = (uint16_t)esp_random();
}

uint16_t getNotesRev()  { return s_notesRev; }
uint16_t getParamsRev() { return s_paramsRev; }

// ============================================================================
// Notes
// ============================================================================

UserDataResult notesRead(SharedNotes* out) {
    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    if (!readNotesRaw(*out)) return UserDataResult::FRAM_ERROR;
    normalizeNoteIdx(*out);
    return UserDataResult::OK;
}

UserDataResult noteSet(uint8_t slot, const char* text) {
    if (slot >= NOTE_COUNT) return UserDataResult::BAD_SLOT;
    if (!validText(text, NOTE_MAX_BYTES)) return UserDataResult::BAD_TEXT;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    SharedNotes n;
    if (!readNotesRaw(n)) return UserDataResult::FRAM_ERROR;
    normalizeNoteIdx(n);

    if (strcmp(n.notes[slot].text, text) == 0 && slotUsed(n, slot) == (text[0] != '\0')) {
        return UserDataResult::OK;  // unchanged — no write, no revision bump
    }
    memset(n.notes[slot].text, 0, sizeof(n.notes[slot].text));
    strncpy(n.notes[slot].text, text, NOTE_MAX_BYTES);
    n.notes[slot].flags = (text[0] != '\0') ? 1 : 0;
    normalizeNoteIdx(n);  // a cleared slot drops off every channel
    return writeNotes(n);
}

UserDataResult noteSelect(uint8_t channel, uint8_t slot) {
    if (channel >= CHANNEL_COUNT) return UserDataResult::BAD_SLOT;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    SharedNotes n;
    if (!readNotesRaw(n)) return UserDataResult::FRAM_ERROR;
    normalizeNoteIdx(n);
    if (slot != NOTE_NONE && !slotUsed(n, slot)) return UserDataResult::BAD_SLOT;
    if (n.ch_note_idx[channel] == slot) return UserDataResult::OK;
    n.ch_note_idx[channel] = slot;
    return writeNotes(n);
}

// ============================================================================
// Parameter tracker
// ============================================================================

UserDataResult paramLogRead(ParamLog* out) {
    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    return readParamLogRaw(out) ? UserDataResult::OK : UserDataResult::FRAM_ERROR;
}

UserDataResult paramAdd(uint8_t tmpl, uint8_t channel, float value) {
    if (tmpl >= PARAM_TMPL_COUNT) return UserDataResult::BAD_SLOT;
    if (channel != PARAM_ANY_CHANNEL && channel >= CHANNEL_COUNT) return UserDataResult::BAD_SLOT;
    if (!isfinite(value)) return UserDataResult::BAD_VALUE;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    ParamLog* log = allocParamLog();
    if (!log) return UserDataResult::BUSY;
    UserDataResult res = UserDataResult::FRAM_ERROR;
    if (readParamLogRaw(log)) {
        if (!(log->templates[tmpl].flags & 1)) {
            res = UserDataResult::BAD_SLOT;
        } else {
            if (channel == PARAM_ANY_CHANNEL) {
                channel = 0;
                uint8_t mask = log->templates[tmpl].channel_mask;
                for (uint8_t c = 0; c < CHANNEL_COUNT; c++) {
                    if (mask & (1 << c)) { channel = c; break; }
                }
            }
            uint8_t head = (log->head < PARAM_RING_SIZE) ? log->head : 0;
            ParamRecord& r = log->ring[head];
            memset(&r, 0, sizeof(r));
            r.value     = value;
            r.timestamp = rtcController.getUnixTime();
            r.tmpl_idx  = tmpl;
            r.channel   = channel;
            r.flags     = 1;
            log->head = (head + 1) % PARAM_RING_SIZE;
            if (log->count < PARAM_RING_SIZE) log->count++;
            res = writeParamLog(log);
        }
    }
    free(log);
    return res;
}

UserDataResult paramEdit(uint8_t rec, float value) {
    if (rec >= PARAM_RING_SIZE) return UserDataResult::BAD_SLOT;
    if (!isfinite(value)) return UserDataResult::BAD_VALUE;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    ParamLog* log = allocParamLog();
    if (!log) return UserDataResult::BUSY;
    UserDataResult res = UserDataResult::FRAM_ERROR;
    if (readParamLogRaw(log)) {
        if (!(log->ring[rec].flags & 1)) {
            res = UserDataResult::BAD_SLOT;
        } else {
            log->ring[rec].value     = value;
            log->ring[rec].timestamp = rtcController.getUnixTime();
            res = writeParamLog(log);
        }
    }
    free(log);
    return res;
}

UserDataResult paramDelete(uint8_t rec) {
    if (rec >= PARAM_RING_SIZE) return UserDataResult::BAD_SLOT;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    ParamLog* log = allocParamLog();
    if (!log) return UserDataResult::BUSY;
    UserDataResult res = UserDataResult::FRAM_ERROR;
    if (readParamLogRaw(log)) {
        if (!(log->ring[rec].flags & 1)) {
            res = UserDataResult::OK;  // already deleted (e.g. by another client)
        } else {
            log->ring[rec].flags = 0;
            res = writeParamLog(log);
        }
    }
    free(log);
    return res;
}

UserDataResult paramTemplateCreate(const char* name, const char* unit, uint8_t channel, uint8_t* outSlot) {
    if (channel >= CHANNEL_COUNT) return UserDataResult::BAD_SLOT;
    if (!name || !name[0]) return UserDataResult::BAD_TEXT;
    if (!validText(name, sizeof(ParamTemplate::name) - 1)) return UserDataResult::BAD_TEXT;
    if (!validText(unit ? unit : "", sizeof(ParamTemplate::unit) - 1)) return UserDataResult::BAD_TEXT;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    ParamLog* log = allocParamLog();
    if (!log) return UserDataResult::BUSY;
    UserDataResult res = UserDataResult::FRAM_ERROR;
    if (readParamLogRaw(log)) {
        int slot = -1;
        for (int i = 0; i < PARAM_TMPL_COUNT; i++) {
            if (!(log->templates[i].flags & 1)) { slot = i; break; }
        }
        if (slot < 0) {
            res = UserDataResult::FULL;
        } else {
            ParamTemplate& t = log->templates[slot];
            memset(&t, 0, sizeof(t));
            strncpy(t.name, name, sizeof(t.name) - 1);
            strncpy(t.unit, unit ? unit : "", sizeof(t.unit) - 1);
            t.flags = 1;
            t.channel_mask = (uint8_t)(1 << channel);
            uint8_t used = 0;
            for (int i = 0; i < PARAM_TMPL_COUNT; i++) if (log->templates[i].flags & 1) used++;
            log->tmpl_count = used;
            res = writeParamLog(log);
            if (res == UserDataResult::OK && outSlot) *outSlot = (uint8_t)slot;
        }
    }
    free(log);
    return res;
}

UserDataResult paramTemplateAssign(uint8_t tmpl, uint8_t channel, bool assigned) {
    if (tmpl >= PARAM_TMPL_COUNT || channel >= CHANNEL_COUNT) return UserDataResult::BAD_SLOT;

    Lock lock;
    if (!lock.ok()) return UserDataResult::BUSY;
    ParamLog* log = allocParamLog();
    if (!log) return UserDataResult::BUSY;
    UserDataResult res = UserDataResult::FRAM_ERROR;
    if (readParamLogRaw(log)) {
        ParamTemplate& t = log->templates[tmpl];
        if (!(t.flags & 1)) {
            res = UserDataResult::BAD_SLOT;
        } else {
            uint8_t mask = assigned ? (t.channel_mask | (1 << channel)) : (t.channel_mask & ~(1 << channel));
            if (mask == t.channel_mask) {
                res = UserDataResult::OK;
            } else {
                t.channel_mask = mask;
                res = writeParamLog(log);
            }
        }
    }
    free(log);
    return res;
}
