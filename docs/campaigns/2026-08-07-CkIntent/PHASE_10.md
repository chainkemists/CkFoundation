# Phase 10 — Playground v2: the combat arena rework

> **Status:** 🔨 OPEN (2026-08-10). **Supersedes:** Phase 9's gym DESIGN (the station-zone
> museum) — not its plumbing; the shared source composition, record readers, and key-ledger
> discipline carry forward. **Does not change:** Phases 0-8, the CkIntent module itself, the
> AutoTest corpus. **Verdict of record:** the maintainer drove the Phase-9 gym on 2026-08-09
> and rejected the design — "this gym was a disappointment." The miss: a station-per-mechanic
> museum instead of one playable combat sandbox where the mechanics are the demo.
> **Execution mode:** step-by-step slices, the maintainer PIEs and steers between every slice
> (their explicit offer). Opus dispatches for the bigger slices, orchestrator-inline for the
> steering-sized ones. **Gate regime:** unchanged, [P2-D4] scoped — `Ck_AutoTest_In
> --discover-fresh --parallel 1`, 123/123 BY NAME; re-gate after each slice's `.as` edits
> once the maintainer's editor frees the log lock.

## The maintainer's spec (2026-08-10, near-verbatim)

1. **True top-down, Diablo-model controls.** The camera does NOT move with the mouse and does
   not rotate the character. The mouse cursor is the pointer the character faces. (Old design's
   orbit was "too fast" and pointless — gone entirely, not tuned.)
2. **Standard keys**: WASD, Q, E, R, F, C, LMB, RMB. No brackets/backslashes/semicolons.
3. **The combat kit**: LMB = LightAttack1; LMB again while the attack is still happening
   chains to LightAttack2, again to LightAttack3. Hold LMB to charge, release = Light
   Special. RMB does all of that but Heavy.
4. **Real feedback**: a basic enemy that takes hits, and that shoots something requiring a
   block. The player has no health — blocked-vs-hit IS the feedback.
5. **Combos**: LMB+RMB is a combo, RMB+LMB is a DIFFERENT combo, W (forward)+LMB is a combo,
   etc.

## Rulings

- **[P10-D1] One arena replaces the four stations (maintainer, fork answered 2026-08-10).**
  Station zones, rings, layer push/pop framework, arcade-pad stick freeze: deleted. The combat
  kit absorbs what souls (tap/hold/charge) and sekiro (block/buffer/chain) demoed; debugger
  traffic comes from real combat plus the `Ck_GymPlayground_Diagnostics` exec. The 9 station +
  move-asset files were deleted 2026-08-10 (uncommitted, archived to the session scratchpad
  `phase10_station_archive/` until the phase closes).
- **[P10-D2] Keyboard+mouse first; gamepad parity only after the maintainer signs off on the
  kit** (fork answered 2026-08-10). The stick-octant/QCF story returns with the pad slice if
  at all — [P8-D5] (octants stick-only) is NOT reopened by this phase.
- **[P10-D3] Proposed key assignment** (steerable): LMB light family, RMB heavy family,
  Q = block, E/R/F/C reserved for maintainer-steered exercises. WASD locomotion; W doubles as
  the direction modifier in combos (spec item 5).
- **[P10-D5] LIBRARY BACKLOG (maintainer, 2026-08-10): matcher candidate-lifecycle signals.**
  Release-fire for a tap with a hold-sibling is CORRECT and stays ("when a hold is on the same
  button, it _has_ to fire on release") — but it should be a first-class library concept: a
  game wants ALL the phase signals (button-down / candidate-deferring, hold-threshold-crossed,
  resolved-as-tap, resolved-as-hold) so it can start a 'tap' animation speculatively on press
  and TRANSITION it into the hold-attack animation at threshold. The deferral spans already
  exist internally (the debugger timeline draws them); the feature is exposing them as matcher
  signals/phases. NOT this phase (CkFoundation production fence) — the gym demos the
  equivalent reads game-side off the record until it lands. Candidate for the post-campaign
  frontier alongside O12.
- **[P10-D6] Chain feel = the wind-up/attack/wind-down model (maintainer, 2026-08-10).**
  The slice-2 chain was unplayable-by-design three ways: the chain input was graded on the
  tap COMPLETION (release), the whole attack duration was the window with no wind-down
  concept, and a late tap reset to step 1. Rework (orchestrator-inline, slice 3b): every
  chain step phases 20% wind-up / strike (swing spawns where the wind-up ends, hit-test at
  the strike; a mid-wind-up charge cancels it for free); the chain window = active +
  wind-down; the buffer answers the PRESS row off the record the moment it lands ([P10-D5]
  played game-side - a later charge verdict clears the buffer it briefly held); wind-up
  presses are NOT chain inputs (both the press path and the completion back-stop gate on
  `_WindUpEndFrame`); a step expiring unbuffered arms a 10-frame grace window where a
  same-family tap CONTINUES the chain instead of resetting. Specials phase-exempt (the
  charge was the wind-up).
- **[P10-D7] hold= threshold is the VERDICT point, ~83ms (maintainer, 2026-08-10):**
  "usually the hold threshold is around 70-80ms so that the non-hold attack doesn't feel
  sluggish." Move table hold=45 -> hold=5 (5 frames at the 60Hz cadence); the tap's
  worst-case wait is now bounded at 5f. The old 45 survives ONLY as the pawn's
  `k_ChargeFullFrames` display number (sphere saturation + counter denominator) - verdict
  threshold and charge-ripeness are two different numbers the old design conflated.
- **[P10-D8] Directional combos are grammar SEQUENCES, not chords (orchestrator-ruled 2026-08-10
  under the maintainer's AFK completion mandate).** `L+H` and `H+L` are semantically ONE chord step
  (atoms are a set — module doc: "`6+LP` and `LP+6` are the same step semantically"), so press
  ORDER is inexpressible as a chord. The two distinct combos are two-step sequences with distinct
  terminals: `"L H w=30"` (terminal H) and `"H L w=30"` (terminal L). Sequences share terminals
  without deferring ("sharing a terminal never defers"), so bare-tap latency is untouched; the
  combo resolves at the existing hold-episode resolution (release <5f), winning over the bare tap
  by priority. Holding through the verdict point still resolves the charge — combos are
  tap-sequenced by construction. Priorities (strict per-terminal order): terminal L =
  `W+L` 960 > `H L` 950 > `L hold=5` 900 > `L` 600; terminal H = `L H` 940 > `H hold=5` 890 >
  `H` 590. w=30 (~500ms) steerable.
- **[P10-D9] W+LMB is a real grammar CHORD `"W+L"` — fork option (a) (orchestrator-ruled
  2026-08-10).** The premise that would kill it is FALSE: chord terminals accept HELD partners
  (module doc: "a partner pressed a frame earlier is still down on the terminal's row"), so
  locomotion-held W satisfies the chord on L's press row and completes IMMEDIATELY (within a row
  the chord is asked first). Zero added latency: the chord cause (3f default window) is dominated
  by L's existing concurrent hold cause (5f). Locomotion cannot be starved by the Consume capture
  on W — WASD movement reads engine-level `PC.IsInputKeyDown` (Pawn.as:640-643), outside the
  routed pipeline. W becomes the ledger's third minted key (A/S/D stay claimed-unminted); the
  swap-gate gains a third minted-key wait (a chord terminal contributes EVERY button, and one
  unresolved terminal rejects the whole swap atomically). The bake's "no chord terminals" comment
  dies; the default chord window (3) now governs and stands.
- **[P10-D4] Slice sequencing:** ① controls (camera/aim/movement) → ② light/heavy chains +
  charge specials on shapes → ③ enemy dummy + projectile + block → ④ chord/direction combos →
  ⑤ gamepad parity (if wanted) + ledger/registry/shared-file cleanup + EDITOR-VERIFY rewrite.
  Each slice ends with a maintainer PIE pass before the next opens.

## Slice log

- **Slice 1 — controls (2026-08-10, orchestrator-inline): INSTALLED, awaiting maintainer PIE.**
  Pawn: camera toggle / mouse+stick orbit / arcade-freeze machinery deleted; fixed
  `UCk_CameraLayer_TopDown` pushed once (blend 0); `DoAdvance_CursorAim` deprojects the cursor
  (`DeprojectMousePositionToWorld`, BlueprintCallable → AS-bound), intersects the pawn's
  ground plane, yaws the actor at the hit point; WASD stays view-yaw-relative. PC: Enhanced
  Input actions (Look/LookPad/ToggleView), zone registry/detection, station spawn table all
  deleted; `bShowMouseCursor = true`; source-composition tick + Status/Diagnostics execs kept.
  GameMode doc header updated. Deleted files: `CkPlaygroundGym_StationBase.as`,
  `_Station_{Fighting,Souls,Sekiro,DebugFodder}.as`, `_{Fighting,Souls,Sekiro,DebugFodder}_Moves_Assets.as`.
  `Shared.as` untouched this slice (dead zone/ledger entries prune in slice 5). Re-gate
  pending editor-lock free.
- **Slice 1 ✅ SIGNED OFF by the maintainer in PIE (2026-08-10)** — "Slice 1 is good." One
  tune applied: movement halved (`UFloatingPawnMovement.MaxSpeed` 1200 → 600 in
  `Request_OnPawnReady`).
- **Slice 2 decided design (orchestrator, pre-dispatch):** buttons `L` (LeftMouseButton) and
  `H` (RightMouseButton) minted in `Shared.as`'s composition — WASD stays unminted until the
  slice-4 combo work; old bracket/pad key registrations die now. Move table (new
  `CkPlaygroundGym_Kit_Moves_Assets.as`, the archived souls assets' Declare_Move idiom):
  `"L"` tap + `"L hold=45"` charge sibling, `"H"` + `"H hold=45"`. Chain = game-layer state in
  the PAWN over tap completions (archived sekiro's transition-table pattern): Idle→Light1
  (0.35s)→Light2 (0.45s)→Light3 (0.6s)→Idle, press during window BUFFERS the next step and
  fires when the current one ends; heavy mirror 0.6/0.8/1.0; ONE state machine — the other
  family's presses during a chain are ignored (debugger still shows the completions, sekiro
  precedent). Charge: hold completion at 45f arms Charged (growing sphere while held, souls
  transform-scale precedent), RELEASE fires the Special (0.8s light / 1.2s heavy burst),
  release read game-side off the record ([P10-D5] demo). Feedback anchored on ACTOR FORWARD
  (= cursor aim): PMG shapes whose duration IS the attack duration ([P9-D1] carries over),
  light family cyan, heavy orange, escalating sizes; floating state label above the pawn.
  FIRST-CALLER PROBE: mouse buttons through the Slate input source are UNPROVEN — the kit's
  status/label surfaces must make a dead LMB visibly diagnosable (`Ck_GymPlayground_Status`
  shows L/H latest-press frames).

- **Slice 2 — combat kit (2026-08-10, Opus dispatch): INSTALLED, gate in flight, awaiting
  maintainer PIE.** Drafted by Opus (COMPLETE, no STOPs; every consumed wrapper signature
  cited file:line and the orchestrator re-verified the uncited ones: matcher params fluent
  setter per module doc :92, `FCk_Intent_ButtonNameRow(_Name,_Button)`
  CkIntentCompiledSet_Data.h:414, `FCk_Request_IntentMatcher_SwapSet(_CompiledSet)`
  CkIntentMatcher_Fragment_Data.h:127, parse/bake result accessors incl. `_OffendingIntent`
  :561). Installed: `CkPlaygroundGym_Kit_Moves_Assets.as` (84 — 4 moves, hold=45 siblings
  outweigh taps 900/890 over 600/590), `CkPlaygroundGym_Shared.as` (736→497 — ledger is
  LMB/RMB minted + WASD/Mouse2D/Tab claimed-unminted; zone geometry, SOCD quad, station
  structs all deleted; new `TryGet_Matcher` via layer-priority walk, `Get_IsKeyMinted`,
  `Get_IsButtonHeld`/`Get_HeldButtonNames`), `CkPlaygroundGym_Pawn.as` (250→1275 — layer
  250 + matcher composed from tick with minted-key wait, parse/bake/swap with SET REJECTED
  latch + label, 11-state machine, single-slot buffer fires on step expiry, charge release
  read off record held-run, swing shapes duration==attack length on actor forward, charge
  sphere transform-grow, beat flash bright=acted/dim=ignored, state + input readout lines —
  the input line is the dead-LMB discriminator). Orchestrator touches: breadcrumb comment
  stripped, PC legend updated to the kit string. **Gate ✅ GREEN: 123/123, 0 failed/skipped/
  contaminated, AS compile first try (`Test-Phase10Slice2.log`, 1m14s)** — covers camera fix
  + slice 1 + slice 2, the tree's first gate since Phase-9 close. Awaiting maintainer PIE.

- **Slice 2 PIE feedback round 2 (2026-08-10):** the YZ nameplates read horizontal but
  MIRRORED — wireframe glyphs are readable from exactly one side and yaw 180 showed the
  back. Maintainer directed the better shape: flat on the floor, just above it, BEHIND the
  character. Implemented as a DERIVED transform (mirror-proof by construction): the
  yaw-0 YZ orientation that reads facing the camera, tipped top-edge-away until flat — the
  readable side stays visible through the tilt, composing to `FRotator(-90, CameraYaw, 0)`
  on a YZ plate, anchored `CameraYaw`-backward of the pawn (150/215cm) at floor+3cm. Floor
  text under a -65 camera is ~face-on (sin 65). Readout heights → behind-offsets renamed
  throughout.
- **Slice 2 PIE feedback round 1 (2026-08-10):** (a) FIXED inline — readout text rendered
  vertical + crushed: flat XY text lays its run along world X, the foreshortened depth axis
  under the fixed -65 camera. Reworked to the deleted stations' proven shape: upright YZ
  nameplates yawed at the camera (live `Get_ViewRotation + 180`, not baked), sizes bumped
  52/28 to absorb the view angle. Re-gate folds into the next slice's gate (label-only
  change; `Test-Phase10Slice2.log` superseded). (b) CONFIRMED: the timeline screenshot shows
  `Kit_Light_Charge` hold spans + tap completions — REAL MOUSE BUTTONS ARRIVE through the
  Slate writer; the slice-2 first-caller probe is settled, `L`/`H` are proven keys. (c)
  **DEFERRED FLAG [P10-F1] — CkIntentDebugger timeline scrub UX**: maintainer "can't really
  scrub the timeline or do anything meaningful with it" (click-marker-to-scrub not doing
  anything useful); also the axis ticks render as `5160s..5380s` — a seconds suffix on what
  the header calls the sampler's logic-frame counter, likely a unit mislabel. Maintainer
  explicitly OK deferring; lives in CkGameplayDebugger (`feature/debugger-qol-campaign`),
  tackle as its own unit after the arena slices or hand to the debugger campaign.

- **Slice 3 — enemy + block (2026-08-10, Opus dispatch): INSTALLED, gate in flight, awaiting
  maintainer PIE.** Drafted COMPLETE, no STOPs; every wrapper signature cited file:line and
  the orchestrator attested the agent's three flagged risks in the corpus (`ck::ToEntity(this)`
  — CkAStar/CkAttribute gyms; `Math::Cos` — CkAggro; `.DotProduct` — CkCrowd/CkPathNetwork).
  Installed: `CkPlaygroundGym_Enemy.as` (603, NEW — `UCk_GenericEntityScript_UE`, spawn via
  the CueGym `FCk_Gym_TransformSpawnParams`/`InitialTransform` name-match idiom [agent's
  correct catch: a self-named generated params struct is a first-compile chicken-and-egg];
  PMG sphere torso + base ring, edge-triggered tint with telegraph > hit-flash, floor HITS
  counter on the solved recipe with camera yaw asked of the pawn, one-projectile-in-flight
  700cm/s straight shot resolving at 80cm into `Request_TakeProjectileHit`, clock HELD at
  interval while out of range, DoEndPlay destroys all owned shapes), `Shared.as` (497→522 —
  `B`=Q minted with NO move rows, ledger states the read-not-graded contrast),
  `Pawn.as` (1301→1620 — enemy registration, live-record `Get_IsBlocking` [not tick-cached:
  the projectile resolves on the enemy's own timer tick], YZ block plate riding the cursor
  aim, `DoSpawnSwing(+family,+step)` runs the hit-test from the same numbers the shape drew
  with [cosine arc 55/75, planar both operands], blocked=cyan-at-plate / hit=red-at-body
  beats, input line `+ B h<n>f`, `Get_CameraYaw` accessor, Q deliberately NOT in the
  minted-key swap gate). Orchestrator touch: PC legend updated. **Gate ✅ GREEN:
  123/123, 0 failed/skipped/contaminated, AS compile first try (`Test-Phase10Slice3.log`,
  1m26s; also covers the slice-2 floor-label fix — the cross-file pawn<->enemy reference
  resolved fine).** Awaiting maintainer PIE.

- **Slice 3b — chain-feel rework + hold=5 (2026-08-10, orchestrator-inline): INSTALLED + GATED.**
  Rulings [P10-D6]/[P10-D7] implemented in the pawn + move table (wind-up phases, press-buffer
  chain, 10f grace, hold=45→5 verdict split from `k_ChargeFullFrames=45` display). First gate
  attempt died at editor boot (stale binaries vs the remote `CkWorldSpaceWidget` extraction);
  second died at build (cross-repo break in CkGameplayDebugger, fixed by that repo's own session
  in `fba4d4c`). **Gate ✅ GREEN 123/123** (0 failed/skipped/contaminated, AS compile first try,
  `Test-Phase10Slice3b-BuildTest2.log`, 2026-08-10) — the 3b edits' first-ever compile. PIE feel
  verdict pending maintainer return (waived as a slice gate by the AFK completion mandate).

- **Slice 4 — combos (2026-08-10, Opus dispatch): INSTALLED, gate in flight.** Drafted COMPLETE,
  no STOPs; the load-bearing risk (a bare `W` token being eaten as the `w=` modifier keyword)
  verified in the tokenizer source (`CkIntentGrammar_Utils.cpp:353-363` — a token is a modifier
  only if it equals `lenient` or contains `=`), and the orchestrator re-verified `DoSpawnSwing`'s
  6-param shape, `k_Attack_SpecialForwardOffset`, and the Bake wrapper's chord-window default (3,
  `utils_intent_grammar.as:31-32`). Installed per [P10-D8]/[P10-D9]: move table +3
  (`Kit_Combo_WL "W+L" 960`, `Kit_Combo_HL "H L w=30" 950`, `Kit_Combo_LH "L H w=30" 940`,
  `Declare_Button(W)`, count 4→7; per-terminal priority audit strict); Shared mints
  `k_Key_Forward = EKeys::W` with the dual-read ledger paragraph (capture ends a ROUTING walk;
  locomotion's `IsInputKeyDown` is engine-level); Pawn gains the W ButtonNameRow, third
  minted-key swap-gate wait, three combo states (11/12/13) entered by unconditional phase-exempt
  `DoOnComboLanded` (clears step/buffer/grace, spawns 0.7s swing at extent 170 in three
  off-family hues, floor label `COMBO L-H`/`H-L`/`W+L`), ALL-SEVEN row reads with combos
  dispatched first; PC legend extended. **Orchestrator touches at install:** two comment-audit
  fixes re-applied (ruling-ID breadcrumbs stripped, they predate the draft), and the PC legend's
  stale `hold 45f` → `hold` (wrong player instruction since the hold=5 verdict split).
  **Orchestrator rulings on the drafter's surfaced calls:** enemy tinting a combo hit as heavy
  special ACCEPTED (enemy out of scope, feedback intact); `k_Family_None`+`k_Step_Special` for
  combo swings ACCEPTED (derivations of documented meanings — also what earns the 75° special
  arc). Known interaction kept: a charge landing the same tick stomps a combo (pre-existing
  "charge interrupts everything" rule, unchanged). **Gate ✅ GREEN 123/123** (0 failed/skipped/
  contaminated, AS compile first try, `Test-Phase10Slice4.log`, 1m10s). Honest scope of that
  green: the gym's Parse/Bake run at PIE time, not under autotests — the three new notations'
  acceptance rests on tokenizer-source verification, with the `SET REJECTED` floor latch as the
  PIE tripwire if that reasoning is wrong.

- **Slice 5 — cleanup + close (2026-08-10, orchestrator-inline): EXECUTED.** Shared.as prune:
  `Get_HeldButtonNames`, `Get_LiveOctant`, `TryGet_Matcher` DELETED (zero callers anywhere —
  station-era leftovers; caller census run per symbol across all six gym files).
  `Format_Bool`/`Get_IsArmed` KEPT (live in the pawn's input readout, Pawn.as:1940-1942).
  Registry verified: exactly "Input Key Binding" + "Input Playground". Comment audit: zero
  TODO/breadcrumb/ruling-ID hits across all playground files. `PHASE_10_EDITOR_VERIFY.md`
  (v2 drive script) authored — supersedes PHASE_9's (whose obsolete banner already stands);
  carries the accumulated PIE debt (3b feel + slice-4 combos), the failure-signature table,
  and the open items (gamepad parity not built — "if wanted" + maintainer AFK). Final scoped
  gate on the pruned tree ✅ **GREEN 123/123** (0 failed/skipped/contaminated,
  `Test-Phase10Slice5-Final.log`, 1m9s). **PHASE 10 technically CLOSED under [P2-D4]** —
  every slice built, gated, committed; the PIE-dependent exit criteria are `[EDITOR-VERIFY]`
  debt carried by `PHASE_10_EDITOR_VERIFY.md`.

## Exit criteria

- [x] Maintainer signs off on the Diablo feel (slice 1) in PIE — "Slice 1 is good", 2026-08-10
- [x] Full light/heavy kit: 3-chain, charge specials, verified against baked notation moves —
      BUILT + GATED; 3b chain-FEEL verdict = `[EDITOR-VERIFY]` §2 (AFK debt)
- [x] Enemy takes visible hits; projectile block demonstrably works — BUILT + GATED; PIE pass =
      `[EDITOR-VERIFY]` §4
- [x] Chord + direction combos resolve as distinct moves (LMB+RMB ≠ RMB+LMB; W+LMB) — BUILT +
      GATED per [P10-D8]/[P10-D9]; PIE pass = `[EDITOR-VERIFY]` §5
- [x] Key vocabulary is exactly the standard set — WASD + Q + LMB/RMB live (W minted), E/R/F/C
      reserved-unused, no brackets/backslashes/semicolons anywhere; ledger updated
- [x] Scoped gate 123/123 BY NAME on the final tree (`Test-Phase10Slice5-Final.log`);
      PHASE_9_EDITOR_VERIFY.md superseded by `PHASE_10_EDITOR_VERIFY.md`
- [x] PROGRESS current; comment audit clean; full suite NOT run ([P2-D4])

## NOT in this phase

Player health/death; enemy AI beyond stand-and-shoot; CkFoundation production changes (missing
binding STOPs to the orchestrator); commits without asking; push ever.
