# Gate 05: Visual LOD Arbiter Tuners

**Status:** Scoped native and PIE static-arbiter evidence verified; full Visual LOD, dynamic crowd/profile panels, browser acceptance, and the wider campaign remain open.

## Scope

Migrate only `SCkVisualLodDebuggerWindow::DoBuild_ArbiterTuners` to an authored CkSlateLayout document. Keep the existing Visual LOD debugger window, live arbiter collector, and runtime request APIs as native owners. The authored view replaces the fixed arbiter-tuner rows; it does not include the dynamic crowd tuner panels, renderer-profile panels, roster, detail rail, activity log, markers, or other Visual LOD window composition.

HTML/CSS owns sections, row placement, labels, tooltips, status, reset placement, responsive wrapping, and scrolling. C++ owns the live `FCk_VisualLodArbiter_RuntimeTuners` snapshot, entity/arbiter availability gate, typed requests, complete validation, optimistic live-copy publication, and weak-owner dispatch.

## Existing control contract

`DoBuild_ArbiterTuners` currently enables controls only when the window has a live arbiter, config, and valid entity. It contains **12 numeric editors**: three integers and nine floats. Every numeric editor is presently unbounded above. Integer values round and clamp to zero or greater; float rows default to a zero lower bound except both fade-anchor values, which deliberately allow negatives.

| Control | Current presentation and setter rule |
|---|---|
| Near budget; Lock budget; Max preempts per tick | Integer, 0+; write `NearBudget`, `LockBudget`, or `MaxPreemptsPerTick`. |
| Promote distance; Demote distance | Float, 0+, 0 digits. Promote clamps to current demote; demote clamps to current promote. |
| Lock max distance; Always in view; Preempt margin | Float, 0+, 0 digits. |
| View margin | Float, 0+, 1 digit. |
| Fade duration | Float, 0+, 3 digits; writes `FCk_Time` seconds. |
| Fade anchor lead frames; Fade anchor bake lag intervals | Float, no authored lower bound, 2 digits. |
| Pool exhaustion | Two choices: `PromoteInstead` and `Unrendered`. |
| Reset to authored | Enabled only while the live snapshot differs from authored; requests the complete reset and updates the local snapshot to authored. |

Each write must copy the full live tuner snapshot, mutate one field, call `UCk_Utils_VisualLodArbiter_UE::Get_AreRuntimeTunersValid`, and only then issue `Request_SetRuntimeTuners` and publish the optimistic copy. Invalid scalar, crowd-order, rate, band, or profile state must produce no request and no local mutation. Current `DoRequest_RuntimeTuners` does not separately short-circuit an equal valid snapshot; preserve that observed request contract unless the runtime request layer proves a no-op policy.

## Integer fidelity decision (shared native gate verified)

The existing number-input transports float and cannot distinguish adjacent int32 values above 16,777,216. Keep its public contract compatible. Add a reusable authored `int32-input` with typed `TAttribute<int32>` model bindings and int32 committed events, composed over the retained TextInput adapter. The shared extension built successfully and passed the rendered serial authoring gate, 123/123, including exact int32 native and collection coverage. The static Visual LOD arbiter consumer and focused PIE path are now verified; dynamic crowd/profile panels remain deferred.

Optional min/max are exact decimal Text literals validated as int32, defaulting to the full int32 range. User drafts accept finite numeric text, round in double, and clamp to the validated bounds before conversion. Double represents every int32 exactly; malformed, nonfinite, or parser-overflow drafts reject without publication. This retains fractional draft normalization while preventing float precision loss and unsafe conversion. The three budget controls author min="0"; the nine float controls retain number-input.

Typed bindings/events require additive document/schema and surface dispatch plumbing. Preserve weak ownership, dispatch admission, atomic reload rejection, active draft/focus retention, and ordinary commits. Do not infer an equal-value request suppression contract from control normalization. Full authoring compatibility and focused native boundary tests are required before migrating the consumer.

## Implementation boundary

- `Plugins/CkGameplayDebugger/Source/CkVisualLodDebugger/Public/CkVisualLodDebugger/Window/SCkVisualLodDebuggerWindow.h/.cpp`: retain the window host and request/reset methods; add a retained authored tuner view, bindings, weak actions, and installed-resource polling only for this static arbiter section.
- `Plugins/CkGameplayDebugger/Resources/UI/VisualLodArbiterTuners.ui.html/.css`: add the document, using registered number-input for the nine float controls, registered int32-input for the three exact integer editors, and ordinary authored actions/selection controls. Do not fold this resource into crowd/profile dynamic layout.
- `Plugins/CkGameplayDebugger/Source/CkVisualLodDebugger/CkVisualLodDebugger.Build.cs`: add `CkSlateLayout` and the explicit installed-resource staging entries if absent; retain existing `CkDebuggerCommon` because it supplies the shared `FCkDebug_UiRegistry` snapshot.
- Add a focused `CkVisualLodDebugger` authored-control spec in that module’s existing test location. Do not place a partially initialized arbiter fixture in CkSlateLayout tests.

The retained view must request its `main` region before initial installed-resource polling, as the Intent slice does. Use `FCkDebug_UiRegistry::TryCreate` rather than a per-window registry. Any resource-load failure stays visible in the existing native host fallback and must not leave a partial view published.

## Target ownership boundary

A retained draft belongs to the arbiter selected when that view was created. Recreate the tuner view under a persistent native host when the selected live entity changes or the session/world resets; retain it across ordinary same-target refreshes and compatible resource reloads. Invalidate the old view generation before detaching its widgets, because focus loss during detachment can synchronously invoke commit callbacks. Every old callback must reject after generation invalidation even if a test or another owner still holds the old view. Avoid capturing an owning FCk_Handle in callbacks across world teardown; use a host generation/lifetime gate and current validated live state.

The PIE fixture must exercise draft-on-A then switch-to-B and prove neither receives a stale commit, followed by a normal new-view commit to B. Reset/unavailable transitions must invalidate in the same order. A read-only public Get_ArbiterTunersView accessor may expose the actual retained view for mounted interaction tests; no private live-snapshot injection is permitted.

## Test fixture and acceptance

The test needs a real Visual LOD arbiter entity with authored config and at least one crowd/profile topology. The existing Visual LOD gym already provides this: `Plugins/CkTests/Script/CkVisualLod/CkVisualLod_GymAssets.as` defines `Asset_VisualLodGym_ArbiterConfig`, and `CkVisualLod_GymStation.as` adds it through `UCk_Utils_VisualLodArbiter_UE::Add`. Start the gym/game-mode path in the same PIE/Game world that owns the debugger window, then assert the collector observes that arbiter. Existing `CkVisualLod` runtime-tuner validation and atomic-set specs remain utility proof only; do not fabricate the window’s `_Live` cache.

The final focused native test must:

1. Mount the actual window/view, prove the unavailable gate is disabled and produces no request, then target a live configured arbiter.
2. Drive every listed numeric, enum, and reset control through native input. Verify value formatting, zero/negative admission rules, promote/demote pair clamps, full-snapshot request result, reset result, and that rejected malformed/invalid input leaves both runtime and local snapshots unchanged. For all three integer fields, exercise an exactly representable value above 16,777,216 and the `int32` upper boundary; out-of-range text must not reach an unsafe conversion.
3. Establish the equal-commit behavior from the real request path before claiming a no-op guarantee. If the request layer has a revision/counter, assert it; otherwise report only the state that is observable.
4. Exercise compatible installed-resource reload, rejected candidate reload, focus/draft preservation for one number input, and view/window owner expiry so held callbacks become inert.
5. Capture a narrow scrolled viewport with a real control reachable. It need not show all controls simultaneously.

## Risks and deferred work

The outer window rebuilds its crowd/profile hosts when topology signatures change, so this slice must keep static arbiter bindings separate from dynamic nested sections. A live request carries a complete tuner snapshot; stale callbacks must re-read or reject against the current target rather than overwrite changed crowd/profile data. This migration is session-only and must not imply config-asset persistence.

Dynamic crowd and profile tuners are explicitly deferred from this slice, along with the broader Visual LOD debugger layout and browser-reference acceptance.

## Source basis

- `SCkVisualLodDebuggerWindow.cpp:1378-1744`: current controls, availability gate, full-snapshot validation/request, and reset.
- `SCkVisualLodDebuggerWindow.h:120-133`: static arbiter and deferred crowd/profile request boundaries.
- `CkVisualLodArbiter_Utils.cpp` and `CkVisualLod_Ranking.spec.cpp`: runtime-tuner validation and atomic application coverage.
- `SCkDebug_NumericEditor.h/.cpp`: exact double-valued native integer adapter; `CkUiNumberInput.cpp`: current float-only shared number contract.
- `Plugins/CkTests/Script/CkVisualLod/CkVisualLod_GymAssets.as` and `CkVisualLod_GymStation.as`: canonical authored arbiter config and creation path.
- `CkVisualLodDebugger.Build.cs`: current debugger dependencies; it presently lacks `CkSlateLayout`.
