# PROMPT — LiveTune implementation campaign

**Campaign:** LiveTune — live-tunable feature params mid-PIE (editor-only change transport; params storage untouched).
**Your role:** executor. You implement phase by phase against a GREEN-LIT design. You do NOT redesign. Each phase ends at a gate; Adam (with a Fable-tier audit) reviews before the next phase starts.
**Written:** 2026-08-05 by the design session (Fable 5).

---

## 0. Read before any code (in this order)

1. `Plugins/CkFoundation/docs/specs/2026-08-05-LiveTune-design.md` — the design of record. GREEN-LIT with all CTO amendments folded in. §4 is the architecture, §5 the tiers, §8 the LOCKED fork calls, §9 the phase plan you execute, §10 the risks each phase must respect.
2. `Plugins/CkFoundation/docs/reviews/2026-08-05-LiveTune-CTO-review.md` — read the **CTO Review Response** section fully: the fork-call rationale, Blocking 1/2 analysis, and the convention spot-check list are implementation-relevant detail the spec compresses.
3. `Plugins/CkFoundation/CLAUDE.md` + `Plugins/CkFoundation/Source/CLAUDE.md` — non-negotiables (esp. #3 ensure discipline, #4 tri-environment), code style, the persistence-handler contract section.
4. `Source/CkEcs/Public/CkEcs/Persistence/CkPersistenceHandlerRegistry.h` — the registry you are mirroring: named `Register_*` variants, designated-init args structs, **`FRequired*` deleted-default-ctor slot enforcement** (carry this over so `ViaRequest.Apply` / `ViaRebuild.ReAdd` are compile-enforced).
5. Precedent files for the module home: `CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h` (WITH_EDITOR fragment in CkEcs), `CkEcs/Subsystem/CkEcsEditor_Subsystem.h` (editor subsystem in-module), `CkEcs.Build.cs:55-70` (existing bBuildEditor dep block).
6. `Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp:36-39, 581-627` — where `OnAssetsReinitialized` gets added (Phase 2; plugin-local, NOT an engine change).

## 1. Locked decisions — do not relitigate

- Explicit `UCk_Utils_LiveTune_UE::Link(handle, asset, memberName)`; no per-feature `Add` overloads.
- Explicit per-feature registration only; **no implicit ViaRebuild**. Unhandled stamped edits log Display: `"LiveTune: no handler registered for [Type] — feature not live-tunable"`.
- Module home: `Source/CkEcs/LiveTune/`. No new module. All of it `WITH_EDITOR`; `Link` is an empty inline outside the editor. No `CkLiveTuneEditor` split unless editor UI appears later.
- Dispatch hygiene is mandatory (design §4.3): per-`(asset, member)` value-diff cache; `Interactive` events → `ViaReplace` only, `ValueSet` → all tiers; authority gate skips client-mode entities for replicated features.
- `ViaRebuild` hydrate rides `FProcessor_Hydration_Dispatch`'s existing machinery — never a parallel dispatcher. `Scope::Entity` refuses non-RuntimeSpawned entities loudly.
- Re-Add sequencing keys on **actual record disconnect**, not a fixed tick delay (design §10.1).
- Observer invalidation across ViaRebuild is accept + document + Display log of dropped bindings (design §10 #6).

## 2. Phase scope — THIS session does Phase 0 only

Execute design §9 Phase 0 (spine): stamp fragment + `Link` (FProperty validation per non-negotiable #3 shape) + editor subsystem (reverse map, `OnObjectPropertyChanged`, value-diff cache, change-type + authority gating) + `FCk_LiveTuneHandlerRegistry` with the three `Register_*` shapes.

**STOP at the Phase 0 gate.** Do not begin Phase 1 (pilots) in the same run unless Adam explicitly says so after the gate is green and audited.

Phase 0 gate (from design §9): AutoTests for — Link validation rejects wrong-type/missing property loudly with zero partial state · registry dispatch by type · stamp cleanup on entity destroy · diff gate suppresses no-op and full-heal dispatch · undo/redo event shape verified on the fork.

## 3. Testing & verification rules

- **Tests for CkFoundation features live in the CkTests plugin**, not BB. Follow `ck-tests-authoring-and-running` skill + `Plugins/CkTests/Script/Common/CkAutoTest_CreationSpecification.txt`.
- Build + run tests via the **Unreal Toolbox only** (`/build-test` skill; `CkAuto/UnrealToolbox.exe`). Never raw Build.bat / UnrealEditor-Cmd for automation. `--build` requires the editor closed. Gate of record: `--test --no-live` on the final binary.
- **Record the baseline first**: run the relevant suites before your first edit and write the pass/fail counts + failing names into PROGRESS.md. "No regressions" claims diff against that record.
- New tests can be silently skipped by the toolbox's cached test list — use `--discover-fresh` after adding tests.
- Never edit `.as`/source while a test run is in flight (contamination, exit 78).
- Grep/Glob can silently miss files under this plugin — zero matches ⇒ re-check with `rg --no-ignore --files` before concluding absence.
- Testability seam (CTO suggestion 4): expose `#if WITH_EDITOR` `Test_SimulatePropertyChange(Asset, MemberName)` on the subsystem so AutoTests drive the full dispatch path headlessly (hand-built `FPropertyChangedEvent` broadcast is the fallback).

## 4. Campaign discipline

- Create `docs/campaigns/2026-08-05-LiveTune/PROGRESS.md` at start (living doc: baseline numbers, decisions made, file list, gate status). Update it before ending any session.
- Commit locally in logical units on the current dev branch of each repo (CkFoundation code; CkTests tests). **Never push.** Stage only files you authored — this machine often has sibling sessions with dirty files; enumerate anything dirty you didn't touch as "left for owning session".
- Close with the comment audit (root CLAUDE.md): delete every what-comment and campaign breadcrumb from your diff.
- Anything only verifiable in an interactive editor (real details-panel edit mid-PIE) is labeled `[EDITOR-VERIFY]` with exact click steps for Adam — do not claim it verified.
- Stuck protocol: two failed attempts on the same failure → stop, write up what's ruled out, present two options in PROGRESS.md. Don't thrash.

## 5. Success criteria for this session

1. `Source/CkEcs/LiveTune/` exists with the Phase 0 surface, compiling with AND without `WITH_ANGELSCRIPT_CK`, and with `Link` compiling to an empty inline in non-editor targets (verify by reading the preprocessed shape or a non-editor target compile, not by assumption).
2. Phase 0 gate AutoTests written in CkTests and green via toolbox `--test --no-live`, with the recorded baseline showing delta-zero on pre-existing suites.
3. PROGRESS.md current; local commits made; nothing pushed.
4. A short close-out report: what was confirmed vs inferred, the one claim most likely to be wrong, and the `[EDITOR-VERIFY]` list.
