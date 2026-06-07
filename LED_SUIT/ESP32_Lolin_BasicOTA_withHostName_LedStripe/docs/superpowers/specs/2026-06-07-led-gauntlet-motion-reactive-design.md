# LED Gauntlet — Motion-Reactive Colors & Power Hardening

**Date:** 2026-06-07
**Project:** `LED_SUIT/ESP32_Lolin_BasicOTA_withHostName_LedStripe`
**MCU:** ESP32-C3 (Lolin C3 Pico)
**Status:** Design approved — ready for implementation planning

## Goal

A wearable gauntlet of 25× WS2812 LEDs that idles as a rotating rainbow and reacts
to arm motion: a **forward swing biases the strip red**, a **backward swing biases it
green**, and the **strength of the swing** drives how strong the bias (and brightness)
becomes. Buttons already handle pause and deep sleep; a LiPo 1S battery is the only
power source, gated by a power switch during sleep/off.

This design covers two things: (1) the **motion → color** firmware that is currently
missing, and (2) **power-architecture hardening** (the LED rail and the power switch).

## Visual references

Interactive HTML diagrams used during brainstorming (open in a browser). Each records the
option that was chosen:

| Diagram | Question it framed | Choice |
|---|---|---|
| [`diagrams/motion.html`](diagrams/motion.html) | What does "forward/backward" physically mean? | **B — arm swing about the shoulder** |
| [`diagrams/colormap.html`](diagrams/colormap.html) | How should motion change the rainbow? | **B — channel push** (C = hue shift as fallback flavor) |
| [`diagrams/power.html`](diagrams/power.html) | LED rail: boost to 5 V vs direct 3.7 V (+ wiring gotchas) | **Option 1 — 5 V boost rail** |
| [`diagrams/power-tree.html`](diagrams/power-tree.html) | How both rails come from one battery; THT parts | confirms Option 1; IRLZ44N is the THT switch |
| [`diagrams/switch.html`](diagrams/switch.html) | Correct power-switch wiring | **Logic-level N-FET, low-side** (see BOM for part), blank data before sleep |

## Decisions made during brainstorming

| Decision | Choice | Why |
|---|---|---|
| Physical gesture | **Arm swing about the shoulder** (rotation) | Cleanest signal; near-pure rotation |
| Sensor signal | **Gyroscope, single signed axis** | Signed (gives direction), gravity-independent; replaces the old `sqrt(x²+y²+z²)` magnitude, which discards direction and rests at ~9.8 |
| Color behavior | **Channel push** (rainbow base, push red/green channel, dim the others) | User preference; dimming off-channels keeps it vivid (avoids "muddy") |
| LED rail | **Boost to a steady 5 V** | Full, true color and constant brightness as the battery drains |
| Level shifter | **74AHCT125 (DIP-14)** powered from 5 V only | Reads 3.3 V GPIO as a valid HIGH; gives clean 5 V data into the strip |
| Power switch | **IRLB8721PbF (TO-220, logic-level)** low-side; IRLZ44N / STP55NF06L as alternates | Through-hole; fully on from a 3.3 V gate; handles >>1.3 A; the 4N35 opto is removed from the power path |
| Brightness | **Reacts to swing strength** with a hard floor | Tunable; floor fixes the prior "brightness → 0 when still" problem |

## Hardware

### Bill of materials (additions / changes)

- **MT3608-class boost module** (THT): set output to 5.0 V; size for ≥1.5 A continuous.
- **74AHCT125** in DIP-14 (THT): one buffer used for data level shifting.
- **Low-side power MOSFET** (TO-220, THT, logic-level): one of, in order of preference —

  | Part | Why | Notes |
  |---|---|---|
  | **IRLB8721PbF** (primary) | Modern default; optimized for low-Vgs (fully on well below 3.3 V), very low R_ds(on), cheap, widely stocked | Closest "better IRLZ44N" |
  | **STP55NF06L** | Modern ST part, very low R_ds(on), well stocked | Good second source |
  | **FQP30N06L** | Cheapest/ubiquitous; Vgs(th) up to 2.5 V so slightly less enhanced at 3.3 V (fine at 1.3 A) | Avoid if ever PWM-switching hard |
  | **IRLZ44N** | The original; older but perfectly serviceable | Use what you have if on hand |

  Wiring is identical for all: gate ← GPIO21 via ~220 Ω, 10 kΩ pulldown gate→GND.
  At ~1.3 A on/off the choice is about gate-drive margin and availability, not heat
  (dissipation is tens of mW for any of them).
- LiPo 1S pack with enough C-rating for ~2 A peaks (boost input current at full white).
- **Remove:** 4N35 optocoupler from the power path (it cannot carry the strip current;
  ~150 mA absolute max, low CTR).

> Trap to avoid: cheap blue `IRF520` MOSFET modules are **not** logic-level and will not
> fully turn on at 3.3 V. Use IRLZ44 / D4184 (logic-level) modules if buying a module.

### Power tree

- Battery → Lolin C3 Pico onboard regulator → 3.3 V → ESP32 (existing, unchanged).
- Battery → boost module → **5 V rail** → strip V+ **and** 74AHCT125 VCC.
- GPIO1 (3.3 V data) → 74AHCT125 input → **5 V data** → strip DIN.
- Strip GND → MOSFET drain; MOSFET source → battery GND. **Common ground throughout.**
- Gate of MOSFET ← GPIO21 (`MOSFET_CTRL_PIN`) via ~220 Ω, with 10 kΩ pulldown gate→GND.

### Power switch note (low-side switching gotcha)

Because the switch cuts the strip's **GND**, a data pin left HIGH while the strip is "off"
back-powers the strip through the WS2812 data-pin clamp diode (faint glow / leakage).
Mitigation is in the firmware sleep sequence (below). Optional future upgrade: a high-side
P-FET load switch on V+ removes the back-power path entirely.

## Firmware

### Sensor pipeline (replaces acceleration-magnitude logic)

1. **Swing-axis selection:** a one-time helper prints `g.gyro.x/y/z`; swing the arm,
   observe which axis carries the largest signed swing, set `SWING_AXIS` accordingly.
2. **Bias calibration at boot:** average ~100 samples while still → `gyroBias`; subtract
   from every subsequent reading.
3. **Responsiveness:** raise `setFilterBandwidth` from `MPU6050_BAND_5_HZ` to ~`21_HZ`;
   sample every ~20–30 ms (was 200 ms). Keep EMA smoothing (`alpha ≈ 0.25`).
4. **Deadzone:** below `SWING_DEADZONE` → `signal = 0` → pure rainbow.
5. **Signed scaling:** `signal = clamp(rate / SWING_FULL_SCALE, -1, +1)`,
   `SWING_FULL_SCALE ≈ 300 °/s` (tunable). Sign → red/green; `|signal|` → intensity.

### Color rendering (channel push over rainbow)

Per pixel, per frame:

- `base = Wheel(...)` — existing rainbow remains the idle look.
- `mag = |signal|`.
- Forward (`signal ≥ 0`): push **red** toward 255 by `mag·CHANNEL_PUSH`; dim **green** and
  **blue** by `mag·OFFCHANNEL_DIM`.
- Backward: push **green**; dim **red** and **blue**.
- **Brightness:** interpolate from `IDLE_BRIGHTNESS` up to `MAX_BRIGHTNESS` by `mag`, never
  below `MIN_BRIGHTNESS`.
- When the swing ends, the EMA decays `mag → 0`, so the tint melts back to rainbow
  automatically (no explicit state needed).

### Sleep / power-down sequence

On deep-sleep request and on "off": `strip.clear(); strip.show();` (push all-black), drive
the data pin low, **then** `digitalWrite(MOSFET_CTRL_PIN, LOW)`. Closes the back-power path.

### Tuning knobs (`#define` / `const`)

`SWING_AXIS`, `SWING_FULL_SCALE`, `SWING_DEADZONE`, `ALPHA_SMOOTHING_FACTOR`,
`CHANNEL_PUSH` (≈0.85), `OFFCHANNEL_DIM` (≈0.8), `IDLE_BRIGHTNESS`, `MAX_BRIGHTNESS`,
`MIN_BRIGHTNESS`, sample interval. One flag to disable brightness-reacts-to-motion and
keep a pure idle pulse instead.

## Testing

1. Serial-plot the raw `SWING_AXIS` gyro value, then the smoothed signed `signal`, while
   swinging: confirm forward/backward give **opposite signs** and rest sits near **0**.
2. On-strip: still = rainbow; forward = red bias; backward = green bias; harder swing =
   more saturation **and** brightness.
3. Sleep/off: confirm **no residual glow** with the strip powered down.

## Out of scope (intentionally)

- No restructuring of the single-file `.ino` into modules.
- No change to the mixed FastLED (status LED) / Adafruit_NeoPixel (strip) setup.
  **This split is intentional, not accidental:** on the ESP32-C3, installing more than
  one FastLED (RMT) controller causes continuous reboots, so the strip is driven by
  Adafruit_NeoPixel on a second library instead. Do **not** merge both LED groups into
  FastLED without first confirming the C3 multi-controller bug is resolved — doing so
  reintroduces the reboot loop.
- No rewrite of the rainbow / `Wheel()` math.

### Possible future cleanup (separate effort)

The user would prefer a single library (FastLED is more flexible than NeoPixel). Worth
checking, as its own task, whether a newer FastLED release with the RMT5 / ESP-IDF 5
driver fixes the C3 multi-controller crash — if so, the strip could move to FastLED and
the NeoPixel dependency dropped. Out of scope for this feature.

## Deferred task

After this feature is finalized, do a **code-coherence pass** over the `.ino`:
- Remove dead remnants (`accelMagnitude`, the commented-out `map()` brightness block).
- Check the new gyro/color path against the `LedsActivityStatus` state machine and the
  `EVERY_N_MILLISECONDS` cadences.
- Confirm the sleep sequence ordering and the FastLED/NeoPixel interaction.
