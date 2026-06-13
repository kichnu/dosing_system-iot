
/**
 * DOZOWNIK - FRAM Controller
 * 
 * Niskopoziomowa obsługa pamięci FRAM MB85RC256V.
 * Odczyt/zapis surowych bajtów + funkcje dla struktur dozownika.
 */

#ifndef FRAM_CONTROLLER_H
#define FRAM_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "dosing_types.h"
#include "fram_layout.h"

// ============================================================================
// FRAM CONTROLLER CLASS
// ============================================================================

class FramController {
public:
    /**
     * Inicjalizacja - sprawdza obecność FRAM, weryfikuje/tworzy header
     * @return true jeśli FRAM gotowa
     */
    bool begin();
    
    /**
     * Czy FRAM jest zainicjalizowana i gotowa
     */
    bool isReady() const { return _initialized; }
    
    /**
     * Sprawdź obecność FRAM na I2C
     */
    bool probe();
    
    // --- Niskopoziomowe operacje ---
    
    /**
     * Odczytaj bajty z FRAM
     */
    bool readBytes(uint16_t address, void* buffer, size_t length);
    
    /**
     * Zapisz bajty do FRAM
     */
    bool writeBytes(uint16_t address, const void* data, size_t length);
    
    /**
     * Wyzeruj obszar FRAM
     */
    bool clearArea(uint16_t address, size_t length);
    
    // --- Header ---
    
    bool readHeader(FramHeader* header);
    bool writeHeader(const FramHeader* header);
    bool validateHeader();
    
    // --- Channel Config (Active) ---
    
    bool readActiveConfig(uint8_t channel, ChannelConfig* config);
    bool writeActiveConfig(uint8_t channel, const ChannelConfig* config);
    
    // --- Channel Config (Pending) ---
    
    bool readPendingConfig(uint8_t channel, ChannelConfig* config);
    bool writePendingConfig(uint8_t channel, const ChannelConfig* config);
    
    // --- Daily State ---
    
    bool readDailyState(uint8_t channel, ChannelDailyState* state);
    bool writeDailyState(uint8_t channel, const ChannelDailyState* state);
    bool resetAllDailyStates();
    
    // --- System State ---
    
    bool readSystemState(SystemState* state);
    bool writeSystemState(const SystemState* state);

    // --- Container Volume ---

    bool readContainerVolume(uint8_t channel, ContainerVolume* volume);
    bool writeContainerVolume(uint8_t channel, const ContainerVolume* volume);
    bool initializeContainerVolumes();

    // --- Dosed Tracker ---

    bool readDosedTracker(uint8_t channel, DosedTracker* tracker);
    bool writeDosedTracker(uint8_t channel, const DosedTracker* tracker);
    bool resetDosedTracker(uint8_t channel);
    bool initializeDosedTrackers();

    // --- Channel Labels & Params ---
    bool readChannelLabel(uint8_t channel, ChannelLabel* label);
    bool writeChannelLabel(uint8_t channel, const ChannelLabel* label);
    bool readChannelParams(uint8_t channel, ChannelParams* params);
    bool writeChannelParams(uint8_t channel, const ChannelParams* params);

    // --- Shared Notes ---
    bool readSharedNotes(SharedNotes* notes);
    bool writeSharedNotes(const SharedNotes* notes);

    // --- Param Log ---
    bool readParamLog(ParamLog* log);
    bool writeParamLog(const ParamLog* log);
    
    // --- Utility ---
    
    /**
     * Pełny reset FRAM do wartości fabrycznych
     */
    bool factoryReset();
    
    /**
     * Dump sekcji do Serial (debug)
     */
    void dumpSection(uint16_t address, size_t length);
    
    /**
     * Oblicz CRC32
     */
    static uint32_t calculateCRC32(const void* data, size_t length);

private:
    bool _initialized;
    
    /**
     * Inicjalizuj FRAM z pustym headerem
     */
    bool _initializeEmpty();
};

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

extern FramController framController;

// Flag blokująca odczyt FRAM z HTTP podczas zapisu
extern volatile bool framBusy;

#endif // FRAM_CONTROLLER_H