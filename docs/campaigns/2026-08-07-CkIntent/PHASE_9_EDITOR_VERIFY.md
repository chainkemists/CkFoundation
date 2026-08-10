# Phase 9 — `Gym_Input_Playground` consolidated drive script

> **☠ OBSOLETE (2026-08-10):** the death condition fired — the maintainer rejected the
> station design after the first drive; Phase 10 rebuilds the gym as one combat arena
> (`PHASE_10.md`). Do not drive this script. Kept for history only; slice 5 writes the v2
> drive script.

> **Freshness:** written 2026-08-09 at phase close, against the tree the `Test-Phase9Unit6.log`
> gate ran on. **Death condition:** superseded the moment any playground `.as` file changes —
> re-derive from the station files' instruction lines, which are generated from the same
> constants the stations listen on and therefore cannot drift.
>
> Setup: PIE `TestGyms_CkTests_Level`, Tab → gym menu → **Input Playground** (or `Ck_Gym_List`
> + `Ck_Gym_GoTo <index>`). Keep the **CK Intent Debugger** docked for the cross-checks.
> `Ck_GymPlayground_Status` is the "why is nothing happening" exec at any point.
> Migrated contract legs: **criterion 1** = step 2.3 (counter reads `DEFER 0f` GREEN on a
> clean pad QCF); **criterion 5** = section 5 (scan-diagnostics fodder + near-miss view).

## 1. Skeleton (unit 9-1)

1. On arrival: white ~90cm capsule (cylinder + sphere caps + nose cube) on a 6000×6000 slab;
   control legend prints for 12s.
2. Self-check shapes ~300cm ahead, straddling in Y: **yellow wireframe DebugDraw sphere**
   (left) vanishes on its own at ~5s; **cyan PMG sphere** (right) turns MAGENTA at ~1s,
   disappears at ~2s, and its entity is GONE from the ECS debugger at ~3s. A sphere that
   vanishes at 1s instead of recoloring = `Request_SetColor` regression.
3. Drive: WASD planar and camera-relative (orbit 90°, `W` still goes into the screen); left
   stick identical with a ~0.15 dead center; pawn never changes altitude.
4. Orbit: mouse and right stick both work (separate sensitivities — flag if the stick feels
   an order of magnitude off; `_LookSensitivity_Gamepad` is the tuning knob).
5. `C` / gamepad view button: blends third-person ↔ top-down over 0.4s, both directions,
   both devices; `Ck_GymPlayground_ToggleView` does the same. `Ck_Gym_Next`/`_Prev` still
   cycle gyms.

## 2. Fighting station — slot 0, "FIGHTING - QCF + PUNCH"

1. Walk in past the ring (~280cm): ring brightens, layer pushes (debugger layer-stack shows
   priority 250). Step onto the inner pad: stick locomotion freezes (Print confirms), WASD
   still walks, stick-click toggles the freeze, leaving the pad always unfreezes.
2. Octant ring: 8 ticks around the pad; exactly one goes bright and follows the stick;
   center = all dim. North = the direction you face reading the label (away from map
   center). Cross-check the debugger rosette frame-for-frame.
3. **[criterion 1]** Roll 236 briskly + pad punch (`Gamepad_FaceButton_Bottom`): green PMG
   burst (center lane) AND the floating counter reads **`DEFER 0f` in GREEN**. This must
   read zero — the law leg.
4. Bare pad punch, stick centered: small blue pip (left lane), `DEFER 0f`.
5. Roll 236 SLOWLY (~1s) then punch: resolves as bare punch — pip PLUS a gray puff (right
   lane). Counter still `DEFER 0f` (the near miss is a display reading, not a grade).
6. Keyboard: tap `;` → pip + counter. Stick-roll 236 then `;` quickly → **this MATCHES the
   keyboard QCF** (burst) — directions and terminals are independent record facts; only
   keyboard-ONLY play lacks the motion (instruction line states it).
7. Debugger: timeline shows press and completion on the SAME frame for the QCF; resolution
   view lists the pad terminal with QCF above bare punch, verdict 0 hold / 0 chord.

## 3. Souls station — slot 1, "SOULS - TAP / HOLD / CHARGE" (no inner ring)

1. `Ck_GymPlayground_Status`: buttons ≥ 10, zones 4. Counter idle `HOLD --`. Stick still
   walks INSIDE this zone (no arcade pad).
2. Tap `[` and `Gamepad_FaceButton_Right`: blue X slash (left lane) ~0.2s, counter flashes
   green `TAP`. The tap answers on RELEASE (it waited on the hold sibling — that wait is
   the lesson; timeline shows the blocked span).
3. Hold `[` ~0.5s, release early: orange sphere (center) grows while down, vanishes on
   release, outcome = slash + `TAP`. No eruption.
4. Hold past 45: counter climbs `HOLD nf / 45f`; AT 45 the sphere is replaced by a larger
   orange eruption (~0.9s), counter `HOLD 45f`. Keep holding: nothing re-fires.
5. Charge: hold `]` / `Gamepad_FaceButton_Top`: purple disc (right lane, facing you) grows,
   `CHARGE nf / 120f`, at 120 → purple eruption + `CHARGE 120f`. Early release → disc
   vanishes + `CHARGE ABANDONED`.
6. Hygiene: start a charge, walk OUT of the zone still holding — the disc disappears
   immediately (no orphaned mesh). Re-enter, re-press: fresh disc from floor scale.
7. Debugger resolution view: hold-sibling verdicts 45f on the two tap/hold keys, 120f on
   the two charge keys; the losing candidate returns to Idle (not Failed) when its sibling
   wins.

## 4. Sekiro station — slot 2, "SEKIRO - BLOCK / BUFFER / COMBO" (no inner ring)

1. Tap attack (`Gamepad_RightShoulder` / `/`) from idle: box at LEFT lane hangs ~0.67s,
   counter `ATTACK 1 <n>f` counting down.
2. Combo: tap attack again while the first box is up: counter `BUFFERED...` (amber) + dim
   amber marker (center); when box 1 expires, a LARGER box appears at the RIGHT lane and
   visibly outlives it (~1.08s vs ~0.67s), counter `ATTACK 2 <n>f`.
3. Third press during Attack 2: nothing changes on the station — but the debugger timeline
   still shows the completion (the module never ate it; the demo chose not to act).
4. Block: hold `Gamepad_LeftShoulder` / `\` from idle and STRAFE: translucent steel-blue
   plane ~60cm in front follows the pawn (orientation stays station-fixed — deliberate).
5. **The headline**: while blocking, tap attack (marker + `BUFFERED...`), then RELEASE
   block: on that frame the shield and marker vanish, the Attack-1 box appears, counter
   flashes green `BUFFER FIRED`.
6. Hygiene: buffer an attack, walk out of the ring still holding block — no orphaned shield
   or marker anywhere; walk back in → idle, clean.
7. Debugger key/state view: block keys sit in the held set attached to NO move; matcher's
   registered capture keys list the two attack terminals only.

## 5. Debugger-fodder station — slot 3, "DEBUGGER FODDER - FEED THE VIEWS" (has inner ring)

**[criterion 5]** Open the debugger's Near-misses tab first.

1. Zone entry arms `ck.Intent.RecordScanDiagnostics` (the tab's warn-dot clears).
2. Slow roll 236 + `Gamepad_FaceButton_Left`: gray puff + CHECK line
   `CHECK: NEAR-MISS VIEW - <verdict>` — a clean-but-slow roll reads **WindowExhausted**, a
   wandering roll **ContiguityBroken**; drive BOTH and confirm the line tracks the ring's
   newest row (frames-examined, step name, walk order in the detail panel; timeline scrubs
   to the terminal frame).
3. Brisk roll + punch: green burst + `MATCHED - COMPARE THE SCAN ROW`.
4. Keyboard leg (`-` = Hyphen): alone → bare tap, no puff; after a stick roll → puff (the
   stick supplied the directions).
5. Deferral: tap `Gamepad_RightTrigger` / `F8`: ~1s of visible NOTHING, then amber burst +
   `CHECK: TIMELINE - BLOCKED LANE, 60f`; the timeline's BLOCKED lane carries a span that
   wide. Hold past 60 instead: orange burst + `CHECK: TIMELINE - HOLD SPAN 60f`.
6. Mask: press `Gamepad_Special_Right` / `F12`: for ~2s the four move keys produce NO
   bursts; layer-stack view shows **225 above 220** with 4 Consume rows; key/state view
   STILL lists the presses (that disagreement is the payload). Mask auto-pops; presses fire
   again. `Ck_GymPlayground_Diagnostics 0/1` is the manual recorder override.
7. Sweep: slow full stick circle — ticks stay half-lit behind you; on the 8th, green burst
   + `CHECK: ROSETTE - ALL 8 + HYSTERESIS` (a lit spoke with the dot in the neighboring
   wedge IS the hysteresis). Exit resets the sweep.
8. CVar hygiene: walk out → near-miss count stops climbing, warn-dot returns. Stop PIE
   while standing IN the zone, restart → warn-dot present on first open (`DoEndPlay`
   cleared it).

## 6. Cross-cutting

- Only ONE station layer on the stack at a time as you walk the ring of five zones
  (layer-stack view); ring-edge loitering does not flap the stack (±20cm hysteresis).
- Zone rings recolor (never blink/respawn) on every crossing.
- Flat labels/instructions readable on the walk-out from spawn; text sizes/heights are the
  expected one-pass PIE tuning item (geometry was authored blind — flag, don't debug).
- Slot 4 is EMPTY (reserved; the five stations occupy 0-3 plus the skeleton's spawn area).
