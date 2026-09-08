# Gate 2: file-based native UI authoring

Authorized 2026-09-07: CTO accepted Texture Health and requested the next layer,
then selected HTML/CSS-like files. This gate is implementation, not another proposal.

## Contract

Edit TextureHealth.ui.html or TextureHealth.ui.css and the open table applies the
valid candidate without a C++ build. Layout containers, labels, button actions,
spacing, and shared style tokens are authored externally. Search, count, virtualized
inventory, preview, and details are named native bindings. The existing native
splitter owns pane proportions and remains mounted throughout reload.

The v1 vocabulary is XML-compatible ui/region/row/column/text/button/native markup,
class selectors, and a documented small CSS property set. This is not HTML/CSS
standards compliance. No JavaScript, arbitrary native class construction, selectors
by ancestry, CSS Grid, or general expression evaluator. Runtime module stays free
of editor dependencies; the consumer supplies tokens and actions.

## Transaction and identity

1. Read both source files completely; parse and resolve all styles into a detached
   typed document. Validate ids, schema, bounds, tokens, binding coverage and actions.
2. Stage the complete region trees with empty native placeholders. No live native
   widget is attached or styled during validation/staging.
3. On the Slate/game thread, detach old native placeholders, attach the retained
   native widgets, and replace all region roots. No fallible semantic operations
   remain at commit. Native widget identity, selection, scroll, and preview ownership
   survive. Authored containers/text/buttons may be recreated; their identity is
   not promised in v1. Native ids cannot silently change kind or binding.
4. A rejected candidate retains the accepted document, tree, and revision and exposes
   source/id/property diagnostics. Polling retries when the source pair changes;
   there is no build and no directory watcher dependency.

## Work and verification

- Parser and style resolver in CkSlateLayout; pure automation in CkTests.
  Verify valid markup, class ordering/tokens, unsupported syntax, malformed and
  nonfinite dimensions, source limits, and whole-or-nothing output.
- Generic multi-region FCkUiView stages/reloads trees; verify production native
  binding identity and rejected-update isolation, not only parsed data.
- Texture Health supplies native bindings/actions/style tokens, two region mounts,
  bounded live polling and a local diagnostic row. External files stage as runtime
  dependencies. Verify source edits change actual Slate geometry/text while preserving
  native state, and the authored Clear action calls existing selection behavior.
- One incremental build with focused authoring tests, followed by one TextureDebugger
  test-only RHI capture/interaction invocation. Toolbox has no union-pattern option;
  separate suites require two invocations. Discovery may run separately;
  parallel1 limits test lanes, not total editor processes. No redundant baseline boot:
  immediately preceding Gate1 result is 20/20 and CTO accepted its live appearance.

## Research and reference patterns

- CkPerfLab_SessionCodec: decode whole candidate before assigning output.
- CkEntityDebugOverlay_Resolve: typed descriptors and explicit validation diagnostics.
- CkStyle: semantic color/font/spacing tokens supplied by the debugger consumer.
- SCkFlexBox: existing batched declarative construction, never repeated AddSlot reload.
- No existing file-based style reload service found in the two plugin source trees;
  file polling and staged region application are new infrastructure to test here.

## Entry and delivery

All four Ck plugin origins fetched at entry: Foundation9398637a4, GameplayDebugger1824d7d,
Tests658ae567, Application7ca8b58; all 0 ahead/behind. Branch feature/yoga-slate-layout,
existing E:/Repos/CkPlugins_Other only. Preserve unrelated dirt. No commit/push/merge
to dev in this gate. Packaged game, controller navigation, localization workflow,
retained authored control state, and advanced CSS remain later acceptance boundaries.
