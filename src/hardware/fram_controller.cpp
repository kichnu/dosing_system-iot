
/**
 * DOZOWNIK - FRAM Controller Implementation
 */

#include "fram_controller.h"

// Global instance
FramController framController;

// Flag blokująca odczyt FRAM z HTTP podczas zapisu
volatile bool framBusy = false;

// CRC32 lookup table (polynomial 0xEDB88320)
static const uint32_t crc32_table[256] PROGMEM = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

// ============================================================================
// CRC32 CALCULATION
// ============================================================================

uint32_t FramController::calculateCRC32(const void* data, size_t length) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ bytes[i]) & 0xFF;
        crc = pgm_read_dword(&crc32_table[index]) ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool FramController::begin() {
    Serial.println(F("[FRAM] Initializing..."));
    
    // Check if FRAM is present
    if (!probe()) {
        Serial.println(F("[FRAM] ERROR: FRAM not found on I2C!"));
        _initialized = false;
        return false;
    }
    
    Serial.println(F("[FRAM] FRAM detected at 0x50"));
    
    // Try to read and validate header
    if (validateHeader()) {
        Serial.println(F("[FRAM] Valid header found"));
        _initialized = true;
        return true;
    }
    
    // Header invalid - initialize fresh
    Serial.println(F("[FRAM] No valid header, initializing..."));
    if (_initializeEmpty()) {
        Serial.println(F("[FRAM] Initialization complete"));
        _initialized = true;
        return true;
    }
    
    Serial.println(F("[FRAM] ERROR: Failed to initialize!"));
    _initialized = false;
    return false;
}

bool FramController::probe() {
    Wire.beginTransmission(FRAM_I2C_ADDRESS);
    return Wire.endTransmission() == 0;
}

bool FramController::validateHeader() {
    FramHeader header;
    
    if (!readHeader(&header)) {
        return false;
    }
    
    // Check magic number
    if (header.magic != FRAM_MAGIC_NUMBER) {
        Serial.printf("[FRAM] Invalid magic: 0x%08X (expected 0x%08X)\n",
                      header.magic, FRAM_MAGIC_NUMBER);
        return false;
    }
    
    // Check version
    if (header.layout_version != FRAM_LAYOUT_VERSION) {
        Serial.printf("[FRAM] Version mismatch: %d (expected %d)\n",
                      header.layout_version, FRAM_LAYOUT_VERSION);
        return false;
    }
    
    // Verify CRC (exclude crc field itself)
    uint32_t calc_crc = calculateCRC32(&header, sizeof(FramHeader) - sizeof(uint32_t));
    if (calc_crc != header.header_crc) {
        Serial.println(F("[FRAM] Header CRC mismatch"));
        return false;
    }
    
    return true;
}

bool FramController::_initializeEmpty() {
    // Create fresh header
    FramHeader header;
    memset(&header, 0, sizeof(header));
    
    header.magic = FRAM_MAGIC_NUMBER;
    header.layout_version = FRAM_LAYOUT_VERSION;
    header.channel_count = CHANNEL_COUNT;
    header.init_timestamp = 0;  // Will be set when RTC is available
    header.last_write = 0;
    header.flags = 0;
    
    // Calculate CRC
    header.header_crc = calculateCRC32(&header, sizeof(FramHeader) - sizeof(uint32_t));
    
    if (!writeHeader(&header)) {
        return false;
    }
    
    // Initialize empty channel configs
    ChannelConfig emptyConfig;
    memset(&emptyConfig, 0, sizeof(emptyConfig));
    emptyConfig.dosing_rate = DEFAULT_DOSING_RATE;
    emptyConfig.crc32 = calculateCRC32(&emptyConfig, sizeof(ChannelConfig) - sizeof(uint32_t));
    
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!writeActiveConfig(i, &emptyConfig)) return false;
        if (!writePendingConfig(i, &emptyConfig)) return false;
    }
    
    // Initialize empty daily states
    ChannelDailyState emptyState;
    memset(&emptyState, 0, sizeof(emptyState));
    emptyState.crc32 = calculateCRC32(&emptyState, sizeof(ChannelDailyState) - sizeof(uint32_t));
    
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!writeDailyState(i, &emptyState)) return false;
    }
    
    // Initialize system state
    SystemState sysState;
    memset(&sysState, 0, sizeof(sysState));
    sysState.system_enabled = 1;
    sysState.active_channel = 255;
    sysState.crc32 = calculateCRC32(&sysState, sizeof(SystemState) - sizeof(uint32_t));
    
    if (!writeSystemState(&sysState)) return false;
    
    // Clear error state
    // if (!clearErrorState()) return false;
    
    // Initialize container volumes
    if (!initializeContainerVolumes()) return false;

    // Initialize dosed trackers
    if (!initializeDosedTrackers()) return false;

    return true;
}

// ============================================================================
// LOW-LEVEL I/O
// ============================================================================

bool FramController::readBytes(uint16_t address, void* buffer, size_t length) {
    if (address + length > FRAM_SIZE_BYTES) {
        Serial.println(F("[FRAM] Read out of bounds!"));
        return false;
    }
    
    uint8_t* buf = (uint8_t*)buffer;
    
    Wire.beginTransmission(FRAM_I2C_ADDRESS);
    Wire.write((uint8_t)(address >> 8));    // MSB
    Wire.write((uint8_t)(address & 0xFF));  // LSB
    
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    // Read in chunks (Wire buffer is limited)
    size_t remaining = length;
    while (remaining > 0) {
        size_t chunk = min(remaining, (size_t)32);
        
        Wire.requestFrom((uint8_t)FRAM_I2C_ADDRESS, (uint8_t)chunk);
        
        for (size_t i = 0; i < chunk && Wire.available(); i++) {
            *buf++ = Wire.read();
        }
        
        remaining -= chunk;
        
        if (remaining > 0) {
            // Set address for next chunk
            address += chunk;
            Wire.beginTransmission(FRAM_I2C_ADDRESS);
            Wire.write((uint8_t)(address >> 8));
            Wire.write((uint8_t)(address & 0xFF));
            Wire.endTransmission();
        }
    }
    
    return true;
}

bool FramController::writeBytes(uint16_t address, const void* data, size_t length) {
    if (address + length > FRAM_SIZE_BYTES) {
        Serial.println(F("[FRAM] Write out of bounds!"));
        return false;
    }

    framBusy = true;
    const uint8_t* buf = (const uint8_t*)data;

    // Write in chunks (Wire buffer is limited to 32 bytes including address)
    size_t remaining = length;
    while (remaining > 0) {
        size_t chunk = min(remaining, (size_t)30);  // 30 data + 2 address

        Wire.beginTransmission(FRAM_I2C_ADDRESS);
        Wire.write((uint8_t)(address >> 8));
        Wire.write((uint8_t)(address & 0xFF));

        for (size_t i = 0; i < chunk; i++) {
            Wire.write(*buf++);
        }

        if (Wire.endTransmission() != 0) {
            framBusy = false;
            return false;
        }

        address += chunk;
        remaining -= chunk;
    }

    framBusy = false;
    return true;
}

bool FramController::clearArea(uint16_t address, size_t length) {
    uint8_t zeros[32];
    memset(zeros, 0, sizeof(zeros));
    
    while (length > 0) {
        size_t chunk = min(length, sizeof(zeros));
        if (!writeBytes(address, zeros, chunk)) {
            return false;
        }
        address += chunk;
        length -= chunk;
    }
    
    return true;
}

// ============================================================================
// HEADER
// ============================================================================

bool FramController::readHeader(FramHeader* header) {
    return readBytes(FRAM_ADDR_HEADER, header, sizeof(FramHeader));
}

bool FramController::writeHeader(const FramHeader* header) {
    return writeBytes(FRAM_ADDR_HEADER, header, sizeof(FramHeader));
}

// ============================================================================
// CHANNEL CONFIG (ACTIVE)
// ============================================================================

bool FramController::readActiveConfig(uint8_t channel, ChannelConfig* config) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_ACTIVE_CH(channel);
    return readBytes(addr, config, sizeof(ChannelConfig));
}

bool FramController::writeActiveConfig(uint8_t channel, const ChannelConfig* config) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_ACTIVE_CH(channel);
    return writeBytes(addr, config, sizeof(ChannelConfig));
}

// ============================================================================
// CHANNEL CONFIG (PENDING)
// ============================================================================

bool FramController::readPendingConfig(uint8_t channel, ChannelConfig* config) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_PENDING_CH(channel);
    return readBytes(addr, config, sizeof(ChannelConfig));
}

bool FramController::writePendingConfig(uint8_t channel, const ChannelConfig* config) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_PENDING_CH(channel);
    return writeBytes(addr, config, sizeof(ChannelConfig));
}

// ============================================================================
// DAILY STATE
// ============================================================================

bool FramController::readDailyState(uint8_t channel, ChannelDailyState* state) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_DAILY_CH(channel);
    return readBytes(addr, state, sizeof(ChannelDailyState));
}

bool FramController::writeDailyState(uint8_t channel, const ChannelDailyState* state) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_DAILY_CH(channel);
    return writeBytes(addr, state, sizeof(ChannelDailyState));
}

bool FramController::resetAllDailyStates() {
    ChannelDailyState emptyState;
    memset(&emptyState, 0, sizeof(emptyState));
    emptyState.crc32 = calculateCRC32(&emptyState, sizeof(ChannelDailyState) - sizeof(uint32_t));
    
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!writeDailyState(i, &emptyState)) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// SYSTEM STATE
// ============================================================================

bool FramController::readSystemState(SystemState* state) {
    return readBytes(FRAM_ADDR_SYSTEM_STATE, state, sizeof(SystemState));
}

bool FramController::writeSystemState(const SystemState* state) {
    return writeBytes(FRAM_ADDR_SYSTEM_STATE, state, sizeof(SystemState));
}

// // ============================================================================
// // ERROR STATE
// // ============================================================================

// bool FramController::readErrorState(ErrorState* state) {
//     return readBytes(FRAM_ADDR_ERROR_STATE, state, sizeof(ErrorState));
// }

// bool FramController::writeErrorState(const ErrorState* state) {
//     return writeBytes(FRAM_ADDR_ERROR_STATE, state, sizeof(ErrorState));
// }

// bool FramController::clearErrorState() {
//     ErrorState emptyError;
//     memset(&emptyError, 0, sizeof(emptyError));
//     emptyError.error_type = ERROR_NONE;
//     emptyError.affected_channel = 255;
//     emptyError.crc32 = calculateCRC32(&emptyError, sizeof(ErrorState) - sizeof(uint32_t));
    
//     return writeErrorState(&emptyError);
// }

// ============================================================================
// FACTORY RESET
// ============================================================================

bool FramController::factoryReset() {
    Serial.println(F("[FRAM] Factory reset..."));
    
    // Clear entire FRAM
    if (!clearArea(0, FRAM_SIZE_BYTES)) {
        Serial.println(F("[FRAM] Failed to clear FRAM"));
        return false;
    }
    
    // Re-initialize
    return _initializeEmpty();
}

// ============================================================================
// DEBUG
// ============================================================================

void FramController::dumpSection(uint16_t address, size_t length) {
    Serial.printf("[FRAM] Dump 0x%04X - 0x%04X:\n", address, address + length - 1);
    
    uint8_t buffer[16];
    
    for (size_t offset = 0; offset < length; offset += 16) {
        size_t chunk = min((size_t)16, length - offset);
        
        if (!readBytes(address + offset, buffer, chunk)) {
            Serial.println(F("  Read error!"));
            return;
        }
        
        // Address
        Serial.printf("  %04X: ", address + offset);
        
        // Hex
        for (size_t i = 0; i < chunk; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        
        // Padding
        for (size_t i = chunk; i < 16; i++) {
            Serial.print("   ");
        }
        
        // ASCII
        Serial.print(" |");
        for (size_t i = 0; i < chunk; i++) {
            char c = buffer[i];
            Serial.print((c >= 32 && c < 127) ? c : '.');
        }
        Serial.println("|");
    }
}

// ============================================================================
// CONTAINER VOLUME
// ============================================================================

bool FramController::readContainerVolume(uint8_t channel, ContainerVolume* volume) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_CONTAINER_CH(channel);
    return readBytes(addr, volume, sizeof(ContainerVolume));
}

bool FramController::writeContainerVolume(uint8_t channel, const ContainerVolume* volume) {
    if (channel >= CHANNEL_COUNT) return false;
    
    uint16_t addr = FRAM_ADDR_CONTAINER_CH(channel);
    return writeBytes(addr, volume, sizeof(ContainerVolume));
}

bool FramController::initializeContainerVolumes() {
    ContainerVolume emptyVolume;
    emptyVolume.reset();
    emptyVolume.crc32 = calculateCRC32(&emptyVolume, sizeof(ContainerVolume) - sizeof(uint32_t));

    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!writeContainerVolume(i, &emptyVolume)) {
            return false;
        }
    }

    Serial.println(F("[FRAM] Container volumes initialized"));
    return true;
}

// ============================================================================
// DOSED TRACKER
// ============================================================================

bool FramController::readDosedTracker(uint8_t channel, DosedTracker* tracker) {
    if (channel >= CHANNEL_COUNT) return false;

    uint16_t addr = FRAM_ADDR_DOSED_TRACKER_CH(channel);
    return readBytes(addr, tracker, sizeof(DosedTracker));
}

bool FramController::writeDosedTracker(uint8_t channel, const DosedTracker* tracker) {
    if (channel >= CHANNEL_COUNT) return false;

    uint16_t addr = FRAM_ADDR_DOSED_TRACKER_CH(channel);
    return writeBytes(addr, tracker, sizeof(DosedTracker));
}

bool FramController::resetDosedTracker(uint8_t channel) {
    if (channel >= CHANNEL_COUNT) return false;

    DosedTracker tracker;
    tracker.reset();
    tracker.crc32 = calculateCRC32(&tracker, sizeof(DosedTracker) - sizeof(uint32_t));

    return writeDosedTracker(channel, &tracker);
}

bool FramController::initializeDosedTrackers() {
    DosedTracker emptyTracker;
    emptyTracker.reset();
    emptyTracker.crc32 = calculateCRC32(&emptyTracker, sizeof(DosedTracker) - sizeof(uint32_t));

    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        if (!writeDosedTracker(i, &emptyTracker)) {
            return false;
        }
    }

    Serial.println(F("[FRAM] Dosed trackers initialized"));
    return true;
}
// ============================================================================
// CHANNEL LABELS (nazwy kanałów)
// ============================================================================

bool FramController::readChannelLabel(uint8_t channel, ChannelLabel* label) {
    if (channel >= CHANNEL_COUNT || !label) return false;
    uint16_t addr = FRAM_ADDR_LABEL_CH(channel);
    if (!readBytes(addr, label, sizeof(ChannelLabel))) return false;
    uint32_t crc = calculateCRC32(label, sizeof(ChannelLabel) - sizeof(uint32_t));
    if (label->crc32 != crc) {
        memset(label, 0, sizeof(ChannelLabel));
        return false;
    }
    return true;
}

bool FramController::writeChannelLabel(uint8_t channel, const ChannelLabel* label) {
    if (channel >= CHANNEL_COUNT || !label) return false;
    ChannelLabel tmp = *label;
    tmp.crc32 = calculateCRC32(&tmp, sizeof(ChannelLabel) - sizeof(uint32_t));
    return writeBytes(FRAM_ADDR_LABEL_CH(channel), &tmp, sizeof(ChannelLabel));
}

// ============================================================================
// CHANNEL PARAMS (min/max dawki per kanał)
// ============================================================================

bool FramController::readChannelParams(uint8_t channel, ChannelParams* params) {
    if (channel >= CHANNEL_COUNT || !params) return false;
    uint16_t addr = FRAM_ADDR_PARAMS_CH(channel);
    if (!readBytes(addr, params, sizeof(ChannelParams))) return false;
    uint32_t crc = calculateCRC32(params, sizeof(ChannelParams) - sizeof(uint32_t));
    if (params->crc32 != crc) {
        memset(params, 0, sizeof(ChannelParams));
        return false;
    }
    return true;
}

bool FramController::writeChannelParams(uint8_t channel, const ChannelParams* params) {
    if (channel >= CHANNEL_COUNT || !params) return false;
    ChannelParams tmp = *params;
    tmp.crc32 = calculateCRC32(&tmp, sizeof(ChannelParams) - sizeof(uint32_t));
    return writeBytes(FRAM_ADDR_PARAMS_CH(channel), &tmp, sizeof(ChannelParams));
}

// ============================================================================
// SHARED NOTES (pula notatek + per-kanał indeks aktywnej notatki)
// ============================================================================

bool FramController::readSharedNotes(SharedNotes* notes) {
    if (!notes) return false;
    if (!readBytes(FRAM_ADDR_SHARED_NOTES, notes, sizeof(SharedNotes))) return false;
    uint32_t crc = calculateCRC32(notes, sizeof(SharedNotes) - sizeof(uint32_t));
    if (notes->crc32 != crc) {
        memset(notes, 0, sizeof(SharedNotes));
        return false;
    }
    return true;
}

bool FramController::writeSharedNotes(const SharedNotes* notes) {
    if (!notes) return false;
    SharedNotes tmp = *notes;
    tmp.crc32 = calculateCRC32(&tmp, sizeof(SharedNotes) - sizeof(uint32_t));
    return writeBytes(FRAM_ADDR_SHARED_NOTES, &tmp, sizeof(SharedNotes));
}
