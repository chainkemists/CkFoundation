# Gate 02k: repeated authored subtrees

Status: implementation in progress. This gate does not complete the full pipeline campaign.

## Production target

Migrate Surface & Lighting's per-material-slot variable-height cards through the
same document renderer used by Resource Inspector and the other Texture pages.
Preserve each card's independent expansion state, stable identity, ordering,
material path, status, material/lighting facts, caveat and empty-context reasons.
Do not substitute a fixed-height table for the existing card layout.

## Shared contract

- `<repeat id="slots" bind="slots">` owns exactly one authored item subtree.
  Its field references are checked against the collection schema even when empty.
- Retain a child view only for the same collection record identity and key.
  Removal followed by reinsertion of a key creates a new interaction scope.
- Item ids are local to the child view. Collection order determines visual order.
  Ordinary field-value changes do not reconstruct widgets or reset local drafts.
- Every schema field has a typed internal alias before child staging. A compatible
  template reload may change which field is referenced without mutating live data
  during preparation. Internal aliases must not collide with authored bindings.
- Explicit item actions receive the stable key. All outbound child events also
  consult a live inherited dispatch gate. It checks parent ownership, transaction
  state, configuration generation and original record identity. Passive reads are
  independent of event eligibility. Externally retained removed widgets are inert.
- Stage every child update before publishing any child or parent configuration.
  Preparation failure leaves mounted content, revision and retained state intact.
  Do not implement this by calling public TryReload on children sequentially.
- FCkUiView::Commit currently transfers focus/capture through Slate callbacks.
  It cannot simply become an ICkUiPreparedWidgetUpdate::Commit: that interface
  prohibits focus changes and reentry. Separate detached configuration publication
  from interaction reconciliation, and reconcile only after the whole parent
  transaction is published while inherited dispatch remains suppressed.
- Use constraint-aware Yoga layout for variable-height children. Reconcile slots
  only for structural changes; preserve child views and their measurement metadata.
- Nested collection/repeat scope is rejected until its ownership and binding
  semantics have an explicit implementation and tests; never silently flatten it.

## Implementation order and evidence

1. Inherited view dispatch eligibility in SCkUiSurface, default-enabled for existing
   consumers. Verify native button/form callbacks, custom widget dispatch, live
   disable/enable, reload rejection, stale callbacks and owner release.
2. Internal multi-view preparation/publication boundary plus SCkUiRepeat. Verify
   failure of a later child leaves every earlier mounted child unchanged; include
   focus/capture callbacks that attempt reentry during reconciliation.
3. Parser/schema/field transport and keyed item actions. Verify malformed and empty
   collections, incompatible field kinds, removal/reinsertion, reordering and
   item action identity against the production collection and renderer.
4. Surface & Lighting migration and Resource Inspector/gallery example. Verify
   independent collapse, live facts, empty/error states and narrow/DPI captures.

Use existing SCkUiTable prepared configuration and context-menu record identity
checks as reference implementations. Extend their contracts for interactive
variable-height item subtrees; table cells intentionally remain read-only.

## Acceptance boundary

Focused shared authoring tests and the affected debugger/app compatibility tests
must pass against the built source. Browser presentation is a visual reference,
not proof of native behavior. Full debugger, package, controller, localization and
performance acceptance remains in COVERAGE.md. No merge to origin/dev in this gate.

## Multi-view transaction increment

`FCkUiView::TryReloadBatch` accepts an immutable snapshot of distinct view/document
requests. It locks every participant against dispatch and reload before parsing,
validates every document before any factory, stages every document before any
publication, captures old interaction state for all views, publishes all staged
configurations, then reconciles focus/capture for all views. Failure before
publication leaves every prior mount and revision intact. Temporary and previous
state is destroyed before releasing the transaction guards.

The ordinary single-document TryReload uses this same path. Cross-view widget
aliasing is rejected before publication, including shared native content and
factory outputs. This increment handles disjoint participating views; it does not
yet claim nested repeat ownership or hierarchical focus reconciliation. The
repeater will integrate its internally staged typed child documents with these
phases instead of serializing item nodes back into markup or calling TryReload
one child at a time.

Next integration boundary: nested repeat updates must enroll child views in one shared transaction with stable pending-item storage. Parent staging must not invoke child TryReloadBatch/Commit independently. Record the explicit parent-to-child mount edges: ownership validation may exclude only those registered child roots from the parent traversal, and focus reconciliation must assign a focused path to its deepest participating view. Arbitrary cross-view aliases remain errors. The current disjoint batch API intentionally does not claim this nested behavior.

## Repeat implementation checkpoint

The nested child staging described above is now implemented. Repeat items are
prepared as typed child documents in the parent transaction, with explicit mount
ownership exclusions and deepest participating focus attribution. Collection
refresh preserves record identities; removed focus/capture is snapshotted before
detachment and reconciled after publication. Five repeat tests pass in the
100/100 authoring suite (UiRepeat-R3-Editor.log); Texture 28/28 and Resource
Inspector 6/6 compatibility also pass. See PROGRESS.md for exact logs and failed
fixture history. Surface & Lighting and the gallery consumer remain to migrate.
Geometry, nested capture/reentry, collection-switch and parent-removal focus
coverage remain open; this checkpoint does not close the gate or campaign.
## Surface & Lighting checkpoint

The production migration now exists in SurfaceLighting.ui.html/css; native slot
builders were removed. Shared authoring passes 102/102, Texture passes 30/30 and
Resource Inspector passes 6/6. See PROGRESS.md for logs and failed-attempt evidence.
Two populated native captures (narrow 1.5x and wide 1x) were inspected. The collector
fixture covers independent card state, live shadow facts, locale identity and
reload rejection. Shared tests cover collection replacement, parent-removal focus,
wrapping, non-overlap, collapse and scaled constraint measurement.

Next: the Resource Inspector/gallery repeat consumer and nested capture/reentry
proof. Nested collection support and full campaign acceptance remain open.
## Pinned snapshots and repeated capture checkpoint

Resource Inspector pinned snapshots use the shared repeat adapter with independent
collapse/removal, duplicate pin updates, accepted/rejected reload and stale-action
checks. ResourceInspector-PinnedSnapshots-R2-Editor.log passes7/7; shared
PinnedSnapshots-R3-Editor.log passes103/103; Texture-PinnedSnapshots-Editor.log
passes30/30. Actual runtime counts match and all error/ensure/fatal/AngelScript-warning
scans are zero. Native wide/narrow captures were inspected, with lower narrow cards
below the scroll fold and requiring additional visual coverage.

Parent repeat removal originally left an externally held child captor active.
Retired child interactions are now captured before detachment and reconciled after
publication. The native regression verifies wrapper transfer, reentry suppression,
collection removal/reinsertion and parent removal. Popup retirement and deeper
nested collections remain open. Authored button labels currently use STextBlock
without width-aware wrapping; add shared support and focused geometry evidence.
## Button wrapping and tabs scroll measurement checkpoint

Ordinary buttons now accept wrap/overflow-wrap/overflow styles and measure native
padding plus constrained SCkFlexText labels. Tabs measure wrapped headers and only
the selected body under the available width, allowing outer vertical scrolls to
reach the full content. No model mutation occurs during measurement.
TabsMeasurement-R2-Editor.log105/105, ResourceInspector-TabsMeasurement-Editor.log7/7,
and Texture-TabsMeasurement-R2-Editor.log30/30 pass with matching actual counts and
zero error/ensure/fatal/AngelScript-warning matches. Inspected Pinned_Narrow_640x480.png
shows complete final snapshot at scroll end; native action bounds are asserted.
This supersedes the earlier button-wrap and below-fold gaps, not the full campaign.