# Testy etapu Z2 — STOP / RESUME

Plan: `home_display-iot/docs/PLAN_DOZOWNIK.md`, etap Z2. Kod: `src/core/node_commands.*`.
Przycisk STOP: Settings kanału, grupa „Pump Control”, pole „Emergency Stop” (po prawej od „Hold to Run”); w stanie zatrzymania w tym miejscu Resume. Pasek „DOSING STOPPED” z przyciskiem Resume pod nagłówkiem.

| # | Scenariusz | Oczekiwane | Wynik |
|---|---|---|---|
| 1 | STOP w trakcie kalibracji | pompa staje od razu, master relay off, pasek „DOSING STOPPED”, STOP znika | |
| 2 | STOP w trakcie „Hold to Run” | jw. | |
| 3 | STOP w trakcie dawki ręcznej / z harmonogramu | pompa staje, event FAILED, „Dziś” nie rośnie o pełną dawkę, dawka nie jest powtarzana | |
| 4 | Kalibracja / hold / dawka ręczna przy zatrzymaniu | odmowa (`STOPPED` / `Scheduler disabled`), pompa nie rusza | |
| 5 | Restart dozownika po STOP | dalej pasek, master relay off, harmonogram wyłączony | |
| 6 | Resume | okno potwierdzenia, potem master relay on, harmonogram działa, pasek znika | |
| 7 | Resume przy błędzie krytycznym | odmowa „Critical alarm active” | |
| 8 | STOP przy błędzie krytycznym | przyjęty, przekaźniki off, błąd zostaje; po Reset alarmu master relay dalej off, aż do Resume | |
| 9 | Druga karta web GUI | po ok. 5 s pokazuje ten sam stan (pasek / brak paska) | |
| 10 | Brak buzzera przy STOP | STOP to działanie użytkownika, nie alarm | |
