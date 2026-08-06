# CkGameSettings — campaign PROMPT (executor: Opus)

**You are the executor.** This package was planned by a Fable session on 2026-08-05. Do not
re-derive the architecture — every design decision is already made and CTO-reviewed. Your job is
to implement phases 0-4 exactly as specified, verify at every gate, and record reality in
`PROGRESS.md` when it diverges. When a decision gate says STOP, you stop and end the session with
a blocker note — you do not improvise.

## Documents (read order)

1. This file, fully.
2. **The spec of record:** `../../specs/2026-08-05-CkGameSettings-design.md` (rev 2 — CTO notes
   already folded in; do NOT also mine the review file for requirements).
3. `PROGRESS.md` — check state before every session; update after every phase.
4. The `PHASE_N.md` for the phase you are executing. One phase per session; do not start the next
   phase if your context is past ~50%.
5. `VALIDATION.md` — only in the final phase.

Reference (rationale only, not requirements): `../../reviews/2026-08-05-CkGameSettings-CTO-review.md`.

## Problem statement

Build `CkGameSettings`: a new CkFoundation runtime module for user-facing game settings — a
headless data-driven settings registry (declare → persist → apply-at-boot → change notification),
Machine-vs-Player scope with a pluggable storage provider (ordered-ini default), opt-in Video and
Audio packs, and an optional widget set (Compass/Minimap accelerant doctrine). It replaces the
role the AutoSettings marketplace plugin played in BusterBlock 5.5.

## Executable spec

No module exists yet, so no red test could be committed in advance. The stand-ins, in priority order:
1. **Pre-designed header layouts** in `PHASE_0.md` §4 — these ARE the API spec; fill in bodies,
   do not redesign shapes.
2. **Named-test rubric per phase** — each phase lists the exact test names to write. Write them
   FIRST where the code they exercise exists as stubs (red), then implement to green. The phase's
   exit criterion is those names green under the toolbox, plus the suite delta-zero vs baseline.

## Chosen approach (locked)

Subsystem-shaped module (CkLoadingScreen structure — feature folders, no ECS quartet, no typesafe
handle). `UCk_GameSettings_Subsystem_UE : UGameInstanceSubsystem` owns registry + apply +
pending-changes; `UCk_Utils_GameSettings_UE` BPFL is the BP/AS surface; storage behind a
`UCLASS(Abstract)` UObject provider whose default implementation is a hand-serialized
order-preserving ini (`Saved/Config/<Platform>/CkGameSettings.ini`) that never touches GConfig.
Apply bindings are typed: `FCk_CVarRef` (via CkCVar) or a registered typed handler.

## Rejected approaches — do not resurrect

| Rejected | Kill reason |
|---|---|
| ECS quartet (fragments/processors/handle) | Settings are not entities; no lifetime, no world, must exist pre-world. CkLoadingScreen is the house precedent for this shape. |
| Engine-subsystem for boot replay | Unnecessary: GameInstance subsystem `Initialize` runs BEFORE `OnPostEngineInit` in packaged games (engine-verified: `GameEngine.cpp:1249-1251` → `LaunchEngineLoop.cpp:3999→4007`). |
| C++-only provider interface (`ICk_...`) | Blocks AS/BP providers (SPUD/EOS routing lives game-side). CkLoadingScreen's C++ interface is justified by per-tick polling; this seam is cold-path. |
| GConfig-registered ini for the default store | 5.7.4 restricts saving custom inis (`ConfigContext.cpp:621-624`), and `FConfigSection` is an unordered `TMultiMap` — application order is a hard requirement. Own the serialization. |
| Reusing `ECk_CVarType` as the setting value type | Its `Command` member (`CkCVar_Data.h:24`) is an invalid setting state every path would have to reject. Own enum + one switch at the CVar seam. |
| Widget-resident setting state (the AutoSettings model) | The audited core flaw: unenumerable, untestable, UI-required. Registry is headless; widgets are pure consumers. |
| Lyra reflection string paths for data sources | Stringly, silent-failure, AS-hostile. Typed bindings only. |
| Storing Video-pack values in the provider | `GameUserSettings.ini` is applied by the engine before the subsystem exists — a second copy drifts. Video pack is `External` policy: read/write-through `GEngine->GetGameUserSettings()`. |

## Skills to load, and when

| When | Skill |
|---|---|
| Every session, before coding | `ck-change-control` (what "done" requires for each change class) |
| Phase 0, before writing headers | `ck-macros-and-codegen` |
| Every phase with tests (all of them) | `ck-tests-authoring-and-running` (CkTests) |
| Phase 0 + Phase 4 (AS surface) | `ck-angelscript-interop` |
| Any build/UHT/AS failure | `ck-debugging-playbook` |
| Phase 3, before widget work | read `CkUI` headers listed in PHASE_3 (no dedicated skill; CkCompass ribbon is the exemplar) |

Build/test ONLY via the Unreal Toolbox (`<BB-root>/CkAuto/UnrealToolbox.exe`) per the `/build-test`
skill. Never `Build.bat`/UBT/`-ExecCmds` directly.

## File inventory

**Create (Phases 0-3):** everything under `Source/CkGameSettings/` per design doc §4's tree, plus
`Source/CkUI/Public/CkUI/CustomWidgets/InputAction/CkInputAction_Widget.{h,cpp}` (Phase 3), plus
the `CkFoundation.uplugin` module entry, plus tests (locations per phase docs), plus
`Source/CkGameSettings/Claude.md` and two rows in `Source/CLAUDE.md` (Phase 4).

**Read before touching (why):**
- `Source/CkLoadingScreen/**` — the structural template; mirror its file/subsystem/settings shapes.
- `Source/CkCompass/Public/CkCompass/UI/CkCompassUI_RibbonWidget.h` — presentation-enum widget
  pattern (misconfig reporting, preview, pooling) to mirror in Phase 3.
- `Source/CkCVar/Public/CkCVar/CkCVar_Data.h` + `Utils/CkCVar_Utils.h` — the apply seam you call
  (`INTERNAL_Bind_*`/`INTERNAL_Get_*`/`INTERNAL_Set_*` are callable from C++; they are BP-internal
  only).
- `Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` + `Subsystem/CkKeyBinding_Subsystem.h` —
  Phase 3 keybinding page consumes these; add NOTHING to the input layer.
- `Source/CkInput/Public/CkInput/Settings/CkInput_Settings.h` — scan-paths precedent for the
  collection scan list.
- `Source/CkUI/Public/CkUI/Layout/CkUI_Layout_Utils.h`, `CkUI/CkUI_GameplayTags.h`,
  `UserWidget/CkUserWidget.h`, `UserWidget/CkActivatableWidget.h`, `Styles/CkCommonButton.h` —
  Phase 3 substrate.
- `Source/CkSettings/Public/CkSettings/ProjectSettings/CkProjectSettings.h` — project-settings base.

## Glossary

- **Definition** — `FCk_GameSettings_SettingDefinition`: the data describing one setting.
- **Scope** — `Machine` (this PC) vs `Player` (per `FPlatformUserId`; v1 exercises user 0 only).
- **PersistencePolicy** — `Provider` (value stored via the storage provider) vs `External` (value
  lives in an external store, e.g. `UGameUserSettings`; read/write-through the handler, never stored).
- **Apply binding** — where a value goes when set/replayed: a CVar (`FCk_CVarRef`) or a registered
  typed handler, or nothing (pure stored value).
- **Deferred-apply queue** — CVar-bound stored values whose CVar isn't registered yet; retried,
  loud `CK_ENSURE_IF_NOT` on timeout. Distinct from…
- **Orphan stored value** — a stored value whose DEFINITION isn't registered yet; retained
  silently forever, applied at registration. NEVER warned. (Conflating these two reintroduces the
  audited AutoSettings data-loss bug.)
- **Pending-changes session** — subsystem-held staging: `Request_BeginPendingChanges` → stage →
  `Request_ApplyPendingChanges`/`Request_RevertPendingChanges`; single session; auto-revert on
  `Deinitialize`/travel.
- **Pack** — opt-in pre-registered setting collection (Video, Audio) toggled in project settings.
- **CodeBuilt / Custom** — widget presentation enum: code builds the whole tree (zero assets) vs
  the WBP owns the tree with `BindWidgetOptional` slots.

## Global fences (apply to every phase)

1. **Zero `GConfig` anywhere in the module.** The default provider hand-serializes its own file.
2. **No `BlueprintInternalUseOnly` on any public API** — it kills the AS wrapper (proven by CkCVar).
3. No UFUNCTION overloads (suffix instead: `_Bool/_Float/_Int32/_String`). BFL class name ends `_UE`.
4. Every new UENUM: first entry value 0, `CK_DEFINE_CUSTOM_FORMATTER_ENUM`. (A `value=N`-only enum
   breaks — known incident.)
5. `{}` construction everywhere EXCEPT UFUNCTION parameter defaults (`()` there); no `= {}` in
   UFUNCTION signatures.
6. No anonymous namespaces / file-local statics — named namespace `ck_game_settings_<file>`.
7. Ensure discipline (non-negotiable #3): hoist the condition, `CK_ENSURE_IF_NOT` with empty body,
   separate ordinary `if` for the failure path. Load-bearing recovery never lives inside the macro.
8. Toolbox gotchas: new tests are invisible without `--discover-fresh`; editor must be CLOSED for
   `--build` (exit 77); never edit `.as`/source during a test run (exit 78); `--test --no-live` is
   the gate of record.
9. Git: work on `feature/game-settings` branched off `dev` in CkFoundation (create it in Phase 0).
   Commit per phase, descriptive messages, **never push**. Stage only files you created/edited —
   never `git add <dir>`. If CkTests changes are needed (Phase 1+ integration tests, Phase 3 gym),
   branch `feature/game-settings-tests` there; note in PROGRESS which repo has what.
10. PIE rule (provisional, design §3.2): CVar-bound replay is SKIPPED under PIE. Confirm the
    detection mechanism against engine source in Phase 1 and record it in PROGRESS; if it proves
    wrong, that is a blocker, not a license to redesign.
11. Baseline discipline: capture the full-suite baseline (counts + failing names) BEFORE Phase 0
    touches anything; every gate reports the delta. 9 pre-existing BB suite failures are known;
    `Ck.*.Net` tests only run via explicit `--test-pattern`.
12. Comment audit before every commit: delete every phase/campaign/PROMPT breadcrumb and every
    *what*-comment from the diff.

## Phase map

| Phase | One line | Doc |
|---|---|---|
| 0 | Module scaffold + schema + registry + typed access + change delegates + collection PDA | `PHASE_0.md` |
| 1 | Storage provider + ordered-ini store + boot replay + deferred queue + pending-changes + PIE rule | `PHASE_1.md` |
| 2 | Audio pack (SoundMix driver) + Video pack (`UGameUserSettings` bridge, External policy) | `PHASE_2.md` |
| 3 | Widget set: screen (CodeBuilt/Custom), rows, type→row mapping, keybinding page, `UCk_InputActionWidget_UE` | `PHASE_3.md` |
| 4 | Docs, AS wrapper verification, BB adoption slice, full-gate close-out | `PHASE_4.md` + `VALIDATION.md` |
