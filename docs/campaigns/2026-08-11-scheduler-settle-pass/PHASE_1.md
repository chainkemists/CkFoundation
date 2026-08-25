# Phase 1 — scheduler plumbing (toggle OFF = provably zero change)

## Entry criteria
- Phase 0 exit criteria met (red spec recorded in PROGRESS.md).
- Working on CkFoundation branch for this campaign (create `feature/scheduler-settle-pass`
  from current `dev`; do NOT commit to `dev` directly).

## Steps

1. **Trait.** In `CkProcessorTraits` land (mimic how `PumpPolicy` / `EmptyViewPolicy` are
   declared and detected — copy the exact SFINAE shape used there):
   ```cpp
   enum class ECk_ProcessorSettleParticipation : uint8 { DoesNotParticipate, Participate };
   ```
   Detection: absent declaration == `DoesNotParticipate`. A processor opts in with
   `static constexpr auto SettleParticipation = ECk_ProcessorSettleParticipation::Participate;`
2. **Descriptor + node.** `FProcessorDescriptor` gains `bool _ParticipatesInSettle = false;`
   derived at registration from the trait; `FProcessorGraphNode` copies it (mimic
   `_CanSkipWhenViewEmpty` end-to-end — same three touch points).
3. **Settle order.** Where `_MainPassOrder` / `_PumpOrder` are built (`FProcessorScheduler`
   construction), build `_SettleOrder` = the subset of `_MainPassOrder` whose nodes have
   `_ParticipatesInSettle` — same relative (topological) order, no re-sort.
4. **Settings.** `UCk_Ecs_ProjectSettings_UE` gains `_EnableSchedulerSettlePass` (bool,
   default **false**) + `UCk_Utils_Ecs_Settings_UE::Get_EnableSchedulerSettlePass()` —
   mimic `_EnableEmptyViewMainPassSkip` verbatim (property, category, getter, tooltip). Cache
   in the scheduler constructor as `_UseSettlePass`.
5. **Dispatch.** In `FProcessorScheduler::Tick`, AFTER the pump block and its
   `_LastFramePumpCount` bookkeeping:
   ```cpp
   if (_UseSettlePass)
   {
       SCOPE_CYCLE_COUNTER(STAT_Scheduler_Settle);
       for (const auto NodeIndex : _SettleOrder)
       {
           auto& Node = _Partition._Nodes[NodeIndex];
           // Same empty-view short-circuit as the main pass (reuse the existing block's
           // shape verbatim — version sum → cached verdict → skip), then:
           (*Node._Instance)->Pump();   // DoTick(DeltaT = 0); settle never advances time
       }
   }
   ```
   Add `DECLARE_CYCLE_STAT(TEXT("Scheduler::Settle"), STAT_Scheduler_Settle, STATGROUP_CkScheduler);`
   next to its siblings. Reuse the node's existing Insights trace spec (same pattern as DoPump).
6. **Observability.** Extend the `Ck.Ecs.Scheduler.ExportOrder` dump
   (`CkEcsWorld_Subsystem.cpp`) with a trailing `[Settle]` section listing `_SettleOrder`
   nodes in order (empty in this phase — no processor opts in yet).

## Fences
- Do NOT add any `SettleParticipation` annotation to any processor in this phase.
- Do NOT touch pump code, Cleanup, `FTag_Transform_Updated`, or any Transform/UnrealComponent
  processor logic.
- Do NOT default the setting to true anywhere (including test configs).

## Decision gates
- Build: `${env:UE-CmdLineArgs} = '-DisablePlugins=RiderLink'` then
  `./CkAuto/UnrealToolbox.exe --build --test --test-pattern OneShotPush` (BusterBlock root,
  editor closed). Expected: build succeeds; `OneShotPushReaches` passes; `SettlesSameFrame`
  still FAILS with the identical Phase-0 message (toggle is off — nothing may change).
  Different failure text or a new pass → STOP, blockers.
- Full suite `--test` (no pattern): counts must match the Phase-0-era baseline recorded in
  PROGRESS.md (same failing names). New failures → STOP, blockers.

## Exit criteria
- Compiles; suite delta vs baseline = zero; `SettlesSameFrame` red, unchanged message.
- `[Settle]` section present-and-empty in a fresh ExportOrder dump (run from any PIE world,
  e.g. temporarily via the Phase-0 test's `System::ExecuteConsoleCommand` trick — revert after).
- Commit on the campaign branch: `feat(scheduler): settle-pass plumbing behind a default-off setting`.
