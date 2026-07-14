# PHASE 0 — Baseline, census, dt==0 fixes, fidelity oracle Tier-1

## Entry criteria
- CkFoundation on `feature/save-load-improvements`, tip `bc484d645` or later, clean tree (`git status --short` →
  only untracked noise, nothing staged). If dirty with files you don't recognize: STOP → Blockers.
- PROMPT.md read in full; skills `ck-change-control`, `ck-tests-authoring-and-running`, `ck-macros-and-codegen` loaded.

## Steps

### 0.1 Record the baseline (BEFORE any edit)
Run, from BB root:
```powershell
CkAuto\UnrealToolbox.exe --build --test --target Editor --config Development --test-pattern "Ck.Snapshot" --output CkAuto\logs\p0-baseline-snapshot.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Attribute.Net" --output CkAuto\logs\p0-baseline-attrnet.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p0-baseline-net.log
```
Record in PROGRESS.md: total/pass/fail per pattern AND the names of every failing test.
**Gate:** expected = all `Ck.Snapshot.*` pass (the branch's own M2 work is green as of `bc484d645`).
If any `Ck.Snapshot.*` fails → STOP (the baseline itself is broken; do not proceed on a red base) → Blockers.
If broader `Net` has failures: record names; they are baseline, not yours.

### 0.2 Census re-derivation (CTO note N3)
```bash
rg --no-ignore -n "^CK_REGISTER_SNAPSHOTABLE\(" Plugins/CkFoundation/Source -g '!*CkSnapshot_FragmentRegistry.h' | wc -l
rg --no-ignore -n "^\s*CK_REGISTER_SNAPSHOTABLE\(" Plugins/CkFoundation/Source -g '!*CkSnapshot_FragmentRegistry.h' --stats
rg --no-ignore -n "^\s*struct\s+\w*FCk_RepData_" Plugins/CkFoundation/Source | wc -l
```
Record invocation-only counts in PROGRESS.md and replace the headline figures in spec §2 (currently "119 / 18
modules" and "24 RepData") with the re-derived numbers, one dated edit.

### 0.3 The three dt==0 fixes (live hazard for today's pre-save pump)
Contract comment to place at each guard (adapt wording, keep the meaning): *zero-dt tick = settle pass; do no
time-dependent work and consume no one-shot markers.*

a) `Source/CkPhysics/Public/CkPhysics/PredictedVelocity/CkPredictedVelocity_Processor.cpp` (`ForEachEntity`, the
   divide at `:40` and the `_PreviousDeltaTime` store at `:42`): early-out when `InDeltaT.Get_Seconds() <= 0.0f`
   BEFORE any read/write — this both kills the divide-by-zero and stops `_PreviousDeltaTime` being poisoned to 0.
   ```cpp
   if (InDeltaT.Get_Seconds() <= 0.0f)
   { return; }
   ```
b) `Source/CkPhysics/Public/CkPhysics/Homing/CkHoming_Processor.cpp` (`ForEachEntity` of the Update processor —
   finite difference `/ InDeltaT` at `:236`) and `CkHoming_ProNav.cpp` (`:37-39`, `:62-65`): same early-out in the
   processor's `ForEachEntity` (one guard at the top covers the ProNav calls it makes).
c) `Source/CkSubstep/Public/CkSubstep/CkSubstep_Processor.cpp` (`:26-39`): early-out before the `FirstUpdate`
   consume + `OnSubstepFirstUpdate`/`OnSubstepUpdate`/`OnSubstepFrameEnd` broadcasts when dt==0.

**Gate:** rebuild + rerun the 0.1 patterns. Expected: delta-zero vs baseline. Any new failure → the guard changed
behavior a test depends on → STOP, revert that one guard, record → Blockers.

### 0.4 `CK_WITH_FIDELITY_ORACLE` define
In `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h`, near the top:
```cpp
// Test-only fidelity-oracle machinery (spec §5). Deep-diff fragment captures compile only under this flag;
// shipping builds carry zero oracle cost. Phase 5 moves the per-fragment CK_REGISTER_SNAPSHOTABLE registrations
// under it (gate-not-delete).
#ifndef CK_WITH_FIDELITY_ORACLE
#define CK_WITH_FIDELITY_ORACLE (!UE_BUILD_SHIPPING)
#endif
```

### 0.5 Oracle Tier-1 (structural diff) — new files
`Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_FidelityOracle.h` + `.cpp`, everything inside
`#if CK_WITH_FIDELITY_ORACLE`. Pre-designed API (fill bodies, do not redesign):

```cpp
namespace ck::snapshot_oracle
{
    // One entity's structural signature. Identity across a rebuild is BY SIGNATURE, not entity id.
    struct CKECS_API FCk_Oracle_EntitySignature
    {
        FString _LabelPath;          // owner-chain of GameplayLabels, "/"-joined; empty if unlabeled
        FString _ScriptClassPath;    // EntityScript class path if any
        TArray<FString> _FragmentTypeNames; // sorted, from registry storage iteration (entt type name)
        TArray<FString> _TagTypeNames;      // sorted
        auto ToString() const -> FString;   // stable, for diff lines
    };

    struct CKECS_API FCk_Oracle_StructuralImage
    {
        TArray<FCk_Oracle_EntitySignature> _Entities; // sorted by ToString()
    };

    CKECS_API auto Capture_Structural(ck::SnapshotRegistryType& InRegistry) -> FCk_Oracle_StructuralImage;

    // Multiset diff of signatures. Each line: "+ <sig>" (after-only) or "- <sig>" (before-only).
    // InAllowlist: exact ToString() prefixes expected to differ (Phase 3B uses this for N1 annotations);
    // matched lines are reported under a separate "annotated" list, not as failures.
    CKECA_API_FIX auto Diff_Structural(
        const FCk_Oracle_StructuralImage& InBefore,
        const FCk_Oracle_StructuralImage& InAfter,
        const TArray<FString>& InAllowlist,
        TArray<FString>& OutAnnotated) -> TArray<FString>;
}
```
(`CKECA_API_FIX` = typo guard for you: use `CKECS_API`.) Implementation notes:
- Iterate `InRegistry.storage()` for fragment/tag presence (the Model-A capture at `CkSnapshot_Capture.cpp:87-112`
  shows the storage-iteration idiom); classify tag vs fragment by `std::is_empty`-equivalent via storage traits —
  if that distinction is awkward, put everything in `_FragmentTypeNames` and leave `_TagTypeNames` empty (Tier-1
  only needs stable signatures, not taxonomy). Note which you did in PROGRESS.md.
- Label path via `UCk_Utils_GameplayLabel_UE` + lifetime-owner walk (`FFragment_LifetimeOwner`).
- Exclude the transient entity and entities carrying any `FTag_DestroyEntity_*`.

### 0.6 Harness autotest (NEW, in CkTests)
`Plugins/CkTests/Source/CkTests/Private/CkSnapshot/Test_Snapshot_Oracle_StructuralBaseline.cpp`, registering
`"Ck.Snapshot.Oracle.StructuralBaseline"`. Shape: copy `Test_Snapshot_Core_RoundTrip.cpp`'s world/registry setup.
Body: build a small world (reuse Core_RoundTrip's fixture entities) → `Capture_Structural` → Model-A
`Run_Capture`/`Run_Restore_Registry` round-trip → `Capture_Structural` → `Diff_Structural` with empty allowlist →
assert diff is EMPTY. **Known physics of Model A:** only REGISTERED-snapshotable fragments round-trip — so for THIS
baseline test, filter both images' `_FragmentTypeNames` down to types present in
`FCk_Snapshot_FragmentRegistry::Get_All()` (add an optional filter parameter to `Capture_Structural`; default =
unfiltered). An unfiltered diff against Model A WOULD legitimately show unregistered types — that is not a bug.
If the FILTERED diff is non-empty: oracle bug or genuine Model-A regression — print lines into the failure message,
STOP → Blockers with the lines.

### 0.7 Gate + commit
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p0-final.log
```
Expected: baseline count + 1 new pass (`Ck.Snapshot.Oracle.StructuralBaseline`), zero new failures. Then re-run the
two Net patterns from 0.1 → delta-zero. Anything else → STOP → Blockers.

Commits (separate, in this order):
1. CkFoundation: `fix(CkPhysics,CkSubstep): dt==0 settle-pass guards for PredictedVelocity/Homing/Substep`
2. CkFoundation: `feat(CkEcs): fidelity-oracle Tier-1 structural capture/diff (CK_WITH_FIDELITY_ORACLE)`
3. CkFoundation: `docs: reconcile snapshot census counts in spec §2` (force-add the spec)
4. CkTests: `test(CkSnapshot): oracle structural-baseline harness`

## Exit criteria (measurable)
- `rg --no-ignore -n "CK_WITH_FIDELITY_ORACLE" Plugins/CkFoundation/Source | wc -l` ≥ 3.
- Test log shows `Ck.Snapshot.Oracle.StructuralBaseline` PASSED; snapshot pattern delta-zero vs 0.1 names.
- PROGRESS.md: baseline table filled (counts + failing names per pattern), census numbers recorded, phase marked done.

## Fences
- Do NOT fix any pre-existing red test you find in 0.1 — record and move on.
- Do NOT start the oracle's Tier-2/Tier-3 here (Phase 1 / Phase 5).
- The oracle is CkEcs-side (it must see the registry types); do NOT put it in CkSnapshot (tier direction).
