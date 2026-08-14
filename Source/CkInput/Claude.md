# CkInput

**Purpose:** Local-player input plumbing on top of UE's Enhanced Input and CommonUI, plus a raw-input
layer that lands device events in ECS and arbitrates them. Four independent surfaces: mapping-context
lifetime (`UCk_Utils_Input_UE`), player key rebinding backed by `UEnhancedInputUserSettings`
(`UCk_Utils_KeyBinding_UE` + `UCk_KeyBinding_Subsystem`), key-glyph resolution through CommonUI's
controller data (`UCk_Utils_KeyIcon_UE`), and the raw-input layer — one input-source entity per local
player, a priority stack of input-layer entities over it, and a Slate writer that feeds it
(`UCk_Utils_InputSource_UE` + `UCk_Utils_InputLayer_UE`).

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings` — plus engine `EnhancedInput`, `CommonInput`,
`CommonUI`, `GameplayTags`, `AssetRegistry`, `InputCore`, `Slate`/`SlateCore`, `DeveloperSettings`.
**Used by:** `CkGameplayDebugger` (`CkDebugger_Bridge.cpp` — the `WasInputKeyJustPressed*` helpers) and
`CkTests` (`Script/CkInput/` — six AngelScript AutoTests over injection/ownership and layer
arbitration). The rebinding and glyph surfaces have no in-repo C++ consumer; they exist for settings-UI
callers in Blueprint/AngelScript.

**The keybinding and glyph surfaces hold no ECS state; the raw-input layer is entirely ECS.** The first
three surfaces have no fragments, no processors and no handles — every entry point takes an
`APlayerController*` and reaches the `ULocalPlayer` through it, and nothing about them is tied to entity
lifetime. The raw-input layer is the opposite: sources and layers are entities, every mutation is a
deferred request, and both live and die with their entity. Neither half does anything on a dedicated
server — the keybinding utils quietly no-op without a local player, an input source is only ever created
by a `ULocalPlayerSubsystem`, and the Slate writer is not registered at all when `FSlateApplication` is
absent, so the raw layer's processors iterate nothing.

---

## Key API

### `UCk_Utils_Input_UE` — `CkInput_Utils.h`

- `AddMappingContexts(PC, TArray<FCk_MappingContextWithPriority>, InClearPrevious)` — batch add;
  `InClearPrevious` calls `ClearAllMappings()` first. Each entry is `LoadSynchronous`'d.
- `RemoveMappingContexts(PC, TArray<TSoftObjectPtr<UInputMappingContext>>)` — uses `.Get()`, so an
  unloaded soft ref is silently skipped (nothing to remove if it was never loaded).
- `SwapMappingContexts(PC, InPreviousContext, InNewContext, InPriority, InUsePreviousPriority)` —
  remove + add in one call; `InUsePreviousPriority` reads the old context's priority via
  `HasMappingContext`.
- `WasInputChordJustPressed` / `WasInputKeyJustPressed` / `WasInputKeyJustPressed_WithCustomModifier` —
  polled edge queries straight off the player controller. The chord variant checks Alt/Shift/Cmd/Ctrl
  held-state itself (`ck_utils_input::Are_ChordModifiersHeld`).
- `Get_EnhancedInputLocalPlayerSubsystem(PC)` / `Get_CommonInputSubsystem(PC)` — **C++ only**, not
  UFUNCTIONs. Both return `nullptr` (no ensure) when the PC has no local player.

### `UCk_Utils_KeyBinding_UE` — `CkKeyBinding_Utils.h`

Everything routes through `Get_InputUserSettings(PC)` → `UEnhancedInputUserSettings`, then through
the file-local `ck_key_binding_utils::Get_CurrentProfile` (`GetCurrentKeyProfile()` on ≤5.5,
`GetActiveKeyProfile()` otherwise — the engine here is 5.7, so the latter compiles).

| Group | Functions |
|---|---|
| Core | `Get_InputUserSettings`, `Get_AllRemappableKeys` → `TArray<FPlayerKeyMapping>`, `Get_KeyForMapping`, `Get_MappingNamesForKey`, `Get_KeyForInputAction`, `Get_MappingNameFromInputAction`, `Get_MappableKeyInfoFromInputAction` |
| Change detection | `Get_DidMappingKeyChange` (poll against a cached `FKey`), `BindTo_OnMappingKeyChanged`, `UnbindFrom_OnMappingKeyChanged` |
| Remapping | `RemapKey`, `RemapKeys` (batch onto one key; defers the settings broadcast until the last entry) |
| Reset | `ResetMappingToDefault` (one row), `ResetAllToDefaults` (whole profile) |
| Persistence | `SaveKeyBindings` |
| Conflicts | `Get_HasKeyConflicts` → `TArray<FCk_KeyBinding_ConflictInfo>`, scoped by `ECk_KeyConflictScope::{All, SameCategory}` |
| Resolution | `SwapKeys` (trade keys with the holder), `UnbindConflictAndRemap` (leave the holder UNBOUND — not reset to its default — then take the key) |

Data shapes: `FCk_KeyBinding_MappableKeyInfo` (MappingName / DisplayName / DisplayCategory /
Metadata, lifted off `UPlayerMappableKeySettings`) and `FCk_KeyBinding_ConflictInfo` (MappingName /
DisplayName / DisplayCategory / CurrentKey / Slot).

The four mutating functions (`RemapKey`, `RemapKeys`, `SwapKeys`, `UnbindConflictAndRemap`) return
`bool` and fill an `FGameplayTagContainer& OutFailureReason` — the reason container is the engine's,
appended from every `MapPlayerKey` call the operation makes, and `true` means it came back empty.
Nothing in this module calls `UnMapPlayerKey`: that engine entry point RESETS a mapping to its
default rather than unbinding it, so an unbind is `MapPlayerKey` with `NewKey = EKeys::Invalid`.

### `UCk_KeyBinding_Subsystem` — `Subsystem/CkKeyBinding_Subsystem.h`

`ULocalPlayerSubsystem`. On `Initialize` it binds `UEnhancedInputUserSettings::OnSettingsChanged`
once and runs the mapping-context registration scan (below). It keeps a flat `TArray<FWatcherEntry>`
of (MappingName, Slot, CachedKey, delegate); on each settings-changed broadcast it re-reads every
watched mapping's key and fires `FCk_OnMappingKeyChanged(MappingName, OldKey, NewKey)` only for the
ones that actually moved. `FCk_Handle_KeybindListener` is the opaque bind receipt — it is a plain
USTRUCT, **not** an ECS handle despite the `FCk_Handle_` prefix.

### `UCk_Utils_KeyIcon_UE` — `CkKeyIcon_Utils.h`

Thin Ck-shaped wrappers over CommonUI:

- `Get_BrushForKey(PC, Key)` — `UCommonInputPlatformSettings::Get()->TryGetInputBrush(...)` keyed on
  the live `UCommonInputSubsystem` device state (`GetCurrentInputType`, `GetCurrentGamepadName`).
  Returns a default-constructed `FSlateBrush` on miss - which is `DrawAs=Image` with a null
  resource, NOT `NoDrawType`, so "did I get an icon" checks must test the resource too.
- `Get_BrushForInputAction(PC, InputAction)` — `CommonUI::GetIconForEnhancedInputAction`, so it
  honours remaps and the active device.
- `Get_ActiveControllerData(PC)` — walks `UCommonInputPlatformSettings::GetControllerData()`,
  `LoadSynchronous` each, returns the CDO whose `InputType` (and `GamepadName` when Gamepad) matches.

### `UCk_Utils_InputSource_UE` — `CkInputSource_Utils.h`

An input source is one local player's inbox of raw device events plus the explicit list of devices that
player owns. `Add(Handle, Params)` composes `ck::FFragment_InputSource_Params` (which carries only
`_LocalPlayerIndex`) and `ck::FFragment_InputSource_Current` onto an entity; `Create(Owner, Params)`
creates a child entity first. Handle: `FCk_Handle_InputSource`.

| Group | Functions |
|---|---|
| Compose | `Add`, `Create`, `Has` (C++ only), `Cast`/`CastChecked` (`DoCast`/`DoCastChecked` in BP/AS), `Get_InvalidHandle` |
| Inbox | `Get_PendingRawEvents` (oldest first), `Get_NumPendingRawEvents` |
| Devices | `Get_OwnedDevices`, `Get_OwnsDevice`, `TryGet_SourceOwningDevice(AnyEntityInWorld, DeviceId)` |
| Identity | `Get_LocalPlayerIndex` |
| Requests | `Request_InjectRawEvent`, `Request_AssignDevice` |

The inbox row is `FCk_InputSource_RawEvent`: `_DeviceClass`
(`ECk_InputSource_DeviceClass::{Keyboard, Mouse, Gamepad}`), `_Key` (an `FKey`), `_EventType`
(`{Pressed, Released, AnalogAxis}`), `_OrderingFidelity` (`{Simultaneous, SubFrameOrdered}`),
`_AnalogValue` and `_RawDeviceUserIndex`. The first three are the essential ctor params, the rest are
fluent setters. Analog values are recorded verbatim — no deadzone, no curve, no filtering — because
conditioning is a later per-game stage and a row that was already conditioned cannot be un-conditioned
downstream.

Ordering fidelity is carried per EVENT, not per source: one local player routinely owns several device
classes at once, and a source-level flag would necessarily lie about one of them. Consumers that need
ordering consult the events in front of them and degrade to `Simultaneous` otherwise.

`FCk_InputSource_DeviceId` is (`_DeviceClass`, `_RawDeviceUserIndex`) — neither half identifies a device
on its own, since every keyboard reports index 0 regardless of which split-screen player is typing on it.

`Request_InjectRawEvent` is the only way into an inbox: the Slate writer, the synthetic test writer and
tooling all append through it, so the inbox has one shape regardless of producer. `Request_AssignDevice`
gives a source exclusive ownership of a device — re-claiming one this source already owns completes
`Succeeded` (the caller's intent holds), a device another source owns ensures and completes `Failed`, and
a negative `_RawDeviceUserIndex` is rejected synchronously with `Failed_NotEnqueued`. Exclusivity is
tested when the request DRAINS, so two sources claiming one device in the same frame are resolved by
drain order, not call order. There is no unassign request: a device stays with its source until the
source entity dies.

### `UCk_InputSource_Subsystem` — `Subsystem/CkInputSource_Subsystem.h`

`ULocalPlayerSubsystem`, and the only thing that maps a `ULocalPlayer` onto an entity — device ownership
and routing address the source by handle from there. It never creates the source at `Initialize`: the
subsystem collection comes up before the engine has given the local player a `PlayerController` (or a
world to create an entity into). Creation is re-attempted from `PlayerControllerChanged` and lazily from
`Get_InputSource()`, and quietly gives up until a local player, a world AND a controller all exist — the
same timing discipline as `UCk_KeyBinding_Subsystem`. The entity comes from
`Request_CreateEntity_TransientOwner(World)`, is debug-named `InputSource`, and is destroyed in
`Deinitialize`.

### `UCk_Utils_InputLayer_UE` — `CkInputLayer_Utils.h`

Any entity becomes a layer in one source's stack by carrying this feature. Handle:
`FCk_Handle_InputLayer`. `FCk_Fragment_InputLayer_ParamsData` is (`_InputSource`, `_Priority`) — both
essential; priority is explicit because entities have no inherent order.

| Group | Functions |
|---|---|
| Compose | `Add`, `Create`, `Has` (C++ only), `Cast`/`CastChecked` (`DoCast`/`DoCastChecked` in BP/AS), `Get_InvalidHandle` |
| Capture makers | `Make_KeyCapture(Key, Behavior)`, `Make_CatchAllCapture(Behavior)` |
| Queries | `Get_Priority`, `Get_InputSource`, `Get_Captures`, `Get_NumCaptures`, `Get_HasCaptureForKey`, `Get_GlobalActionPriority` |
| Stack lookups | `TryGet_LayerWithPriority(Source, Priority)`, `TryGet_GlobalActionLayer(Source)`, `TryGet_PressOwner(Source, Key)` |
| Requests | `Request_AddCapture`, `Request_RemoveCapture`, `Request_AddGlobalAction` |
| Signal | `BindTo_OnCaptureTriggered` / `UnbindFrom_OnCaptureTriggered` |

`FCk_Delegate_InputLayer_CaptureTriggered(FCk_Handle_InputLayer, FCk_InputSource_RawEvent,
FCk_InputLayer_Capture)` is the delivery channel, behind
`ck::UUtils_Signal_OnInputCaptureTriggered`. Bind it on the LAYER — that is what "receiving input" means
here; nothing is delivered to the source.

### `UCk_InputSlate_Subsystem` / `FCk_InputSlate_Preprocessor` — `Subsystem/CkInputSlate_Subsystem.h`, `CkInputSlate_Preprocessor.h`

The producer that turns real device input into inbox rows: a `UGameInstanceSubsystem` owning one
`IInputProcessor`. Neither exposes a callable API — they exist, and their behaviour is the contract.
Documented under *The Slate writer* below.

### `UCk_Input_ProjectSettings_UE` — `Settings/CkInput_Settings.h`

One config field: `_MappingContextScanPaths` (`TArray<FDirectoryPath>`, `ContentDir`), read through
`UCk_Utils_Input_Settings_UE::Get_MappingContextScanPaths()`.

---

## Mapping-context registration — the scan path is a feeder, not the mechanism

Enhanced Input only exposes a mapping for rebinding if its Input Action has a
`UPlayerMappableKeySettings` **and** its owning `UInputMappingContext` has been registered with the
user settings. The actual mechanism is one call:

```cpp
Settings->RegisterInputMappingContext(IMC);   // CkKeyBinding_Subsystem.cpp:62
```

`MappingContextScanPaths` is only what feeds it. At subsystem `Initialize`, if the array is non-empty,
CkInput runs an asset-registry `FARFilter` (`UInputMappingContext`, `bRecursivePaths = true`) over
those package paths, `GetAsset()`s every hit — a synchronous load of every IMC in scope — and
registers each one. The point is that rebinding works for *every* action in the project, not just the
ones whose context happens to be pushed onto the stack right now.

**With no scan paths configured the whole block is skipped** and the key profile contains only rows
from contexts registered by some other route. `Get_AllRemappableKeys` then comes back empty, and
`Get_KeyForMapping` returns an invalid `FKey` for every name — with no ensure and no log. An empty
rebinding screen is the expected symptom of an unconfigured project, not a bug in the queries.

---

## Broadcast discipline

`OnSettingsChanged` is the only change channel the watcher subsystem listens on, so every mutation
path has to reach it exactly once:

- `RemapKey` sets `bDeferOnSettingsChangedBroadcast = false` — one call, one broadcast.
- `RemapKeys` defers on every entry except the last, so a batch produces a single broadcast.
- `SwapKeys` / `UnbindConflictAndRemap` defer the conflicting-side write and let the final
  `MapPlayerKey` broadcast.
- `ResetAllToDefaults` calls `Profile->ResetToDefault()`, which bypasses the
  `UEnhancedInputUserSettings` layer entirely — CkInput fires `Settings->OnSettingsChanged.Broadcast`
  by hand afterwards (`CkKeyBinding_Utils.cpp:360`). Without that line no watcher would ever see a
  reset-to-defaults.

None of this writes to disk. `SaveKeyBindings` → `AsyncSaveSettings()` is the only persistence call.

---

## Processor groups — collect, then bias, then route, then gameplay

Three groups in `CkInput_ProcessorGroups.h` (the first two registered in
`CkInputSource_Processor.cpp`, `FGroup_Input_Bias` in `CkInputBias_Processor.cpp`):

```
FGroup_Gameplay_TimeDelta → FGroup_Input_Collect → FGroup_Input_Bias → FGroup_Input_Route → FGroup_Gameplay
```

- **`FGroup_Input_Collect`** — `FProcessor_InputSource_HandleRequests` (drains inject/assign into the
  inbox and the owned-device list), `FProcessor_InputLayer_HandleRequests` (drains capture edits),
  `FProcessor_InputButtonMap_HandleRequests` (drains button-map derivations), and
  `FProcessor_InputLayer_SetupRouterState`, which stamps `FFragment_InputLayer_RouterState` onto every
  input source whether or not a layer was ever registered on it — without that, the router's view would
  skip layer-less sources and their inboxes would grow for the life of the session.
- **`FGroup_Input_Route`** — `FProcessor_InputLayer_Route` drains the inbox and arbitrates every row.

The split is two GROUPS rather than two processors sharing one because registration order *within* a
group is not an ordering guarantee, and a consumer module can only order against a named group.
**Anything that reacts to delivered input declares `RunAfter = TDepList<ck::FGroup_Input_Route>`.** The
`RunBefore = FGroup_Gameplay` edge is what keeps the whole raw layer ahead of gameplay, so a press is
visible on the frame it arrives rather than the one after.

`FProcessor_InputSource_CancelPendingRequests`, `FProcessor_InputLayer_CancelPendingRequests` and
`FProcessor_InputButtonMap_CancelPendingRequests` sit in
`FGroup_EndPlay` under `CK_IF_END_PLAY` and call `ck::request::FireCancelledForPending`. Every drain
excludes `FTag_DestroyEntity_Initiate`, so a source, layer or map destroyed on the same stack that enqueued a
request deterministically reaches the cancel processor and its caller completes `Failed_Cancelled`
instead of hanging.

---

## Layer arbitration — priorities, declarative captures, press ownership

**Layers are entities with explicit int priorities, and a priority is unique per source.**
`DoGet_RegistrationIsValid` runs every rejection BEFORE anything is composed or created — an invalid
`_InputSource`, or a priority already held by another layer on that source — so a rejected registration
leaves neither a half-composed entity nor an empty child behind. `Add` additionally rejects an entity
that is already a layer: one entity holds at most one place in a stack. The collision check is scoped to
one source, so the same priority on two different sources is fine. A collision is an ensure plus an
invalid returned handle, never a silent tie-break — arbitration order must be unambiguous.

**Captures are data rows, not callbacks.** `FCk_InputLayer_Capture` is
(`_MatchMode` `{Key, CatchAll}`, `_Key`, `_Behavior` `{Consume, PassThrough}`) and carries no callback
and no return value, so the "matched but declined" state a handler's return would express does not exist
and cannot silently eat a key. A layer whose own state decides whether it consumes expresses that by
adding or removing captures. All of a layer's captures live as rows in ONE stable
`ck::FFragment_InputLayer_Current` — never a fragment or tag per capture, because fragment pools are
tombstone-mode and churning fragment TYPES per frame is the expensive shape; keeping them in one array
is also what makes the live arbitration set inspectable (`Get_Captures`).

`Request_AddCapture` REPLACES the behavior of an existing row with the same (match mode, key) rather
than duplicating it; a `Key` capture naming an invalid key is rejected synchronously with
`Failed_NotEnqueued`. `Request_RemoveCapture` removes every row matching (match mode, key) and reports
`Succeeded` even when nothing matched — the caller's intent holds either way.

**Capture edits are deferred, with a one-frame visibility contract.** They drain in
`FGroup_Input_Collect`, so the routing pass that follows sees a capture set that cannot change underneath
it. An edit enqueued from gameplay — which runs after `FGroup_Input_Route` — therefore first takes effect
on the NEXT frame's routing: a modal pushed by this frame's Escape does not swallow the rest of this
frame's input. The guarantee is relative to ROUTING, not to the tick; an edit enqueued from a group
ordered before `FGroup_Input_Collect` lands in the same frame's routing.

**The router walk.** `FProcessor_InputLayer_Route` copies the source's inbox and `Reset()`s it, then (if
there was anything) rebuilds the ranked layer list for that source from the registry — filtered to layers
naming this source, excluding destroy-initiated ones, sorted priority descending — and routes each event
top-down:

- A layer that is invalid or already pending destroy (`BeginDestroy` phase) is skipped.
- The first capture that matches (`CatchAll` always; `Key` on `FKey` equality) is delivered via
  `UUtils_Signal_OnInputCaptureTriggered::Broadcast(Layer, {Layer, Event, Capture})`.
- `PassThrough` continues the walk; **`Consume` ends it** — nothing below sees the event. A catch-all
  Consume on a top layer IS suspension, structurally.

**Press → release ownership belongs to the router, not the layers.** When a `Pressed` event is matched by
a `Consume` capture, the router records `(Key, Layer, Capture)` in
`FFragment_InputLayer_RouterState::_PressOwners` — which lives on the input SOURCE precisely because it
has to outlive the layer it points at. A later `Released` for that key is delivered straight to the
recorded owner and the walk is skipped entirely, so the owner still gets its key-up even if its captures
changed or it was popped in between. If that owner is gone, the release is **dropped**, not offered
downward: the layers below never saw the press, and a key-up for a press they never received would leave
them holding state no press ever opened. Either way the entry is removed when the release arrives, and a
fresh press on the same key replaces any stale entry, so an unreleased press never wedges the key.
`PassThrough` matches never take ownership, so their releases walk the stack normally.
`TryGet_PressOwner(Source, Key)` exposes the table.

**Delivery visibility — the router also retains what it did.** Alongside arbitration, the router writes one
`FCk_InputLayer_RoutedEvent` per routed event into `ck::FFragment_InputLayer_RoutedThisFrame` on the SOURCE:
the verbatim event plus its outcome, `ECk_InputLayer_DeliveryOutcome::{ConsumedByLayer, PassedThrough,
DroppedNoOwner}`, with the consuming layer named only in the first case. These are the router's three terminal
paths and nothing else. `PassedThrough` means no layer ENDED the walk — every `PassThrough` capture on the way
down was still delivered — which is the property a consumer reasoning about masking actually needs; it is not
"nobody saw it". Retention rather than a signal because the value is the SHAPE of the frame (what was masked,
what fell through, what was dropped), which no per-event delivery can express.

The array is cleared at the top of EVERY Route pass, including one that finds an empty inbox, so a stale frame
can never be read as the current one. **Only a consumer ordered AFTER `FGroup_Input_Route` sees this frame's
outcomes** — gameplay qualifies by group edge; anything in `FGroup_Input_Collect`/`FGroup_Input_Bias` reads the
cleared array. Read it with `UCk_Utils_InputLayer_UE::Get_RoutedEventsThisFrame(Source)`. The fragment is
stamped by `FProcessor_InputLayer_SetupRouterState` in the same pass as the router state, so the two always
co-exist. First consumer: `CkIntent`'s frame record (`CkIntent/Claude.md`).

**Global actions are the reserved bottom of the stack.** `ck::input_layer::GlobalActionPriority` is
`TNumericLimits<int32>::Lowest()`, and `Add`/`Create` ensure-reject it so an ordinary layer cannot
accidentally sit underneath the debug keys. `Request_AddGlobalAction(Source, {Key}, ...)` creates that
reserved layer on first use (synchronously, through the same `DoCreateLayer`) and then enqueues a
`Key` + `Consume` capture on it — no binding profile, no intent definition, no bake. Global actions are
arbitrated by the same top-down walk as everything else, which is what makes a debug key stop working
while a modal is up without the modal knowing the debug key exists. Bind `OnCaptureTriggered` on
`TryGet_GlobalActionLayer(Source)` to receive them; it is invalid until the first one is registered.
Because the completion delegate is forwarded to that inner `Request_AddCapture`, a successful
registration completes with the global-action LAYER as the owner handle while every synchronous
rejection completes with the INPUT SOURCE — do not identify the operation by the owner it reports.

---

## Axis conditioning — the raw fact and the value you sample

Conditioning is the per-game stage between a raw axis event and the number gameplay acts on: deadzone,
response curve, sensitivity, inversion. It is composed onto the input SOURCE
(`UCk_Utils_InputBias_UE::Add`, handle `FCk_Handle_InputBias`) — one conditioning table per local
player, no `Create`, because a table on a child entity would have no inbox to read.

**Raw and conditioned are two values, and the raw one is never overwritten.** `FCk_InputSource_RawEvent`
in the inbox stays exactly as its producer wrote it; a layer's `OnCaptureTriggered` still receives the
verbatim row even though conditioning has already run that frame. What conditioning writes is separate
state on the source: per axis key, the last raw value AND the last conditioned value
(`Get_LastRawAxisValue` / `Get_ConditionedAxisValue`). Consumers sample the conditioned one; anything
that needs the physical fact — a recording, a debugger, ordering — reads the raw one. The split exists
because conditioning is lossy and not invertible: a deadzone maps a whole band onto zero, so a row that
had been conditioned in place could never be read back or re-tuned.

**The order is fixed: inversion → deadzone → exponent → sensitivity.**

1. **Inversion** negates the raw value. Because every later stage works on the magnitude and restores
   the sign, this is observationally the same as a final negation — do not "simplify" it into one, the
   declared order is what the contract names.
2. **Deadzone** — magnitudes at or below the threshold read as exactly `0`; above it the remaining band
   is rescaled so the threshold maps to `0` and full deflection still maps to `1`
   (`(m - d) / (1 - d)`). Sign is preserved. A stick therefore leaves the deadzone smoothly instead of
   jumping to the threshold value.
3. **Exponent** raises the rescaled magnitude to the declared power. Sign preserved.
4. **Sensitivity** scales the result last, so `sensitivity` is exactly what full deflection produces.

An axis with no declared bias — and an axis whose row is left at its defaults — is an **identity
passthrough**: same value, still recorded, still sampled. `Get_ConditionedAxisValue` on an axis no event
has ever arrived for is `0`; that is derived-from-no-samples, not a bias result. Buttons are never
conditioned: a press has no magnitude for a deadzone or a curve to act on. The stages assume a
normalised axis (full deflection = 1), so on an unnormalised one — a `MouseX`/`MouseY` pixel delta —
only sensitivity is meaningful.

**Ranges are rejected, never clamped.** Deadzone must be in `[0, 1)`, exponent and sensitivity strictly
greater than zero, and the axis key must be real. `Add` validates every declared row plus uniqueness of
the axis key and rejects the WHOLE composition on the first bad row, leaving nothing composed;
`Request_SetAxisBias` rejects before enqueue and completes `Failed_NotEnqueued`, leaving the table
untouched. A flipped axis is asked for with inversion, not with a negative sensitivity.

**Retunes are deferred, one group ahead of conditioning.** `Request_SetAxisBias` is add-or-update per
axis key and drains in `FGroup_Input_Collect`; the conditioning pass runs in `FGroup_Input_Bias`, whose
edges are `RunAfter FGroup_Input_Collect` and `RunBefore FGroup_Input_Route`. Two consequences worth
knowing: a retune enqueued from gameplay first affects the **next event to arrive** and never
re-conditions a sample already taken (a stale conditioned value stands until its axis is sampled again),
and conditioning necessarily runs BEFORE the router — the router drains the inbox, so anything reading
those rows has to read them first. The upside is that a consumer woken by routing already sees this
frame's conditioned values rather than last frame's. The full chain is
`FGroup_Gameplay_TimeDelta → FGroup_Input_Collect → FGroup_Input_Bias → FGroup_Input_Route → FGroup_Gameplay`.

`TryGet_AxisBias` answers "no bias declared" with a row whose `AxisKey` is INVALID rather than with an
identity row — a stored row always names a real key, so the two can never be confused. `Get_AxisBiases`
is the live table, and like every deferred edit in this module a retune enqueued this frame is absent
from it until the request drains.

---

## The button space — stable identity for a pressable thing

A `ButtonId` is what a definition names when it must not care which key is pressed. It is composed onto
the input SOURCE (`UCk_Utils_InputButtonMap_UE::Add`, handle `FCk_Handle_InputButtonMap`) — one button
space per local player, no `Create`, because a map on a child entity would have no player whose profile
to derive from. It is **opt-in**: a source without it simply has no button space, and nothing else in the
module notices.

**Identity is `(Tier, FName)` and is immutable.** Once a button has been derived or registered it never
changes and is never removed for the map's lifetime; a re-derive moves key ASSOCIATIONS only. That is the
whole point — a definition that names Jump keeps naming Jump after the player rebinds it, with no edit
anywhere. Deliberately not a dense int: packing buttons into a bitmask is a bake-time job for a consumer
that knows exactly which buttons it references, and a dense index assigned here would have to stay stable
across profiles it knows nothing about.

**Two tiers, and they answer to different authorities.**

- **Tier 1 — `Mapped`.** One button per Enhanced Input player-mappable MAPPING NAME (the name the
  settings store keys on, which is stable across rebinds by construction). Its key association is read
  from the player's resolved mappings and re-read on every derive. A name owns one button no matter how
  many slots it has, and the association carries **every bound slot's key, primary first** (ascending
  slot order, so the First slot leads) — a keyboard binding in one slot and a gamepad binding in
  another both produce the button. Scalar queries answer the primary; `Get_KeysForButton` answers them
  all.
- **Tier 2 — `Physical`.** One button per raw `FKey` nothing maps, identity = the key's own `FName`,
  association = that key, fixed forever and never touched by a re-derive. This is the tier for
  prototyping, for synthetic tests, and for anything that has to work before a binding profile exists.

**The association is MANY-TO-MANY by design.** Key → button: two mappings in different categories
legitimately share a key, and duplicate bindings exist in real profiles, so `Get_ButtonIdsForKey`
returns EVERY holder — a consumer that wants "the" button has to say which tier and name it means.
Button → keys: a Mapped button carries one key per bound slot, so `Get_KeysForButton` returns every
key (primary first) and answers EMPTY both for a button the map never minted and for one that is
currently unbound — both mean "pressing nothing produces it", which is all a consumer can act on.
`TryGet_KeyForButton` answers the PRIMARY key alone (invalid on the same two states) and exists for
consumers that need one display key, not for delivery logic. An invalid key handed to
`Get_ButtonIdsForKey` answers empty rather than listing the unbound buttons: "no key" is a state those
buttons are in, not a key they answer to.

**Derivation is deferred and drains in `FGroup_Input_Collect`**, alongside every other CkInput request and
ahead of `FGroup_Input_Route` — so a consumer woken by a delivered event resolves buttons against this
frame's map. `Add` composes the fragments and then enqueues its declared tier-2 registrations followed by
the first `Request_Rederive` through the ordinary request path, so composition-time state and steady-state
state are produced by the same code and cannot disagree. The consequence is the same one-frame boundary
capture edits obey: **the map is EMPTY on the calling stack of `Add`** and fills in when the requests
drain. `Get_AllButtons` right after `Add` returning nothing is the contract, not a race.

**`Request_Rederive` rebuilds tier-1 from the profile rather than patching it.** Every mapped association
is cleared first, then re-established from `Get_AllRemappableKeys`, so a mapping that stopped being
player-mappable is left holding an EMPTY key list instead of its last keys. Newly seen names mint new
identities; existing ones are never re-minted. A derive against an unmoved profile is an accepted no-op
and completes `Succeeded`.

**An UNREADABLE profile is a graceful no-op that completes `Succeeded`, and it changes nothing.** Three
absences count, and all three are transient: no `PlayerController` yet, no `UEnhancedInputUserSettings`,
and no active key profile. A source composed before the engine has handed the local player a controller is
the normal startup order — the same quiet early-out the keybinding utils and both subsystems already take.
The first is caught by its own null check; the other two are indistinguishable from a real empty profile at
the query boundary (`Get_AllRemappableKeys` answers `{}` for all three), so they are caught by reading the
query BEFORE the clearing pass and skipping the derive when it comes back empty while the map still holds
Mapped keys. A transient absence must not wipe associations that were correct a frame ago: the wipe would
last a frame, and a frame is long enough to cancel every live episode and Active level row a `CkIntent`
matcher is holding on those buttons. A map that has no Mapped keys yet is not in that state, so the FIRST
derive of a session still runs against a genuinely empty profile.

**`Request_RegisterPhysicalButton` is idempotent.** Re-registering a key the map already carries —
whether it came from a declaration or an earlier request — completes `Succeeded`, because a physical
button's association is the key itself and the caller's intent already holds. An invalid key is rejected
before enqueue with `Failed_NotEnqueued`. `Add` validates every declared key and their mutual uniqueness
and rejects the WHOLE composition on the first bad one, leaving nothing composed.

**The re-derive triggers are `UCk_InputSource_Subsystem`, not the keybinding half.** It binds
`UEnhancedInputUserSettings::OnSettingsChanged` AND `OnMappingContextRegistered` with the same
late-binding discipline it already uses for source creation, and on each broadcast enqueues a
`Request_Rederive` on its own source — guarded on the map being present, since the feature is opt-in.
Both delegates are load-bearing: a REBIND broadcasts only `OnSettingsChanged`, while REGISTERING an
IMC (`RegisterInputMappingContext` — the path every runtime-built, `bNotifyUserSettings` context
takes) broadcasts only `OnMappingContextRegistered`; before the second bind existed, mapping names
first registered after the map's initial derive never minted, permanently starving any consumer
gating on them (the second-engaged-station bug, 2026-08-12). The subsystem already owns the handle
the request has to land on, and putting the seam here keeps the dependency (raw layer READS user
settings) on the raw side: no keybinding path learns that entities exist. A rebind or registration
therefore reaches the map on the next frame's collect pass, since gameplay runs after routing.
Related caveat: `UnregisterInputMappingContext` never removes key-profile rows, so a Mapped button
keeps its keys after its IMC is gone — `Get_IsMappedButtonMinted`-style gates can be satisfied by a
stale key.

**Re-registering the SAME IMC OBJECT broadcasts nothing, so it produces no re-derive.**
`UEnhancedInputUserSettings::RegisterInputMappingContext` returns `false` immediately when its
`RegisteredMappingContexts` set already holds that object, before it reaches the
`OnMappingContextRegistered.Broadcast` at the end of the internal path. The consequence bites the
re-engage case that motivated the second bind in the first place: a station that pushes the same IMC
asset a second time gets a rederive on the FIRST engage only, so a consumer relying on re-engagement
to refresh the map is relying on something that does not happen. Anything that needs a rederive
without a genuinely new IMC has to enqueue `Request_Rederive` explicitly. (An IMC built fresh at
runtime is a different object each time and does broadcast — which is why the original bug's fix
worked and this gap stayed hidden.)

**Both triggers are also cheap to fire repeatedly.** A rederive carries no payload, so
`Request_Rederive` folds into one already pending on the same map when the incoming request has no
completion delegate — a load that registers a dozen contexts in one frame therefore does one derive,
not a dozen.

---

## The Slate writer — what actually reaches an inbox

`UCk_InputSlate_Subsystem` owns one `FCk_InputSlate_Preprocessor` for exactly as long as the game
instance lives. A Slate pre-processor is application-global while an input source belongs to a local
player, so the game instance is the narrowest engine seam that owns one of each: registration and
unregistration bracket a single game session, and a second PIE session brings up its own subsystem rather
than a second registration against the same one. Registration is skipped entirely when
`FSlateApplication::IsInitialized()` is false — dedicated server, headless run.

It registers at **Slate index 0** (the earliest possible view of the event stream, which is what makes
arrival order recoverable) and is **observe-only — every handler returns `false`.** Consumption belongs
to ECS routing, and that is exactly what lets it share index 0 with the loading screen's and the
debugger's pre-processors without any of them arbitrating against another.

| Slate handler | Row written |
|---|---|
| `HandleKeyDownEvent` | `Pressed` — OS auto-repeat (`IsRepeat()`) is dropped; a held key is one Pressed until its Released |
| `HandleKeyUpEvent` | `Released` |
| `HandleAnalogInputEvent` | `AnalogAxis` carrying the analog value |
| `HandleMouseMoveEvent` | the cursor delta split into `EKeys::MouseX` / `EKeys::MouseY` `AnalogAxis` rows; a component that did not move writes no row rather than a zero one |
| `HandleMouseButtonDownEvent` / `HandleMouseButtonUpEvent` | `Pressed` / `Released` on the effecting button |
| `HandleMouseButtonDoubleClickEvent` | `Pressed` — the OS classifies the SECOND press of a rapid double click as its own event type and Slate routes it here, not to the down handler; the record wants the physical fact (the button went down again), so it lands as an ordinary `Pressed` row. Without this row a fast double click records as one click |
| `HandleMouseWheelOrGestureEvent` | `Pressed` **and** `Released` pairs on `EKeys::MouseScrollUp`/`MouseScrollDown`, then one `AnalogAxis` row on `EKeys::MouseWheelAxis` carrying this event's signed delta. See *The wheel* below. Trackpad gestures write nothing at all. Zero-delta events write nothing |

Every row funnels through `DoRecordEvent` and is dropped unless it clears, in order: a valid `FKey`;
**DIRECT viewport focus** (`GetGameViewportWidget()->HasAnyUserFocus()` on THIS game instance — what
keeps the editor's own keystrokes out of a source during PIE, stops a background PIE window recording the
foreground window's input, and keeps CONSOLE/CHAT typing out of the record: a text field steals focus to
a viewport DESCENDANT, so descendant-counting focus recorded `slomo 0.1` as gameplay presses); and
resolution to a live source. A UMG widget that takes keyboard focus mid-game therefore also pauses
recording — the flush below releases anything held at that boundary, so nothing phantoms.

**Losing viewport focus flushes every recorded-down key as a synthetic `Released`.** The focus gate
means a release that happens while unfocused (alt-tab, a click on an editor panel) never records, so a
key pressed in-focus and released out-of-focus would otherwise stay down in every consumer forever —
router press-owners, the intent record's held set, the device debugger. The writer tracks each
(key, raw user index) it recorded a `Pressed` for and, from its Slate `Tick`, writes the matching
`Released` rows the moment the viewport is unfocused while any are outstanding — the pipeline's
equivalent of the engine's `FlushPressedKeys`. Consequence: a hold does NOT survive a focus gap; a key
still physically held when focus returns reads as up until it is re-pressed (the OS resends no edge, and
auto-repeat is dropped). Analog axes are not flushed — they hold their last conditioned value.

### The wheel — notch UNITS, banked, one pair per notch

`FPointerEvent::GetWheelDelta()` is a count of notches, not a notch. A fast flick reaches Slate coalesced
into ONE event carrying `2.0` or more; a high-resolution wheel or a precision trackpad reports fractions
like `0.33`. So the writer keeps a signed accumulator, adds each event's delta to it, emits `floor(|acc|)`
independent `Pressed`+`Released` pairs, and carries the fraction to the next event. One pair per event
would have swallowed most of a flick and fired a whole notch for a third of one.

- **Each notch is its OWN pair**, which is what lets `CkIntent`'s record split them into one frame row
  each rather than collapsing several presses of one key into a single edge.
- **Direction comes from the ACCUMULATOR's sign**, not from the current event's — a small up-flick landing
  on a banked down remainder resolves down.
- **The axis row carries this event's own delta, unaccumulated.** It is the analog fact the device
  reported, and a sub-notch scroll that emitted no pair still emitted motion.
- **The order is `Pressed`, `Released`, then the axis**, matching `FSceneViewport::OnMouseWheel`. Mouse
  rows are `SubFrameOrdered`, which promises within-frame order is real, so the record must not invent one
  the engine never produces.
- **The remainder resets on the focus-loss flush**, with the held keys: a half-notch banked before an
  alt-tab must not complete a notch after it.
- **A gesture-sourced scroll writes NOTHING** — not the pair and not the axis. A gesture is surfaced by
  its type and carries no wheel key to record under, and the early-out precedes both writes. On a platform
  that routes all scrolling through gestures (a macOS trackpad) the wheel is therefore absent from the
  record entirely.

Device class is derived from the KEY, not from which handler fired: `IsGamepadKey()` → Gamepad, else
`IsMouseButton()` → Mouse, else Keyboard. Ordering fidelity follows from the class — Gamepad is
`Simultaneous` (a polled bitmask can only say "same frame"), everything else is `SubFrameOrdered` (off
the OS message queue, order preserved within the frame).

Routing (`DoTryGet_RoutedSource`), in order:

1. Resolve local player 0's source. **If it does not exist yet, nothing is recorded at all** — including
   gamepad events belonging to other local players.
2. An explicit `Request_AssignDevice` claim wins: `TryGet_SourceOwningDevice` on the
   (class, raw user index) pair.
3. Otherwise keyboard/mouse → local player 0's source.
4. Otherwise gamepad → the source of the local player whose INDEX equals the raw device user index.

`ck.Input.DumpRawEvents` (bool, default off) logs one grep-friendly `[CkInputDump]` `Display` line per
RECORDED event, carrying frame, device class, key, event type, analog value, raw user index, ordering
fidelity and destination source.

---

## AngelScript

Generated wrappers ship for the three PlayerController-scoped utils classes:
`Script/Generated/utils_input.as`, `utils_key_binding.as`, `utils_key_icon.as` — full coverage of the
UFUNCTION surface, so `utils_key_binding::RemapKey(...)` etc. are available without further work. The
C++-only subsystem accessors on `UCk_Utils_Input_UE` are absent by design.

`CkKeyIcon_Utils.cpp` additionally hand-binds two makers at `EOrder::Late`:
`Make_CommonInputKeyBrushConfiguration` and `Make_CommonInputKeySetBrushConfiguration`. CommonUI
declares those structs' fields `UPROPERTY(EditAnywhere)` without `BlueprintRead*`, which makes them
unreachable from AngelScript property access — the makers are the only way to build one from script.
`Late` matters: these signatures name `FKey`/`FSlateBrush`/`TArray`, which are not registered yet at
`Early`.

The raw layer ships `Script/Generated/utils_input_source.as` and `utils_input_layer.as`. Both utils
classes carry `Meta = (ScriptMixin = "FCk_Handle_InputSource" / "FCk_Handle_InputLayer")`, so their
handle-first functions bind as handle MEMBERS and the `utils_*` wrapper forwards to the member form —
`utils_input_layer::Request_AddCapture(Layer, Request)` and `Layer.Request_AddCapture(Request)` are the
same call. Completion delegates come through with the emitted
`= FCk_Delegate_Request_OnCompleted()` default, so the trailing argument is optional in script. `Has` is
a plain C++ static on both classes rather than a UFUNCTION and is therefore absent from script — use
`DoCast`, which returns a `TOptional`. `UCk_InputSource_Subsystem::Get_InputSource` is a UFUNCTION on the
subsystem, not part of either namespace.

---

## Anti-patterns

1. **Don't expect entity-lifetime cleanup.** A mapping context added through `AddMappingContexts`
   stays on the Enhanced Input subsystem until something removes it. The mapping-context surface has no
   ECS integration and no `EndPlay` processor of its own (the raw layer's `EndPlay` processors only
   cancel undrained requests) — pair every add with its remove/swap yourself.
2. **`SaveKeyBindings` outlives the session.** `AsyncSaveSettings()` writes real user settings under
   `Saved/`. Test and gym code that remaps must call `ResetAllToDefaults` + `SaveKeyBindings` on
   teardown, or the next run starts from the mutated bindings — the failure looks like a flaky test
   and is actually leftover state on disk.
3. **Don't unbind a mapping/slot two widgets are both watching.**
   `UnbindFrom_MappingKeyChanged` → `_Watchers.RemoveAll` matches on (MappingName, Slot) only, not on
   the delegate. One widget unbinding silently kills every other listener on the same row.
4. **Glyph resolution rides CommonUI device detection.** Both brush getters read
   `UCommonInputSubsystem::GetCurrentInputType()` at call time, so a cached `FSlateBrush` goes stale
   the moment the player switches keyboard↔gamepad. Re-resolve on the device-change event; don't
   cache. `Get_ActiveControllerData` additionally **ensures** when no controller data matches the
   current device — don't call it speculatively from a device-agnostic path.
5. **Don't call `Profile->ResetToDefault()` (or any `UEnhancedPlayerMappableKeyProfile` mutator)
   directly.** It skips the settings layer and no `OnSettingsChanged` fires, so watchers and any
   bound UI go stale. Go through `ResetAllToDefaults` / `ResetMappingToDefault`.
6. **`SwapKeys` on an unbound source unbinds the other side.** `OldKey` starts at `EKeys::Invalid`
   and is only overwritten if the source mapping currently has a key; the "swap" then assigns Invalid
   to whoever held the target key. Check `Get_KeyForMapping` first if that isn't what you want.
7. **Missing-profile failures are silent.** Every query does `CK_ENSURE_IF_NOT` on the player
   controller, then plain early-outs on a null settings object or null profile. A wrong-`MappingName`
   query is indistinguishable from an unregistered-context one — both return an invalid `FKey`. Start
   diagnosis at `Get_AllRemappableKeys`.
8. **Local player only.** Everything resolves through `APlayerController::GetLocalPlayer()`; on a
   remote or server-side controller the accessors return null and the utils quietly no-op.
9. **Don't poll the inbox — it drains every frame.** `FProcessor_InputLayer_Route` copies
   `_PendingRawEvents` and `Reset()`s it unconditionally, for every source carrying router state. Anything
   reading `Get_PendingRawEvents` from gameplay (which runs after `FGroup_Input_Route`) sees an empty
   array, and a consumer that "polls harder" is racing the drain. Receiving input means binding
   `OnCaptureTriggered` on a layer. The getters exist for tests and debug views that run inside the input
   groups.
10. **Don't hand-roll a second input source for a local player.** `UCk_InputSource_Subsystem` owns the one
    source per local player, and the Slate writer resolves destinations exclusively through it
    (`LocalPlayer->GetSubsystem<UCk_InputSource_Subsystem>()->Get_InputSource()`). A source you compose
    yourself is perfectly functional for injected events and will never receive a single real one — the
    failure looks like "input works in tests, not in PIE". Compose your own only for synthetic
    producers.
11. **Two layers may not share a priority on one source.** `Add`/`Create` ensure and return an INVALID
    handle on collision, and on the reserved `GlobalActionPriority`. Callers that ignore the return value
    get a silent no-layer, then wonder why nothing is delivered — check `ck::IsValid` on the result, or
    ask `TryGet_LayerWithPriority` first.
12. **A capture edit is not visible this frame.** `Get_Captures` / `Get_HasCaptureForKey` read the drained
    set, so reading straight back after `Request_AddCapture` returns the OLD rows. Test assertions and
    "did my push take" checks must wait a frame; that one-frame gap is the contract, not a race.
13. **Captures name physical `FKey`s and do not follow rebinds.** Nothing in the router consults
    `UEnhancedInputUserSettings`, so a player who remaps an action in the settings UI does not move any
    capture. No routing path reads user settings, and no keybinding path knows a layer exists; the one
    place the two halves meet is the button map (*The button space*), which reads the profile and which
    nothing in the router consults. In particular, do not reach for
    `SaveKeyBindings` (or any keybinding util) from a capture callback: it needs an `APlayerController`
    the layer does not have, and it writes real user settings to disk (see #2).
14. **A release reaches the press owner or nobody.** Don't write a layer that assumes it will see a
    key-up for every key-down it would match — if a layer above consumed the press, the release goes there
    and never walks down; if that owner was destroyed, the release is dropped outright. Layers holding
    press-scoped state must also clear it on their own teardown.
15. **Neither devices nor layers are "removed".** There is no unassign-device request and no layer
    `Remove` — device ownership ends when the source entity dies, and popping a layer means destroying its
    entity. `Request_DestroyEntity` on the layer is the pop.
16. **Don't read `MouseWheelAxis` to ask whether the wheel moved this frame.** Axes are sampled state
    and are never flushed, so the last notch's delta stands until the next one — a poll cannot tell a
    stationary wheel from one that just turned. The notch is an IMPULSE and its edges are what carry
    that fact: read the `MouseScrollUp`/`MouseScrollDown` press rows (or a button/intent terminal on
    them). The axis row exists so the magnitude is answerable, not so it can be polled.
17. **`ck.Input.DumpRawEvents` silence is not evidence that no input arrived.** The line is emitted at the
    END of `DoRecordEvent`, after the valid-key check, the viewport-focus gate and destination
    resolution. An unfocused window, an invalid key and a missing local-player-0 source all produce
    exactly zero output, indistinguishable from a device that sent nothing.

---

## See also

- `CkEcs/Claude.md` — processor groups and the scheduler edges the raw layer orders against, plus the
  request-completion mechanism every `Request_*` here uses.
- `CkUI/Claude.md` — widget layer that consumes the glyph brushes.
- `CkSettings/Claude.md` — `UCk_Plugin_ProjectSettings_UE`, the base for `UCk_Input_ProjectSettings_UE`.
- UE Enhanced Input user-settings docs (`UEnhancedInputUserSettings`, `FPlayerKeyMapping`,
  `FMapPlayerKeyArgs`) and CommonUI's `UCommonInputPlatformSettings`.
