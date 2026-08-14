# CkGameSettings campaign — PROGRESS (living document)

**Rule for executors:** update this file at every phase boundary, at every deviation from a PHASE
doc, and before ending any session. A fresh session trusts THIS file over its memory.

## Status board

| Phase | Status | Session date | Gate result (tests green / suite delta) | Commit(s) |
|---|---|---|---|---|
| 0 — Scaffold + registry | DONE | 2026-08-05/06 | build Succeeded; `Ck.CkGameSettings.Registry.*` 7/7 green (single-lane re-run after one environmental flake — see Deviations 11); full suite: baseline 8 failing → 5 failing, all 5 ⊂ baseline set, ZERO new (3 pre-existing flaky BB reds flipped green: SidewalkPathNetworkDetector.MainMapLandscapeTrace, Brainwash_LineToKiosk, ThrowItem_AimedThrow_LandsAtTarget). NOTE: the toolbox no-pattern suite EXCLUDES all name-based `Ck.<Module>.*` C++ families (verified: CkCompass.Math/CkDynamic.ScriptQuery/CkMinimap all 0 in the suite run) — the pattern run is the gate of record for the 7 specs. | `23adcb3e4` |
| 1 — Persistence + apply | DONE | 2026-08-06 | `Ck.CkGameSettings.{Registry×7,Store×3}` + `Ck_AutoTest_GameSettings_×5` all green (15/15, exit 0). Full suite 1456 total (+5 = the new AS tests; the `Ck.*` C++ families stay excluded by toolbox convention): 8 failing, 7 ⊂ baseline set + `Ck_AutoTest_CkJolt_ChaosParity_CcdProjectileStopsAtThinWall` adjudicated as a lane-hitch flake (its lane logged a 10.0s tick stall at test start; solo re-run on the SAME binary = green). One baseline red (ThrowItem) flipped green this run. Restart smoke ✓: every lane's boot logged `GameSettings boot: loaded [7] Machine and [0] Player.0 stored value(s)` after the prior run's flush; `Saved/Config/WindowsEditor/CkGameSettings.ini` inspected — ordered, correct end-state. `rg "GConfig\|SaveConfig" Source/CkGameSettings` = 1 hit, a comment. Iterations recorded: first AS boot exit 76 (generated wrappers DROP the WorldContext param — AS auto-fills; call forms corrected), settings ctor needed the FObjectInitializer form. | CkF `908565222`, CkTests `dcf60c1` |
| 2 — Packs | DONE | 2026-08-06 | All 19 GameSettings tests green (11 C++ specs incl. `Ck.CkGameSettings.Packs.VideoValueMapping` + 8 AS AutoTests incl. the 3 new: AudioPack_HandlerReceivesVolume, VideoPack_ExternalNeverStored, ResolutionConfirmWindow_RevertsOnExpiry). One iteration: AS handler had to declare `float32` (AS `float` is 64-bit — delegate bind rejected). Full suite 1459 total (+3), 8 failing: all in the known flaky BB family; the two names new vs baseline (`SidewalkPathNetworkDetector.LandscapeSegmentOwnership`, `Bb_AutoTest_NpcAI_HitreactAfterRevive`) BOTH solo-green on the same binary → flakes. Exit sweeps: every `GetGameUserSettings` hit is `GEngine->`; `SetSoundMixClassOverride` only in the audio pack. `[EDITOR-VERIFY]` items (audibility, real resolution switch, benchmark, countdown UX) belong to VALIDATION.md per the phase doc. | CkF + CkTests commits below |
| 3 — Widgets | DONE | 2026-08-06 | Pattern gate (`--test-pattern GameSettings --discover-fresh --parallel 1`, run on the FINAL binary): **21/21 green, exit 0** — 12 C++ specs (7 Registry + 3 Store + 1 Packs + new `Ck.CkGameSettings.UI.RowClassResolution`) + 9 AS AutoTests (8 prior + new `ScreenGeneratesRowsFromRegistry`). One red on the first pattern attempt was the Phase 2 audio test's re-run nondeterminism (Deviations 13, fixed in CkTests) — NOT a Phase 3 regression. Full suite (`--test --no-live`, 3 lanes, 14m57s): **1460 total (+1), 5 failing, ALL ⊂ baseline-8 set, ZERO new names** ({LoyaltyParity, Checkout_MansCounter, RevivedNpcCalms, DeathExplodes, LootCount} + a lane-logged PhysicalLayoutModeSwitch — all baseline). AS compiled clean in every lane (the 13 `Angelscript: Error` lines are one runtime error trail inside the pre-existing flaky BB combat family, not compile errors, not our files). Sweeps: `rg CkGameSettings Source/CkInput` = 0 hits; no `NativeTick` in any row/screen/page widget; CkFoundation tree = source/docs only (zero Content/); CkTests carries exactly ONE mechanical populator-wrapper uasset (Deviations 13). Iterations: two failed builds first (protected `OnButtonBaseClicked`, missing InputCore/CkEcs link deps, non-exported native-tag extern, AS `NOT`) — all in Deviations 13; earlier "builds green ×2" checkpoint note was WRONG (a `; echo EXIT=$?` swallowed the toolbox exit code — logs showed `Build FAILED`; corrected before any gate claim). | CkF `8237b000a`, CkTests `f6dc67c` |
| 4 — Close-out | NOT STARTED | | | |

## Baseline (fill in Phase 0, before any change)

- Date/commit baseline was captured at: 2026-08-05 (run completed 2026-08-06 02:43 local), CkFoundation
  `dev` @ `db426136508892b0237bfb5e463b0a0dfd6f73b6` (= branch point of `feature/game-settings`),
  BusterBlock tree clean, no editor open. Toolbox auto-sized to 3 lanes; duration 22m 51s.
- Full suite (`--test --no-live`): total = 1451, passed = 1443, failed = 8
- Failing names (verbatim):
  - `BusterBlock.Editor.SidewalkDesigner.PhysicalLayoutModeSwitch`
  - `BusterBlock.UnitTests.AI.Navigation.SidewalkPathNetworkDetector.MainMapLandscapeTrace`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_Brainwash_LineToKiosk`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_Customer_LoyaltyParity`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_Employee_Task_Checkout_MansCounter`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_NpcCombat_RevivedNpcCalms`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_RentGoon_DeathExplodes`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_Shelf_LootCount`
  - `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_ThrowItem_AimedThrow_LandsAtTarget`
- Known context: memory/toolbox notes said 9 pre-existing BB suite failures; this run measured 8
  (all BB-side, none `Ck.*`). The editor exited 0xFF AFTER completing every test ("results kept" —
  toolbox's own note); `Ck.*.Net` only runs via explicit `--test-pattern`.

## Branches

| Repo | Branch | Created | Tip SHA (update each session) |
|---|---|---|---|
| CkFoundation | `feature/game-settings` | 2026-08-05, off `dev` @ `db4261365` | `8237b000a` (Phase 3; Phase 2 = `2d40e4a53`, Phase 1 = `908565222`, Phase 0 = `23adcb3e4`) |
| CkTests | `feature/game-settings-tests` | 2026-08-06, off `dev` @ `403d887` | `f6dc67c` (Phase 3; Phase 2 = `103a49b`, Phase 1 = `dcf60c1`) |
| BusterBlock | `feature/game-settings-adoption` (Phase 4 only) | | |

## Decisions verified at implementation time (executor fills)

- PIE detection mechanism used (Phase 1, PROMPT fence 10): `GetGameInstance()->GetWorldContext()`
  (GameInstance.h:294, always set before subsystem Initialize) `->WorldType == EWorldType::PIE`
  (`FWorldContext` defined in Engine/Engine.h:333, member at :339 — NOT EngineBaseTypes.h as the
  phase doc guessed). Refinement of the provisional rule, recorded per fence 10: under PIE only
  the terminal CVar WRITE is skipped; deferred-queue admission, retries, and the loud timeout all
  still run (otherwise the deferred machinery would be untestable from PIE AutoTests, and the
  design's own intent — "the store remains the source of truth; the editor's CVar state is never
  contaminated" — only requires suppressing writes). Handler-bound apply runs everywhere.
- `FPlatformUserId` BP-exposability finding (Phase 1.1): NOT BP-exposable — it is a plain struct
  (CoreMiscDefines.h:469), not a USTRUCT, on 5.7.4. The reflected boundary stays `int32
  InPlatformUserId` carrying `GetInternalId()`, as the phase doc's default.
- Toolbox audio device finding (Phase 2.4): toolbox test boots have a REAL XAudio2 mixer — lane
  logs show `FMixerPlatformXAudio2::StopAudioStream()` at teardown, and `-nosound` is not passed
  (only audio CAPTURE is absent). SetSoundMixClassOverride would therefore function headless, but
  the AS test still asserts at the handler seam (spy handler receives the volume, including the
  immediate apply at handler registration) — the SoundMix audibility itself is asset-dependent and
  stays `[EDITOR-VERIFY]`.
- Registry-vs-spec test split, if the Phase 0.7 fallback was needed: NOT needed. The subsystem is
  exercised directly in the spec via `NewObject<UGameInstance>(GEngine)` +
  `NewObject<UCk_GameSettings_Subsystem_UE>(GameInstance)` — no `GameInstance->Init()` (which would
  boot every GameInstance subsystem), no `Initialize()` call (Phase 0 `Initialize` is empty). All
  seven rubric tests live in the one spec file.
- Expected-ensure mechanics verified for the spec tests: in unattended/automation runs
  `Ensure_Impl` logs `CkEnsure` Error on EVERY fire and never registers an ignore
  (`CkEnsure.cpp:172-210` — WITH_EDITOR path returns at the unattended check without
  `Request_IgnoreEnsureAtFileAndLine`), so `AddExpectedError(<substring>, Contains, 0)` is
  deterministic even when several tests share an ensure site.

## Deviations from PHASE docs

1. **Phase 0.7 — extra file `Private/CkGameSettings_SpecSupport.h`.** The doc's file list has only
   the spec cpp; reality: `SetValueFiresChangeOnce` must bind a dynamic delegate, which requires a
   reflected UFUNCTION receiver, and UHT does not parse UCLASSes out of `.cpp` files. Added a
   minimal non-Blueprint UCLASS listener (`UCk_GameSettings_SpecListener_UE`) in `Private/`.
2. **Phase 0.5 — one extra registration validation case.** The doc lists five rejection cases;
   implemented those five PLUS "Min/Max set on a numeric type but not parseable as that type".
   Why: the set-time range check parses the bounds; an unparseable bound would otherwise be
   silently ignored at set time (non-negotiable #3 — no silent failure). Covered in
   `AtomicBatchRejectsAll`.
3. **Phase 0.5 — `Get_SettingKeysByCategory` empty-query semantics.** Doc is silent on the empty
   `FGameplayTagQuery`; implemented empty = matches everything (house precedent: CkCompass
   `_CategoryFilter`, documented "empty accepts everything"). Contract-commented on the UFUNCTION.
4. **Phase 0.7 — options-rejection coverage rides in `RangeViolationRejected`.** The rubric has no
   dedicated options test; options rejection is the same clamp-or-reject validation family, so its
   invalid-input coverage (reject + zero mutation) was added to that test rather than a new name.
5. **Session event, not a doc deviation:** between baseline and gate, a sibling session (PID 18276)
   launched 4 headless BB test editors at 22:44:37 and died, leaving a STALE toolbox lock (also one
   for CkPlugins, PID 9608). While its editors were alive the uplugin edit was temporarily REVERTED
   (an uplugin entry with no compiled DLL breaks any fresh BB editor boot), then re-applied once
   the machine was quiet. Toolbox auto-recovers stale locks on the next build.
6. **Phase 1.3/1.5 — four API additions beyond the phase doc's list** (all recorded because the
   subsystem initializes ONCE per PIE session, so AS AutoTests cannot re-boot it):
   `Request_FlushStorage()` (deterministic flush for tests + menu-close/ops surface),
   `Request_ReloadFromStorage()` (re-runs the boot merge — the only way to exercise the orphan
   path in-world; also genuinely useful for cloud-save arrival), `Get_StorageProvider()` (tests
   seed/inspect the store through the real provider), and UFUNCTION exposure of the ini provider's
   `Get_StorageFilePath`/`Set_FilePathOverride` (fresh-provider file round-trip assertions from AS).
7. **Phase 1.3 — deferred-apply timeout is CVar-backed:** `ck.GameSettings.DeferredApplyTimeoutSecs`
   (`FAutoConsoleVariableRef` static in the subsystem cpp; the settings property carries
   `ConsoleVariable` meta — the `UDeveloperSettingsBackedByCVars` base syncs settings → CVar, the
   LoadingScreen pattern). Reason: the AS timeout test must shorten it at runtime; also a live ops
   knob. The subsystem reads the CVar static, not the CDO.
8. **Phase 1.2 — newline rejection implemented at BOTH boundaries:** the subsystem's String-set
   boundary (keeps memory and store consistent) and the ini provider's store boundary (guards the
   format against any caller, including BP/AS-implemented providers' consumers).
9. **Phase 1.5 — AS tests use `>=` count asserts and per-key asserts** (the registry and the real
   `Saved/Config/<Platform>/CkGameSettings.ini` are shared across the PIE world's tests and across
   runs; keys are prefixed `astest.*` and written so re-runs are deterministic). The real ini
   accumulating `astest.*` keys is accepted — it is a gitignored per-machine Saved/ file.
10. **Phase 2 decisions (executor):**
   - The doc's "seven `video.sg.*` scalability Int32s" left the seven unnamed; 5.7 has ten groups.
     Chose the classic seven: view_distance, anti_aliasing, shadow, post_process, texture, effects,
     foliage (GlobalIllumination/Reflection/Shading omitted — game-side additions if wanted).
   - **`Request_ResetAllToDefaults` now SKIPS External-policy settings, and single
     `Request_ResetToDefault` on an External key rejects with ensure.** External schema defaults
     are placeholders; resetting them would stomp the user's real GameUserSettings (and, from the
     shared-PIE test suite, the dev's). This also protects the dev machine from the AS suite.
   - Confirm-window: nothing is saved to GameUserSettings.ini until Confirm (ConfirmVideoMode +
     Save) or expiry-revert (restore prior + Save); the countdown rides the existing 1 Hz
     FTSTicker rather than a second ticker.
   - Followed the phase doc's LITERAL signatures for
     `Request_SetResolutionWithConfirmWindow(const FString&, float)` — loose params + float
     seconds, overriding the house request-struct and FCk_Time defaults, since PHASE docs are the
     spec of record.
   - Headless detection uses `IsRunningCommandlet() || NOT FApp::CanEverRender()` (the
     CkLoadingScreen precedent) instead of `GUsingNullRHI`, avoiding a new RHI module dependency.
   - Packs register through PUBLIC subsystem API only (fence held); the one subsystem-side
     cooperation is GC-rooting the pack handler UObjects (`_PackHandlerObjects` UPROPERTY array)
     since dynamic delegates hold only weak refs. Runtime opt-in surface added:
     `Request_RegisterAudioPack`/`Request_RegisterVideoPack` (idempotent; Initialize calls them
     when the config toggles are on; AS tests and games can call them directly).
11. **Gate flake (environmental, root-caused):** the first `--build --test` pattern run came back
   6/7 — `DuplicateKeyRejected` failed on an UNEXPECTED Error log that is not ours:
   `LogFileInfo: Error: Failed to open database for '<BB>/Saved/Search'` (SQLite
   `Saved/Search/FileInfo.db`, disk I/O error). Cause: the engine's AssetSearch plugin (pulled in
   transitively by BlueprintAssist's optional dep) opens that DB with `locking_mode=EXCLUSIVE`
   from an unconditional retry loop (`AssetSearchManager.cpp:1055-1064` — the
   `bEnableSearch`/unattended gate stops only asset SCANNING, not the DB thread), so with 3
   concurrent toolbox lanes sharing one `Saved/`, the losing lanes emit the Error every ~30s and
   it randomly lands inside a test's window. Same error appears 45x in the baseline run's output.
   Adjudication: single-lane re-run (`--parallel 1`) → 7/7 green. Follow-up chip filed
   (task_161439af); investigation results recorded in this session's close-out report.

12. **Phase 3 decisions (executor):**
   - **Slider drag semantics:** "live-preview writes on drag" implemented as thumb + numeric
     readout updating during drag; the single persistence-affecting `Request_SetSettingValue_*`
     fires on capture release (mouse AND controller capture-end; a value change with no capture
     active — gamepad step / programmatic — commits immediately). The alternative (an apply-only
     `Request_PreviewSettingValue_*` core API so e.g. volume is audible mid-drag) was NOT built —
     it needs new core API the phase doc doesn't authorize; named here for the CTO.
   - **Type shapes the doc left unmapped:** numeric (Int32/Float) without a FULL [min,max] range →
     Select row acting as a ±1 stepper (clamped to any single authored bound); String without
     options → Select row display-only (games provide a row-class override for free-text entry).
     Sliders require BOTH bounds.
   - **Edit-condition disposition:** required platform traits missing + no `_DisabledReason` →
     Hidden (omitted from the screen); traits missing + reason authored → Disabled with the reason
     as tooltip. Traits sourced from CommonUI's `UCommonUISettings::GetPlatformTraits()`.
   - **Options-present resolution ignores the per-type override** — `_SelectRowClassOverride` is
     the dedicated hook for that case (spec-tested).
   - **Screen API beyond the doc's list:** `Request_SetActiveCategory(FName)` (deep-link a tab;
     also makes the AS test deterministic in the shared-registry PIE world),
     `Get_HasRowForKey`/`Get_RowClassForKey` (generation introspection the AutoTest asserts
     against), `Get_GeneratedRowCount`/`Get_CategoryTabCount`.
   - **Custom-presentation Apply/Cancel slots are `UCommonButtonBase`** (what designers author in a
     CommonUI screen); the CodeBuilt strip uses plain UButtons (zero style-asset deps). In
     `_AutoApplyMode` the Apply button is hidden and Cancel is labeled "Close".
   - **Custom mode without `_CategoryTabBar` renders all categories as one flat list** (graceful
     degradation, not a misconfig; `_RowContainer` absence IS the misconfig).
   - **KeyBinding page is CodeBuilt-only** — the phase doc defines no designer slots for it.
   - **`UCk_InputActionWidget_UE` unbinds in `ReleaseSlateResources`** — `UCommonActionWidget` is
     a UWidget (no NativeDestruct). Device-change refresh is inherited from the base (verified:
     `CommonActionWidget.cpp` OnWidgetRebuilt → ListenToInputMethodChanged); this subclass adds
     only the remap listener.
   - **Gym opens the screen via AddToViewport + ActivateWidget, not PushWidgetToLayer:** the
     CkTests gym world has no `ACk_HUD_UE`/PrimaryGameLayout config (the plugin ships no layout
     assets — zero-uasset fence). The screen remains a layer participant for games that own a
     layout. Gym demo categories use `utils_gameplay_tag::ResolveGameplayTag` (editor-only tag
     creation; gyms are editor surfaces).
   - **Implementation note:** pooled rows that are collapsed on a tab switch keep their previous
     change-subscription until reinjected — a hidden row refreshing is benign, and reinjection
     rebinds. Recorded so nobody reads it as a leak.
13. **Phase 3 gate findings:**
   - **Phase 2 test defect fixed in passing (CkTests):** `AudioPack_HandlerReceivesVolume` was
     re-run NON-deterministic — its first-ever run persisted `astest.audiopack.master = 0.42` to
     the real machine ini, so every later run boot-absorbed 0.42 and the "immediate apply carries
     1.0" assert failed (Phase 2's gate was the test's first run, hence green then). Fix follows
     the deviation-9 discipline: the test now normalizes the value to 1.0 BEFORE registering the
     handler. Not a Phase 3 code regression — the failing assert predates every Phase 3 change.
   - **One `.uasset` committed in CkTests** (`Content/__ExternalActors__/.../AV42I4BQH9EJEKO9VSUXV1.uasset`):
     the AutoTest populator's auto-generated wrapper actor for the new `ScreenGeneratesRowsFromRegistry`
     test — the same mechanical output Phases 1-2 committed (8 of them). The Phase 3 "zero uassets"
     fence is read as targeting the WIDGET layer (no WBPs/layouts/materials shipped — that holds);
     an AutoTest cannot exist without its populator wrapper.
   - **Two failed builds first (exit-code hygiene):** `UCommonButtonBase::OnButtonBaseClicked` is
     protected → rebound Custom-mode Apply/Cancel via the public `OnClicked()` native event
     (RemoveAll(this)+AddUObject); missing link deps surfaced by the widget substrate → Build.cs
     gains `InputCore` + `CkEcs` inside the widget-layer block; the UI spec resolves its stand-in
     trait tag by name (`UI.Layer.Menu`) because `UE_DECLARE_GAMEPLAY_TAG_EXTERN` externs are not
     API-exported cross-module. Also: AS has no `NOT` macro — gym uses `!`.

14. **Post-Phase-3 maintainer directive (2026-08-06): NO hidden code-built widget trees.**
   Adam: *"the native widget base is there to do the plumbing and expose the necessary events so
   that you only have to style it yourself."* Applied to ALL FIVE accelerant widget families, not
   just GameSettings:
   - **CkGameSettings rows/screen/keybinding page** — every `WidgetTree->ConstructWidget` path
     deleted. `ECk_GameSettingsScreen_Presentation` deleted (the CodeBuilt/Custom fork has no
     meaning once there is only one mode). Screen gained `OnRowGenerated`, keybinding page gained
     `OnRowCreated` + `_RowWidgetClass`, so styling and category headers are WBP work.
   - **CkCompass ribbon** — `ECk_CompassRibbon_Presentation`, `_RibbonMaterial`, the code-built
     canvas, the eight constructed cardinal TextBlocks, `_CardinalFont`/`_CardinalColor`/
     `_ShowCardinals`, and the debug-UImage marker fallback (`_IconSize`/`_IconTint`) all deleted.
     Cardinals are now eight `BindWidgetOptional` slots (`_CardinalN`.._CardinalNW`) anchored by
     the same arc math; markers require `_MarkerWidgetClass` + `_MarkerCanvas`.
   - **CkMinimap frame** — `DoBuildWidgetTree` deleted; `_BlipCanvas`/`_MapImage`/`_FrameImage`/
     `_ObserverMarker` are bound slots, blips are pooled `_BlipWidgetClass` instances, the "▲"
     glyph observer marker is gone.
   - **Consequence, recorded deliberately:** the CkTests **Minimap gym** and **Compass gym**
     instantiate the native classes directly with no WBP, so they now render NOTHING. Their
     AutoTests (11 Compass + 9 Minimap) are data-layer tests and stay green — but as VISUAL
     surfaces those two gyms are dead until someone authors WBPs. Not a regression; the direct
     cost of the directive.
   - Design-time preview: KEPT on the compass (it only positions already-bound widgets);
     REMOVED from the settings screen (it created fake rows via `CreateWidget` at design time —
     unverified in the widget designer and now valueless without a built-in look). `InjectPreview`
     stays on the ROW widgets so a designer can preview their own row WBP.
15. **Quality pass on the Phase 3 diff (same session):**
   - Extracted `ck::game_settings_ui::BindClick(Button, Listener, Handler)` — 11 call sites had
     each repeated the 4-line `IsValid` + `OnClicked().RemoveAll` + `AddUObject` idiom.
   - Select row's `_PrevButton`/`_NextButton` switched `UButton` → `UCommonButtonBase`, matching
     the screen and keybinding page (the whole layer is CommonUI; gamepad nav wants it).
   - Screen button bindings moved from per-rebuild (`DoResolveBindings`, called on every
     `Request_RebuildRows`) to once in `NativeOnInitialized`. `DoResolveBindings` deleted; the
     missing-`_RowContainer` ensure moved to `NativeOnActivated` where it is actually meaningful.
   - Comment audit: the surviving comments are contract docs on public API + slot-binding
     instructions for designers + the load-bearing *why*s (cardinal index↔yaw order, the
     rotate-about-center translation math, async-load-before-first-row). No phase/campaign
     breadcrumbs anywhere in the diff.

## Blockers

_(STOP entries land here: phase/step, expected observation, actual observation, verbatim
error/log excerpt, what you ruled out. Do NOT improvise past a STOP.)_

## Follow-ups (out of campaign scope, recorded not fixed)

- CkCVar AS gap (`INTERNAL_*`/`BlueprintInternalUseOnly` → no AS wrapper) — known, pre-existing,
  separate follow-up.
- Multi-lane test flake (any test can randomly red): engine AssetSearch plugin's DB thread opens
  `Saved/Search/FileInfo.db` (EXCLUSIVE SQLite lock) unconditionally, even unattended; losing
  lanes log an Error that fails whichever test it lands in. Root-caused 2026-08-06; ready-to-apply
  engine-fork patch handed off as chip task_a99f1bea (this session was denied write access to the
  engine tree). Until it lands: adjudicate any suspicious single red with a `--parallel 1` re-run
  and grep the failing window for `FileInfo`.
- Full BB `bb.*` settings migration beyond the Phase-4 slice.
- Upscaler pack (DLSS/FSR/XeSS), cloud storage provider, split-screen exercise of Player scope.

## Handoff (Phase 4 fills)

- Ready for audit: NO
- `[EDITOR-VERIFY]` items outstanding: see VALIDATION.md §Human
- Branch/SHA summary for Adam:
