# Shared slider lifecycle

This is the next control after verified numeric entry. Current evidence lives in
../PROGRESS.md. It is not a replacement for remaining forms, debugger migration,
or game acceptance.

## Public interaction contract

FCkUiNumberInteraction carries Begin, Commit or Cancel, a Pointer/Keyboard/Controller
source, and a finite float value. Pointer includes mouse and touch. The public
NumberInteraction registry event and number-interaction template type transport
this payload. The view rejects malformed phases/sources/nonfinite values, suppresses
events during reload/publication and after release, and disallows edit events in
readonly collection cells. Phase ordering is the control's responsibility; a
transport test cannot prove native slider behavior.

A slider will combine optional NumberChanged previews with required interaction
notification. Begin carries the initial authoritative value; Commit carries the
final proposed value; Cancel carries the initial value so consumers can discard
previews. The committed model remains authoritative after the interaction. A preview
callback that mutates external state owns its rollback when Cancel arrives.

## Native evidence and required adapter design

Selected engine: D:/Repos/UnrealEngineAngelscript_Other. SSlider.cpp mouse-down
emits begin then value change before returning CaptureMouse. Mouse-up returns a
release reply; the end delegate actually comes from OnMouseCaptureLost. Therefore
capture theft and normal mouse release cannot be distinguished by the end delegate.
Touch end emits end BEFORE its final value update and then releases capture, causing
a second end via OnMouseCaptureLost. Do not publish semantic completion from those
base end delegates.

Use a small SSlider subclass and an explicit interaction state. Mark normal release
before invoking the native mouse-up/touch-end handler; finish once on capture loss
after final touch value delivery. Unexpected capture loss and Escape cancel once.
Begin must occur BEFORE the first native value callback, not after base mouse-down
returns. The native begin callback may be used as an internal pointer-session hook;
its end callback must not directly publish completion.

Keep controller lock semantics explicit. Prefer disabling SSlider's private lock
and owning the lock session in the subclass so focus loss can cancel and switching
to pointer input can cancel the previous session before starting another. Accept
starts/finishes a locked controller session; locked navigation updates the local
draft; unlatched keyboard steps each form a complete interaction. Read-only/disabled
state suppresses new changes but must still permit an outstanding Cancel notification
when a preview needs to be undone. View publication/release still suppresses all
consumer events.

Use a fixed native normalized range with bound value and step attributes so prepared
publication can swap only configuration rather than call imperative setters on a
mounted widget. Domain range and step must be finite and usable; define changes to
range/step during an active interaction before allowing them.

## Retained capture requirement

FCkUiView::Commit now extends splitter-only cursor repair with a generic
opt-in ICkUiRetainedWidget contract: GetPointerCaptures reports weak widget/user/pointer
descriptors; BeginPointerCaptureTransfer/EndPointerCaptureTransfer bracket public
Slate path repair. The view verifies current ownership and component/mount ancestry,
then restores only a surviving compatible component. Stale/foreign reports are inert.
The paired hooks preserve local state across Slate's synchronous synthetic capture
loss; CanDispatchEvents alone is insufficient because removal must still reset local
state. Removal releases owned capture without consumer notification during publication.
Preview state must also be scoped to view/control lifetime. Touch pointer indices and
multiple Slate users require actual capture tests; cursor-only proof is insufficient.
Focus refresh remains a separate control acceptance requirement.

## Verification before acceptance

Adapt Test_UiSplitter's real SWindow, generated FWidgetPath and Slate.ProcessReply
fixture so CaptureMouse/ReleaseMouseCapture actually change Slate capture. Test
mouse release exactly once, forced capture loss cancel, touch threshold/no-drag/end
ordering, keyboard steps, controller Accept/navigation/Accept and focus-loss cancel,
readonly/disabled transitions, rejection and external model changes, compatible
reload during drag, invalid reload, removal and owner release. Then add a useful
Resource Inspector preview control through authored files and run rendered/PIE
compatibility. Event transport alone does not complete this gate.

## Current implementation boundary

FCkUiSlider and the Resource Inspector count slider exist. The callback-ordering
checkpoint passes61 authoring and4 app tests (exact logs in PROGRESS.md).
Verified cases include Begin-triggered reload before capture acquisition using a
fresh mounted path, Changed-triggered removal with suppressed deferred callbacks,
nested input rejected during cancellation with subsequent input accepted,
controller-to-touch handoff, idle Escape/gamepad cancel propagation, disabled
transition and externally held widget owner release. These replace the earlier
source-review concerns; they are no longer pending fixes.

This gate remains open. Required follow-up includes real focus/navigation routing
beyond direct native calls, active external-model and pending-touch edge cases,
and complete authored control styling. Vertical orientation is verified below. Preserve the verified
mouse/touch reload and model-authoritative behavior. Full game/platform/controller
acceptance and the wider pipeline campaign remain separate requirements.
## Authored orientation implementation

Add optional literal orientation="horizontal|vertical", default horizontal, with
invalid values rejected during preparation. Keep the retained widget and apply
SSlider::SetOrientation only during publication; the engine setter invalidates
layout. Reject axis changes during active interactions. Pending touch before the
native threshold also needs an explicit compatibility rule: it is not represented
by FSlider::_Interaction, so checking IsActive alone is insufficient.

SCkUiSlider::OnNavigation now gates from the configured axis before delegating to
SSlider. Both horizontal and vertical native navigation are covered. Selected-engine
SSlider handles Up/Down and vertical pointer geometry internally.

Acceptance: invalid value atomic rejection; retained pointer identity and geometry
on idle axis reload; real vertical pointer acquisition and bottom/top values;
active and pending-touch axis-change rejection without lost state; vertical
keyboard/controller steps and orthogonal navigation behavior. Preserve horizontal
sibling coverage. Implemented and verified by the Orientation fixture:62 authoring tests and4 app tests pass. See PROGRESS.md for logs and scope.
## Shared style transport and native styling follow-up

Custom schemas now declare typed -ck- CSS properties (Number, Length, Color).
Parser and registry acceptance is tracked separately in PROGRESS.md. Slider should
consume this shared path for normal/hovered/disabled bar and thumb colors, thumb
dimensions and bar thickness. Native style storage must live with the native widget,
because SSlider borrows its FSliderStyle pointer and externally held widgets may
outlive the retained component. Prepare immutable configuration; copy into stable
widget-owned style storage only at publication. Geometry-changing style reloads
need the same active/pending-gesture decision as orientation. Do not claim the
slider styling complete from transport tests alone.

Routed input fixture should set actual user focus and use Slate.ProcessReply with
FReply::Handled().SetNavigation(Direction, Genesis), then assert focused identity
after reload. Controller accept/back use ProcessKeyDownEvent. Blur uses SetUserFocus
to another mounted focusable widget, so cancellation comes from routed OnFocusLost.
Direct OnNavigation calls are retained as unit coverage but are not routed proof.

Native color/dimension consumption is now implemented and verified65+4 (see latest
PROGRESS.md). Style storage is native-widget-owned; all six state tints and thumb
dimensions are observed through Slate's actual paint output. The active Starship
style emits rounded boxes. Native IsEnabled must be bound on outer SAssignNew
arguments, not passed only to manual SSlider::Construct. Remaining brush/resource
customization and routed/platform acceptance are not implied by this checkpoint.

Routing investigation correction: ProcessKeyDownEvent does consume navigation
replies through FEventRouter::RouteAlongFocusPath -> Route -> ProcessReply. Native
SWidget::OnKeyDown generates those replies for a focusable widget. SCkUiSlider
must delegate unclaimed keys there after its custom Accept/Cancel handling.
The routed fixture now includes physical keyboard Right and gamepad D-pad events,
not only synthetic SetNavigation replies, and compares the full post-reload path.

## Next shared focus and input ownership increment

FCkUiView::Commit currently snapshots/clears/restores only user0. Use actual Slate
users and per-user owned focus snapshots, preserving external focus and callback
redirect precedence; include virtual-user tests. A retained slider controller
session currently starts without storing its key-event user index. Multi-user
acceptance must also cover session ownership: another user's keys/navigation or
focus loss must not commit/cancel the owner's preview. Audit both contracts before
claiming local-multiplayer focus support. Current routed fixture proves user0 only.

Checkpoint: the per-user increment above is implemented and verified by68 authoring +4 app tests; see PROGRESS.md for terminal logs and remaining scope. Focus snapshots now include actual/virtual users, and keyboard/controller Begin requires its event user. Broader game/platform input acceptance remains open.
