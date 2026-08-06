# CkGameSettings design — CTO Review

> **Workflow:** Read the brief below, then read the linked design doc. Fill in the **CTO Review Response** section at the bottom of this file and commit — the plan author / their assistant picks up your notes from there. You need no context from any prior conversation; everything required is in this file, the design doc, and the repo.

---

## Reviewer brief

### Your role

Senior reviewer / architect for CkFoundation. Your responsibilities:

- Catch architectural issues before implementation starts (module shape, layer boundaries, seams that will block future work).
- Catch convention/idiom mismatches with the existing codebase (naming, file layout, macro usage, module tiering, ensure discipline).
- Identify unclear, missing, or unsafe steps in the phase plan.
- Green-light, green-light with notes, or list concrete blockers.

You are expected to **read code in the repo**, not just review the plan in isolation — the "Critical context" list below is the minimum reading set.

### What's being built

`CkGameSettings` — a new CkFoundation runtime module providing user-facing game settings: a headless, data-driven settings registry (declare → persist → apply-at-boot → change notification), a Machine-vs-Player scope split with a pluggable storage provider (ini default), opt-in Video and Audio setting packs, and an optional settings-menu widget set following the Compass/Minimap "accelerant widget" doctrine (code-built default, zero shipped uassets). It replaces the role the marketplace **AutoSettings** plugin played in BusterBlock 5.5, redesigned around that plugin's audited faults and Lyra's `UGameSettingRegistry` architecture. BusterBlock is the first consumer and reference vocabulary, **not** the spec — the module is designed for future Ck projects.

### Reference material the design is informed by

- **AutoSettings 1.31 (vendored in BB 5.5)** — `D:\Repositories\CkRepos\BusterBlock_5.5\Plugins\AutoSettings\Source`. Full audit findings are summarized in §1.1 of the design doc (what BB used, what shipped dead, the fault list the design inverts).
- **Lyra GameSettings** (`UGameSettingRegistry`) — the architectural model for the headless registry, edit conditions, Local/Shared scope split, and type→row-widget mapping. Its C++-only authoring and reflection-string data sources are explicitly rejected.
- **Engine-native systems being reused, not rebuilt:** `UEnhancedInputUserSettings` (via CkInput), `UGameUserSettings` (via the Video pack), CommonUI (via CkUI).
- **Sibling modules to mirror:** `CkLoadingScreen` (subsystem-shaped module structure), `CkCompass`/`CkMinimap` (accelerant-widget doctrine).

### Plan location

[2026-08-05-CkGameSettings-design.md](../specs/2026-08-05-CkGameSettings-design.md)

### Critical context — read before reviewing

You **must** read these (paths relative to `Plugins/CkFoundation/`):

- `CLAUDE.md` (plugin root) — non-negotiables (esp. #3 ensure discipline, #4 tri-environment), code style, request-completion contract **including the subsystem carve-out**.
- `Source/CLAUDE.md` — module tier table, module-authoring rules, the `Request_*`-takes-a-struct rule.
- `Source/CkLoadingScreen/` (+ its `Claude.md`) — the structural template being mirrored: feature folders instead of the ECS quartet, settings+CVar layering, fail-open watchdog, BP/AS-facing dynamic delegates.
- `Source/CkCompass/Public/CkCompass/UI/CkCompassUI_RibbonWidget.h` + `Source/CkCompass/CLAUDE.md` — the `CodeBuilt`/`Custom` presentation pattern and zero-asset doctrine the widget layer copies.
- `Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` — the complete existing rebinding surface (the design builds only a page widget over this; verify nothing is being duplicated).
- `Source/CkCVar/Public/CkCVar/` (`CkCVar_Data.h`, `Utils/CkCVar_Utils.h`) — the CVar surface being reused for apply bindings; note the `INTERNAL_*`/`BlueprintInternalUseOnly` AS gap.
- `Source/CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h` — confirm for yourself that the existing "user settings" base is `Config = EditorPerProjectUserSettings` (editor-only), which is why a new runtime persistence surface is justified.
- `Source/CkUI/Public/CkUI/Layout/CkUI_Layout_Utils.h` + `CkUI/CkUI_GameplayTags.h` — the layer-stack plug-in point (`PushWidgetToLayer`, `TAG_UI_Layer_Menu`).

### Design decisions already locked in (do NOT relitigate unless you see a real problem)

1. Module name is `CkGameSettings` (user-approved).
2. Settings are headless data in a registry — never widget-resident state (direct inversion of AutoSettings' core flaw).
3. Key rebinding stays on CkInput / `UEnhancedInputUserSettings`; this module adds only the keybinding page UI.
4. Scope (`Machine` | `Player`) is first-class in the schema; persistence goes through a pluggable storage-provider seam whose default implementation is an order-preserving ini (`CkGameSettings.ini`).
5. Apply bindings are typed — a `FCk_CVarRef` or a registered typed handler; Lyra-style reflection string paths are rejected.
6. Both Video and Audio packs ship in v1, opt-in.
7. Widget layer follows the Compass/Minimap accelerant doctrine: `CodeBuilt`/`Custom` presentation enum, zero shipped uassets, CkUI/CommonUI substrate, rows generated from the registry.
8. The module is subsystem-shaped (CkLoadingScreen precedent): no ECS quartet, no typesafe handle; synchronous API under the request-contract's subsystem carve-out, but `Request_Set*` still takes request structs.
9. Replication/authority is out of scope; settings are local-machine.
10. Edit conditions (platform-trait tags + optional predicate + player-facing disabled reason) are in the schema from day one, even though v1 barely exercises them.

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- **Boot timing (the load-bearing question, design doc §6.1):** the registry is a `UGameInstanceSubsystem`, so boot replay happens at GameInstance `Initialize` — later than AutoSettings' `OnPostEngineInit`. Is that early enough for every realistic setting class, or does anything (rendering CVars? early scalability?) force an engine-subsystem split for the replay step?
- Is the storage-provider seam (`ICk_GameSettings_StorageProvider`, C++ interface) the right shape, or should it follow CkLoadingScreen's dual model (C++ interface + UObject holder) so BP/AS can implement a provider?
- Is one module the right cut, or should the widget layer be split out? (Precedent says one module with an annotated widget-only dep — Compass. But this module's core is bigger than Compass's.)
- Does the pending-changes session (subsystem-held apply/revert staging) have the right ownership, vs. per-screen staging?
- Is `ECk_CVarType` the right value-type enum to reuse for setting values, or does that couple the schema too tightly to CkCVar?

#### B. Convention compliance

- File layout vs CkLoadingScreen; naming (`UCk_GameSettings_Subsystem_UE`, `UCk_Utils_GameSettings_UE`, `FCk_GameSettings_SettingDefinition`, widget classes without `_UE`) vs house rules.
- Tier placement: T4 with deps `CkCore, CkCVar, CkInput, CkLog, CkSettings, CkUI` — legal per the tier table? Any dep pointing at a higher band?
- Ensure discipline in the deferred-apply queue and atomic collection registration (non-negotiable #3 shape: hoisted condition, empty ensure body, separate ordinary failure branch).
- AS-exposure constraints respected (no `BlueprintInternalUseOnly` on public API, no UFUNCTION overloads, `_UE` suffix on the BFL, delegates last)?

#### C. Version-specific API specifics (UnrealEngine-Angelscript 5.7 fork)

- The custom-ini save restriction (5.4-era `bCanSaveAllSections` / `"User"`-in-filename quirk) — still real on the 5.7 fork? The default provider's viability depends on it.
- `UEnhancedInputUserSettings` maturity on the fork — any known deltas from stock 5.7?
- `UGameUserSettings` interplay when a project subclasses it (does the Video pack need a "bring your own subclass" hook?).

#### D. Test coverage

- Phase gates lean on headless AutoTests for registry/persistence and `[EDITOR-VERIFY]` for video/UX — is the split sized right? Anything listed as `[EDITOR-VERIFY]` that could actually be automated (and vice versa)?
- Is the invalid-input rejection coverage (per the ensure-boundary test rule) called out everywhere a validation boundary exists?

#### E. Risks — sized correctly?

- Design doc §6 lists five risks. Are any missing (e.g. PIE-vs-packaged config paths, multiple GameInstances in PIE multi-client, editor-time widget preview touching the live registry)?

#### F. Forward-compat

- Does the schema carry enough for: split-screen per-player settings, console platform-trait gating, cloud-save routing, a future upscaler pack — without v1 building any of them?
- Anything in the design that would block migrating BB's 27 `bb.*` CVar settings incrementally (Phase 4's adoption example)?

### Output format — fill in the CTO Review Response section below

Be direct. If the plan is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers tied to a phase/section, not vague concerns.

---

## CTO Review Response

> Fill in the sections below. Replace the `_(your notes here)_` placeholders.

### Verdict

**GREEN-LIGHT WITH NON-BLOCKING NOTES**

The architecture is sound, the decomposition matches the codebase's own precedents (verified against CkLoadingScreen, Compass, Minimap, CkInput, CkCVar, CkUI — see spot-checks), the tier placement is legal, and both flagged engine risks are now resolved with engine-source evidence. Implementation can start. Notes 1–4 below correct or tighten the design doc itself and are cheapest folded in before Phase 0 begins, but nothing in them invalidates the phase plan — they are all additive.

### Blocking issues

None.

### Non-blocking suggestions

1. **§6.1 — correct the boot-timing premise; it is inverted (favorably).** Confirmed against engine source (5.7.4 fork, `D:/Repositories/UnrealEngine-Angelscript`): in a packaged game, `FEngineLoop::Init` calls `GEngine->Init(this)` (`LaunchEngineLoop.cpp:3999` and `:4763`) and only *afterwards* broadcasts `FCoreDelegates::OnPostEngineInit` (`:4007` / `:4769`). `UGameEngine::Init` creates the GameInstance and calls `InitializeStandalone()` (`GameEngine.cpp:1249-1251`), which runs `UGameInstance::Init()` → `SubsystemCollection.Initialize(this)` (`GameInstance.cpp:96`, `:128`). So the registry's boot replay runs **before** `OnPostEngineInit` — *earlier* than AutoSettings' hook, not later. Consequences to fold into §6.1: (a) no engine-subsystem split is needed — the flagged question is closed; (b) the deferred-apply queue is *structural*, not defensive — `PostEngineInit`/`PostDefault`-phase modules' CVars cannot exist yet at replay; (c) anything genuinely earlier (RHI init, device profiles, early `r.*`) is unreachable by *any* user-settings system and is owned by `GameUserSettings.ini`/`DefaultEngine.ini`, which the engine itself loads and applies inside `UGameEngine::Init` (`GameEngine.cpp:1226-1235`) before the subsystem exists. In PIE the GameInstance (and thus replay) is per-PIE-session — see note 6.

2. **Phase 0 schema (§3.1/§3.3) — add a per-definition persistence policy (`Provider` | `External`).** The engine applies `GetGameUserSettings()->LoadSettings() + ApplyNonResolutionSettings()` before the subsystem exists (`GameEngine.cpp:1226-1235`). If Video-pack values are *also* stored in `CkGameSettings.ini` and replayed at boot, there are two sources of truth that drift the first time a hardware benchmark or a manual `GameUserSettings.ini` edit runs. Video-pack definitions should be `External`: read-through and write-through their typed handler into `UGameUserSettings`, never stored in the provider. Audio and generic settings stay `Provider`. This is one enum field + a read-through path on handler-backed definitions; retrofitting it mid-Phase-2 would be awkward, adding it in Phase 0 is trivial.

3. **Phase 1 (§3.2) — state the two distinct "late" cases explicitly.** (a) *Registered setting whose bound CVar is missing* → deferred-apply queue, retry, loud `CK_ENSURE_IF_NOT` timeout — already designed. (b) *Stored value whose setting definition hasn't been registered yet* → must be retained indefinitely and applied at `Request_RegisterSetting` time, with **no** warning and **no** timeout — BP/AS registrations from game code after map load are the normal flow, not an error. AutoSettings' silently-dropped-saved-values fault (§1.1) was case (b); as written, an executor could plausibly implement the loud timeout across both cases and reintroduce a loud version of the same data loss. One paragraph closes the hole.

4. **Phase 0 (§3.1, brief A5) — own the value-type enum instead of reusing `ECk_CVarType`.** The design text says the enum is "Int32/Float/Bool/String", but the real enum has a fifth member: `Command` (`CkCVar_Data.h:24`) — a parameterless console command, nonsensical as a setting value type, which every registration path would have to explicitly reject. A module-owned `ECk_GameSettings_ValueType { Bool, Int32, Float, String }` plus one switch at the CVar apply seam removes the invalid state entirely and decouples the schema's spine from CkCVar (which remains a dep for `FCk_CVarRef` regardless). Cheap now, breaking later.

5. **Phase 1 (§3.2, brief A2) — make the storage-provider seam a `UCLASS(Abstract)` UObject base (BlueprintNativeEvents), not a raw C++ interface.** CkLoadingScreen's C++-only `ICk_LoadingProcess` is justified by per-tick polling *and* ships a script-facing alternative (`UCk_LoadingProcess_Task_UE`). A C++-only storage seam has neither: it's cold-path (boot load + batched flush) and there is no script alternative for "route Player scope into my save system" — which in AS-first consumers is exactly where a custom provider would live. One UObject-shaped seam serves all three environments (non-negotiable #4) with no dual model.

6. **§6 — add the PIE/multi-instance risk and decide its rule in Phase 1.** Each PIE client creates its own GameInstance → its own subsystem instance → its own boot replay onto **global process CVar state**, shared with the editor and every other client, persisting after PIE ends; all instances also share one `CkGameSettings.ini` for flushes. Any decided rule beats the implicit one (candidates: replay CVar-bound Machine-scope only for the first PIE instance; or skip CVar replay under PIE entirely and let `Get_SettingValue` remain correct from the store). Related: `ShouldCreateSubsystem` should return false on dedicated servers, and the registry/store must stay alive in headless runs (CkLoadingScreen's bookkeeping-vs-presentation split) so the Phase 0/1 AutoTests can run at all.

7. **Phase 1 (§3.2) — sidestep the ini quirk instead of working around it.** The quirk is real on the 5.7.4 fork: `ConfigContext.cpp:621-624` grants `bCanSaveAllSections` only to files whose base name contains `"User"` (or editor-settings files), and `FConfigFile::Write` otherwise consults a `[SectionsToSave]` allow-list (`ConfigCacheIni.cpp:2647-2669`). **But** it applies only to hierarchy/GConfig-loaded files — a standalone `FConfigFile` constructs with `bCanSaveAllSections = true` (`ConfigCacheIni.cpp:1292`). Since the provider must preserve *application order* anyway and `FConfigSection` is a `TMultiMap` with no ordering contract across load/save, the stronger move is to own the serialization outright (hand-written ordered ini read/write, or at minimum a standalone `FConfigFile` never registered with GConfig). That deletes risk §6.2 rather than mitigating it, and keeps the "raw GConfig only inside the default provider" rule trivially true — the provider touches GConfig zero times.

8. **Phase 0 (§3.2, brief F) — key `[Player.<Id>]` by `FPlatformUserId`, not LocalPlayer/controller index.** Survives split-screen join/leave reordering and maps cleanly onto cloud-save identities later; the section-name scheme carries either, so this is free now and a migration later.

9. **Phase 0/3 naming polish.** (a) `Begin_PendingChanges` is off the house verb vocabulary — `Request_BeginPendingChanges` / `Request_ApplyPendingChanges` / `Request_RevertPendingChanges`, with single-session semantics defined (second `Begin` while active → ensure + reject) and a decided disposition for an open session at `Deinitialize`/travel (auto-revert is the safe default). (b) The two accelerant-widget precedents disagree on shape: Compass is `UCk_CompassUI_RibbonWidget` in `CkCompassUI_RibbonWidget.h`; Minimap is `UCk_MinimapFrame_Widget` in `CkMinimapFrame_Widget.h`. The design currently takes Compass file names (`CkGameSettingsUI_Screen.h`) with Minimap-shaped class names (`UCk_GameSettingsScreen_Widget`) — pick one axis and hold it. (c) Keeping `UCk_InputActionWidget_UE` *with* the `_UE` suffix is correct despite feature widgets dropping it — the name is already promised verbatim at `CkKeyIcon_Utils.h:31`, and CkUI's own widget bases (`UCk_UserWidget_UE`, `UCk_ActivatableWidget_UE`) carry the suffix.

10. **Phase 2 gate — verify the AutoTest harness has a live audio device before promising the "audio mix application" AutoTest.** If the toolbox runs `-nosound`/null-device, assert at the seam instead (handler invoked with the right `USoundClass`/volume) and move audibility to `[EDITOR-VERIFY]`.

11. **Phase 2 (§3.3, brief C3) — one line in the design: the Video pack routes exclusively through `GEngine->GetGameUserSettings()`** (class resolved from the project's `GameUserSettingsClassName` ini) and never assumes the concrete class. That makes a "bring your own subclass" hook unnecessary — a project's subclass (BB's `bb.ScalabilityPreset` successor) composes via a game-registered setting with a typed handler.

### Convention compliance spot-checks performed

All paths relative to `BusterBlock/Plugins/CkFoundation/` unless noted. Root `CLAUDE.md` and `Source/CLAUDE.md` were loaded in full (subsystem carve-out present at root `CLAUDE.md:278`).

- `docs/specs/2026-08-05-CkGameSettings-design.md` — the design doc, in full.
- `Source/CkLoadingScreen/Claude.md`, `Public/CkLoadingScreen/Subsystem/CkLoadingScreen_Subsystem.h`, `CkLoadingScreen.Build.cs` (+ full directory enumeration) — structural template.
- `Source/CkCompass/CLAUDE.md`, `Public/CkCompass/UI/CkCompassUI_RibbonWidget.h`, `CkCompass.Build.cs` — accelerant doctrine + annotated widget-only dep precedent (`Build.cs:31-39`).
- `Source/CkMinimap/Public/CkMinimap/UI/CkMinimapFrame_Widget.h` — second widget-naming precedent (class line).
- `Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` (in full), `CkKeyIcon_Utils.h:1-60`, `Settings/CkInput_Settings.h` (scan-paths lines) — rebinding surface + the `UCk_InputActionWidget_UE` promise (`CkKeyIcon_Utils.h:31`) + scan-list precedent.
- `Source/CkCVar/Public/CkCVar/CkCVar_Data.h` (in full), `Utils/CkCVar_Utils.h:1-150` — `ECk_CVarType` (incl. `Command`), `FCk_CVarRef`, and the confirmed `INTERNAL_*`/`BlueprintInternalUseOnly` AS gap.
- `Source/CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h` — confirmed `Config = EditorPerProjectUserSettings` at `:9`.
- `Source/CkUI/Public/CkUI/Layout/CkUI_Layout_Utils.h` (in full), `CkUI/CkUI_GameplayTags.h` (`TAG_UI_Layer_Menu` at `:20`), class declarations of `UCk_UserWidget_UE` (`UserWidget/CkUserWidget.h:16`) and `UCk_ActivatableWidget_UE` (`UserWidget/CkActivatableWidget.h:17`).
- **Engine source** (`D:/Repositories/UnrealEngine-Angelscript`, resolved from the BB `.uproject` GUID; `Build.version` = 5.7.4): `Runtime/Launch/Private/LaunchEngineLoop.cpp` (`:3999-4007`, `:4763-4769`, `:4811`), `Runtime/Engine/Private/GameEngine.cpp` (`:1205-1252`), `Runtime/Engine/Private/GameInstance.cpp` (`:96`, `:128`, `:189`), `Runtime/Core/Private/Misc/ConfigContext.cpp` (`:621-624`), `Runtime/Core/Private/Misc/ConfigCacheIni.cpp` (`:1292`, `:2647-2669`), `Runtime/Core/Public/Misc/ConfigCacheIni.h` (`:577`).

### Design / architecture observations

Keyed to the brief's scrutiny sections. **Confirmed** = checked against source; **inferred** = reasoned, not executed.

- **A1 (boot timing) — resolved, no split needed.** See note 1: GameInstance-subsystem replay runs *before* `OnPostEngineInit` in packaged games (confirmed at `LaunchEngineLoop.cpp:3999→4007` / `GameEngine.cpp:1249-1251` / `GameInstance.cpp:128`). Everything earlier is engine-ini territory the engine itself replays before the subsystem exists. The GameInstance-subsystem choice is *correct*, and it additionally gives per-PIE-session replay for free.
- **A2 (provider seam)** — recommend the UObject-base shape (note 5); the CkLoadingScreen dual model exists to solve a hot-path problem this seam doesn't have.
- **A3 (one module vs split)** — one module is right; Compass's `Build.cs:31-39` is the exact precedent for annotated widget-only deps. Note that **CkInput is also widget-layer-only** here (the core registry never touches it) — annotate it alongside CkUI/UMG/CommonUI.
- **A4 (pending-changes ownership)** — subsystem-held is correct: survives widget destruction, headless-testable, and it's the inversion of the audited AutoSettings flaw. Needs single-session + teardown-disposition semantics defined (note 9a).
- **A5 (`ECk_CVarType`)** — reuse is the wrong call by one enum member; own the enum (note 4).
- **B (conventions)** — verified: BFL `UCk_Utils_GameSettings_UE` matches house; subsystem name matches `UCk_LoadingScreen_Subsystem_UE` (incl. the `DisplayName="CkSubsystem_..."` pattern worth copying); tier math is legal (CkSettings T0; CkCore/CkCVar/CkLog T1; CkInput T2; CkUI T4 → module at T4, no upward dep); file layout mirrors CkLoadingScreen's feature folders. The design's plan to keep the public API free of `BlueprintInternalUseOnly` is right — CkCVar's `Utils/CkCVar_Utils.h` is *entirely* `INTERNAL_*` + `BlueprintInternalUseOnly` (confirmed), so routing settings through it AS-side is impossible today; the module's own BFL is the AS surface, and the CkCVar AS gap remains a separate follow-up as stated.
- **C (5.7 fork specifics)** — ini quirk **confirmed real but avoidable** (note 7, with engine line numbers). `UEnhancedInputUserSettings` maturity is de-risked by shipped usage: CkInput's rebinding surface consumes it end-to-end today (`CkKeyBinding_Utils.h`); I did not diff the fork against stock 5.7 — no deltas encountered by inspection (inferred). `UGameUserSettings` subclassing needs no hook (note 11).
- **D (test split)** — sized right. The keybinding page genuinely duplicates nothing: `CkKeyBinding_Utils.h` already covers query (`Get_AllRemappableKeys`, `Get_KeyForMapping`), remap (`RemapKey/RemapKeys`), conflicts (`Get_HasKeyConflicts`, `SwapKeys`, `UnbindConflictAndRemap`), reset, change-listen, and persistence (`SaveKeyBindings`) — the page is pure UI over a complete surface (confirmed). Nothing listed as `[EDITOR-VERIFY]` is realistically automatable (resolution switch needs a real swapchain; countdown/gamepad-nav are judgment calls); the one item at risk of the *reverse* error is the audio AutoTest (note 10).
- **E (risks)** — the five listed are real and sized fairly; the missing one that matters is PIE multi-instance/global-CVar contamination (note 6). Editor-time widget preview is already safe as designed — fake rows through the shared generation path, same as Compass's `DoLayoutPreview`, never touching the live registry.
- **F (forward-compat)** — the schema carries enough: scope + edit-conditions + options cover split-screen (with note 8's keying), console trait-gating, and cloud routing via the provider seam; an upscaler pack is just another registrant. Nothing blocks incremental BB migration — CVar-bound definitions coexist with raw `TAutoConsoleVariable` declarations, and consumers keep reading CVars natively while rows migrate one at a time.

One meta-observation: §1.1's audit-driven inversions (headless registry, listener hygiene on `Deinitialize`, loud drops, batched flush, order-preserving writes) are the strongest part of this design — each maps one-to-one onto a named AutoSettings fault. Note 3 exists precisely to keep the best of those inversions from being implemented one case too broadly.

### Sign-off conditions (only if "CHANGES REQUESTED")

Not applicable — green-lit. Notes 1–4 are recommended as design-doc edits before Phase 0 starts (all additive, no re-review needed); the remainder land naturally in their named phases.

---

### Reviewer

- **Name:** Claude (CTO review session)
- **Date:** 2026-08-05
