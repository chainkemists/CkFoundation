# Pipeline test workbench

The browser reference is a visual acceptance target. Resource Inspector remains
the native test host and must load documents through the production CkSlateLayout
parser, registry, and retained view. Do not reproduce its layout in bespoke Slate
or add a test-only renderer. Graph internals remain the agreed native exception.

## Delivery order

1. Create a polished browser reference with navigation, searchable/sortable table,
   resizable inspector, editable forms, tabs, menus, dialogs, repeated cards, and
   registered custom containers. Include narrow, empty, loading, and error states.
2. Translate that design into installed authored resources for Resource Inspector.
   Track unsupported features explicitly and implement them in the shared pipeline.
   Browser-only reference scripting is not a dependency of the native renderer.
3. Add a focused component gallery with an inspectable example for every supported
   control, style property, binding kind, and composition feature.
4. Automate input, data mutation, accepted/rejected reload, ownership, and teardown.
   Run visual/geometry checks at representative widths, scales, and text lengths.
5. Keep COVERAGE.md tied to concrete examples and production-path assertions.

## Coverage contract

"100%" means every declared supported capability has an example and appropriate
assertions. It does not mean a large screenshot proves every interaction, all
browser CSS is supported, or line coverage proves correct behavior.

| Area | Required evidence |
| --- | --- |
| Layout and styling | Isolated examples plus workbench geometry at wide/narrow widths and multiple scales; clipping, wrapping and scroll reachability assertions. |
| Controls | Real input and model assertions for enabled, disabled, read-only, validation, focus and selection states. |
| Data and collections | Stable keys, sorting/filtering, empty and large data, live mutation, independent repeated actions and bounded virtualization. |
| Composition | Templates, custom widgets and named slots compose through production APIs; retained identity and scope isolation survive reload. |
| Reload and failure | Valid updates commit together; invalid syntax, bindings, factories and ownership leave accepted UI and state intact. |
| Lifetime | Removal, held stale callbacks, focus/capture loss, owner expiry, reopen and game teardown. |
| Game delivery | Packaged resource loading, local-player input ownership, controller navigation and relevant multiplayer lifecycle checks. |

Existing tests remain necessary: the integrated workbench supplements focused
parser, widget, transaction and lifecycle tests. Full debugger migration and the
game/package/localization/performance campaign gates remain separate obligations.

## Reference translation inventory

The standalone reference is CkTests/Resources/ResourceInspector/Workbench.reference.html.
Automated browser preview was denied by URL policy; browser visual acceptance is
pending. Its browser scripting remains outside the native runtime.

The existing native ResourceInspector.ui.html already authors the three-pane
splitter, category tree/select, search, sortable virtualized table, empty overlay,
Overview/Properties tabs, note validation/lock, menus, dataset controls and pinned
snapshot repeat. Do not rebuild these in the host. Extend the existing production
model/resources and preserve their input/lifetime behavior.

Activity is implemented with a bounded newest-first keyed collection and shared
repeat. Explicit loading/error presentation is implemented and verified. The retained
confirmation dialog is now implemented through the shared dialog registry and is being
validated against the Resource Inspector production host. Editable
resource labels/groups need an explicit sample-model contract; the existing note
form is session data and must not silently become selected-resource data.

The gallery must separately expose layout direction/wrapping/alignment/constraints,
text overflow and localization, images, buttons, search, registered form controls,
scroll/splitter/tabs/menu behavior, typed collections/repeats/tables/trees, templates,
and retained custom widgets with named slots. Each example must connect to a test
and state variants. The realistic workbench alone does not prove this inventory.
## Shared retained dialog implementation and acceptance boundary

The retained in-surface modal container is implemented through the shared registry
with authored `content` and `body` slots, an open bool binding, a dismiss action and
explicit owning Slate-user context. Qualified retained getters use
`container-id/slot-name/child-id` paths so Resource Inspector can access controls
inside `inspector-dialog/content` without exposing or reloading the nested child
view. The registry supplies staged updates, typed bool/action bindings and
`ReleaseTransientInteraction` hooks. Resource Inspector passes the local player's
Slate user and uses the dialog to confirm clearing pinned snapshots.

Keep the dialog inside the owning surface. A global modal SWindow would block
unrelated local-player UI. While open, disable the underlying content subtree,
consume backdrop input within this surface, and scope initial focus/navigation,
Escape/back dismissal and conditional focus restoration to the owning user.
Dialog body remains authored, including its confirmation/cancel buttons. Rejected
reload must preserve an open dialog; accepted reload must preserve compatible retained child
identity and reconcile focus after publication. Removal, hidden ancestry, owner
release and game teardown must release only owned transient interaction.

The native Resource Inspector dialog test exercises routed backdrop blocking,
confirm/cancel behavior, retained pins, rejected reload preservation, accepted
reload identity and a 960x640 capture. Final focused evidence is now 114/114 authoring (R8) and 10/10 Resource Inspector (R5). Owner/foreign-user focus, Tab/DPad/Back, hidden/removal/owner-release and content-open/body-close capture release have native coverage. Full local-player/controller sessions, callback-adversarial teardown, arbitrary custom capture adapters, game/package, accessibility and performance remain open. Next: capability gallery/reference parity and remaining debugger migrations.
