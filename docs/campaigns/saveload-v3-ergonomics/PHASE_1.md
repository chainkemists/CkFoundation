# PHASE 1 — Round-trip test harness + StaticCast sweep + Timer Jump absolute (build 1)

Three concerns, **one build + one gate at the end**. Commit order: harness (CkTests) → cast sweep (CkF) →
Jump absolute (CkF). Do not build between them.

## Entry criteria (verify, record in PROGRESS.md)

1. `git -C Plugins/CkFoundation status --short` and `git -C Plugins/CkTests status --short` → **empty**
   (both trees clean). If dirty with files you did not author → STOP, Blocker (the parity executor may be
   mid-work; its Phase 5 must be committed first — check `docs/campaigns/saveload-v3-parity/PROGRESS.md`
   shows Phase 5A/5B with commit hashes).
2. Record both HEAD hashes.
3. Baseline gate (fresh, against the committed binary — build first if the last build predates HEAD):
   `--test --test-pattern "Ck.Snapshot"` and `--test --test-pattern "Ck.Net"`. Record counts + any failing
   names in PROGRESS.md. **The recorded numbers ARE the campaign baseline** — planner expectation is
   ≥30 Snapshot / ≥90 Net all-green (the parity campaign's Phase 5 ADDS tests before this campaign starts,
   so counts above 30/90 are normal). Any FAILING test → proceed only if the parity PROGRESS baseline
   lists the same failure by name; else STOP.

## Step 1 — Harness files (CkTests)

Create `Source/CkTests/Public/CkTests/Snapshot/CkSnapshot_TestHarness_Common.h`:

```cpp
#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "CkTests/Net/CkNetAutomation_Common.h"
#include "CkEcs/Handle/CkHandle.h"

class UCk_Snapshot_Subsystem_UE;
class UCk_EntityScript_UE;

namespace ck::auto_test::snapshot
{
    // One save->load->assert round-trip. All ServerAction/Assertion lambdas run on the (post-travel) server
    // world via the CkNetAutomation latent commands. Assert re-runs after EVERY cycle (two-cycle rule,
    // CkSnapshot/Claude.md §5 — cycle 2 catches double-apply stacking).
    struct CKTESTS_API FCk_SnapshotRoundTrip_Spec
    {
        int32   NumPIEClients = 2;                        // 2 = listen-server window + 1 client
        FString MapPath = TEXT("/Engine/Maps/Entry");
        FName   SlotName;                                 // REQUIRED, unique per test
        float   ReloadTimeoutSeconds = 60.0f;
        int32   SettleFrames = 60;
        int32   NumCycles = 2;

        FCk_NetAutoTest_ServerAction Spawn;               // REQUIRED: spawn/compose the subject
        FCk_NetAutoTest_Assertion    SubjectReady;        // optional poll gate before Mutate (unset = skip)
        FCk_NetAutoTest_ServerAction Mutate;              // optional: drive to a non-default state
        FCk_NetAutoTest_Assertion    ReloadSettled;       // optional extra predicate ANDed onto the default
                                                          // (server world changed + HasBegunPlay + NOT IsLoadInProgress)
        FCk_NetAutoTest_Assertion    Assert;              // REQUIRED: final assertions
    };

    // Enqueues the full latent chain (StartPIE ... EndPIE). Call from RunTest, then return true.
    CKTESTS_API auto EnqueueRoundTrip(FAutomationTestBase* InTest, FCk_SnapshotRoundTrip_Spec InSpec) -> void;

    // Shared helpers (extracted from the ~21 per-file anon-namespace copies):
    CKTESTS_API auto Get_SnapshotSubsystem(UWorld* InWorld) -> UCk_Snapshot_Subsystem_UE*;
    CKTESTS_API auto Get_PostTravelServerWorld() -> UWorld*;
    CKTESTS_API auto ResolveEntityBySpawnRecipe(UWorld* InWorld, UClass* InScriptClass) -> FCk_Handle;
}

#endif
```

Create `Source/CkTests/Private/Snapshot/CkSnapshot_TestHarness_Common.cpp`. Fill bodies by EXTRACTING the
proven code — do not invent:
- `Get_PostTravelServerWorld` ← `Test_Snapshot_M2b_LevelReload_Gate.spec.cpp:34-52` (netmode-agnostic
  post-travel enumeration).
- `Get_SnapshotSubsystem` ← the `<Feature>_Subsystem(UWorld*)` copies
  (`InWorld->GetGameInstance()->GetSubsystem<UCk_Snapshot_Subsystem_UE>()` + null guards).
- `ResolveEntityBySpawnRecipe` ← `Test_Snapshot_TransformParity_MPReload_Gate.spec.cpp:97-116` (raw
  `registry->view<ck::FFragment_SpawnRecipe>()` matched by `Get_ScriptClass()`).
- `EnqueueRoundTrip` composes, in order: `FCk_Latent_StartPIEMultiClient(NumPIEClients, MapPath)` →
  `FCk_Latent_WaitForPIEReady(NumPIEClients, 30.0f)` → `FCk_Latent_RunOnServer(Spawn)` →
  [if SubjectReady set] `FCk_Latent_WaitForCondition(SubjectReady, 30.0f)` →
  [if Mutate set] `FCk_Latent_RunOnServer(Mutate)` → `FCk_Latent_TickWorlds(SettleFrames)` →
  then per cycle (NumCycles): RunOnServer(save via `Request_Save(SlotName, {})`) →
  `FCk_Latent_TickWorlds(SettleFrames)` → RunOnServer(load via `Request_Load(SlotName, {})` +
  `InTest->TestTrue("load in progress", Sub->Get_IsLoadInProgress())`) →
  `FCk_Latent_WaitForCondition(default-reload-predicate AND ReloadSettled, ReloadTimeoutSeconds)` →
  `FCk_Latent_TickWorlds(SettleFrames)` → `FCk_Latent_AssertCondition(InTest, Assert, "cycle N assert")` →
  `FCk_Latent_EndPIE()` last. Mirror the exact save/load call shapes from
  `Test_Snapshot_TimerParity_MPReload_Gate.spec.cpp` (empty `FCk_Delegate_OnSaveComplete{}` /
  `FCk_Delegate_OnLoadComplete{}` — completion is inferred by the wait predicate, house convention).

Fences:
- File-level guard `#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS` on BOTH files (PIE-dependent).
- Named namespace `ck::auto_test::snapshot` — NO anonymous namespace in the shared .cpp (unity builds).
- Do NOT modify `CkNetAutomation_Common.*`. Do NOT add Build.cs deps (already present).

## Step 2 — Convert `Ck.Snapshot.Parity.Timer_MPReload` to the harness

Rewrite `Test_Snapshot_TimerParity_MPReload_Gate.spec.cpp` to build a `FCk_SnapshotRoundTrip_Spec` from its
existing stage lambdas (same test name string, same class name, same assertions, NumCycles=2). Preserve its
slot name and probe classes; delete only the per-file skeleton the harness now owns.

**Decision gate (post-build, at Step 5):**
- Cycle-1 passes, cycle-2 passes → continue.
- Cycle-1 passes, cycle-2 FAILS → a real double-apply/stacking defect surfaced. STOP: record verbatim
  failure in PROGRESS.md Blockers, revert ONLY this conversion commit (`git revert <hash>`), leave the
  harness itself in, end session. Do NOT drop NumCycles to 1 to go green.
- Cycle-1 fails → harness composition bug; compare the enqueued chain against the original file's stage
  order; if not resolved in 2 attempts → STOP + Blocker.

## Step 3 — `ck::StaticCast` → utils `Cast` sweep (CkFoundation)

Every target utils class already generates a public validating `Cast` via
`CK_DEFINE_CPP_CASTCHECKED_TYPESAFE` (Timer `CkTimer_Utils.h:41`, AnimPlan `CkAnimPlan_Utils.h:29`,
attribute kinds e.g. `CkFloatAttribute_Utils.h:22`). Exemplar of the target pattern:
`CkMontagePlayer_Fragment.cpp:21`.

1. `CkTimer_Fragment.cpp:55,105` → `auto TimerHandle = UCk_Utils_Timer_UE::Cast(Entity);` (the preceding
   `Has` NotReady gate guarantees success; drop the now-unneeded `#include "CkEcs/Handle/CkHandle_Typesafe.h"`
   comment reference if it becomes unused).
2. `CkAnimPlan_Fragment.cpp:62` → `UCk_Utils_AnimPlan_UE::Cast(Entity)`.
3. `CkAttribute_RestorePersistence.h:86` — the shared template cannot name a kind's utils class; add a
   template parameter. Change `HydrationApply`'s signature from
   `template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct, typename T_ApplyEntryFn>`
   to add `typename T_UtilsType` (immediately after `T_RepDataStruct`), replace line 86's
   `ck::StaticCast<typename Current::HandleType>(InEntity)` with `T_UtilsType::Cast(InEntity)`, and update
   the five kind registrars' explicit template arguments (Float/Byte/Integer/Vector/Rotator
   `_Fragment.cpp` — each passes its `UCk_Utils_<Kind>Attribute_UE`). Check whether
   `CkAttribute_RefillPersistence.h` has the same pattern (`rg --no-ignore -n "ck::StaticCast"
   Source/CkAttribute`) — if yes, same treatment.
4. Exit grep: `rg --no-ignore -n "ck::StaticCast" Source/CkTimer Source/CkAnimation Source/CkAttribute`
   → **0 hits in persistence-handler code** (hits elsewhere, e.g. the Cast macro's own body in
   `CkHandle_TypeSafe.h`, are out of scope — do NOT sweep the whole repo).

## Step 4 — Timer Jump absolute mode (CkFoundation)

1. `CkTimer_Fragment_Data.h` — add to `FCk_Request_Timer_Jump` (after `_JumpDuration`):
```cpp
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _JumpMode = ECk_RelativeAbsolute::Relative;
```
   with `CK_PROPERTY(_JumpMode);` (fluent setter; `CK_DEFINE_CONSTRUCTORS` keeps `_JumpDuration` as the
   only essential). Include for `ECk_RelativeAbsolute` comes from `CkCore/Enums/CkEnums.h` (likely already
   transitively included — verify, add if not).
2. `CkTimer_Processor.cpp:199-240` (`DoHandleRequest` for Jump) — the delta is DIRECTION-DEPENDENT in
   absolute mode. Mirror the proven math from the current hydration handler (`CkTimer_Fragment.cpp:66-73`)
   exactly; it moves INTO the switch (this is the single source of truth — no other formula):
```cpp
    const auto RequestedSeconds = InRequest.Get_JumpDuration().Get_Seconds();
    const auto IsAbsolute = InRequest.Get_JumpMode() == ECk_RelativeAbsolute::Absolute;
    const auto CurrentElapsedSeconds = TimerChrono.Get_TimeElapsed().Get_Seconds();

    switch(InParamsComp.Get_CountDirection())
    {
        case ECk_Timer_CountDirection::CountUp:
        {
            // Absolute: RequestedSeconds is the TARGET elapsed; Tick moves elapsed forward by the delta.
            const auto DeltaToApply = IsAbsolute ? RequestedSeconds - CurrentElapsedSeconds : RequestedSeconds;
            TimerChrono.Tick(FCk_Time{DeltaToApply});
            break;
        }
        case ECk_Timer_CountDirection::CountDown:
        {
            // Absolute: Consume moves elapsed the opposite way — delta is Current - Target.
            const auto DeltaToApply = IsAbsolute ? CurrentElapsedSeconds - RequestedSeconds : RequestedSeconds;
            TimerChrono.Consume(FCk_Time{DeltaToApply});
            break;
        }
    }
```
   The signal block below the switch derives `JumpDirection`/`JumpAmount` from the request's raw
   `Get_JumpDuration()` today — change it to derive from the branch's `DeltaToApply` (hoist the value out
   of the switch) so an absolute jump reports the ACTUAL movement. Everything else in the block unchanged.
3. `CkTimer_Fragment.cpp` HydrationApply — replace the baseline-math block (the `ParamsDirection` /
   `CurrentSeconds` / `TargetSeconds` / `JumpSeconds` computation and its comment, lines ~66-79) with:
```cpp
    UCk_Utils_Timer_UE::Request_Jump(TimerHandle,
        FCk_Request_Timer_Jump{Payload.Get_Elapsed()}.Set_JumpMode(ECk_RelativeAbsolute::Absolute));
```
   **KEEP**: the `Has` NotReady gate, the `FTag_Timer_NeedsSetup` NotReady gate (Setup would still stomp a
   jump that drains before it), the direction-restore step, the run-state restore step, and the
   never-`Request_Complete` rule + its comment.
4. Do NOT add a new UFUNCTION or overload to `CkTimer_Utils.h` — the mode rides the existing request
   struct (addon-as-parameter house rule; BP/AS construct the struct and call the fluent setter).

## Step 5 — Build + gate (the phase's ONLY build)

1. Close the editor if open. `--build --target Editor --config Development` → exit 0, no `Error:` naming
   your files in the log.
2. `--test --discover-fresh --test-pattern "Ck.Snapshot"` → expected **baseline count, all green** (the
   Timer conversion keeps the same test name; count unchanged). Decision gate: delta-zero → continue;
   Timer_MPReload red → Step 2's decision gate; any OTHER test red that was green at baseline → STOP, you
   regressed it — diagnose or revert your commits; do not proceed.
3. `--test --test-pattern "Ck.Net"` → **delta-zero vs the recorded baseline**. Anything else vs the
   recorded names → STOP + Blocker.
4. `--test --test-pattern "Ck.Timer"` — planner found NO tests matching this pattern; if it matches zero,
   note that in PROGRESS.md and rely on Snapshot's Timer_MPReload (which exercises Jump-absolute via the
   rewritten HydrationApply — this is the real functional gate for Step 4).

## Commits (in order; stage by explicit file name)

1. CkTests: `test(CkSnapshot): round-trip harness (ck::auto_test::snapshot) + Timer_MPReload converted as proof`
2. CkFoundation: `refactor(persistence-handlers): feature-utils Cast over ck::StaticCast (Timer/AnimPlan/attribute template)`
3. CkFoundation: `feat(CkTimer): Jump absolute mode (ECk_RelativeAbsolute) — hydration drops the relative-baseline math`

## Exit criteria (measurable)

- Both suites at baseline counts with zero new failing names; build exit 0.
- `rg --no-ignore -n "ck::StaticCast" Source/CkTimer Source/CkAttribute Source/CkAnimation` → 0 handler hits.
- `rg --no-ignore -c "EnqueueRoundTrip" Plugins/CkTests/Source` ≥ 2 (decl + Timer conversion).
- PROGRESS.md updated (phase row, baseline, deviations).
