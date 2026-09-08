# Gate 02d collections: next implementation contract

Status: design for the next slice; no implementation or passing evidence implied.

## Ownership and authoring boundary

CkSlateLayout owns the reusable typed collection and virtualized table adapter.
Consumers supply typed data fields, stable record keys and behavior. Markup owns
columns, header labels, widths, alignment and cell templates. Do not move the
Texture Health column layout into a C++ table schema and call it authored.
Ordinary text/image/status cell composition uses the production template renderer.
Custom painted cells use registered controls with row-scoped typed bindings.

The reference for pointer reconciliation is Texture Health Rebuild_Rows in
CkGameplayDebugger/Source/CkTextureDebugger/Public/CkTextureDebugger/Window/
SCkTextureDebugger_TextureHealthTable.cpp. Its FRowKey includes component,
material, texture object/path identity and material slot; a texture name alone
cannot represent a row. The adapter must preserve that distinction.

## State and failure contracts

- A collection snapshot contains unique nonempty keys and schema-valid typed
  values. Validate the entire snapshot before changing any accepted record.
  Duplicate keys are a failure, not implicit aggregation; Texture Health's exact
  duplicate aggregation stays in its domain adapter.
- Preserve the shared row object for every surviving key. Update its values in
  place only after validation. Refresh the Slate list only when visible membership
  or ordering changes. Sort ties use a stable total key order.
- Keep accepted collection data separate from visible sorted/filtered pointers.
  For Texture Health, filtering out the selected key clears selection, matching
  its existing contract. The general binding must declare hidden-selection policy
  rather than silently selecting a neighboring row.
- User selection emits one keyed event. Programmatic reconciliation uses Direct
  and does not echo the user's event. Snapshot refresh during held mouse input
  must preserve the selected row and avoid deferred-selection snapback.
- Cell getters hold weak row references. Consumer navigation/preview ownership
  stays outside the generic collection; the selected texture strong root belongs
  to Texture Health and is released on selection loss/teardown.
- Collection publication and markup reload are separate atomic operations. Neither
  can expose half-updated record values or partially staged authored columns.
  Invalid markup leaves the accepted table, selection and scroll position usable.

## Virtualization and editing

Instantiate authored cells only for generated visible Slate rows, not for every
record. A 10000-record fixture must demonstrate bounded live widget count while
scrolling and updating/removing visible records. Table row/cell expansion needs its
own bounded template budget; do not charge all records against the document's
512-node limit or bypass limits with unrestricted cell recursion.

Do not assume virtualized cell widgets are permanent. Editable cell drafts must be
owned by stable record key outside a recycled widget; focus and edit commit/cancel
need explicit behavior on recycling, filtering and record removal. The first table
slice may provide read-only live cells, but editable cells remain required for the
full campaign. Stateless-only custom cells are not the final extension contract.

## Focused acceptance

Use the production table and authored cells with 0,1,12,1000,10000 records. Cover
stable-key reorder, equal sort values, held-mouse refresh, unchanged refresh without
row reconstruction, hidden selection, duplicate/type-invalid snapshot rejection,
row resource release, reload rejection and viewport-bounded cell generation.
Reuse the existing Texture Health held-mouse regression after migrating its table.
No hand-built native inventory island qualifies as the resource-inspector result.
## D2a collection model checkpoint

Implemented FCkUiCollection/FCkUiRecord in CkSlateLayout with focused tests in
Test_UiCollection.cpp. Current evidence belongs in PROGRESS.md. This completes the
data model only; it does not satisfy the authored table or virtualization gate.

## D2b table integration constraints from current engine source

Keep one SListView and its original ListItemsSource array. SetItemsSource clears
selection/generated widgets, and RebuildList clears generators. Use in-place
stable-key records and RequestListRefresh only for visible membership/order changes.
Rows generate during Tick/ReGenerateItems, not Construct; offscreen WidgetFromItem
is null. Tests must tick and arrange the production table before asserting row
widgets or virtualization.

SHeaderRow insert/remove/clear operations broadcast OnColumnsChanged synchronously.
RefreshColumns does not broadcast. SMultiColumnTableRow caches cell widgets by
ColumnId, so retaining IDs alone does not update an authored cell's inner layout.
The adapter must own per-column cell view/port state and stage replacement content
before publication. Do not implement reload by ClearColumns/RebuildList and claim
cell focus/state retention. Header changes need an internal guarded commit with
all callbacks/factories prepared beforehand, or a dedicated batched header adapter.
This choice remains part of D2b implementation review, not an accepted shortcut
that rejects structural column edits forever.

### D2b implemented contract awaiting the focused gate

`table` binds a named FCkUiCollection; its `table-column` children own labels,
width styles, optional sort fields and one authored cell subtree each. Text/image
`bind-field` and boolean `visible-field` resolve against required schema fields.
Ordinary global bindings remain available and cannot be overwritten by row-field
aliases. Empty collections receive the same cell-document validation as populated
ones, before any widget publication. Table selection is per-view, with an optional
named keyed selection delegate in the view data bindings.

The initial cells are read-only row/column/text/image/scroll subtrees. Editing,
interactive custom cells, localization-aware numeric formatting and richer
selection reasons remain required follow-on work. Current ESelectInfo callbacks
distinguish input from Direct updates but do not yet distinguish removal from
filtering. These restrictions are phase boundaries, not the completed product.

Author columns and cell trees in resources. Data field schemas stay in C++; header
labels, order, widths and ordinary cell layout do not. Row-scoped references need
schema/type validation before table publication and bounded per-cell expansion
separate from record count. The row renderer must use the same production staging
path as ordinary authored content rather than a second hard-coded Slate renderer.
Selection needs explicit user/programmatic/removal/filter reasons and per-view
state. Do not put global selection/filter/sort into the shared collection itself.
