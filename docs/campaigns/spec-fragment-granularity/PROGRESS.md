# PROGRESS — spec-fragment-granularity

Living doc. Newest entries on top within each section. See PROMPT.md for phases/rules.

## State

- **Phase:** P1 (Timer pilot) — in progress
- **Baseline (2026-08-05):** superproject `3cf103f`; CkFoundation `7ebe720f2` (dev); CkTests
  `0ea0d6a` (dev); CkGameplayDebugger `77aff95` (DETACHED HEAD — resolve before committing there).
  Superproject shows pre-existing modified submodule pointers for CkFoundation+CkTests (not ours).
  CkFoundation untracked `Tools/`, `docs/digests/` — NOT ours, never stage.
  Toolbox config: **Development** (last-built; DebugGame refused as config-flip).
  Baseline build+`--test-pattern Timer` run: PENDING (fill in below).
- **Baseline test counts (2026-08-05):** `--test-pattern ck.timer` → **25/25 passed, 0 failed**,
  55s, no AS errors (Saved/Logs/BaselineTest2.log). Baseline build: green (BaselineBuildTest.log).
  NOTE: first baseline attempt hit `AS_COMPILE_FAILED` from a stale orphan
  `Script/Generated/utils_web_umg.as` (module no longer exists); the failing boot itself cleaned
  it. Re-run was clean. Bare pattern `Timer` also drags in engine `BuildPatchServices.*ProcessTimer`
  tests that never run and abort lanes — always use `ck.timer`.

## P1 — Timer pilot plan (frozen before edits)

Renames: `FCk_Fragment_Timer_ParamsData`→`FCk_Timer_Spec`,
`FCk_Fragment_MultipleTimer_ParamsData`→`FCk_MultipleTimer_Spec` (UFUNCTION param names and
UPROPERTY member names — e.g. `_TimerParams` — unchanged by rule).

Fragment restructure (CkTimer_Fragment.h): alias dies; `FFragment_Timer_Current`→`FFragment_Timer`
(chrono, primary state); NEW residue `FFragment_Timer_Params{ ECk_Timer_Behavior _Behavior }`.

Behavior changes (all deliberate, each needs a test eye):
1. Request handlers (Reset/Complete/Jump/Consume) branch on `FTag_Timer_Countdown` instead of
   stale Spec `_CountDirection` — FIXES live divergence bug (doc'd in design spec §1.2).
2. `Has()`/`Cast()` anchor: `{Current, Params}` → `FFragment_Timer` alone.
3. `MakeStatIdFromParams(Params)` → `MakeStatId(Handle)` reading GameplayLabel (STATS-only path).
4. `AddOrReplace` now re-unpacks the spec fully: re-adds `FTag_Timer_NeedsSetup` (countdown chrono
   gets its Setup Complete()), sets/clears `FTag_Timer_Countdown` and `FTag_Timer_NeedsUpdate` per
   spec. Old code left stale direction/run-state tags and skipped Setup — latent countdown bug.
5. Setup processor reads the countdown TAG (no Spec/Params dependency at all).

Consumer sweep list (C++): CkVfxCue_EntityScript.cpp, CkCue_EntityScript.cpp,
CkAudioCue_EntityScript.cpp, CkTween_Utils.cpp, CkSmTask_Delay.cpp, CkSmCondition_Timer.cpp,
CkTests/Net/CkAutoTest_NetSubject_EntityScript.cpp, CkTests/UnitTests/CkArchetypeTyped.spec.cpp
(direct fragment emplaces!), CkGameplayDebugger CkEcsDebugger_FeatureFlags.cpp:79
(RegisterFlag<FFragment_Timer_Params> → switch to FFragment_Timer), comment at
CkEcs/Handle/CkDebugCallstack_Macros.h:59.

AS sweep (textual): 1 site CkFoundation `Script/CkUtils_Timer.as`; ~90 files CkTests Script;
2 sites superproject Script. Pattern is plain `FCk_Fragment_Timer_ParamsData(` construction —
pure rename is safe. `Script/Generated/utils_timer.as` regenerates.

CoreRedirects (before any editor/test run):
`/Script/CkTimer.Ck_Fragment_Timer_ParamsData`→`Ck_Timer_Spec`,
`.Ck_Fragment_MultipleTimer_ParamsData`→`Ck_MultipleTimer_Spec`.
Asset carrying the FName: `Plugins/CkFoundation/Content/CkTimer/Utils_CkTimer_FL.uasset`
(BP function library) — `[EDITOR-VERIFY]` recompile+resave after campaign.

## Log

- 2026-08-05: P1 CODE COMPLETE (gate pending): CkTimer module converted (Fragment_Data/Fragment/
  Processor/Utils), 152 files swept for the two Spec renames (CkFoundation Source+Script, CkTests,
  superproject Script), fragment-name consumers updated (ArchetypeTyped.spec.cpp, GameplayDebugger
  FeatureFlags→`FFragment_Timer`, CkDebugCallstack_Macros.h comment), 2 StructRedirects appended
  to DefaultCkFoundation.ini:363-364, CkTimer/Claude.md updated + new "Fragment shape" section.
  Gate launched: build + `ck.timer` (baseline 25/25).
- 2026-08-05: P2 DONE: root CLAUDE.md (lingo row, two-tier table rewritten, canonical example →
  FCk_Timer_Spec, new "Spec unpacking" paragraph), Source/CLAUDE.md (composition ritual step 3,
  VfxCue example note), CkEcs/Claude.md (processor templates → FFragment_MyFeature, feature-flag
  marker wording), Script/CLAUDE.md (2 examples), skills (macros add-a-new-x §3.1 rewritten,
  8 other files token/phrase-updated, ckecs-architecture-contract tier table rewritten),
  DECISIONS.md §111 appended.
- 2026-08-05: Campaign opened. Design spec finalized (F1=Spec, F7=Params kept, F2–F6 = standing
  recs accepted via blanket mandate). Baseline build+test launched (background).

## Decisions / discards

- Historical campaign docs (voxelnav-port research, saveload PHASE_4B) that mention ParamsData are
  ARCHIVES — never sweep them.
- `FProcessor_Timer_Replicate` appears in FFragment_Timer_Current's friend list but no such
  processor exists — carried over as-is to FFragment_Timer (out of scope to prune friends).
