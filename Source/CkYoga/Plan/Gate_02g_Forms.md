# Shared authored forms

Implementation follows the responsive toolbar checkpoint. Moving evidence belongs in ../PROGRESS.md. This gate extends the full debugger pipeline mission; it does not replace remaining tree, menu, tabs, custom-control, game and debugger migration acceptance.

## Existing contracts to extend

- CkUiWidgetRegistry.h: typed custom property schema and resolved widget arguments; immutable registrations; ICkUiRetainedWidget and prepared non-failing Commit.
- SCkUiSurface.h/.cpp: live text attributes and FOnTextChanged used by built-in search, complete prevalidation before factories, retained state compatibility, staged publication.
- Test_UiRetainedRegistry.cpp: real editable draft/focus retention and rejected candidate behavior. Use this production-path fixture as the neighbor rather than adding a second form renderer.
- CkResourceInspectorModel.cpp: weak model-backed bindings and command actions; authored controls belong in ResourceInspector.ui.html/.css.

## First increment: typed edit events and retained text input

Add a value-carrying text change event property to the shared registry, resolved against existing TextChanged delegates. Do not encode a changed value in an FSimpleDelegate or make controls look up a consumer by name. Validate required event bindings and supported property kinds before constructing any candidate widget. Row field scopes remain read-only until editable collection semantics are implemented explicitly.

Register a generic retained text-input control in CkSlateLayout through the same public registry used for future custom widgets. Registration failure must reject the entire requested standard-control set. It must remain usable in runtime/game builds and must not depend on the debugger plugin or FAppStyle.

Define the editor contract before implementation: authoritative model text, local draft, active editing, external model changes, Enter/focus-loss commit, Escape cancellation, validation failure, and compatible/rejected document reload. A successful document reload changes presentation/callback configuration without writing the model or resetting a draft. Changing a binding's state owner under a surviving id must reject or explicitly recreate by a new id, never silently retarget unfinished input. Avoid SetText polling that replaces the caret or selection every frame.

Initial construction emits no edit. Only native user input emits the typed change event. Model callbacks capture owners weakly and can reject or normalize input without recursive edit echoes. Commit/cancel semantics require an explicit typed event contract; do not falsely claim them through the change-only callback. Removed controls detach and release bindings without a synthetic commit to a dead owner. Programmatic model changes must be observable when not editing; behavior while a draft is active must be deterministic and tested.

## Following increments

Use the same shared registry and event contracts for checkbox/switch, numeric input/slider, and typed option selection. Cover enabled/read-only state, validation text, labels/tooltips, keyboard navigation and accessible naming. Reusable authored form components and tabs host the controls. Collection editing requires stable record keys, rejection feedback and selection/input coexistence before it is allowed in table/tree cells.

## Verification and consumer

1. Parser/schema and missing/mistyped event cases reject atomically, with zero factories or callbacks where prevalidation is possible.
2. Production registered editor mounted in a real Slate window: initial value, native typing/change delivery, model-driven update, no recursive writes, focus/caret/draft across accepted reload, late rejection leaves the live editor unchanged, removal/readdition and owner release.
3. Add a real Resource Inspector detail-edit scenario using the shared control and model, without a bespoke Slate form island. Preserve category/query/selection behavior and check narrow/wide captures.
4. Verify keyboard commit/cancel and invalid-input behavior only when their actual typed event contracts are implemented. Track remaining contracts explicitly rather than labeling one editor as complete form coverage.

## Boolean control increment

The shared `checkbox` uses a live boolean `value-bind` and a required typed
`changed` callback carrying the proposed boolean. The model remains authoritative:
rejecting the callback leaves the displayed state unchanged, and external model
updates must never echo a change event. `label-bind`, `enabled-bind` and
`read-only-bind` are live properties. This first control is two-state; mixed-state
selection requires a separate explicit contract rather than coercing it to bool.

Retain the native checkbox across compatible reloads, reject a surviving id's
value-binding retarget, and suppress dispatch during staging/publication and after
owner release. Configuration publication must not synthesize toggles. Readonly
collection cells continue to reject edit events.

Resource Inspector consumes the control as Lock note, binding the same model
boolean to the session-note editor's read-only property. Acceptance exercises
native keyboard toggle, rejected model updates, external updates, retained
identity/focus, disabled/readonly input, and the installed app resources. Source
implementation and planned tests are not runtime evidence until the focused gate.


## Numeric entry increment

Use shared retained text input for drafts, native focus, Escape, validation display,
and compatible reload. Numeric entry adapts this common editor; it must not grow a
second implementation of text editing. Add public NumberChanged(float) and
NumberCommitted(float, ETextCommit::Type) events through the same registry,
prevalidation and staged-publication guards. Nonfinite event payloads never reach
consumers.

The initial `number-input` has required `value-bind` and `committed`, optional
`changed`, literal `kind` (float or integer), optional finite min/max, and shared
placeholder/enabled/read-only/error properties. Parsing consumes the entire trimmed
numeric string; empty, junk, NaN, infinity and overflow reject without callbacks.
Changed events propose valid raw input; commits normalize against bounds and
integer rounding. Integer ranges must contain a representable integer, with
rounding unable to escape the accepted interval. The model remains authoritative.

Preserve drafts while external values change and across compatible reload. A
surviving id cannot change its value binding or numeric kind. Publication only
swaps prepared configuration; it must not dispatch edits or call SetText. Numeric
entry uses the existing float transport; wider precision, mixed/optional values,
localized numeric input and slider/step interaction remain explicit required
follow-up contracts where debugger/game consumers need them. This is not a claim
that every numeric Slate widget is covered.

Resource Inspector adds an editable Resource count next to the preset scenario
controls. The installed-resource test must type a count, keep the live collection
unchanged before commit, reload with the draft active, commit to the actual typed
collection, reject garbage, clamp a negative count, and reflect an external model
update. Retain prior checkbox and text-entry tests in the final app lane.

## Typed option selection: next implementation contract

Use a retained select control and the existing FCkUiCollection model. Option identity
is the record FString Key; display labels are required Text fields. Never convert
localized display text into identity. Introduce or reuse typed key/string binding
and selection-event transport through the custom registry, alongside a collection
binding. Schema and binding validation must finish before any native factory runs.

The model remains authoritative. Native user selection proposes one key; after the
callback returns, re-read the model and resolve the current collection record.
Rejected edits restore model selection without an event echo. Collection changes
reuse option identity by key; labels may change independently. Missing selected
keys display an explicit unselected state without mutating the model. A surviving
control cannot silently retarget its value or options binding during reload.

Own native option storage with the component; SComboBox borrows its source pointer.
Handle externally retained native widgets safely on component release. Weak change
listeners must detach on release. Options and selected model key must not be cached
across consumer callbacks. Programmatic selection is silent. Reload publication
cannot emit callbacks, mutate focus, or close/open popup windows; establish an
explicit open-popup reload contract before allowing it. Disabled/read-only input
must neither emit proposals nor leave a divergent native selection.

Tests must cover real native keyboard/pointer selection, rejected model edits,
label-only updates with stable keys, removal/reordering while open, compatible
reload identity/focus, reentrant model/view change, and teardown with a held native
widget. Verify both accepted and rejected declarations atomically. Resource
Inspector will consume the shared control for a real data filter; no bespoke Slate
form island. Browser reference parity and narrow/wide native captures remain part
of consumer acceptance. This contract is planned, not implemented or verified.
Checkpoint: typed select and Resource Inspector category consumption above now pass70 authoring +5 app tests. The initial control uses shared observable option storage and rejects open-popup reload. Next implement the required popup-preserving reload/ownership contract and verify option mutation while open, explicit pointer selection and callback reentry. These remain unfinished; see PROGRESS.md.

Current select checkpoint:72 authoring +5 app tests pass with open-popup retention in both hosting modes. Callback-reentrant reload/removal/destruction coverage is the next focused increment. The historical70-test open-rejection checkpoint above is superseded. Authored tabs follow under Gate_02i_Tabs.md; remaining controller, accessibility, localized numeric and mixed-state requirements are still open.
