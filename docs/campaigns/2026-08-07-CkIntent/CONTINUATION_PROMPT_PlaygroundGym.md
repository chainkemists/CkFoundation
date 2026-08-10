# CONTINUATION — Gym_Input_Playground: interactive rework of the Phase 8 intent gyms

**One-liner:** the maintainer drove the three Phase-8 text-panel gyms and rejected them as a
verification surface; this session designs and builds ONE playable gym — a visible-capsule player
character walking a map of stations where every input (punch/kick/hold/charge/combo/block/buffer)
produces immediate visual feedback via CkDebugDraw + CkPmg shapes — and deletes the three text gyms.
Design is approved; scouting is done (findings below); execution has not started.

Read `PROGRESS.md` in this directory before doing anything — it is canonical over this file and over
any memory. This is a **campaign** context: load `meta-campaign` + `ck-methodology`; orchestrate with
Opus dispatches + orchestrator review; record every ruling as a numbered `[P9-Dn]` entry.

---

## 1. Repo state (as of 2026-08-09, end of prior session)

All phases 0–8 of the CkIntent campaign are CLOSED and COMMITTED. Campaign-end full suite was
`1065/1062/3`, delta-zero BY NAME (details in PROGRESS.md). **Push has NEVER been authorized** —
ship is a separate future conversation (`/ck-ship-dev`). **Commits require asking first**; the prior
session's commits were explicitly authorized, that authorization does NOT carry forward.

| Repo | Branch | State |
|---|---|---|
| CkFoundation | `dev` | clean except foreign dirt (below). Campaign commits through `79d9efc72` (docs), CkIntent/CkInput source through `5a2a15595`. |
| CkTests | `dev` | clean. This session's commits: `f67fbf5c` (40-move bake), `c350a184` (coverage-gap tests), `03b977ca` (the three gyms — **the ones being replaced**), `9fa88957` (wrapper regen + 4 populator external actors). Was ~13 behind `origin/dev` pre-session; rebase happens at ship. |
| CkGameplayDebugger | `feature/debugger-qol-campaign` (**NOT dev**) | clean. `0f51a76` = CkIntentDebugger module (27 files). SHIP FLAG: ship conversation must decide cherry-pick-to-dev vs ship-that-branch BEFORE any pointer bump. Do not switch branches under an open editor. |
| CkPlugins (superproject) | `dev` | pointer bumps deferred to ship (publish guard: only pushed SHAs). |

**Foreign dirt — NEVER stage (other sessions own it):** CkFoundation: `Content/CkUsf/GeneratedLooks/*.uasset`
(~80), `docs/reviews/`, `docs/superpowers/`, `docs/campaigns/request-completion-delegates/CONTINUATION_PROMPT_Gate00CloseAndShip.md`.
Superproject: `Config/DefaultGameplayTags.ini`, `CONTINUATION_PROMPT_CrowdDebuggerRuntimeSidewalksInvisible.md`, `_scratch/`,
all submodule gitlink entries.

**Outstanding human queue (do not drop):** the maintainer was mid-EDITOR-VERIFY of phases 7–8 (the
CkIntentDebugger 8-step drive incl. criterion-5 scrub, plus gym drive-throughs) when they made this
request; only the gym verdict ("not great") has been delivered. Debugger-view results, the 0A
hardware spike, and the maintainer review flags (in PROGRESS) are still open.

## 2. The request — maintainer's own words

> "The gym is not great. I understand what you were trying to do, but it's very difficult for me, as
> a dev, to understand what's going on. I would rather we use CkPmg debug shapes that change
> colors/size etc. to essentially have examples of games like Sekiro, Souls, Fighting Games etc. I
> would like you to create an Unreal Player Character, a floor, and these stations strewn throughout
> the map, so that I can go to any station and test what the station is asking me to test. This can
> be a top-down/3rd-person style mini-game where the capsule of the player is clearly visible. Then,
> when I press the punch/kick keys, I clearly see pmg shapes appear and disappear denoting that I
> did that action. Then, hold attacks, combo attacks, blocking and then attacking with input
> buffering. … I want a more interactive version that is easy to test but also _very clearly_ tells
> me that the inputs are working AND the different types of game inputs we support."

Follow-up rulings from the maintainer (treat as decided; record as `[P9-D*]` entries when opening the phase):

1. **DebugDraw for single-frame events, PMG for long-running visuals.** "we don't _have_ to use
   CkPmg for single frame events. We can use CkDebugDraw_Utils.h instead which is designed for
   single frame events. It's just that CkPmg looks better, especially if the attack is long running
   (e.g. showing Light Attack1 comboing into Light Attack2 with input buffering and varying length
   of the attacks)."
2. **Keyboard AND gamepad both first-class.** Movement, attacks, motions — everything bound on both.
3. **Deleting the three gyms is approved** — "if there is nothing to salvage." Salvage identified:
   the shared plumbing in `CkIntentGym_Shared.as` (player-source doctrine, source composition,
   key-conflict audit, priority allocation). The station scripts + text-panel display code die.

## 3. Approved design (presented to and accepted by the maintainer)

One gym, `Gym_Input_Playground` (name negotiable), replacing `Gym_Input_{Fighting,Souls,Debugger}`:

- **Pawn:** subclass `ACk_Gym_Base_Pawn`, visible capsule (cylinder + sphere caps from
  `/Engine/BasicShapes/`, `BasicShapeMaterial`), `default bAddDefaultMovementBindings = false`,
  WASD/left-stick planar movement at fixed floor height via the built-in FloatingPawnMovement
  (`AddMovementInput` polled in Tick against camera yaw — the `ACk_CameraGym_Pawn` recipe). No Jolt
  (flat floor; gravity buys nothing). Third-person camera default (`utils_camera::Add` +
  `UCk_CameraLayer_ThirdPerson`), one key toggles `UCk_CameraLayer_TopDown`. Spawns `ACk_Gym_Floor`.
- **Stations are floor zones, not text panels:** a colored ring (PMG donut/circle) per station; walk
  in → ring brightens and the station **pushes its input layer**; walk out → pop. Layer push/pop is
  thereby demonstrated by movement itself. For motion-input stations, standing in the inner ring
  freezes locomotion so the stick becomes the fighting-game stick (arcade-cabinet semantics).
- **Feedback vocabulary** (anchored to the capsule):
  - Punch/kick tap → instant color-coded DebugDraw burst in front; distinct shape per button.
  - Fighting/QCF station → octant tick marks light as the stick rolls (DebugDraw), big PMG burst on
    match; too-slow attempt → gray puff (near-miss made visible). Floating deferral-frame counter
    above the burst — **criterion 1's EDITOR-VERIFY leg (must read 0, green) migrates here intact**.
  - Souls station → tap = small slash; hold past threshold = PMG shape that grows while held
    (transform-scale per tick — PMG has NO size-mutation request) then erupts; charge accumulator =
    filling donut.
  - Sekiro/block station → block hold = persistent PMG shield plane at the capsule; attack pressed
    DURING block = dim queued PMG shape that fires the frame block releases (visible input
    buffering). LightAttack1 → LightAttack2 combo with varying attack lengths shown as long-running
    PMG shapes whose lifetime IS the attack duration.
  - Debugger-fodder station → same traffic generators as the old Debugger gym (failed 236 scan with
    `ck.Intent.RecordScanDiagnostics` on, deferral episode, layer push/pop, octant sweep), each
    firing a visible shape + floating label naming which `CkIntentDebugger` view to cross-check —
    **criterion 5's scrub fodder migrates here intact**.
- **Registry:** 3 rows ("Input Fighting", "Input Souls", "Input Debugger") collapse to 1 in
  `Script/Common/CkTests_GymRegistry.as`.
- **Gate:** scoped `Ck_AutoTest_In` compile run (gyms add no autotest rows; the 40-move bake +
  coverage-gap tests from `f67fbf5c`/`c350a184` must stay green BY NAME). Visuals → EDITOR-VERIFY
  queue with exact drive steps. No full suite ([P2-D4]; the ship conversation regates anyway).

## 4. Scout findings (verified 2026-08-09, file:line cited — do NOT re-scout these)

### Gym framework / player
- One shared level for all gyms (`Plugins/CkTests/Content/TestGyms/TestGyms_CkTests_Level.umap`),
  one PlayerStart, **no floor in the level** — gyms spawn `ACk_Gym_Floor` themselves (walkable
  surface at actor origin; Z-scale ≥ 0.5 for navmesh). Gym switching = ServerTravel `?game=`
  (`Script/Common/CkGym_Base_GameMode.as:52,75`), registry `Script/Common/CkTests_GymRegistry.as`.
- `ACk_Gym_Base_Pawn : ADefaultPawn` (`Script/Common/CkGym_Base_Pawn..as` — filename really has a
  double dot) = free-fly WASD+mouse, invisible (`bOwnerNoSee` engine mesh), spawns a
  `UCk_EntityScript_WithActor_UE` then calls virtual `Request_OnPawnReady()`.
- **The recipe to copy: `Script/CkCamera/CkCameraGym_Pawn.as`** — the only visible-body gym pawn.
  Visible meshes in `ConstructionScript` (`:54-72`, `/Engine/BasicShapes/` + `BasicShapeMaterial`),
  `default bAddDefaultMovementBindings = false` (`:34`), camera-relative WASD polled in Tick
  (`:176-197`, `AddMovementInput` vs `utils_camera::Get_ViewRotation` yaw), camera via
  `utils_camera::Add` (`:89`) with layer classes `UCk_CameraLayer_ThirdPerson`/`_TopDown`/`_LockOn`
  (`:96-98`) — camera director auto-creates `UCk_CameraComponent`; default PlayerCameraManager reads it.
  Spawns its own floor/pillars (`:111,:133`).
- Station grid framework (`Script/Common/CkGym_Base_PlayerController.as`): `Get_RequiredStations()`
  (`:74`), 800cm auto-grid (`:112-138`), `Request_StartGym()` (`:161`),
  `Get_StationAnchorTransform(tag, ECk_GymStation_Anchor::…)` (`:226-253`). Usable for placement even
  though panels are being abandoned. Stations face world -X (player camera comes from -X).
- Pawn-with-probe precedent (station-overlap detection): `ACkAudioGym_Advanced_Pawn`
  (`Script/CkAudio/CkAudioGym_Advanced_Pawn.as:31-50`).
- **What does NOT exist:** any walking character (zero hits for ACharacter/CharacterMovement/
  SpringArm/Possess/SetViewTarget in all Script/), any shared visible-pawn base, PMG size mutation,
  PMG pulse/fade helper, any AS caller of `utils_pmg_debug_shape::Request_*` (we'd be first).

### Input plumbing (from `Script/CkInput/CkIntentGym_Shared.as` — SALVAGE this file's logic)
- Real device input reaches the intent stack via `FCk_InputSlate_Preprocessor` →
  `UCk_InputSource_Subsystem` (LocalPlayer subsystem) → the PLAYER's source. Gated on viewport
  focus, NOT possession — pawn and intent stack coexist freely. Doctrine comment at
  `CkIntentGym_Shared.as:29-33`: use the player's own source, never a synthetic one.
- `intent_gym::TryGet_PlayerSource()` (`:334-345`); idempotent composition from display tick
  `Request_EnsureSourceComposed()` (`:399-422`) — one bias + one button map + one sampler (SOCD
  quad) per source. Stations own only their layer + matcher at a unique priority (`:299-320`).
- Key-conflict audit (`:150-236`): gym menu HUD claims digits, arrows, Home/End/PgUp/PgDn, Enter,
  Esc, Tab, Backspace (`CkGym_MenuHUD.as:209-210,311,673`) — never bind those. Old gym allocations
  (punctuation + pad faces, `[`/`]`/`\`/`=`, F6/F7/F8/F12/Ins/Del) are freed by the deletion; the
  playground can re-plan, but WASD now joins the claimed set (movement).
- **New risk the old gyms never had:** movement axes vs sampler axes. The sampler reads the gamepad
  left stick; locomotion wants the same stick. The arcade-cabinet freeze (design §3) is the ruling —
  in-zone the stick is game input, out-of-zone it is locomotion. Keyboard: WASD = locomotion always;
  intent DIRECTION keys must be separate (e.g. IJKL or numpad-style) or zone-gated the same way.
  This wants a `[P9-D*]` ruling at phase open.

### Drawing APIs
- **CkDebugDraw** (`Plugins/CkFoundation/Source/CkCore/Public/CkCore/Debug/CkDebugDraw_Utils.h`):
  `UCk_Utils_DebugDraw_UE::DrawDebugSphere/Line/Circle/Circle_PlaneAxis/Point/Arrow/…` — ~35
  BlueprintCallable statics, WorldContext + color + duration + thickness params, DevelopmentOnly.
  Plus `Create_ASCII_ProgressBar` (`:34`). Made for single/short-duration draws. Verify AS binding
  exists early (BlueprintCallable ⇒ normally auto-bound; grep `Script/Generated/` or just call it).
- **CkPmg** three tiers (`Plugins/CkFoundation/Script/Generated/utils_pmg_*.as`): fire-and-forget
  `DrawFilledSphere(...)` (`utils_pmg_basic_shapes.as:14`); child entity `Create_Sphere/Cone/Capsule/Box`
  (`:63,91,98,105`); on-entity `Add_*` (`:119+`, once per entity). Families: basic/flat/angular/
  directional/icon/symbol/text/donut.
- PMG post-creation mutation (`utils_pmg_debug_shape.as`): `Request_SetColor` (`:58`),
  `Request_SetVisible` (`:8`), `Request_SetDuration`, `Request_SetText`, etc. **No size mutation** —
  grow = drive entity transform scale (`utils_transform::Request_SetTransform`) or destroy+respawn.
- **Duration footgun:** PMG `InDuration == 0` (the default) = destroyed on FIRST tick; `> 0` =
  timed; `< 0` = persist until explicit destroy (`utils_entity_lifetime::Request_DestroyEntity`).
  The PMG gym passes `500.0f` for "persistent" (`Script/CkPmg/CkPmgShapesGym_PlayerController.as:281`).
- Transient-shape precedents: `Script/CkCue/CkCueGym_GenericCues.as:56-74` (draw + destroy-on-signal),
  `Script/CkCrowd/CkCrowdGym_Locomotion_PlayerController.as:181,228` (per-tick redraw, Duration=0).

## 5. Gotchas (hard-won; do not re-derive)

| Trap | Rule |
|---|---|
| AS reserves `Cast` | never `auto Cast = ...` — name locals `CastResult`; diagnostics are misleading ("reserved keyword 'auto'") |
| Gamepad key names | `EKeys::Gamepad_RightTrigger` (no `Button` suffix on triggers) |
| AS misc | no adjacent string literals; no `NOT` macro (use `!`); `Math::PI` uppercase; f-strings can't format handles (`utils_handle::Get_DebugName`); delegates must match `const X &in` signatures verbatim; no brace-init TArray defaults; `ck::IsValid(h)` not `h.IsValid()` |
| Request structs | always pass the request STRUCT, never loose params |
| Script/ serialization law | NO `.as` edits while a toolbox run is in flight (AS hot-reload poisons runs); draft to scratchpad, install between runs ([P1A-D2]) |
| No source edits during builds; no builds during test runs | git-status all submodules before gating |
| Toolbox | `./CkAuto/UnrealToolbox.exe --test --test-pattern "Ck_AutoTest_In" --discover-fresh --parallel 1 --output=Saved/Logs/<name>.log --project=D:/Repos/CkPlugins/CkPlugins.uproject`; verdict = trailing `=== Test summary ===` block, never exit code; exit 76 = AS compile failure (results invalid); `--generate` only if .Build.cs/.uplugin changed (they won't — this is Script-only) |
| Editor lock preflight | probe `[IO.File]::Open('D:/Repos/CkPlugins/Saved/Logs/CkPlugins.log','Open','Write','None')` — the maintainer often has the editor open; WAIT, don't fight it |
| Gym framework | stations face world -X; `ACk_Gym_Floor` walkable surface at actor origin, Z-scale ≥ 0.5; gym menu HUD key claims (§4) |
| PMG | duration 0 = first-tick destroy; no size mutation; `Add_*` once per entity; flat-shape 4th float = 2nd in-plane extent |
| Sandboxed agents | dispatch via Agent tool per meta-campaign; agents draft `.as` to scratchpad; orchestrator installs + gates; agents cannot PIE — visuals are `[EDITOR-VERIFY]` |

## 6. Critical files

- `Plugins/CkFoundation/docs/campaigns/2026-08-07-CkIntent/PROGRESS.md` — canonical tracker; open Phase 9 here.
- `Plugins/CkTests/Script/CkInput/CkIntentGym_Shared.as` — salvage source (doctrine, composition, key audit, priorities).
- `Plugins/CkTests/Script/CkInput/CkIntentGym_*.as` (24 files) — the delete set (commit `03b977ca` shows the full list).
- `Plugins/CkTests/Script/Common/CkTests_GymRegistry.as` — 3 rows → 1.
- `Plugins/CkTests/Script/CkCamera/CkCameraGym_Pawn.as` — the visible-pawn/camera/movement recipe to copy.
- `Plugins/CkTests/Script/Common/CkGym_Base_PlayerController.as`, `CkGym_Base_Pawn..as`, `CkGym_Base_GameMode.as` — base classes.
- `Plugins/CkFoundation/Source/CkCore/Public/CkCore/Debug/CkDebugDraw_Utils.h` — single-frame draw API.
- `Plugins/CkFoundation/Script/Generated/utils_pmg_basic_shapes.as`, `utils_pmg_debug_shape.as`, `utils_pmg_donut.as`, `utils_pmg_text_shapes.as` — PMG surface.
- `Plugins/CkFoundation/Source/CkIntent/Claude.md` — module doc; its gym line must be updated when the gym names change.
- `Plugins/CkTests/Script/CkInput/CkAutoTest_Intent_FortyMoveBake.as` + `CkIntent_Moves_Assets.as` — the notation vocabulary; the playground's move definitions should reuse this notation style (Parse/Bake), NOT hand-built structs.

## 7. Ruled out / don't re-investigate

- Jolt character controller for the pawn — rejected: flat floor, fixed-Z FloatingPawnMovement suffices. Revisit only if the maintainer asks for slopes/jumping.
- Synthetic input sources for gym stations — doctrine says player's own source ([P1A ruling, Shared.as header]).
- Keeping the three text gyms alongside — maintainer approved deletion.
- Hand-authoring `.uasset`s — still banned without asking (populator-placed external actors are pipeline artifacts and fine).
- `Ck.AS.Net.*` stub churn, PathNetworkFollower reds, greenfield-prefix full-suite discovery gap — known foreign issues, documented in PROGRESS; not ours.
- The old gyms' panel SM step-state machinery (`CkGym_StationSm`, `Update_StationDisplay_Colored`) — not needed for the playground's shape-first feedback; small floating PMG text labels replace panels. (Registry + GameMode plumbing IS still needed.)

## 8. Recommended flow

1. Read `PROGRESS.md`. Author `PHASE_9.md` (entry criteria, unit split, `[P9-D1..]` rulings incl.
   the DebugDraw/PMG split, both-devices rule, deletion+salvage ruling, the movement-vs-sampler
   key-plan ruling). Add the Phase 9 row + session-log line to PROGRESS. Present the unit split to
   the maintainer briefly (they've approved the design, not the decomposition).
2. Read `CkIntentGym_Shared.as` and `CkCameraGym_Pawn.as` IN FULL before dispatching anything.
3. Suggested units: 9-1 pawn + camera + floor + GameMode + registry swap (playable skeleton, no
   stations); 9-2 shared rework (salvage Shared.as → playground shared file; key re-plan; zone
   framework with layer push/pop + locomotion freeze); 9-3..9-6 stations (fighting, souls,
   sekiro/buffer, debugger-fodder), sequential (shared-file collisions); 9-7 delete the 24 old gym
   files + registry cleanup + Claude.md line. Early in 9-1: probe `Request_SetColor`/`SetVisible`
   from AS (first-ever callers) and DebugDraw AS binding — fail fast if either is unbound.
4. Gate each unit: scoped `Ck_AutoTest_In` run (compile green + the +42-row battery green BY NAME).
   Editor-lock preflight before every run; the maintainer may be in PIE.
5. Close: comment audit over new `.as`, EDITOR-VERIFY drive script (per-station: what to press on
   BOTH devices, what shapes to expect, which CkIntentDebugger view cross-checks it), update
   PROGRESS, ask before committing.

## 9. Suggested first message

> I'm continuing the CkIntent campaign — Phase 9, the interactive playground gym. Read
> `D:\Repos\CkPlugins\Plugins\CkFoundation\docs\campaigns\2026-08-07-CkIntent\CONTINUATION_PROMPT_PlaygroundGym.md`
> fully, then PROGRESS.md in the same directory, before doing anything. The design is approved:
> playable capsule character, stations as floor zones, DebugDraw for single-frame hits, PMG for
> long-running attack/combo/buffer visuals, keyboard + gamepad both first-class, delete the three
> text gyms (salvage the Shared.as plumbing). Open PHASE_9.md with the rulings, show me the unit
> split, then execute. Same rules: scoped gates, Opus dispatches with orchestrator review, ask
> before commits, never push.
