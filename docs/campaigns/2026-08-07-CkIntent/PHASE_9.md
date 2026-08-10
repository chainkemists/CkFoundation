# Phase 9 — `Gym_Input_Playground`: one interactive gym replaces the three text gyms

> **Status:** ✅ CLOSED (2026-08-09) under [P2-D4] — six consecutive scoped gates green
> (final: `Test-Phase9Unit6.log`, 123/123 on the phase-final tree), visuals on the
> `[EDITOR-VERIFY]` queue (`PHASE_9_EDITOR_VERIFY.md`), work UNCOMMITTED pending
> authorization. Note: "five stations" in the exit criteria below reads as FOUR station
> zones (slots 0-3) + the playable skeleton — the tap vocabulary lives inside the fighting
> station; every designed feedback surface landed. **Depends on:** Phase 8 (✅ — the three gyms being replaced,
> the 40-move bake, the shared plumbing) and Phase 7 (✅ — the debugger the fodder station
> points at). **Gate regime:** scoped per [P2-D4] — `Ck_AutoTest_In --discover-fresh
> --parallel 1`, PASS = AS compile green + **123/123 BY NAME** (the Phase-8 close set; gyms add
> no autotest rows). Visuals are `[EDITOR-VERIFY]` by definition. **Design of record:** the
> maintainer-approved design in `CONTINUATION_PROMPT_PlaygroundGym.md` §3 (playable capsule,
> stations as floor zones, shape-first feedback); the maintainer's verbatim request is §2.

## Rulings at phase open

- **[P9-D1] Feedback vocabulary split (maintainer, verbatim ruling 1):** `CkDebugDraw_Utils`
  for single-frame / short-lived events (taps, octant ticks, near-miss puffs); **CkPmg for
  long-running visuals** whose lifetime carries meaning (hold-grow shapes, charge donut,
  block shield plane, buffered-queue shape, combo shapes whose duration IS the attack
  length). PMG duration law: `0` = first-tick destroy, `> 0` = timed, `< 0` = persist until
  explicit destroy. PMG has NO size mutation — growth = `utils_transform` scale on the shape
  entity per tick, or destroy+respawn.
- **[P9-D2] Both devices first-class (maintainer, verbatim ruling 2):** movement and every
  BUTTON action bound on keyboard AND gamepad. **Octant motions stay stick-only per [P8-D5]**
  (structural: the octant derives solely from the conditioned axis pair; a keyboard cannot
  move it — that ruling is not reopened). Station labels state the constraint in plain text
  where it applies.
- **[P9-D3] Deletion + salvage (maintainer, verbatim ruling 3):** the 23 `CkIntentGym_*.as`
  files from `03b977ca` die, along with their 3 registry rows and the panel/SM step-state
  display machinery. SALVAGED into the playground's shared file: the player-source doctrine
  (subsystem source, never synthetic), the idempotent one-per-source composition
  (bias + button map + sampler with SOCD quad, composed from a tick, not a construct), the
  key-audit method and its claimed-key ledger, the priority-band allocation discipline, the
  record-reading helpers (`Get_LatestPressFrame`, `Get_HeldRunFrames` incl. its
  anti-pattern-23 display-only contract, `Get_RowCarriesButton`, live octant/frame reads),
  the anti-pattern-15 attempt bookkeeping (`Request_RecordAttempt` completion-frame
  freshness), and `Get_IsArmed`'s matcher-not-layer read (anti-patterns 18/21).
- **[P9-D4] Movement vs sampler axes — the arcade-cabinet plan:** two concentric PMG rings
  per station. **Outer ring = the station zone:** pawn enters → ring brightens + the station
  pushes its input layer; pawn exits → pop. Layer push/pop is thereby demonstrated by walking.
  **Inner ring (motion stations only) = the arcade pad:** entering freezes STICK locomotion
  so the left stick becomes the fighting-game stick; **WASD locomotion stays live at all
  times** (the keyboard always walks you out — locomotion can never deadlock); leaving the
  inner ring always unfreezes. Gamepad-only escape: **left-stick-click toggles the freeze
  while inside the inner ring** (ring color states the mode). Keyboard direction keys for
  octants do not exist ([P9-D2]); keyboard legs are buttons only.
- **[P9-D5] Key plan is re-derived by unit 9-2, constraints fixed here:** claimed and
  untouchable — gym menu HUD keys (digits, arrows, Home/End/PgUp/PgDn, Enter, Esc, Tab,
  Backspace) and NOW the locomotion set (WASD + the camera-toggle key + left-stick-click +
  gamepad left stick). Freed by [P9-D3]: the old gyms' punctuation cluster, `[`/`]`/`\`/`=`,
  F6/F7/F8/F12/Ins/Del/Hyphen, NumPad operators, and all pad faces/shoulders/triggers —
  EXCEPT F6/F7 which the 8-5 AutoTests now own (`UnbindStopsDelivery` et al) and which stay
  off-limits. The new ledger lives in the shared file header with the same grep-proof
  discipline (`rg --no-ignore -o 'EKeys::…' Script/`), one key = one claimant.
- **[P9-D6] Registry choreography keeps every intermediate state green:** 9-1 ADDS the
  playground row (registry briefly carries 4 intent-gym rows); 9-7 removes the 3 old rows
  with the files. No unit leaves the registry pointing at a deleted GameMode.
- **[P9-D7] Migrated EDITOR-VERIFY legs are contract, not decoration:** criterion 1's leg
  (deferral-frame counter must read 0, green, on a clean pad QCF+P) renders as a floating
  counter above the fighting station's match burst; criterion 5's scrub fodder (failed 236
  with `ck.Intent.RecordScanDiagnostics` on, deferral episode, layer push/pop, octant sweep)
  fires from the debugger-fodder station with a floating label naming the CkIntentDebugger
  view to cross-check. Deleting the old gyms must not orphan either criterion.
- **[P9-D8] First-caller probes happen in 9-1, fail-fast:** the playground is the repo's
  first AS caller of `utils_pmg_debug_shape::Request_SetColor`/`Request_SetVisible` and of
  the `UCk_Utils_DebugDraw_UE` surface. 9-1 wires a spawn-time self-check (one DebugDraw
  burst + one PMG shape that recolors then hides then destroys) so the scoped gate's AS
  compile settles the bindings before any station is built on them. STOP if either surface
  is unbound.

## Units — all Opus dispatches, drafts to scratchpad, orchestrator installs + gates

**9-1 Playable skeleton (CkTests, AS):** `CkPlaygroundGym_GameMode/_PlayerController/_Pawn`.
Pawn = `ACk_Gym_Base_Pawn` subclass per the `ACk_CameraGym_Pawn` recipe: visible capsule
(cylinder + sphere caps, `/Engine/BasicShapes/` + `BasicShapeMaterial`),
`default bAddDefaultMovementBindings = false`, WASD/left-stick planar movement at fixed floor
height polled in Tick against `utils_camera::Get_ViewRotation` yaw, third-person camera
default + one-key top-down toggle (`utils_camera::Add`, `UCk_CameraLayer_ThirdPerson` /
`_TopDown`), spawns `ACk_Gym_Floor` (Z-scale ≥ 0.5). Registry +1 row ([P9-D6]). The [P9-D8]
binding self-check. No stations. Gate: scoped run, 123/123 + compile green.

**9-2 Shared plumbing + zone framework (CkTests, AS):** `CkPlaygroundGym_Shared.as` carrying
the [P9-D3] salvage and the [P9-D5] key ledger; the station-zone framework — PMG double
rings from station anchor transforms (`Get_StationAnchorTransform` still serves placement),
pawn-position enter/exit detection, layer push/pop on the outer ring, [P9-D4] freeze on the
inner; floating PMG text-label helper; the shared feedback helpers (DebugDraw burst shapes,
PMG grow-by-transform, queued-shape idiom). Gate: scoped run.

**9-3 Fighting/QCF station:** octant tick marks light as the stick rolls (DebugDraw), big
PMG burst on match, gray puff on too-slow attempt, floating deferral-frame counter ([P9-D7]
criterion-1 leg). Move tables via the 8-1 `Declare_Move` notation idiom — no per-move structs.

**9-4 Souls station:** tap = small slash (DebugDraw); hold past threshold = PMG shape grown
per-tick via transform scale, erupting at threshold; charge = filling PMG donut. Thresholds
literal in notation + asserted against baked verdicts (the Shared.as constant discipline).

**9-5 Sekiro/block/buffer station:** block hold = persistent PMG shield plane at the capsule;
attack pressed DURING block = dim queued PMG shape that fires the frame block releases;
LightAttack1 → LightAttack2 combo with varying attack lengths as long-running PMG shapes
whose lifetime IS the attack duration ([P9-D1]).

**9-6 Debugger-fodder station:** the [P9-D7] criterion-5 traffic generators, each firing a
visible shape + floating label naming which CkIntentDebugger view to cross-check. CVar armed
on entry / cleared on exit + EndPlay (the 8-2 station-3 discipline).

**9-3 … 9-6 sequential** (all extend the shared file + their own station files). Gate each:
scoped run, compile green + 123/123 by name.

**9-7 Deletion + close (CkTests, AS + docs):** delete the 23 `CkIntentGym_*` files, registry
−3 rows, `Source/CkIntent/Claude.md` gym line updated (CkFoundation touch), comment audit
over all new `.as`, assemble the consolidated `[EDITOR-VERIFY]` drive script (per station:
what to press on BOTH devices, what shapes to expect, which debugger view cross-checks).
Gate: scoped run — proves nothing referenced the deleted files.

## Exit criteria

- [ ] Playground gym compiles, registered, playable skeleton verified by gate (visuals queued)
- [ ] All five stations render shape-first feedback per [P9-D1]; zero text panels
- [ ] The three old gyms deleted; registry net −2 rows vs Phase-8 close; nothing dangling
- [ ] Criterion 1 + criterion 5 `[EDITOR-VERIFY]` legs migrated intact ([P9-D7])
- [ ] Scoped gate green (123/123 BY NAME) on the final tree state
- [ ] Consolidated drive script on the `[EDITOR-VERIFY]` queue; PROGRESS current; comment audit done
- [ ] Full suite NOT run ([P2-D4] — the ship conversation regates)

## NOT in this phase

No CkFoundation production-code changes (a missing binding or API STOPs to the orchestrator);
no new PMG requests (size mutation stays transform-driven); no Jolt/character-controller pawn
(ruled out — flat floor); no synthetic input sources; no `.uasset` hand-authoring; no commits
without asking; no push ever.
