# GUI: Configuration panel redesign

**Date:** 2026-06-09  
**Files:** `src/web/html_pages.cpp`

## Changes

### Topbar / card-header
- Moved `state-badge` from inside `card-header` to topbar logo area, next to "DOZOWNIK" text
- Removed `card-header` div entirely (including `channel-number` span and `btn-toggle-enable` button)

### Configuration section (3 → 1)
Merged three separate sections (Dosing Volume, Container Volume, Calibration) into a single `CONFIGURATION` section with a unified `params-grid` layout:

| Desktop (4-col) | Mobile (2-col) |
|---|---|
| Daily Dose / Single Dose / Pump Time / Weekly | 2×2 per group |
| Container Size / Days Left / Remaining / Dosed | same |
| Pump Calibration btn / Measured input | last row |

Action buttons at the bottom: **Save**, **Refill**, **Reset Dosed**.

### CSS cleanup
Removed unused classes: `.volume-row`, `.volume-group`, `.volume-label`, `.volume-unit`, `.calc-grid/*`, `.calc-item*`, `.calib-row`, `.calib-input-group`, `.calib-label`, `.calib-input`, `.volume-group.full-width`, `.calc-item-full`, `.calc-grid .save-btn`, `.bar-group`, `.bar-row/*`, `.buttons-row/*`

Added: `.params-grid`, `.param-item`, `.param-lbl[.low]`, `.param-val[.hl]`, `.param-bar-box`, `.param-bar-val[.low]`, `.params-actions`

## JS compatibility
All element IDs preserved (`dose_N`, `single_N`, `pumpTime_N`, `weekly_N`, `saveBtn_N`, `container_N`, `daysLeft_N`, `remainingLabel_N`, `containerBar_N`, `dosedLabel_N`, `dosedBar_N`, `calibBtn_N`, `calibMl_N`).

## Next
- Visual verification on device
- JS/CSS spaghetti check after view confirmed OK
