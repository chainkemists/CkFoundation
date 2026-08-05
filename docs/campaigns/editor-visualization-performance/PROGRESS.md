# Progress

Last updated: 2026-08-04 18:02 PDT

## Current state

- Implementation, focused verification, adversarial review, and a post-review restarted-editor non-PIE trace are complete for the idle transform pipeline and retained visualizer architecture.
- The trace gate passes for the reported processor stack: the complete timer export for representative settled frame 2600 contains none of the target processors.
- The whole editor is improved but is still above the 16.67 ms target on average; the remaining settled cost is dominated by Slate/editor UI rather than the reported CK processor stack.
- Root HEAD: `63efdffc7e0b6613c7ad7e546cef7bc08a06c1b5` on `bugfix/npc-navigation`.
- CkFoundation HEAD: `7598146517eb911ac148352d2970ed7ba4e1a4d6` on `dev`.
- CkTests HEAD: `e5bb948b0e666f0dbc989ac911548b727e2d2757`.

## Dirty boundary

Campaign-owned edits cover the scheduler metadata, transform request and dirty-owner propagation, editor selection invalidation, retained CkPmg/CkIsm visualizers, transient ISM caching, editor preview registration removal, and focused tests.

Unrelated and untouchable:

- `Source/CkGoap/Public/CkGoap/Planner/CkGoap_Planner_Utils.cpp`.
- Root external actor assets, NPC test script, save/load prompt, and CkGameplayDebugger prompt shown by root status.

## Implemented behavior

- The legacy transform and probe preview processors are no longer registered. Stationary visualizers are retained instead of being resubmitted every editor frame.
- `CkEntityVisualizer` is PMG-first for selected and low-count transforms. Preview-all and ownerless bulk visualization routes shared high-count gizmos through CkIsm.
- Ownerless entities create no transform/probe visualization unless preview-all is explicitly enabled.
- Shared CkIsm primitives cover transform gizmos, boxes, spheres, cylinders, and cones; 1,024 transform sources share two renderer batches.
- Preview-all always uses the shared ISM transform-gizmo path, including selected owners; it cannot accidentally expand a high-count selection into retained PMG children.
- Editor selection changes invalidate retained visualizers through selection events rather than a full per-frame selection scan.
- Shape-dimension and probe-debug-info mutations invalidate retained visuals through coalesced editor events, so stationary previews stay correct without per-frame rebuilding.
- `FProcessor_Transform_HandleRequests` declares its request storage as a main-pass requirement. The scheduler skips dispatch when no transform requests exist and wakes when request storage is populated.
- `FProcessor_UnrealComponent_PushTransform` iterates owners marked with `FTag_Transform_Updated`, performs initial setup explicitly, and does not poll externally drifted components while the ECS transform is idle.
- No compatibility transform-poll processor or opt-in poll tag remains. External transform reassertion must use the normal transform request path.
- ISM proxy transform work declares `FTag_Transform_Updated` as a main-pass requirement and compares desired transform against rendered state before writing.
- ISKM crowd advance is runtime-only, so it is excluded from non-PIE editor dispatch.
- State-machine first-sync processing includes `FTag_Sm_NeedsInitialStateEntry` in its view instead of scanning every state machine after initialization.

## Verification evidence

- `Saved/Logs/Build-EditorVisualization-FinalGates2.log`: final full BusterBlockEditor build succeeded.
- `Saved/Logs/Test-UnrealComponent-DirtyOwnersOnly.log`: `Ck.UnrealComponent.TransformPropagation.DirtyOwnersOnly`, 1/1 passed. The test proves idle external drift is not polled, a real transform request propagates, and subsequent idle drift remains untouched.
- `Saved/Logs/Test-Scheduler-CustomMainPassRequiredFragments-Final.log`: scheduler empty-storage skip/wake test, 1/1 passed.
- `Saved/Logs/Build-EditorVisualizer-IdlePropagation-Clean.log`: retained visualizer and idle propagation build succeeded.
- `Saved/Logs/Test-EntityVisualizer-FinalHighCount.log`: retained PMG/ISM visualizer tests, 4/4 passed, including 1,024 sources composing 6,144 retained children into exactly two ISM renderer batches.
- `Saved/Logs/Test-Pmg-PersistentDuration-Clean.log`: persistent CkPmg lifetime tests, 2/2 passed.
- `Saved/Logs/Test-StateMachine-FirstSync-DirtyView.log`: non-owning-client first-sync regression, 1/1 passed.
- `Saved/Logs/Build-EditorVisualization-FinalReview.log`: post-review BusterBlockEditor build succeeded.
- `Saved/Logs/Test-UnrealComponent-FinalReview.log`: post-review dirty-owner transform propagation, 1/1 passed.
- `Saved/Logs/Test-EntityVisualizer-FinalReview.log`: post-review retained PMG/ISM visualizer suite, 4/4 passed.
- Focused logs contain no relevant CK ensures or script errors.
- `git diff --check` passes for CkFoundation and CkTests; line-ending warnings are informational.
- Supplied baseline, frames 490-515 (`Saved/Logs/Baseline-Frames490-515.json`): 82.231 ms average frame, 106.893 ms p95; CK ECS 33.424 ms average exclusive and 41.895 ms p95 exclusive.
- Intermediate trace (`Saved/Logs/Intermediate-20260804-144431-AllFrames.json`): CK ECS 6.255 ms average exclusive and 10.140 ms p95 exclusive.
- Fresh restarted-editor trace (`Saved/Profiling/FinalIdleEditor-MainMap.utrace`), settled frames 3400-3500 (`Saved/Logs/FinalIdleEditor-MainMap-Tail3400-3500.json`): 23.629 ms average frame, 30.256 ms p95; CK ECS 1.126 ms average exclusive and 1.626 ms p95 exclusive.
- Representative settled frame 3450 (`Saved/Logs/FinalIdleEditor-MainMap-Frame3450-AllTimers.json`): 22.69 ms frame; CK ECS 0.99 ms exclusive; scheduler dispatch 0.162 ms exclusive. The complete 1,249-timer export contains no transform preview, probe preview, transform request handler, Unreal-component push, ISM transform, ISKM crowd advance, or state-machine first-sync scope.
- Post-review restarted-editor trace (`Saved/Profiling/FinalReviewIdleEditor-MainMap-Retry.utrace`), settled frames 2500-2650 (`Saved/Logs/FinalReviewIdleEditor-MainMap-Tail2500-2650.json`): 19.104 ms average frame, 21.463 ms p95; CK ECS 1.348 ms average exclusive and 1.666 ms p95 exclusive.
- Post-review representative frame 2600 (`Saved/Logs/FinalReviewIdleEditor-MainMap-Frame2600-AllTimers.json`): 17.866 ms frame and CK ECS 1.362 ms exclusive. The complete 727-timer export has zero matches for transform preview, probe preview, transform request handler, Unreal-component push, ISM transform, ISKM crowd advance, state-machine first-sync, line batcher, or composite debug primitives.
- Remaining tail costs are editor/UI work, led by Slate, details/group bracket drawing, and non-target game processors. The original line-batcher/composite-debug-primitives signature is absent from both representative final frames.
- The analyzer wrote valid reports, but its shutdown log records pre-existing TraceServices MemAlloc tag/no-event diagnostics and can return a nonzero process code after report generation; report existence and contents were verified directly.
- The first post-review editor launch hit an unrelated `FAmbientCubemapCompositePS` startup fatal before reaching idle; a clean retry completed 2,702 trace frames and exited normally after the benchmark window.

## Remaining scope

1. Treat the remaining Slate/editor-UI cost as a separate optimization pass; this campaign does not claim a whole-editor 60 fps result.
2. Add live component/instance-write instrumentation if a future gate needs to prove ISM renderer update counts beyond the current batch-composition test and idle trace.
3. Keep the unrelated root external-actor assets, NPC test script, save/load prompt, CkGameplayDebugger changes, and CkGoap planner edit untouched.
