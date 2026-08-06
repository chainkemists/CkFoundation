# CkGameSettings — acceptance protocol (run in Phase 4; human items after)

## Automated (executor runs; paste actual outputs)

All from BB root, editor closed.

1. **Build:** `CkAuto\UnrealToolbox.exe --build` → Succeeded.
2. **Module tests:** `CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.CkGameSettings" --discover-fresh`
   → every name below green:
   - Registry: `RegisterAndQuery`, `DuplicateKeyRejected`, `AtomicBatchRejectsAll`,
     `TypedAccessAndMismatchRejected`, `SetValueFiresChangeOnce`, `ResetAllRestoresDefaults`,
     `RangeViolationRejected`
   - Store: `RoundTripPreservesOrder`, `UnknownLinesSurviveRewrite`, `NewlineValueRejected`
   - Packs: `VideoValueMapping`
   - UI: `RowClassResolution`
3. **CkTests AutoTests** (pattern per their registered names): `PersistRoundTrip`,
   `OrphanValueAppliedOnRegistration`, `DeferredCVarTimesOutLoudly`,
   `PendingChanges_RevertRestoresLiveValues`, `ResetAll_PersistsDefaults`,
   `AudioPack_HandlerReceivesVolume`, `VideoPack_ExternalNeverStored`,
   `ResolutionConfirmWindow_RevertsOnExpiry`, `ScreenGeneratesRowsFromRegistry`,
   `AS_RegisterSetRead`, + the gym boot AutoTest.
4. **Gate of record:** `CkAuto\UnrealToolbox.exe --test --no-live` → delta-zero vs the Phase-0
   baseline, reported with failing names on both sides.
5. **Static checks:**
   - `rg -n "GConfig" Source/CkGameSettings` → 0 value-path hits (justify any hit in writing).
   - `rg -n "BlueprintInternalUseOnly" Source/CkGameSettings` → 0.
   - `rg --no-ignore -n "Request_RegisterSetting" Script/Generated/utils_game_settings.as` → ≥1
     (run from plugin root; the Grep tool is blind under `Script/` — use `rg --no-ignore`).
   - `git diff dev...feature/game-settings --stat` → no `Content/`, no `.uasset`.
   - Fresh test-boot log: zero ensures and zero `Angelscript: Error` naming campaign files
     (startup FProInstance/FSplineCurves/Debugger noise is known pre-existing).
6. **Three environments:** C++ = the spec tests; AS = `AS_RegisterSetRead` + generated wrapper
   check; BP = UHT pass + `[EDITOR-VERIFY]` item 3 below.

## Human — `[EDITOR-VERIFY]` (Adam or a driven editor session)

1. **Settings screen, CodeBuilt:** open the CkTests GameSettings gym (PIE) → Tab menu → screen
   shows category tabs + one row per demo setting; toggle/slider/select all mutate values
   (verify via console `Ck` log lines or re-open persistence).
2. **Pending changes:** in the gym screen with `_AutoApplyMode` off — change a slider, Cancel →
   value reverts live; change again, Apply, leave, reopen → value kept.
3. **Blueprint environment:** in the editor, place `[Ck][GameSettings] Get Setting Value (Float)`
   and `Request Set Setting Value (Float)` nodes in a scratch BP — nodes exist with sane pins
   under category `Ck|Utils|GameSettings`. (Delete the scratch BP after.)
4. **Keybinding page:** rebind a key (capture prompt), trigger a conflict (bind an already-used
   key) → conflict dialog offers swap; reset-all restores; restart editor → rebind persisted
   (Enhanced Input user settings).
5. **Video pack (packaged or PIE):** change window mode + resolution → confirm-countdown appears
   (Phase 3 dialog over the Phase 2 primitive); let it expire once → reverts; VSync/quality
   changes stick across restart via `GameUserSettings.ini` (NOT `CkGameSettings.ini` — check both files).
6. **Audio pack:** with a game-configured SoundMix/SoundClasses, drag master volume → audible
   change; restart → volume restored.
7. **PIE contamination check:** in PIE, change a CVar-bound demo setting; end PIE; verify the
   editor's CVar value did NOT change (the PIE rule) while the ini did persist the value.

## Definition of done (route through `ck-change-control`)

Class: new runtime module + CkUI/CkTests touches + BB adoption slice → full gate + Fable
`/audit-package` on the campaign diff + Adam review. Done =
§Automated all green with recorded outputs, §Human items either verified or explicitly listed as
outstanding in the handoff, PROGRESS §Handoff filled, all branches committed and **unpushed**.
Push/merge is Adam's call after audit.
