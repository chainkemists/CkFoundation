# Gate 05: StateMachine inspector

Campaign disposition (2026-09-15): representative performance is explicitly CTO-deferred, not measured or accepted. See [current closure gates](CampaignFinishCensus.md); this supersedes historical performance prerequisites below.

**Status:** Authored and accepted in Editor with core-action, populated-history/timeline, variant, negative-policy, navigation, and exact-once teardown evidence. Known Iris ensure debt remains. No publication is implied.

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

The follow-up incremental Development build and fresh real-RHI run of the same production fixture passed 1/1 with zero failed, skipped, or contaminated tests in `scratch/yoga-ecs-state-machine-history8-20260914.log` (SHA256 `00536766BE4F3188B4B8A774732D93B8EDEA8AA086ACA5CE96912CD22174C93C`). A separate captured run issues nine real `Request_Transition` operations through the StateMachine processors and proves exact source/target ordering, nondecreasing render-frame IDs, increasing timestamps, native-to-authored populated field parity, chronological last-eight repeat rows with stable entity/version/run/index keys, all-nine native timeline tooltip payload parity, compatible reload identity/content, and populated view/timeline release. The red predecessor established that `GFrameNumber` may remain equal across completed transitions because it advances with viewport rendering; the final oracle preserves strict timestamp ordering and accepts only nondecreasing render-frame IDs.

The whole-log scan found no compiler error, automation failure, ensure, fatal, authored-resource/CSS/parser error, or AngelScript compile error. Inherited missing development assets, scheduler ordering, Iris, generator-without-editor, and Chromium USB warnings remain outside this focused gate.

## Final inspector acceptance extension

The final incremental Development build succeeded in `scratch/yoga-state-machine-acceptance-build-r13-20260915.log` (SHA256 `79F105848563C0A2C364B74DEB3C021793B975A05A6EF8A21320D035A83DE8A4`). Fresh real-RHI `Ck.UiAuthoring.EcsDebugger.StateMachineInspector` passed 2/2 across two Toolbox lanes with zero failed, skipped, or contaminated tests in 42s in `scratch/yoga-state-machine-acceptance-final-r13-20260915.log` (SHA256 `6347C633CCE8C1C6AD655CB20D7B73C6C5EE8BC10CBBFD1BACD22410B153268B`). The acceptance extension proves:

- real Task, Transition, Condition, requested/resolved-class and multi-level hierarchy projection with native parity;
- full-handle Sub-StateMachine ID parity and physical navigation through the exact `SCkDebug_EntityRef`;
- default and stale handle rejection, atomic invalid-initial-class failure, and client authority refusal;
- Hidden and physical OnHover action policy;
- direct `DoExitState` recording exactly once for physical Stop and active owner destruction, with no second invocation after teardown; and
- fixture UI, inspector, handle, cursor and style release before EndPIE.

One inherited Iris handled ensure at `DataStreamChannel.cpp:244` is emitted during the multi-client fixture. Unreal forwards it as multiple independent `LogOutputDevice` error records, so the fixture uses a finite signature-and-record-shape allowlist: another ensure or a changed condition/message fails the test. The complete accepted raw log was also audited and contains no other Unreal error block, fatal, script, AngelScript or automation error. The inherited ensure remains explicit gate debt rather than a claim that it is absent. Authored text values also remain non-selectable/non-copyable like other shared authored text surfaces; that campaign-wide limitation is not StateMachine-only parity.

## Remaining campaign boundary

All 47 registered ECS inspectors instantiate authored views in current source, and the StateMachine inspector's recorded acceptance matrix is now complete in Editor with the Iris ensure debt above. Fifteen of the campaign's 27 outer consumer shells are accepted; Audio is authored but unaccepted, and eleven others retain incomplete or partial native outer composition. Grid toolkit and common Gallery supporting/editor surfaces also remain native and must stay explicit outside that denominator.

The twelve unaccepted consumer rows are Audio, Crowd, Launcher/Suite, Entity Debug Overlay, Insights, Intent, Jolt Bake, Jolt, Map, Optimization, Save, and SM debugger. Audio already authors its shell/pages and retains acceptance gaps. Save already has an authored entity-navigation tree but retains native outer splitters. The SM module constructs the same `SCkSmDebuggerWindow` in Editor and packaged Development; the separately named `SCkSmDebuggerPackagedWindow` is explicitly tombstoned. That active SM debugger row is distinct from the accepted ECS StateMachine inspector. See [the current source census](CampaignFinishCensus.md).

The approved local-player/gamepad and two-Slate-user ownership gate, representative measured performance on an identified reference machine, and production-host every-debugger teardown matrix remain required. Packaging, keyboard-only traversal, localization, and browser/narrow-breakpoint restacking remain deferred. Resource Inspector `+` remains excluded. Network multiplayer remains conditional.
