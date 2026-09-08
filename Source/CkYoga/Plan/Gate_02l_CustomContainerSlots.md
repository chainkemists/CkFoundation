# Retained custom containers with authored child slots

Full campaign scope is unchanged. Moving status belongs in PROGRESS.md.

## Production need and neighbors

Style Lab's SCkStyleLab_ControlsPane::Build_ProfileControls uses the shared
SCkDebug_InspectorPanel, including style-axis treatment and retained expansion.
Its profile body can use typed records and item actions, but replacing its shell
with a generic column would lose existing behavior. Register the shared inspector
panel as a retained custom container and author its body through child slots.

Implementation neighbors are FCkUiView::PrepareRepeat, FStagedDocument::Nested,
FNestedUpdate, CaptureCommit/PublishConfiguration/ReconcileInteractions and the
retained custom registration/prepared update contract. Do not introduce a second
renderer or serialize child nodes back to markup.

## Contract

- A retained registration may declare named required/optional single-root slots.
  Stateless registrations cannot declare slots in this increment.
- A custom element contains `<slot name="body">one authored root</slot>`.
  Slot declarations are parser-only; they create no layout widget or public ID.
- Validate schema names, duplicate declarations, unknown/missing/empty slots,
  malformed wrapper attributes and all child nodes before any factory runs.
  Templates carry the same typed slot definitions and lexical ID rules.
- The view owns one persistent mount per custom instance and declared slot name.
  A trusted retained factory receives these mounts as opaque SWidget references,
  not raw child trees. It attaches each exactly once beneath its detached output.
  On reload it must preserve those mount identities and ownership edges.
- Candidate child documents join FStagedDocument::Nested, sharing the existing
  multi-view transaction. Staging never replaces content in a mounted slot.
  Publication commits parent/child configuration and mounts the accepted content
  before focus/capture reconciliation; all consumer dispatch remains gated.
- Use explicit ownership edges in custom alias validation. Exclude only the
  declared child mount subtree owned by this instance, never arbitrary descendants.
  Duplicate/shared/missing/reparented mounts fail atomically.
- Retain child views by parent id/tag/state-key and slot name. Descendants inherit
  bindings/registry but dispatch through the parent scope. An absent optional slot
  publishes empty content and retires its previous child interactions.
- Capture retired child interactions before detachment. Focus belongs to the
  deepest participating child, using the same nested reconciliation as repeats.
- The custom container owns layout/measurement of its internal chrome and mount.
  Slot mount metadata forwards the authored child's constraints, and the registered
  inspector adapter includes its padding/header/collapse state in measurement.

## Ordered implementation and evidence

1. Registry schema and typed parser transport. Test positive/template expansion,
   required/optional slots, duplicate/unknown/missing/empty wrappers, invalid schema
   registration and declaration/depth limits. Rejection must precede factories.
2. Nested staging, owned mount edges, atomic publication and retirement. Native
   tests verify child field/control events, live data, retained search drafts and
   selection, rejected sibling/factory updates, external alias rejection, parent
   removal, owner expiry, focus/capture and reentry suppression.
3. Register SCkDebug_InspectorPanel in CkDebuggerCommon with its existing visual
   treatment and collapse behavior. Migrate Style Lab's curated-profile section
   including copy, profile commands, active indication, blurbs and current label.
   Preserve application of profiles and preview rebuild notifications.
4. Add a composed example to Resource Inspector/gallery. Verify installed-resource
   reload and wide/narrow/scale geometry. Gate shared tests, the real consumer,
   Resource Inspector and Texture compatibility with fresh runtime evidence.

Named child slots do not close cross-file libraries, deeper nested collections,
all debugger migrations, game/package/controller/localization or performance gates.
No dev merge/publication is authorized by completion of this slice.

## Inspector adapter implementation boundary

Use a retained debug-inspector registration in CkDebug_UiRegistry with a required
localized title binding and required body slot. The component owns one shared
SCkDebug_InspectorPanel, validates stable body mount identity in PrepareReload,
and returns a commit-only title-binding update. Reload must not call Set_Expanded:
the existing panel owns the user's collapse state.

The panel currently stores Title as FText. Promote it to a TAttribute<FText> with
a rebinding setter and bind header text/icon meaning without converting to FString.
Keep the existing style-axis surface, header padding, icon and chevron behavior.
Add constrained measurement to the panel itself: actual header desired size plus
expanded authored body metadata, subtracting header height when forwarding final
arrangement. Collapsed measurement is header-only. Follow SCkUiTabs' constraints
and metadata forwarding rather than estimating header height in the adapter.

Widget-specific registry/measurement tests belong in CkDebug_UiRegistry.spec.cpp:
localized live title, required body rejection, retained collapse and child identity,
wide/narrow wrapped body geometry and collapsed height. Generic slot/capture tests
stay in CkTests. The current table/tree cell validator explicitly permits stateless
read-only custom cells only (SCkUiSurface ValidateCell); retained slot containers in
cells remain a declared capability gap, not a proven supported path.
