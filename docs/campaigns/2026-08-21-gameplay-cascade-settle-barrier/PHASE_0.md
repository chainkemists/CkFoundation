# Phase 0 — the red executable spec (BusterBlock side)

## Entry criteria
- `PROMPT.md` status reads `APPROVED`.
- CkFoundation checkout contains `e4cf54edf` (`git log --oneline --grep="group-local settle barriers" -1`).
- For the `OneShotPushReaches` floor gate only: BusterBlock branch `bugfix/rewind-oneshot-push-visuals`
  (BB #2706) + CkF #717 (`bugfix/oneshot-transform-push-pump-drain`) checked out or merged — neither is
  on `dev` as of 2026-08-21. The spec below is RED with or without #717; only the floor gate needs them.
- No `BusterBlockEditor.exe` running.

## Steps
1. Read `Plugins/CkTests/Script/CkStateMachine/CkAutoTest_StateMachine_BasicTransition.as` +
   `CkStateMachine_TestStates.as` (the copy-me shape), `Plugins/BusterBlockTests/Script/Tests/NpcAI/BB_AutoTest_NpcAI_SmTransitions.as`
   (game-side reference), and `Script/ECS/RewindStation/BB_RewindStation_Hfsm_Manual.as`
   (state/task/condition authoring: `AddTask`, `AddTransition`, `AddCondition(UBb_SmCondition_AllTasksSucceeded)`).
2. Create `Plugins/BusterBlockTests/Script/Tests/StateMachine/BB_AutoTest_Sm_CascadeWriteReachesComponentSameFrame.as`
   (test class `UBb_AutoTest_Sm_CascadeWriteReachesComponentSameFrame : UCk_AutoTest_Base`; the SM
   state/task classes may live in the same file if the harness's one-class-per-file rule applies
   only to the test class — check `CkAutoTest_CreationSpecification.txt`; otherwise a sibling
   `_SmParts.as`).
3. Arrange: a standalone transform entity at A with a `UStaticMeshComponent` via
   `utils_unreal_component::Add`. Store the entity handle where the StateB task can reach it (a
   fragment on the test entity, or a static-ish lookup the task reads — follow whatever existing BB SM
   tests do to pass context to tasks).
4. SM: `StateA` with `TaskA` (`_TaskMode = Tick`) whose `DoTick` returns `Succeeded` once a go
   entity-tag is present on the SM owner (`utils_entity_tag::Has`) — **ignition MUST be a Tick-mode
   task**: AutoTest step callbacks run in the frame's tail region (after both barriers — measured
   2026-08-24), so a step that starts/ignites the SM directly is a tail-born producer the barrier
   cannot reach and the spec stays red for the wrong reason. The step only flips the tag
   (`utils_entity_tag::Add`); `SmTask_Tick` picks it up next frame's AI main pass. Transition
   `StateA → StateB` on `UBb_SmCondition_AllTasksSucceeded` (**event-driven — mandatory**; polled or
   zero-condition transitions are armed by `SmState_Update` next frame). `StateB` with `TaskB` whose
   `DoEnterTask` calls `utils_transform::Request_SetLocation(<subject>, B)`, B = A + (500,0,0).
   Add the SM in `DoBeginPlay`.
5. Steps: `Add_Step_WaitUntil("SM reaches StateB with component already at B", n"Check_SameFrame")`
   whose predicate: if current state != StateB → OutResult=false (keep waiting, bounded by the
   timeout); else read the live component world location: within 1uu of B → FinishSuccess;
   otherwise `FinishFailure(f"cascade write landed late: component={...} expected={...}")` —
   **fail on the FIRST tick StateB is observed**, never wait for it to catch up. (Measured: the
   predicate rides the harness step timer, which executes in the frame's TAIL region — after the
   push slot. First-observation stays sound: with the barrier, StateB and the push land in the same
   frame, so the first observation already sees the moved component; without it, the write is
   pump-drained and swallowed, so every observation sees the stale component.)
6. Second spec, same folder: `BB_AutoTest_Sm_PolledTransitionDoesNotTripBarrier.as` — an SM started
   into a state with one transition gated by a polled condition that returns false; `Add_Step_WaitFrames(30)`
   then FinishSuccess. Its only assertion is the runner's own warning escalation (a
   `Local settle after group` warning fails it). Trivially green today; stays green in Phase 1 only if
   `SmCondition_Polled`/`SmState_Update` stay excluded.
7. Run from the BusterBlock root: `${env:UE-CmdLineArgs}='-DisablePlugins=RiderLink'` then
   `./CkAuto/UnrealToolbox.exe --test --discover-fresh --test-pattern "Sm_"` (or the two names).

## Decision gate
- **Expected: `CascadeWriteReaches…` FAILS** with `cascade write landed late` and component at A
  (the cascade's enqueue drained in the tail pump; the push — with #717 — catches up next frame, after
  the predicate already observed StateB). `PolledTransition…` PASSES. Paste the failure line into
  PROGRESS.md. → Phase 1.
- Exit 76 → AS authoring error; fix only your syntax; rerun.
- **`CascadeWriteReaches…` PASSES** → the cascade converged in-frame without the barrier (e.g. the
  whole chain ran in the main pass because no step needed a re-dispatch). STOP, record in blockers:
  the reviewer must re-examine whether the spec exercises a multi-dispatch cascade (it must require at
  least one spawn — StateB's creation on entry guarantees that) — do not invent a different trigger.
- Anything else → STOP, blockers.

## Exit criteria
- Both tests discovered, `CascadeWriteReaches…` RED with the expected message, line recorded;
  `PolledTransition…` GREEN.
- If #2706/#717 are checked out: `--test-pattern OneShotPushReaches` still passes (#717 floor intact).
  Otherwise record "floor gate skipped — branches not present".
