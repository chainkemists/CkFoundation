# Gate 05: StateMachine inspector

**Status:** Authored in production with bounded core-action and lifetime evidence. Full inspector acceptance remains open. No publication is implied.

## Ownership and invariant

`FCkInspector_StateMachine` now authors its root, state, task, transition, condition, requested-class, hierarchy, history, sub-state-machine and unavailable presentation from `EcsInspectorStateMachine.ui.html/.css`. HTML/CSS owns the visible layout and action placement. C++ remains authoritative for typed variant projection, live request admission, public Start/Stop/Pause/Resume dispatch, entity navigation, native capture/diff, the specialized timeline port, atomic startup fallback, and teardown.

Routed action identities use immutable entity ID plus generation. Every callback re-resolves the current StateMachine composition and authority immediately before dispatch. Missing composition, pending destruction, deactivation and inspector destruction fail closed. The native fallback uses the same current-composition predicate.

## Bounded production-path evidence

The final incremental Development build succeeded and fresh real-RHI `Ck.UiAuthoring.EcsDebugger.StateMachineInspector.AuthoredComposition` passed 1/1 with zero failed, skipped, or contaminated tests in `scratch/yoga-ecs-state-machine-final6-20260914.log` (SHA256 `1DF13F6CDBFD102E985CD527793D719AEF81C25FC225757588A99DEC5B186FE4`). The fixture mounts the production inspector in a real Slate window and proves:

- independent authored root views, installed resources, native-to-authored root status/current-state parity, and selected diff tones;
- physical Start, Pause, Resume and Stop clicks, exactly one queued production request per action, and settlement through the real processors;
- entered-state identity through Pause/Resume, state clearing on Stop, and Enter then Exit lifecycle ordering;
- entered State class projection, compatible reload identity, and atomic rejection of a missing action;
- missing-Params refusal and recovery, pending-destruction refusal, deactivated held-action physical refusal, and view release on deactivation and inspector destruction;
- fixture-owned UI and handles released before EndPIE.

The lifecycle recorder observes its virtual `ExitState` override again during EndPlay before the production base method deduplicates through `FTag_SmState_Active`. The accepted assertion therefore requires one Enter followed by Exit and permits only trailing Exit attempts; it does not claim direct exactly-once `DoExitState` instrumentation.

The whole-log scan found no compiler error, automation failure, ensure, fatal, authored-resource/CSS/parser error, or AngelScript compile error. Inherited missing development assets, scheduler ordering, Iris, generator-without-editor, and Chromium USB warnings remain outside this focused gate.

## Unproven inspector rows

- Real Task, Transition, Condition, requested-class and hierarchy variants.
- Physical sub-state-machine navigation.
- Populated history and timeline projection, last-eight ordering and stable keys, compatible reload identity for populated timeline state, and timeline release.
- Default/stale handles, invalid initial class, authority refusal, and Hidden/OnHover history policy.
- Direct exactly-once `DoExitState` instrumentation.
- Authored text values are not selectable/copyable like the native read-only editable-text rows; this is a shared authoring-surface limitation, not StateMachine-only parity.

These rows prevent a full StateMachine acceptance claim. The smallest additional production-path slice is one fresh real-PIE history/timeline fixture using actual transitions, native-to-authored projection checks, compatible reload identity, and retained timeline/view release.

## Remaining campaign boundary

All 47 registered ECS inspectors now instantiate authored views in current source. This is a source-composition statement, not a completed acceptance matrix. Twelve of the campaign's 27 outer consumer shells are complete and fifteen retain native layout composition. Grid toolkit and common Gallery supporting/editor surfaces also remain native and must stay explicit outside that denominator.

The fifteen incomplete consumer rows are AStar, Audio, Crowd, Launcher/Suite, ECS main, Entity Debug Overlay, Input HUD Overlay, Insights, Intent, Jolt Bake, Jolt, Map, Optimization, Save, and SM debugger. Save already has an authored entity-navigation tree but retains native outer splitters, so it is partial rather than wholly unauthored. SM debugger includes editor and packaged variants in one campaign row.

The approved local-player/gamepad and two-Slate-user ownership gate, representative measured performance on an identified reference machine, and production-host every-debugger teardown matrix remain required. Packaging, keyboard-only traversal, localization, and browser/narrow-breakpoint restacking remain deferred. Resource Inspector `+` remains excluded. Network multiplayer remains conditional.
