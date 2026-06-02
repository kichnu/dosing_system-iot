# Monitorowanie stanu pompy — dźwięk / wibracje

**Data:** 2026-06-02  
**Kontekst:** Zamiana przekaźników na ULN2003AN + przeniesienie monitorowania stanu pompy z GPIO (styk przekaźnika) na rzeczywistą detekcję pracy mechanicznej.

**Przypadki do wykrycia:**
1. Pompa nie pracuje, a powinna (brak przepływu, zablokowana, uszkodzona)
2. Pompa pracuje, a nie powinna (wyciek sterowania, zwarcie w ULN2003AN)

---

## Opcja A: SW-420 — czujnik wibracji

### Co to jest
Prosty czujnik wibracji ze sprężynką kontaktową. Przy wibracji zwiera kontakt → sygnał cyfrowy HIGH/LOW na GPIO.

### Zalety
- Jeden czujnik = jedna pompa (bezpośrednia korelacja)
- Sygnał cyfrowy — zero przetwarzania sygnału
- Koszt ~1–3 zł/sztuka
- Trivial w kodzie: `digitalRead(PIN_VIBRATION_CH1)`
- Odporne na zakłócenia akustyczne z otoczenia

### Wady
- Wymaga mechanicznego kontaktu z obudową pompy (montaż przez przyklejenie lub opaskę)
- Próg czułości regulowany potencjometrem na module
- Przy bardzo słabych pompach może nie reagować

### Schemat podłączenia
```
SW-420 VCC  → 3.3V
SW-420 GND  → GND
SW-420 DO   → GPIO ESP32-S3 (INPUT_PULLUP)
```

### Integracja z kodem
Zastępuje obecną logikę `GPIO_VALIDATION` w `RelayController`:
- Faza PRE: brak wibracji przed startem → OK
- Faza RUN: wibracja obecna w ciągu ~500ms od startu → pompa potwierdzona
- Faza POST: brak wibracji po zatrzymaniu → OK (brak "stuck")

---

## Opcja B: Edge Impulse TinyML — klasyfikacja dźwięku

### Co to jest
Platforma do trenowania modeli ML dla urządzeń embedded (TinyML). Pozwala wytrenować klasyfikator audio i wyeksportować go jako bibliotekę Arduino/PlatformIO.

**Strona:** https://edgeimpulse.com (bezpłatny plan wystarczający)

### Mikrofon: INMP441 (I2S)
- Cyfrowy mikrofon MEMS, interfejs I2S
- Koszt ~10–20 zł
- Bezpośrednio obsługiwany przez ESP32-S3 (I2S peripheral)
- Biblioteka: `driver/i2s.h` (ESP-IDF) lub `ESP_I2S` (Arduino)

```
INMP441 VDD  → 3.3V
INMP441 GND  → GND
INMP441 SD   → GPIO (I2S Data)
INMP441 WS   → GPIO (I2S Word Select / LRCLK)
INMP441 SCK  → GPIO (I2S Bit Clock)
INMP441 L/R  → GND (lewy kanał)
```

### Pipeline trenowania (Edge Impulse)

1. **Zbierz dane** (~2–5 min każda klasa):
   - `pump_running` — nagranie pracującej pompy
   - `pump_stopped` — cisza / tło bez pompy
   - `noise` (opcjonalnie) — inne dźwięki otoczenia

2. **Create Impulse:**
   - Window size: 1000 ms, Stride: 500 ms
   - Processing block: **MFCCs** (Mel-Frequency Cepstral Coefficients)
   - Learning block: **Classification (Keras)**

3. **Trenowanie:** ~20–50 epok, model ~20–80 KB RAM

4. **Eksport:** `Arduino library` → `.zip` → PlatformIO `lib/`

5. **Wynik:** funkcja `classifier.run()` zwraca:
   ```cpp
   result.classification[0].label  // "pump_running"
   result.classification[0].value  // 0.0–1.0 (pewność)
   ```

### Zalety TinyML
- Odporna na fałszywe alarmy (ignoruje przypadkowe hałasy)
- Jeden mikrofon może obserwować kilka pomp (przy sekwencyjnej pracy)
- Model działa w pełni lokalnie, bez chmury
- ESP32-S3 ma wystarczającą moc obliczeniową i RAM

### Wady
- Wymaga zebrania danych treningowych (jednorazowo)
- Przy jednoczesnej pracy kilku pomp — trudniejsza separacja
- Model trzeba retrenować przy zmianie typu pompy
- Zajmuje część zasobów ESP32-S3 (RAM ~50–100 KB, Flash ~100–200 KB)

### Alternatywa: dedykowany MCU
Jeśli ESP32-S3 jest przeciążony lub dla separacji odpowiedzialności:
- **Arduino Nano 33 BLE Sense** — wbudowany mikrofon PDM + wsparcie TinyML
- Komunikacja z głównym ESP32-S3 przez UART lub I2C
- Wynik: prosty sygnał `PUMP_OK` / `PUMP_FAULT` na dedykowanym GPIO

---

## Porównanie

| Kryterium             | SW-420           | TinyML + INMP441         |
|-----------------------|------------------|--------------------------|
| Złożoność             | Niska            | Średnia–wysoka           |
| Koszt (4 pompy)       | ~8–12 zł         | ~15–20 zł (1 mikrofon)   |
| Selektywność          | Wysoka (per pompa)| Wysoka (klasyfikacja)   |
| GPIO                  | 1 per pompa      | 3 (I2S) + 0 per pompa   |
| Odporność na hałas    | Wysoka           | Bardzo wysoka            |
| Rozróżnienie pomp     | Tak (1:1)        | Trudne przy równoległej pracy |
| Czas wdrożenia        | Godziny          | Dni (zbieranie danych)   |

---

## Rekomendacja

Dla systemu z 4–6 pompami pracującymi **sekwencyjnie** (mutex w `RelayController`):
- **TinyML jest wykonalne** — w danej chwili pracuje tylko jedna pompa
- Jeden mikrofon INMP441 umieszczony blisko pomp
- Model klasyfikuje: czy w tej chwili słychać pompę → TAK/NIE

Dla maksymalnej niezawodności: **SW-420 jako backup** na każdej pompie + TinyML jako dodatkowa weryfikacja.
