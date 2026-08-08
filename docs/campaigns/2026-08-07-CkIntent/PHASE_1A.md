# Phase 1a — `Gym_Input_KeyBinding` (regression net for existing CkInput)

> **Status:** ✅ Code-complete, gate green (2026-08-08) — full suite `1012/1010/2` delta-zero vs
> baseline, 7 new AutoTests + 5-station gym landed, one shipped defect found+fixed (dead
> change-signal hook — see PROGRESS.md). **Open:** `[EDITOR-VERIFY]` steps (human) + commit
> (withheld pending user authorization; exit-criteria items coupled to the commit land with it).
> **Depends on:** Phase 0 research (done for the parts this needs). **Does NOT depend on the 0A
> hardware spike** — PHASE_0.md records the gate exemption.
> **Estimate:** 2–3 days — raised from 1–2 after the content prerequisite below was found.
> **Scope:** `CkTests` only. ~~No host-project config change~~ **AMENDED by [P1A-D4]** (2026-08-08):
> one host config line is REQUIRED — `bEnableUserSettings=True` in `Config/DefaultInput.ini` — because
> the engine defaults EI user settings OFF (`EnhancedInputDeveloperSettings.cpp:17`) and the whole
> rebinding surface is un-instantiable without it. Scan paths remain unconfigured. No `.uasset` —
> input content is authored as AngelScript script-literal assets (see 1a-0). **No production code in
> `CkInput` or `CkIntent`** — 1a-0's fallback wrapper proved unnecessary
> (`RegisterInputMappingContext` is `BlueprintCallable`, `EnhancedInputUserSettings.h:804-805`).

## Goal

After this gate: a human can open one gym, rebind a key, watch the glyph change, force a conflict,
swap it, reset to default, hot-swap controller, and see every result on screen — against the
**already-shipping** `UCk_Utils_KeyBinding_UE` / `UCk_Utils_KeyIcon_UE` surface, with no new
framework code involved.

## Why this is first

**VERIFIED:** that surface has **zero** coverage. A census of `Plugins/CkTests` for
`KeyBinding` / `KeyIcon` / `utils_key` returns nothing — no gym, no AutoTest. It is 361 lines of
shipped remap / conflict / swap / reset / persist logic plus glyph resolution, and CkGameSettings'
`UCk_GameSettingsUI_KeyBindingPageWidget` is being built on it right now.

Building this before touching `CkInput` means the rest of the campaign extends a module that has a
regression net, instead of one that has never been exercised. It also derisks the sibling campaign
at no extra cost.

## Entry criteria (pre-flight — run these, don't assume them)

- [ ] Baseline re-confirmed still valid: `1005 / 1003 / 2` with
      `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward` and
      `..._ProjectsRibbonWaypointWithinNavQueryExtent` failing. **Re-capture if the tree's CkTests
      branch or pointer moved** — it was `fd26b553` on `backup/pre-branch-audit-265-gfd26b553`.
- [ ] Read `Script/CLAUDE.md` (AS rules) and `ck-tests-authoring-and-running` before authoring.
- [ ] Read the exemplars named below and confirm their shapes still match current doctrine.

## Work items

Each names the proven pattern it replicates.

### 1a-0 — Input content prerequisite, authored in AngelScript (DO THIS FIRST)

**VERIFIED 2026-08-08:** no `IMC_*`/`IA_*` assets and no `MappingContextScanPaths` config exist in
this project, so today `Get_AllRemappableKeys` returns an **empty array** and the whole gym would
appear to pass while testing nothing.

**Author the content in AngelScript — do NOT create `.uasset` files.** This project is game-agnostic
plugin work and the standing preference is C++/AngelScript over Blueprint/binary assets.
Script-literal assets of *engine* types are an established pattern here: `Script/curves.as` declares
30 × `asset CurveFloatEaseX of UCurveFloat` inside `namespace curve`, each with an imperative body.
`UInputAction` and `UInputMappingContext` are both `UDataAsset` subclasses and take the same shape.

**The asset-registry scan is NOT required.** `UCk_KeyBinding_Subsystem::Initialize` uses the scan
only as a *feeder* — the real mechanism is `Settings->RegisterInputMappingContext(IMC)`
(`CkKeyBinding_Subsystem.cpp:58`). An AS-authored IMC passed straight to that call registers its
mapping rows identically. So Phase 1a needs **no host-project config change** and stays CkTests-only.

1. Declare the input actions and one mapping context as AS script-literal assets, with several
   player-mappable keys carrying display metadata — **two mappings must be able to collide**, or the
   conflict station has nothing to test.
2. Register them on the local player via `Get_InputUserSettings(PC)` →
   `RegisterInputMappingContext(...)`.

→ **verify:** `Get_AllRemappableKeys` returns a non-empty array whose count equals the mappable keys
declared. **If it returns empty, stop — nothing below is meaningful.**

⚠ **One unverified hop:** `Get_InputUserSettings` is AS-exposed (`utils_key_binding.as:86-89`), but
whether the engine's `RegisterInputMappingContext` is itself `BlueprintCallable` — and therefore
AS-visible — is **UNVERIFIED**. The AS compile fails loudly if not. Fallbacks, in order of
preference: (a) add a `Register_MappingContext` wrapper to `UCk_Utils_KeyBinding_UE` — arguably a
genuine gap in that surface given D2 makes EI the binding store and non-negotiable #4 demands all
three environments; (b) a small C++ helper in CkTests.

### 1a-1 — Gym scaffold
Replicate the station pattern from `CkTests/Script/CkAStar/CkAStar_GymStation.as` (spawn-params
USTRUCT + `UCk_GenericEntityScript_UE` subclass with `ExposeOnSpawn`), and the
GameMode/PlayerController pair from `CkGymStation_Showcase/`.
→ **verify:** the gym appears in the cycler and loads with an empty station list.

### 1a-2 — Binding inspection station
Surface `Get_AllRemappableKeys`, `Get_KeyForMapping`, `Get_MappingNamesForKey`,
`Get_MappableKeyInfoFromInputAction` on screen — one row per mapping, live.
→ **verify:** every mapping in the active profile renders with its current key and display name.

### 1a-3 — Remap + conflict station
`RemapKey`, `RemapKeys`, `Get_HasKeyConflicts`, `SwapKeys`, `UnbindConflictAndRemap`.
→ **verify:** rebinding to an unused key succeeds and the row updates; rebinding to a *taken* key
reports the conflict and names the other mapping; swap exchanges both rows; unbind-and-remap
leaves the loser unbound.

### 1a-4 — Reset + persistence station
`ResetMappingToDefault`, `ResetAllToDefaults`, `SaveKeyBindings`.
→ **verify:** reset restores the asset-authored key; save then relaunch shows the remapped key
surviving. `[EDITOR-VERIFY: rebind, save, close PIE, re-enter PIE, confirm the key persisted]`

⚠ **Teardown is mandatory here.** `SaveKeyBindings` writes real user settings under `Saved/`, which
outlive the run and will bleed into later tests and the next baseline capture. Every station and
AutoTest that saves must finish with `ResetAllToDefaults` + `SaveKeyBindings`. Treat a leaked rebind
as a test failure, not an inconvenience.

### 1a-5 — Key-icon station
`UCk_Utils_KeyIcon_UE` glyph resolution, driven by `UCommonInputSubsystem` device changes.
→ **verify:** each bound key renders its glyph; unplugging the gamepad / pressing a key swaps the
whole row to keyboard glyphs. `[EDITOR-VERIFY: hot-swap controller mid-session, observe glyphs change]`

### 1a-6 — Change-signal station
`BindTo_OnMappingKeyChanged` / `UnbindFrom_OnMappingKeyChanged`, `Get_DidMappingKeyChange`.
→ **verify:** a remap performed in station 1a-3 fires the bound delegate exactly once and the
listener row updates without a manual refresh.

### 1a-7 — Headless AutoTests where possible
Anything not requiring a real device or a rendered glyph: remap round-trip, conflict detection,
swap symmetry, reset-to-default, save/load round-trip.
→ **verify:** new tests appear in the suite (`--discover-fresh` — the toolbox caches discovery) and
pass; total count rises by the number added, with the two known failures unchanged.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| 1a-0 prerequisite | `Get_AllRemappableKeys` count == mappable keys authored | Empty array | **Stop.** Scan path not picked up, or EI needs a runtime register call. Do not proceed — every later station will false-pass. |
| 1a-3 conflict case | Conflict reported, naming the colliding mapping | Silent overwrite | First check 1a-0 actually passed — an empty profile cannot conflict and produces this exact symptom. If the profile is populated, it is a **defect in shipped code**, not a gym bug: record it, tell the CkGameSettings owner (their page depends on it), fix only if trivial. |
| 1a-4 persistence | Remap survives a PIE restart | Reverts to default | Same: shipped-code defect. Capture the repro in the gym, escalate before Phase 1 builds on it. |
| 1a-5 hot-swap | Glyph row swaps device on controller change | Stale glyphs | Determine whether `UCommonInputSubsystem` fired and CkInput missed it, or it never fired. The answer decides whether this is ours or CommonUI's. |
| 1a-7 discovery | New AutoTests appear in the total | "No tests matched" | Known trap — a new test needs a relink and `--discover-fresh`; `--generate` alone does not fix it. Re-run before investigating. |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] Every station above works, with `[EDITOR-VERIFY]` steps written out and their results recorded
- [ ] Full suite re-run: delta-zero against `1005 / 1003 / 2` **plus** the new AutoTests, with no new
      failing names
- [ ] Any shipped-code defect found is recorded in PROGRESS.md and communicated to the CkGameSettings
      owner — this gym exists partly to protect them
- [ ] `ck-change-control` done-checklist run for this change class
- [ ] This file's Status header updated; PROGRESS.md dated entry appended — same commit
- [ ] Comment audit run over the diff (root doctrine closing step)
- [ ] **No leaked user settings:** confirm `Saved/` carries no rebind from the run (teardown ran)
- [ ] No `.uasset` was created — all input content is AngelScript script-literal assets
- [ ] If 1a-0's fallback (a) was taken, the `UCk_Utils_KeyBinding_UE` addition is called out
      separately in the commit message, not folded into the gym work

## Explicitly NOT in this phase

- No `CkIntent` module. No layer stack. No Slate preprocessor. No biasing stage.
- No changes to `UCk_Utils_KeyBinding_UE` / `UCk_Utils_KeyIcon_UE` beyond fixing a trivial defect the
  gym exposes — and any such fix is called out separately, not folded in silently.
