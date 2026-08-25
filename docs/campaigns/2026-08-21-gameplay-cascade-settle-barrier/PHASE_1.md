# Phase 1 — declare the barrier; the spec goes green

## Entry criteria
- Phase 0 red spec recorded. Campaign branch created from current CkF `dev`
  (`feature/gameplay-cascade-settle-barrier`); never commit to `dev` directly.

## Steps
1. Add to EACH of the 16 trigger processor classes listed in PROMPT.md's two trigger tables, in the
   `public:` trait block right after `using Group = ...;` (mimic the exact placement used by
   `FProcessor_Transform_HandleRequests` in `CkTransform_Processor.h:84-88`):
   ```cpp
   using LocalSettleAfter = FGroup_Gameplay_Script;
   static constexpr auto LocalSettleTrigger = true;
   ```
   `FGroup_Gameplay_Script` is declared in `CkEcs/Scheduler/CkProcessorGroups.h` (already
   included transitively wherever `using Group` compiles). The 16: Sm_Setup, Sm_HandleRequests,
   Sm_FirstSyncInitialState, Sm_CommitPendingTransition, **SmScript_CommitPendingAttach** (same
   header, `CkStateMachine_Processor.h:24`), SmState_Evaluate, SmState_Exit, SmTransition_Evaluate,
   SmTransition_Exit, SmTask_FireFinishedSignal, SmTask_Exit, SmCondition_Exit,
   EntityScript_SpawnEntity_HandleRequests, EntityScript_ContinueConstruction,
   EntityScript_FinishConstruction, EntityScript_BeginPlay.
2. **No replay-only participants.** Do NOT add `LocalSettleAfter` to `FProcessor_SmCondition_Polled`,
   `FProcessor_SmState_Update`, or any marker-less processor (marker-less participants replay every
   pass — see PROMPT.md "The primitive").
3. Do NOT touch any processor in the EXCLUDED list. Do NOT add any `MarkedDirtyBy`.
4. Optional hardening (one line, maintainer-confirmable — say in the commit if taken):
   `FProcessor_SmCondition_Exit::ForEachEntity` — `Try_Remove<FTag_SmCondition_PendingExit>()`
   BEFORE the invalid-script early return, mirroring `FProcessor_SmTask_Exit`
   (`CkSmTask_Processor.cpp:92-97`, which carries the why-comment). Without it that path leaves a
   trigger dirty until the entity finalizes, burning the shared budget for the frame.
5. Comments: one why-line on `FGroup_Gameplay_AI` in `CkProcessorGroups.h` ("decision-logic
   tier — state machines, state trees, planners; SM cascades converge at the Gameplay_Script
   barrier, after their EntityScript spawn pipeline") — match the style of the existing group
   comments there. No per-processor comments (the trait lines are self-explanatory; the docs
   carry the why).
6. Docs: one paragraph in `Source/CkStateMachine/Claude.md` ("Cascade convergence" — including
   the event-driven-only scope and why Polled/State_Update are fenced) and extend
   `Source/CkEcs/Claude.md` § "Group-local settle barriers" with the second declared barrier AND the
   two under-stated semantics (marker-less participants replay every pass; trigger check is fragment
   presence, so a trigger must REMOVE its marker).
7. Build + run, editor closed, `-DisablePlugins=RiderLink` env, from the BusterBlock root:
   `./CkAuto/UnrealToolbox.exe --build --test --test-pattern "Sm_"`.

## Decision gates
- **Expected: build ok; `CascadeWriteReaches…` PASSES; `PolledTransition…` still PASSES.** Still
  red → STOP: capture the failure line AND grep the run log for `Local settle after group` (did the
  barrier activate? did it hit the pass limit? which trigger is named still-dirty?) and for the
  registration ensures (`opts into local settle` / `Local-settle plan`) — one invalid participant
  silently disables the whole plan; record in blockers; do not widen the participant set on your own
  (that is review question 3).
- Then, no rebuild: `--test-pattern StateMachine` (CkTests + BB SM tests — record the count; any
  shifted-timing failure is review question 8 material: list names + assertion text, do not fix),
  `--test-pattern Rewind`, `--test-pattern Transform` (record the count; `DirtyOwnersOnly` green),
  `--test-pattern Tween`, and `--test-pattern OneShotPush` if #2706/#717 are present.
- Log checks on every run: `Local settle after group [FGroup_Gameplay_Script] reached` → must be
  **0**; `Pump limit [` → **0** (the tail pump's own limit — barrier passes consume its budget);
  `Dirty marker conflict` → 0; `Angelscript: Error`/`Warning` naming touched files → none.
- Full suite `--test`: compare against the Phase-0 baseline by NAME; new failures → STOP, list them.

## Fences
- No scheduler source edits. No new groups. No `SkipPump` removals to make something eligible.
- No replay-only participants, ever, without re-running the Q4 livelock analysis.
- If a chosen participant turns out to declare `SkipPump` (the barrier rejects it at registration
  with an ensure and invalidates the WHOLE plan): STOP and record which — do not flip its pump policy.
- Never add `Local settle after group` to the AutoTest runner's warning-suppression list.

## Exit criteria
- Both specs green; gates at baseline; zero barrier-limit and zero pump-limit warnings; commit on the
  campaign branch: `feat(statemachine): converge SM cascades at a Gameplay_Script settle barrier`.
