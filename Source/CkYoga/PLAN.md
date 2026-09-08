# Yoga Slate campaign plan

Written: 2026-09-07. Expanded pipeline and test-app campaign approved; moving status is recorded only in [PROGRESS.md](PROGRESS.md). Historical Gate 0–2b contracts describe completed slices, not the expanded completion boundary.
Retire on feature delivery and replace with permanent module documentation.

## Recommendation: Texture Debugger, Texture Health table

Use `Plugins/CkGameplayDebugger/Source/CkTextureDebugger/Public/CkTextureDebugger/Window/SCkTextureDebugger_TextureHealthTable.cpp` as the first production surface.

The current widget has a 64/36 native splitter, dual search controls and row count, a virtualized six-column `SListView`, selection/context actions, a native Clear button, a texture preview, and wrapped scrollable detail text. `Set_Snapshot` already separates collection from presentation. Its existing `Private/Tests/CkTextureDebugger_Ux.spec.cpp` supplies a production-path test entry point.

Retain snapshot collection, row identity, texture preview ownership, native splitter interaction, table header resizing, commands, and selection reconciliation. Replace only the chosen layout containers. Yoga should arrange native widget islands; it should not own texture snapshots or produce one node per offscreen list row.

EQS is a reasonable second stress case, but has less text/control variety. Crowd adds viewport, picking, overlays, and simulation dependencies that would obscure layout failures in a first prototype.

## Architecture

| Layer | Responsibility | Dependency boundary |
| --- | --- | --- |
| CkYoga | Unmodified native Yoga and isolated Unreal build rules | Runtime; Core only; no Slate, ECS, editor, BP, or AS interface |
| CkSlateLayout | Retained native panel/slots, Yoga ownership, constrained measurement, invalidation, diagnostics | Runtime; Slate/SlateCore and CkYoga; use CkCore for CK validation as needed |
| Debugger consumer | Existing live snapshots, selection, controls, styling, preview lifetime | CkGameplayDebugger consumes the generic panel; foundation must not depend on debugger types |
| Later authoring layer | Typed layout/style declarations, bindings/actions, reload, source diagnostics | Design after native measurement and identity contracts are proven |

Yoga configuration and node ownership must be explicit: nodes die before their configuration, removed children are detached/freed once, and configuration is fixed before constructing a tree. The adapter validates finite dimensions and tree membership before calling Yoga. Yoga's fatal assertion path is not a recoverable public validation API.

## Phases and observable exits

The expanded scope supersedes the old prototype-only consumer boundary above. Native islands are an intermediate migration mechanism; only graph layouts remain exempt at final acceptance. The next contract is [Plan/Gate_02c_TestApp.md](Plan/Gate_02c_TestApp.md).

| Gate | Scope | Exit observation |
| --- | --- | --- |
| 0: Dependency | Vendor, module registration, independent consumer tests | Native public API compiles/links across modules and focused tests pass in the selected host |
| 1: Native prototype | Generic panel + one Texture Health layout path | Real actions and list behavior survive resizing; wrapped text settles correctly; no state loss on value updates |
| 2: Authoring and iteration | Small typed layout/style vocabulary and live preview | Layout/style edits appear without a C++ build, preserve identity, and reject invalid updates without partial application |
| 2c: Reference and coverage | Interactive browser reference, debugger inventory, executable feature matrix | Reference interactions work; every debugger module and required capability is accounted for, with absent implementation explicitly pending |
| 2d: Shared contracts and native test app | Registry, typed bindings, components, identity, production-rendered resource inspector | Native test app loads real authored files; control behavior and invalid/reload/lifetime paths pass focused automation |
| 2e: Complete Texture Health | Declarative splitter, virtualized table, cell templates and shared components | Bespoke non-graph layout removed; production selection, filtering, preview, context actions and state survive |
| 2f: All debugger layouts | Contrasting tree/form/menu/tab debuggers, then remaining inventory | Every non-graph presentation uses shared authoring; per-module behavior and visual evidence recorded |
| 3: Game vertical | One representative menu using the same primitives | Packaged Development run proves runtime dependency graph, controller navigation, scaling, localized text, and teardown |
| 4: Completion | Agreed visual polish, profiling, docs, upgrade checks, final rebase | Full feature acceptance criteria in PROMPT.md satisfied; only then prepare delivery for authorization |

Gate 1 is specified in [Plan/Gate_01_Prototype.md](Plan/Gate_01_Prototype.md). Gate 2 is specified in [Plan/Gate_02_Authoring.md](Plan/Gate_02_Authoring.md). Game integration remains Gate 3. Shared tree contracts are in [Plan/Gate_02f_Trees.md](Plan/Gate_02f_Trees.md); the following form/event contracts are in [Plan/Gate_02g_Forms.md](Plan/Gate_02g_Forms.md). Slider interaction and capture retention are specified in [Plan/Gate_02h_Slider.md](Plan/Gate_02h_Slider.md). Current execution evidence remains in PROGRESS.md.

## Integration policy

Use the repository's compiled third-party precedent in `CkThirdParty` while isolating Yoga in a dedicated module. Preserve source bytes and license, record tag/commit and source hashes, and keep Unreal build policy outside upstream files. Do not patch upstream for layout quirks before reproducing them with an isolated case.

At each phase boundary: inspect dirt and shared checkout ownership; fetch `origin/dev` for all Ck plugins; checkpoint authorized campaign changes; rebase in place; inspect semantic changes; run the gate appropriate to the rebased result. Do not continually rebase during a running build. No automatic scheduling or dev publication is part of this plan.

## Principal risks

- **Constrained text measurement:** Slate's cached desired size is not sufficient evidence for width-dependent wrapping. Gate 1 must establish a consistent constrained measurement path before broad conversion.
- **Invalidation:** DPI, font/text, padding, child order, visibility, and allotted size changes require well-defined dirty propagation. Do not rebuild the entire widget tree per refresh or solve twice through circular measurement.
- **Overflow and interaction:** preserve native clipping, scrolling, splitter capture, and focus. Define minimum viewport behavior; at very small sizes use deliberate scrolling or a defined stacked layout instead of overlap.
- **Cross-platform support:** the module follows the plugin's Win64/Mac/Linux declarations, but this host only proves Win64. Other platforms and packaged configurations require actual builds later.
- **Scope:** native dependency integration provides no AS/BP authoring API and no screenshot parity. These are later outcomes, not implied by Gate 0.

## Next shared structural capability

Authored tabs follow the typed select callback acceptance increment. See [Plan/Gate_02i_Tabs.md](Plan/Gate_02i_Tabs.md) for stable-key selection, retained panels, input/focus/popup ownership and the Resource Inspector Overview/Properties consumer. Tabs are implemented with focused evidence; remaining acceptance is tracked in PROGRESS.md. Shared authored command menus follow under [Plan/Gate_02j_Menus.md](Plan/Gate_02j_Menus.md), targeting Resource Inspector and the remaining Texture Health/Scene Audit native menu builders.
