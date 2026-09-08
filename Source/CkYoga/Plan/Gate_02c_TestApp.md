# Expanded authoring pipeline and test application

Approved 2026-09-07. Moving status belongs only in ../PROGRESS.md.

## Entry and references

Preserve the existing CkSlateLayout parser and FCkUiView atomic staging contract,
the Texture Health snapshot/selection model, and existing Ck.UiAuthoring tests.
These are the implementation neighbors. The browser is a visual/interaction
reference, never proof of native behavior or a replacement renderer.

## Ordered work and observable verification

1. Design/ResourceInspector.html: build a self-contained resource inspector with
   search, navigation tree, sortable table, selection/details, tabs/forms, splitter,
   native-graph reference, themes, and empty/long-text/large-data scenarios.
   Verify actual browser interactions and narrow/wide visual layout.
2. Inventory CkGameplayDebugger modules and shared widgets. Record source paths and
   concrete capabilities in COVERAGE.md. Separate implemented, tested, and pending.
   Verify each module is represented; explicitly classify custom painting.
3. Add a shared typed widget registry and reusable authored components. Validate
   entire documents before construction callbacks; stage detached candidates before
   committing. Stable IDs retain state; model collection keys retain row identity.
   Verify invalid types, duplicate registrations/IDs, missing bindings, failed late
   nodes, owner destruction and reentrant callbacks cannot publish partial state.
4. Build the native resource-inspector app using the production parser/view and
   installed authored resources. Add controls by capability family, with focused
   production-path tests. No bespoke Slate replica or alternate test renderer.
   Verify input, state retention, virtualization, error recovery, resource ownership,
   clipping, resizing, wrapping, DPI, styles and registered native graph placement.
5. Complete Texture Health through shared table/splitter/component adapters and
   continue all debugger migrations. Verify existing interactions with production
   events plus captures; preserve collection and business logic.
6. Exercise a packaged game surface, keyboard/controller navigation, localized
   text, resource staging and teardown. Measure performance against declared
   budgets. Other platform claims require platform-specific evidence.

## Native host placement

Use CkTests' existing Runtime module for the sample host and deterministic resource
model. CkSlateLayout remains independent of the debugger and the test app. CkTests
already declares direct Slate, SlateCore, Engine, Projects and CkSlateLayout
dependencies. Stage exact authored resource paths as NonUFS in CkTests.Build.cs,
following the existing TextureHealth resource staging pattern.

The runtime host should follow CkGym_Switchboard_Subsystem's local-player lifetime:
resolve the player from the console command's world, attach/remove viewport
content, and release widget/view/ticker references in Deinitialize. Keep UI input
ownership explicit; the switchboard's HitTestInvisible/CkInput navigation is its
own behavior, not a pattern to copy blindly for native Slate controls. An editor
inspection host may mount the same view; it must not introduce editor dependencies
into the shared renderer or be the only path tested.

Place model/view/host sources under CkTests' CkResourceInspector directories and
authored files under Resources/ResourceInspector. Inject deterministic resource
snapshots in tests; do not depend on AssetRegistry discovery or random cooked assets.
The native app's tables, trees, splitters and forms must use shared declarative
adapters. Do not mount bespoke inventory controls to claim this app is complete.

The packaged Development target must actually enable CkTests and stage its files;
being a Runtime module alone is not packaged execution evidence. No Shipping-
mandatory sample window, startup launch, or new Foundation dependency is required.

## Coverage semantics

Coverage means every declared supported feature has positive, malformed-input,
update/reload, and lifecycle evidence where applicable. Interactions between
features need targeted scenarios (for example filtering a selected virtualized
row while editing a field and reloading styles). A line-coverage percentage does
not prove complete behavior. Unsupported declarations reject usefully.

The matrix is the acceptance ledger, not a list generated only from passing tests.
Missing evidence remains pending. Browser screenshots cannot close native gates.

## Constraints

Work in the current checkout only. Preserve unrelated edits. Fetch/rebase all Ck
plugins at frozen phase boundaries, never during active builds or file edits.
Use incremental UnrealToolbox gates, state planned boots before launch, inspect
full fresh logs, and retain evidence paths. No dev merge or publication until
the full campaign is complete and delivery is authorized.
