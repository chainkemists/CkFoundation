# Authored tabs and retained panels

Status: implemented in the shared parser/view/native tabs and Resource Inspector. Focused evidence now includes responsive headers, hidden-panel popup/capture release, and virtual-user navigation/disabled focus/reload. Remaining acceptance below is not complete; current gate evidence stays in ../PROGRESS.md.

## Consumer and scope

Resource Inspector's browser reference has tab buttons and panels. Add the same capability to the native authored detail pane: Overview for resource facts and Properties for the existing session-note form. Keep the existing data model, collection selection and editing controls. This is an in-view tab set; Unreal dockable editor tabs remain host ownership.

## Shared contract

Use explicit `tabs` and `tab` structural nodes so panels contain ordinary authored layout and registered controls. A tabs node has a stable id, required `value-bind` using existing FString transport, and `changed` proposing a stable tab key. Each immediate tab has a unique nonempty key, localized label (literal or text binding), optional enabled binding, and normal authored children. Reject misplaced tab nodes, duplicate keys, missing bindings and malformed descendants before factories run. Do not transport identity in FText or introduce bespoke consumer Slate layouts.

The model owns the active key. User activation proposes one key; callbacks may reject, normalize, replace data, reload or remove the whole view. Re-read the authoritative key after callbacks. Invalid or disabled selected keys show no active panel and emit no synthetic selection. Disabled tab headers do not accept activation. External selection changes are silent.

Retain every panel's child widgets across tab switches: inactive panels are collapsed, excluded from layout, hit testing and keyboard traversal. Panel scroll state survives switching and compatible reload. Compatible reload preserves an active editor draft; switching away from a focused editor follows that control's existing focus-loss commit/cancel policy, preserving its resulting model state rather than silently inventing a second draft policy. Preserve native header identities by tab key. Removal releases removed panel bindings and popup/capture ownership. Reordering must not reset surviving panel state or accidentally select another key by index.

Keyboard navigation moves header focus among enabled tabs with Left/Right and Home/End; Enter/Space activates the focused header. Navigation alone does not change selection. Mouse click activates directly. Document controller bindings explicitly and verify them on the native production path. Tab headers expose localized names and selected state through Slate accessibility.

When a selected panel becomes inactive, focus/capture cannot remain on hidden descendants. Move owned focus to the corresponding enabled header when available, otherwise clear owned focus. Do not disturb focus outside this tab set or another Slate user's unrelated focus. Honor callbacks that redirect focus. Changing tabs while editing must use the existing control's commit/cancel policy; tab logic must not invent form commits. Open owned popups must dismiss when their panel becomes inactive.

## Implementation and gates

Extend the shared parser/prevalidation/staged view and retained-container paths. Reuse standard typed string bindings; do not embed a secondary document renderer in a custom leaf. Native tab-strip styling belongs to the shared style contract, with selectable state available to authored styling. Stage every panel before publication; late failure leaves active key, draft, focus and mounted content intact.

Test real mounted native input, rejected/normalized callbacks, external selection, invalid keys, disabled headers, keyed reorder/removal, inactive clipping/input, multi-user focus, open-popup dismissal, draft and scroll retention, and callback reload/view destruction. Parser tests must assert zero factories on malformed declarations. Run relevant authoring siblings in the same gate, then Resource Inspector installed-resource tests and wide/narrow captures. Packaged/controller/accessibility claims require their own actual evidence; editor tests alone cannot close those requirements.