# Gate 07: Jolt Bake Inspector authored shell

## Status and boundary

Implemented and accepted for the focused Editor boundary, 2026-09-15; the census is 16/27. The pre-edit inventory and remaining campaign gates are recorded in [CampaignFinishCensus.md](CampaignFinishCensus.md).

HTML/CSS owns ordinary toolbar/action placement, summary cards, list-row presentation, stable splitters and selected-item details. Native ports retain the exact search/list mechanics and preview-world renderer. AssetRegistry inventory, one-item-per-tick analysis, filter/selection policy and public editor bake APIs remain C++ responsibilities. Native startup fallback is a disjoint tree, not a second parent for mounted ports.

## Required focused evidence

- Installed authored resource admission and live model/style projection in the production window.
- Physical Slate hit-test routing for safe controls and selection, with no test invoking bake, cook or write-asset actions.
- Compatible reload retains the exact preview/list, selected row and preview state; rejected resources/required-port contracts leave the committed view unchanged.
- Malformed startup provides a usable independent fallback and can recover without duplicate parenting.
- Actual module close, reopen and pre-exit release: stop analysis and its Jolt lease first, explicitly tear down preview targets before the preview world, revoke callbacks and release presentation. Externally retained widgets must be inert.
- Existing four `Ck.Jolt.Cook.Inspector` tests remain compatible siblings. Their source inventory is not a pre-change passing result.

## Verification plan and limits

Use only detached UnrealToolbox with the absolute host project path, incremental Editor build and a fresh serial real-RHI focused run. Allow one discovery boot only if needed to include newly compiled test registrations. Do not claim execution until the final artifact is inspected and recorded here.

This Editor slice does not establish actual asset baking, packaged loading, browser pixel parity, gamepad/two-user ownership, representative performance or the all-debugger lifecycle matrix. Packaging, including Audio packaged-resource proof, remains explicitly deferred. Audio collector/Radar/production-host teardown and the separate SM debugger remain open.

## Evidence

The final r6 incremental build succeeded. An explicit sequential discovery then focused-test run used two Editor boots and fresh serial D3D12/SM6 real-RHI. The exact six passing tests were `Ck.Jolt.Cook.Inspector.AnalysisQueue.OnePerTickAndCancel`, `Ck.Jolt.Cook.Inspector.Authored.ModuleLifecycle`, `Ck.Jolt.Cook.Inspector.Authored.Presentation`, `Ck.Jolt.Cook.Inspector.Policy.RepairableActionsExcludeUnsafeRows`, `Ck.Jolt.Cook.Inspector.Preview.RendersValueOnlyAuditTriangles`, and `Ck.Jolt.Cook.Inspector.Window.Constructs`: 6/6 passed, zero failed/skipped/contaminated, 31s, exit 0. `Saved/Logs/Yoga-JoltBake-Authored-Gate-20260915-r6.log` SHA256 `BB1C55E05798E0618D54EE0108A79277E84293D5C26ADBA1032A40FCD70BC3F1`; raw `Saved/Logs/Yoga-JoltBake-Authored-r6-Editor.log` SHA256 `B54C6B68081FC9B1D9BF288BF98F18056E48DB037C02CEFBD279A8940EB74FB1`. The audit found no relevant ensure, parser, script, fatal or error; inherited missing-development-asset/CVar warnings and a Chromium USB diagnostic remain unrelated.

The physical Analyze All fixture uses 128 default unresolved `AssetData` records and asserts `GetAsset() == nullptr`, so it does not bake, cook, or write assets. It verifies public module close and pre-exit while analysis is active release the queue, state and Jolt lease before preview teardown; externally held previews become inert. Presentation additionally covers installed admission, live model/style projection, physical safe controls and selection, compatible/rejected reload atomicity, independent malformed-startup fallback/recovery, and held-control revocation.

Attempts are historical diagnostic evidence, not additional acceptance: r1 compile errors (0 boots); r2 rejected forbidden region-root sizing (4/6, 2 boots), corrected by moving sizing under unstyled roots; r3-r4 exposed the clipped WouldFail control, unknown legend CSS class and the intentionally disabled released button (5/6 each, three boots total). Before r5, the fixture scrolled the actual ancestor before physical hit-testing and checked released-button rejection, and the unknown class was removed. r5 passed 6/6 (2 boots) but lacked the active-analysis assertion; r6 is the final 6/6 run (2 boots), nine boots total.
