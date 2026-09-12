# Gate 05: AI debugger authored roster slice

**Status:** accepted bounded slice (2026-09-08). This is not a whole-window migration.

## Scope

Replace only the AI Overview's native `SCkDebug_EntityHealthList` roster with one retained authored selectable table. Keep the existing AI debugger window, Chrome, picker, selected model, overview/stage/evidence/event/topology panes and spatial viewport as native owners. HTML/CSS owns roster column layout, empty/status presentation and responsive scrolling. C++ owns Crowd collection, physical-agent identity, live values, selection routing and lifecycle admission.

## Decision

`[G5-AI-ROSTER-D1]` Use the existing typed `FCkUiCollection` plus virtualized authored table; add no new adapter and no native roster island. Row keys use session/world generation plus the physical Crowd handle entity number/version, never display names or conceptual-owner identity. Native table selection enters the existing entity-selection pipeline through the roster-authorized `Select_EntityImpl(PhysicalHandle, true, true)` path, while public generic selection retains its existing AI-layout admission rule. Conceptual selection, Crowd synchronization, roster refresh and debugger broadcasts remain authoritative.

## Required invariants

- Projection comes only from the production `FCkCrowdDebugger_ViewModel::Get_AllAgents()` snapshot.
- Preserve current roster text, status/health semantics, ordering and physical-to-conceptual mapping. Display names are presentation only.
- Surviving physical keys retain collection records and native table row identity through value refresh and compatible resource reload. Removal is atomic.
- Missing world and zero-agent world remain explicit states. A world/session change invalidates generation and clears handle-bearing maps before detaching the view.
- Selected-key callbacks re-resolve through the current generation and physical-handle map. Held callbacks reject after world/view/window release.
- Stage strip, evidence/topology lists, event log, behavior/drill-down panels, picker and spatial viewport are deferred. Do not claim whole-AI or whole-campaign completion.

## Files

- `Plugins/CkGameplayDebugger/Source/CkAiDebugger/Public/CkAiDebugger/Window/SCkAiDebuggerWindow.{h,cpp}`
- `Plugins/CkGameplayDebugger/Source/CkAiDebugger/CkAiDebugger.Build.cs`
- `Plugins/CkGameplayDebugger/Resources/UI/AiDebuggerRoster.ui.html/.css`
- `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkCrowd/Test_AiDebugger_AuthoredRosterPie.spec.cpp`
- `Plugins/CkTests/Source/CkTests/CkTests.Build.cs`

## Acceptance

1. One-client authority PIE on `/Engine/Maps/Entry` creates one transient fixture owner and two real Crowd-agent children through public EntityLifetime, Transform and CrowdAgent APIs.
2. The mounted real AI window observes both through its production Crowd view model; authored rows carry the physical identities and drive the existing selection route.
3. Live values update without replacing surviving record/row identity; removal changes membership atomically.
4. Compatible and rejected installed-resource reload retain/reject correctly; world/window release makes held selection inert and clears handles before EndPIE.
5. Fresh wide/narrow captures are readable, non-overlapping and scroll-reachable.
6. Final evidence uses one incremental UnrealToolbox build-plus-focused-test invocation, at most two editor starts, followed by actual runtime-log and capture inspection.

## Accepted evidence (R14, 2026-09-08)

The R14 incremental Development Editor build succeeded in 12.02 seconds. Its focused `Ck.AiDebugger.AuthoredRoster.PIE` gate passed 1/1 in 33 seconds with exit 0 and zero failed, skipped, or contaminated tests. Toolbox started exactly two editor processes: the inline-discovery and focused-test lanes. `Saved/Logs/AiDebuggerRoster-R14-Final.log` SHA256 is `E891F1191ADF91AAB234253071B3175CE14B790E559215D53A13C1266F335B10`; the archived focused runtime `Saved/Logs/AiDebuggerRoster-R14-Editor.log` SHA256 is `EC1D55AA4F2BE6F54A72CAF8F4001832DDFEFA8D24AF544D9F3F740F17DC9CEC`; discovery SHA256 is `6031017F972B068B7030C5A53596513B9667E2271BE6C848806ECCBB189051E9`.

The fresh runtime scan found no automation errors, ensures, fatal/critical/unhandled diagnostics, script errors or warnings, `FullReload=true`, or timeouts. Only environmental generator/USB and existing PIE warnings remain. Wide capture SHA256 `752962D29BB426954E94EA2B224D12233A3B6F7F1880ABD4918D2E5831F07609` and narrow capture SHA256 `87D768CC6B698A58864B03A1010CB5D4CBE9175EC4F210EC5B5D2FCE51A2B9D3`, both captured at 23:07:44, were inspected. The scoped roster has two readable names, statuses, and selection at wide width; narrow evidence demonstrates horizontal reach and later context without roster overlap.

R13's test-only direct `FWidgetPath` pointer routing crashed in Slate because the synthetic path carried no virtual-pointer positions. It is superseded by the source-backed generated-row `OnMouseButtonDown` route used by `Test_UiTableInteraction`, matching the instantaneous `SCkUiTable` rows. This accepts authored physical identity, retained row update/removal, selection routing, reload rejection/compatibility, and teardown behavior for the roster slice. Legacy surrounding AI-window shell content clips/overlaps at constrained widths; that independent whole-window responsiveness work is explicitly deferred.

## Stop conditions

- The production view model cannot observe the two real fixture agents.
- Authored selection cannot route through the existing physical-handle `Select_Entity` path.
- The roster requires a new custom/painted adapter or changes current health/status semantics.
- Handle-bearing state survives world invalidation or teardown.

