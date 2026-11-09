# session_manager.cpp — dwie sprawy przeniesione z THERMO CONTROL

Kod `src/security/session_manager.cpp` (i `src/hardware/buzzer_controller.cpp`)
jest niemal 1:1 współdzielony z projektem THERMO CONTROL (thermo_control-iot) —
jeden sterownik budowany był na bazie drugiego. Podczas debugowania resetu po
zalogowaniu w THERMO CONTROL (2026-07-13) wypłynęły dwie sprawy, które
prawdopodobnie dotyczą też DOZOWNIKA.

## 1. Stack overflow buzzerTask — ZASTOSOWANE w tym projekcie

**Plik:** `src/hardware/buzzer_controller.cpp`

`xTaskCreate(buzzerTask, "buzzer", 1024, ...)` — 1024 B stosu to za mało dla
FreeRTOS taska na ESP32-S3. W THERMO CONTROL objawiało się to jako
`Guru Meditation Error: Stack canary watchpoint triggered (buzzer)` i restart
urządzenia — dokładnie w momencie, gdy po zalogowaniu przeglądarka odpalała
kilka równoległych żądań HTTP naraz (GUI + kilka wywołań `/api/...`), co
generowało skok obciążenia WiFi/TCP/przerwań na rdzeniu 1. `vTaskDelayUntil()`
pod takim obciążeniem może chwilowo zużyć znacząco więcej stosu niż przy
niskim obciążeniu (idle boot), stąd task działał poprawnie miesiącami zanim
ujawnił się przy pierwszym realnym użyciu GUI.

**Fix zastosowany tutaj (2026-07-13):** stos zwiększony 1024 → 2048 B,
analogicznie do THERMO CONTROL. Build zweryfikowany (`pio run`), nie
testowane jeszcze fizycznie na sprzęcie DOZOWNIKA (nie pod ręką w trakcie tej
sesji) — objaw (reset po zalogowaniu) tutaj się jeszcze nie ujawnił, ale ten
sam wzorzec kodu = to samo ryzyko.

## 2. analogRead(A0) koliduje z GPIO1 — ZNALEZIONE, NIEZAAPLIKOWANE (celowo)

**Plik:** `src/security/session_manager.cpp:92`, funkcja `createSession()`:

```cpp
randomSeed(millis() ^ micros() ^ analogRead(A0));
```

Na wariancie płytki `XIAO_ESP32S3` (framework-arduinoespressif32) `A0` jest
zdefiniowane jako **GPIO 1**:
`~/.platformio/packages/framework-arduinoespressif32/variants/XIAO_ESP32S3/pins_arduino.h:24`
→ `static const uint8_t A0 = 1;`

`analogRead()` przełącza IO_MUX danego GPIO z funkcji cyfrowej na ADC. W
DOZOWNIKU GPIO1 to **`PUMP_MONITOR_TX_PIN`** (`src/config/config.h:127`) —
czyli UART TX do monitora pompy. Efekt tego samego mechanizmu, który w THERMO
CONTROL psuł buzzer (tam GPIO1 = `BUZZER_PIN`), tutaj potencjalnie zerwałby
komunikację UART z monitorem pompy po **pierwszym zalogowaniu** do web GUI —
`digitalWrite`/UART TX na GPIO1 przestałby działać do resetu urządzenia.

**Status:** świadomie NIE naprawione w tej sesji — user poprosił o
ostrożność, bo objaw (błąd `IO 1 is not set as GPIO` / zerwana komunikacja z
monitorem pompy) jeszcze się w DOZOWNIKU jawnie nie ujawnił. Do potwierdzenia
w terenie / przy najbliższej okazji.

**Proponowany fix (taki sam zastosowano w THERMO CONTROL):** zastąpić
`randomSeed()+analogRead(A0)+random()` sprzętowym RNG ESP32, który nie
dotyka żadnego GPIO:

```cpp
#include <esp_random.h>
...
for (int i = 0; i < 32; i++) {
    newSession.token += String(esp_random() % 16, HEX);
}
```

(usuwa też potrzebę `randomSeed()` — `esp_random()` jest kryptograficznie
losowy, nie wymaga seedowania).

**Przed wdrożeniem tutaj:** sprawdzić, czy `PUMP_MONITOR_TX_PIN` (GPIO1) jest
w ogóle aktywnie używany w momencie, gdy `createSession()` jest wołane w
runtime (może komunikacja z monitorem pompy odbywa się tylko w innej fazie i
kolizja czasowo nie szkodzi — do zweryfikowania zamiast zakładać identyczny
scenariusz jak w THERMO CONTROL).
