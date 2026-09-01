# CkInventory

**Purpose:** Inventory system. Two type-safe inventory shapes — *Spatial* (grid placement, items occupy one or more grid cells) and *DataOnly* (slot-based, optionally bounded by a maximum). Both shapes share an item model: each inventory is a Record of item entities, items carry `CkTagSet` tags for filtering, and replication is per-shape.

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkGrid`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`, `CkTagSet`.
**Used by:** Player inventory, loot containers, shops, anything else that needs typed item containment.

---

## Type-safe handle hierarchy

```
FCk_Handle_Inventory                  // shared base; queries that work for both shapes
├── FCk_Handle_Inventory_Spatial      // grid placement; FIntPoint + ECk_CardinalRotation per item
└── FCk_Handle_Inventory_DataOnly     // slot-based; ECk_Inventory_DataOnly_BoundMode
                                      //   {Unbounded, BoundedByUniqueEntries, BoundedByTotalUnits}

FCk_Handle_Item                       // the item entity; one per item, never owned by an actor directly
```

Handles live in `*_Fragment_Data.h` (not `*_Fragment.h`) so UHT-reflected types stay separate from ECS templates — same convention CkAttribute uses for `FCk_Handle_FloatAttribute` etc.

## Public API surface (Blueprint / AngelScript)

- **`UCk_Utils_Inventory_UE`** — the canonical home for all `Request_*` operations. Hosts: `Request_AddItem`, `Request_RemoveItem`, `Request_StackItems`, `Request_SplitStack`, `Request_AddItemByDefinition`, `Request_Sort`, `Request_TransferItem_ToSpatial`, `Request_TransferItem_ToDataOnly`, `Request_MassTransfer` (standalone bulk op — see *ItemResolution — standalone ops* below). Each performs auth + signal bind once at the public Utils boundary, then runtime-branches on the inventory's shape tag (one tag check) via `ck::inventory_helpers::DispatchEnqueue` and constructs the typed entry. Also hosts shape-agnostic queries: `Get_CanAcceptItem`, `Get_ContainsItem`, `Get_Items`, `Get_NumItems`, `Get_TotalUnits`, `Get_AbsorbableUnits`, `Get_StackRoomFor`, `Get_InventoryType`, `Get_IsSpatial`, `Get_IsDataOnly`, `RecordOfInventories_Utils`, `RecordOfInventoryItems_Utils`. No `Add()` / `AddMultiple()` / `Make_*` here — those need shape-specific Params and live on the typed Utils.
- **`UCk_Utils_Inventory_Spatial_UE`** — Spatial-only API. `Make_Params`, `Add` / `AddMultiple`, queries (`Get_Dimensions`, `Get_NumFreeCells`, `Get_FirstAvailablePlacement`, `Get_CanPlaceItemAt`, `Get_ItemPlacementCoordinate`, `Get_ItemPlacementRotation`, `Get_ItemActiveCells_Rotated`, `Get_ItemAtCoordinate`, `Get_Grid`). Two **placement-aware overloads** of operations whose default lives on base: `Request_AddItem(Handle, Request, FCk_SpatialPlacement, Delegate)` and `Request_SplitStack(Handle, Request, FCk_SpatialPlacement, Delegate)` — these accept an explicit placement; the placement-free versions on the base default to `AutoPlace`. **`Request_RelocateItem`** is Spatial-only (no shape-agnostic equivalent — DataOnly has no cell placement to relocate).
- **`UCk_Utils_Inventory_DataOnly_UE`** — DataOnly-only API. `Make_Params` / `Make_Params_Bounded` (entries metric) / `Make_Params_BoundedByTotalUnits` (units metric), `Add` / `AddMultiple`, queries (`Get_BoundsInfo`, `Get_BoundMax`, `Get_EffectiveBoundMode`, `Get_RemainingSlots` (entries only), `Get_RemainingCapacity` (metric-aware)), and `Request_OverrideBounds` (DataOnly-specific operation; changes the limit VALUE only — the metric is immutable at creation). **No `Request_*` for the standard inventory operations** — those live on base only and reach DataOnly handles via mixin propagation.
- **`UCk_Utils_Item_UE`** — create / destroy items, query their definition / parent inventory.
- **Item traits** under `ItemTrait/` — `Stackable`, `Dimensions`, `Tags`. Stackable stack-count is itself an `IntegerAttribute`; not stored as a raw int on the item. `UCk_Utils_ItemTrait_Stackable_UE` additionally exposes the inventory-context capacity reads: `Get_EffectiveMaxStackSize` (+ `_ByDefinition`) and `Get_RemainingStackCapacity_InInventory` — see *Capacity policies* below.
- **`UCk_Utils_ItemResolution_UE`** — item-placement decisions + standalone ops (see *ItemResolution — standalone ops* below).

## ItemResolution — standalone ops

Cross-inventory placement logic that is NOT a request on a single inventory:

- **`UCk_Utils_ItemResolution_UE::ResolveBestTransferTarget(Item, FCk_BestTransferTargetParams)`** —
  pure (no mutation) "which candidate should receive this item" ranker: filters candidates by
  `Get_CanAcceptItem` (which rejects the item's own current inventory), honors `StackingPreference`
  (Prefer / Require / Ignore) and an optional custom sort. Returns an invalid handle if none qualify.
- **`UCk_Utils_Inventory_UE::Request_MassTransfer(AnyHandle, FCk_Request_Inventory_MassTransfer, OnComplete)`**
  — bulk move every (filtered) item out of a set of source inventories into the best-fitting candidate,
  paced over multiple pump passes. Lives on `UCk_Utils_Inventory_UE` (NOT on `UCk_Utils_ItemResolution_UE`,
  whose only member is the per-item `ResolveBestTransferTarget` primitive the churn reuses).
  **Standalone + self-owned:** it spawns a transient-owned op entity — a **plain `FCk_Handle`** (NOT a
  typesafe handle), discriminated by `FFragment_Inventory_MassTransfer_InFlight` — and returns it;
  it is NOT a mixin on an inventory handle. `AnyHandle` is any live handle, used only to resolve
  world/registry context + authority. The
  churn (`FProcessor_Inventory_MassTransfer_Churn`, in `CkInventory_Processor.{h,cpp}`,
  `FGroup_Gameplay`, `RunAfter` both shapes' HandleRequests) resolves one item per pass and submits it
  to the registry-level operation coordinator. The coordinator atomically reserves the source and target, then executes the
  transfer **synchronously** through `inventory_handlers::ExecuteTransferNow` (the same `DoTransfer`
  body the deferred `Request_TransferItem_*` path uses). One operation per participating inventory per pump lets
  deferred stack-count writes fold before the next capacity read — coherent reads, no over-commit.
  `OnComplete` fires once with the result +
  `(UnitsMoved, ItemsFullyMoved, ItemsFailed)`. There is **no public cancel** — a teardown net
  (`FProcessor_Inventory_MassTransfer_CancelOnEndPlay`) fires `Failed_OperationCancelled` if the op
  is destroyed mid-flight (world teardown). The in-flight fragment + completion signal live in
  `CkInventory_Fragment.h`; coordinator state lives under `Inventory/Coordinator/`;
  the paced operation processor lives in `CkInventory_Processor.{h,cpp}`. This is the canonical
  "standalone paced operation" pattern (mirrors CkGoap / CkAudioTrack transient-owned ops).
  **Pump budget:** each merge-step cascades extra pump passes (deferred fold + inventory signal), so the
  request's `_MaxStepsPerFrame` defaults to **1** — at 2+ merge-steps/frame the worst case trips the
  scheduler's pump-count warn (≥8). Measured: at 2 steps/frame the worst case (every item merges into
  one bounded stack) hits **8** pump passes — exactly the warn threshold — and any value >1 also forces
  a churn re-bump pass. Relocate-heavy (non-merging) transfers may raise it, provided the caller
  re-verifies zero pump warns.

  **§4.3 caveat (deferred to Adam's pass):** mass transfer into **≥2 candidate inventories that share
  one lifetime-owner AND shape** inherits the pre-existing multi-same-owner replicated-container
  clobber on clients. The MVP is correct for distinct-owner targets (the common case). This doc is the
  only home for the caveat — the gather site carries no marker.

## Capacity policies

Two orthogonal data knobs on the inventory params govern capacity; both are designer data
(SaveGame + snapshot-serialized), not code wiring:

1. **Bound metric** (DataOnly only) — `ECk_Inventory_DataOnly_BoundMode`:
   - `Unbounded`
   - `BoundedByUniqueEntries` — `_BoundLimit` counts item ENTRIES (record entries); stack counts
     are invisible to it.
   - `BoundedByTotalUnits` — `_BoundLimit` counts the SUM of stack counts (a non-stackable item
     is 1 unit); entry count is unconstrained.
   The limit value lives in the `IntegerAttribute.Inventory.BoundMax` attribute (so capacity
   buffs/modifiers work for both metrics); the METRIC is immutable at creation.
   `Request_OverrideBounds` changes the value only. `Get_EffectiveBoundMode` reports what is in
   effect right now (a runtime override on a declared-Unbounded inventory reads as
   BoundedByUniqueEntries — the legacy interpretation).

2. **Stacking policy** (both shapes) — `ECk_Inventory_StackingPolicy`:
   `UseItemDefinition` (default) / `ClampMaxStackSize` (`_MaxStackSizeClamp`) / `NoStacking`.
   Effective max per stack = `min(item definition max, inventory clamp)`; computed at decision
   time via `Get_EffectiveMaxStackSize` — the item's attribute keeps its definition-level Max
   clamp, because items move between inventories. Threaded through every stack-capacity site:
   `Get_StackRoomFor`, `Get_CanStackItems`, `Request_FillExistingStacks`, `TStackItems`,
   `TAddByDefinition`, `DoTransfer` pre-fill and split, `TSplitStack`.

"6 unique items, stack ≤ 1 each" = `Make_Params_Bounded(6)` + `_StackingPolicy = NoStacking`.

**Pre-fill parity.** `Request_FillExistingStacks` — the pre-fill step behind `AddByDefinition` and
`DoTransfer` — must apply the same gates `Get_CanStackItems` applies on the explicit `StackItems`
path: the trait-level `CanStackWith` check AND the inventory's custom stack hooks. Without both,
pre-fill silently merges items a direct `Request_StackItems` would reject — same-definition items
can still be unstackable when a trait distinguishes runtime state (the Tags trait: VHS rewound vs
NotRewound), and a per-inventory stacking restriction would be enforced on `StackItems` but bypassed
by pre-fill. `InSourceItem` is invalid on definition-driven fills (`AddByDefinition` has no runtime
source to compare) — custom validators must tolerate that.

### Acceptance contract — categorical vs quantitative

Custom acceptance rules split into two hooks with DIFFERENT retry semantics:

- **Categorical** (`_CustomCanAcceptItem*` bool predicate) — "this item never belongs here"
  → `Failed_RejectedByCustomAcceptanceLogic`. Permanent; retry loops should give up.
- **Quantitative** (`_CustomGetAbsorbableUnits*` → int32, MAX_int32 = unconstrained) — "how many
  MORE units can this inventory absorb under custom rules (weight, volume, ...)"
  → `Failed_NoSpaceAvailable`. Transient; retryable, partial amounts meaningful. Composes by
  `min()` with the built-in metrics (can only tighten). Must be a pure function of committed
  state. This is how a game ships weight without a built-in weight metric. **`InItem` may be
  INVALID** — the hook is also called for definition-level planning
  (`Get_AbsorbableUnits(Inventory, Definition)` passes an invalid item); implementations must
  tolerate that.

**`ECk_AddAcceptance::AlreadyValidated`** (transfer path) — `ExecuteTransfer`'s split branch mints a
copy whose OnSplit tag copy is a *deferred* request, so a synchronous categorical check on the copy
would read not-yet-copied tags and wrongly reject it. The categorical question is therefore decided
against the SOURCE (tags committed) — before the source stack is touched, so a rejection needs no
rollback — and skipped on the copy. It skips **only** the categorical recheck: placement /
grid-space / dimension checks still run, and only a *categorical* rejection aborts. A `NoSpace`
result must fall through to the normal partial-transfer path — quantitative capacity is already
clamped into `TransferCount` by `Get_AbsorbableUnits`.

`Get_AbsorbableUnits(Inventory, Definition)` is the planning number: min over (existing-stack
room with effective max, new-entry room, the bound metric's remaining capacity, the custom
quota). Handlers re-derive it at execution time and use budget-decrement accounting within a
drain (their own stack writes are deferred attribute modifiers — re-reading mid-loop would
double-count room).

### Per-definition caps recipe ("max 3 potions in this container")

Not a built-in — express it with the categorical predicate: bind `CustomCanAcceptItem`, count
entries of that definition via `Get_Items` + `UCk_Utils_Item_UE::Get_Definition`, reject at the
cap. For unit-counted caps, use the quantitative quota hook instead (return
`Cap − units of that definition currently held`, MAX_int32 for other definitions).

## Internal layering

`UCk_Utils_Inventory_*_UE` are the public BP / AS surface (UFUNCTIONs only). All shared C++-only mutation, creation, and transfer code lives in `ck::inventory_helpers::` (in `CkInventory_Internal_Helpers.{h,cpp}`) and is consumed by:

- the typed processors (`FProcessor_Inventory_Spatial_HandleRequests`, `FProcessor_Inventory_DataOnly_HandleRequests`, …),
- the typed Utils' `Add` forwarders.

When adding new shared helpers between typed processors, put them in `inventory_helpers::` — never as non-UFUNCTION statics on the public Utils class.

### Helpers worth knowing about

- **`ck::inventory_helpers::CreateInventory(...)`** — the single primitive that materializes an inventory entity, attaches the right type tag (`FTag_Inventory_Spatial` / `FTag_Inventory_DataOnly`), wires up the integer-attribute used for DataOnly bound max, and registers the type-correct replication container fragment. The typed `Add()` UFUNCTIONs forward to this.
- **`ck::inventory_helpers::ExecuteTransfer<TSource, TTarget>(...)`** — the consolidated cross-inventory transfer template. Handles partial transfers, stack splitting, custom-acceptance rejection rollback. Explicitly instantiated for the four direction combinations (each cpp-side `template CKINVENTORY_API auto ExecuteTransfer<...>` in `CkInventory_Internal_Helpers.cpp`):
  - `ExecuteTransfer<FCk_Handle_Inventory_Spatial,  FCk_Handle_Inventory_Spatial>`
  - `ExecuteTransfer<FCk_Handle_Inventory_Spatial,  FCk_Handle_Inventory_DataOnly>`
  - `ExecuteTransfer<FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory_Spatial>`
  - `ExecuteTransfer<FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory_DataOnly>`

  Adding a new inventory type means adding the four corresponding instantiations alongside the new typed handle / Utils / processors. There is no other shared dispatch site to find.

- **`ck::inventory_helpers::DispatchEnqueue<TSignal>(InInventory, InRequest, InDelegate, Context, EnqueueFn)`** — the dispatcher used by every `Request_*` method on `UCk_Utils_Inventory_UE`. Performs auth check + signal bind, then runtime-branches on the inventory's shape tag (Spatial / DataOnly) and hands the typed handle to a generic-lambda `EnqueueFn`. The lambda body resolves `TFragment_Inventory_Requests<TShape>` via `decltype(Typed)` and constructs the correct typed entry. The shape-branch lives **only here at the public Utils boundary** — processors, per-shape `DoHandleRequest` overloads, and `ExecuteTransfer<...>` stay typed at compile time.
- **`ck::TFragment_Inventory_Requests<TShape>`** — primary template forward-declared in `CkInventory_Fragment.h`. Explicit specializations in each shape's `*_Fragment.h` define the per-shape `*Entry` typedefs (each is `TInventory_RequestEntry<TBaseRequest, TAddon>`) and the `std::variant` alternatives.
- **`ck::TInventory_RequestEntry<TBaseRequest, TAddon>`** — internal-only carrier (no UHT reflection). Pairs a public base-request USTRUCT with an optional shape-specific addon (`FCk_SpatialPlacement` for Spatial AddItem/SplitStack; `FCk_EmptyAddon` everywhere else). Variant alternatives are concrete distinct types per shape; anti-pattern #5's slicing concern doesn't apply.
- **`ck::inventory_helpers::DispatchRequests(InHandle, RequestsFragment, Visitor)`** — drains the per-tick `std::variant` of typed requests via `ck::Visitor`. Used by the templated processor base.
- **`ck::inventory_helpers::ApplyReplicatedEntryDiff(...)`** — diffs incoming replicated entries against the previous snapshot using `ck::algo::Except` with a `&TEntry::Get_ItemHandle` projection. **Per-entry order is load-bearing for replication correctness.**
- **`ck::TRequestResultGuard<TSignal>` / `ck::MakeRequestResultGuard<TSignal>`** — RAII guard that fires the per-request completion signal at scope exit using a payload-builder lambda that captures the local `Result`. Declare it after the locals it captures.

### Request handling — handler templates + per-shape Traits bundle

Mirrors CkAttribute's pattern: templated processor in the root, per-shape folder supplies the constructs the template needs.

- **`CkInventory_RequestHandlers.h`** (root) — declares one `TXxx<TInventoryHandle, TAddon = FCk_EmptyAddon>` struct per request operation (`TAddItem`, `TRemoveItem`, `TStackItems`, `TSplitStack`, `TAddByDefinition`, `TSort`, `TTransfer<TSource, TTarget>`, `TRelocate`). Each carries `Entry`, `Result`, and a static `Handle(...)` method. Bodies live in `CkInventory_RequestHandlers.cpp` with explicit instantiations per concrete (Handle, Addon) pair.
- **`TInventoryRequestTraits<TInventoryHandle>`** — primary template declared in the root header, **specialized in each typed inventory's folder** (`Spatial/CkInventory_Spatial_RequestTraits.h`, `DataOnly/CkInventory_DataOnly_RequestTraits.h`). Each specialization aliases the operations its shape supplies (e.g. `using AddItem = inventory_handlers::TAddItem<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>`) and exposes a `Variant` typedef built from those operations' `Entry` types. **Adding a new typed inventory is a single file.**
- **`TFragment_Inventory_Requests<TInventoryHandle>::RequestType`** is `Traits::Variant`. The variant is auto-derived — the trait bundle is the single source of truth for "what does this inventory shape support."
- **`TProcessor_Inventory_HandleRequests_Base<T_Derived, TInventoryHandle, TRequestsFragment>`** in `CkInventory_Processor.h` — typed request intake. It follows the standard CK copy/reset/drain pattern, converts each typed entry into a coordinator submission, and removes the live request fragment only when callback re-entry did not repopulate it.
- **`FProcessor_Inventory_OperationCoordinator_HandleRequests`** under `Inventory/Coordinator/` — registry-level arbiter hosted on the transient entity. Public request calls reserve a monotonic ordinal; the coordinator preserves the earliest operation per source, admits candidates by ordinal, and atomically reserves every source/target participant for the whole pump. Conflicting work waits for the next pump while disjoint operations may execute together. Its callback batch is removed from coordinator state before dispatch. Ordinary participant teardown cancels exactly once; an internal MassTransfer step clears its submitted flag and re-resolves on the next churn pass.
- **Concrete processor classes** (`FProcessor_Inventory_Spatial_HandleRequests`, `FProcessor_Inventory_DataOnly_HandleRequests`) are thin derived classes — Group/RunAfter/MarkedDirtyBy declarations only; body inherited from the templated base.
- **Shared algorithmic bodies** (`inventory_helpers::DoStackItems<T>`, `DoSplitStack<T>`, `DoAddByDefinition<T>`) are templated functions in `Internal_Helpers.cpp` with explicit instantiations per shape. Shape divergence inside them uses typed-overload helpers (`Stack_OnSourceFullyConsumed`, `SplitStack_TryPlace`, `AddByDefinition_TryPlace`) — overloaded per shape, no captured lambdas. Sort and Relocate bodies are too shape-divergent to template usefully and are defined per-shape directly.

Architectural reference: `TProcessor_AttributeModifier_Compute` + `TAttributeMinMax` in CkAttribute use the same templated-base + per-derived-supplies-constructs pattern.

### Adding a new request type

1. Define the public `FCk_Request_Inventory_X` USTRUCT in `CkInventory_Fragment_Data.h` (BP / AS surface).
2. Declare a `TX<TInventoryHandle, TAddon = FCk_EmptyAddon>` handler struct in `CkInventory_RequestHandlers.h` (`Entry` / `Result` / static `Handle`).
3. Define `TX::Handle` in `CkInventory_RequestHandlers.cpp` — body sets up the result guard, delegates to a shared body in `inventory_helpers::DoX` (templated on `TInventoryHandle`, with typed-overload divergence helpers for shape-specific steps), returns. Add explicit instantiations for each `(Handle, Addon)` pair the framework uses.
4. Add the shared body to `Internal_Helpers` — templated on `TInventoryHandle` (then explicit-instantiate per shape), or per-shape overloads if the bodies don't share a structure.
5. Add a branch to `inventory_handlers::DispatchToHandler` matching the new operation's `Entry` type.
6. Alias the new operation in **each typed inventory's** `RequestTraits.h`, and extend the Traits' `Variant` typedef.
7. Add a `Request_X` UFUNCTION in `CkInventory_Utils.cpp` that uses `inventory_helpers::DispatchEnqueue` to construct the entry on the typed handle.

### Adding a new typed inventory shape

1. Create `<Shape>/CkInventory_<Shape>_Fragment_Data.h` with the typed handle + any shape-specific request structs/addon types.
2. Create `<Shape>/CkInventory_<Shape>_RequestTraits.h` specializing `TInventoryRequestTraits<FCk_Handle_Inventory_<Shape>>` with the operations the shape supports + the resulting `Variant`. **This is the explicit contract — missing operations are caught at compile time when the templated processor instantiates.**
3. Create `<Shape>/CkInventory_<Shape>_Fragment.h` declaring `TFragment_Inventory_Requests<...>` (delegates to Traits) and any shape-specific replication fragments.
4. Create `<Shape>/CkInventory_<Shape>_Processor.{h,cpp}` — thin `FProcessor_Inventory_<Shape>_HandleRequests` derived from `TProcessor_Inventory_HandleRequests_Base<...>` + Sync/Replicate processors.
5. Create `<Shape>/CkInventory_<Shape>_Utils.{h,cpp}` for `Make_Params`, `Add` / `AddMultiple`, queries, and any shape-only `Request_*` operations.
6. Update `CkInventory_Internal_Helpers.{h,cpp}` to add typed-overload helpers for the new shape (`Stack_OnSourceFullyConsumed`, `SplitStack_TryPlace`, `AddByDefinition_TryPlace`, `DoAddItem`, `DoRemoveItem`, etc.) and explicit instantiations of the templated bodies (`DoStackItems<NewShape>`, etc.).
7. Add the four `ExecuteTransfer<TSource, TTarget>` instantiations involving the new shape.

### Known gap — Relocate-on-DataOnly

`FCk_Request_Inventory_Spatial_RelocateItem` is currently Spatial-only. The DataOnly Traits bundle intentionally omits the `Relocate` alias; the dispatcher SFINAE-detects this and skips the Relocate branch for DataOnly. DataOnly inventories should support slot reordering as the analog operation. Adding it: rename the request to a shape-agnostic `FCk_Request_Inventory_RelocateItem` (or define a DataOnly-specific variant), wire it into the DataOnly Variant, alias `Relocate` in DataOnly's Traits, and define `DoRelocate(FCk_Handle_Inventory_DataOnly&, ...)` in `Internal_Helpers`. Once DataOnly declares `Relocate`, the dispatcher's SFINAE check will activate that branch automatically.

## Replication

- **Per-shape RepData**: `FCk_RepData_Inventory_Spatial_Items` carries `FIntPoint Coordinate` + `ECk_CardinalRotation Rotation` per entry; `FCk_RepData_Inventory_DataOnly_Items` skips both fields. The split exists so DataOnly entries don't pay for spatial-only data on the wire.
- **Registration**: each shape's `*_Fragment.cpp` registers on `FCk_PersistenceHandlerRegistry` via a named `Register_NetAndSave_*` shape (Spatial uses `Register_NetAndSave_SplitApply` — distinct net vs load appliers). If a `SyncReplication` processor never fires on clients, that registration is the first place to look.
- **Containers** are added on the *outer* (lifetime-owner) entity by `CreateInventory`, not on the inventory entity itself. Items themselves replicate through standard entity replication.

### DataOnly contents save opt-out

`FCk_Fragment_Inventory_DataOnly_ParamsData::_PersistContents` defaults to `Enable`. Set it to `Disable` for a DataOnly inventory whose contents are reconstructed session state: snapshot Produce emits no item payload, and HydrationApply ignores an older item payload against the rebuilt opted-out inventory. This is save transport only; live replication still publishes the current item projection normally.

## Known invariant — typed ParamsData duplication

USTRUCTs cannot inherit cleanly with UHT reflection. As a result, the shared field set (`_Name`, `_CustomCanAcceptItem*`, `_CustomCanStackItems*`, `_CanAcceptItemRef`, `_CanStackItemsRef`) is duplicated by hand across three structs:

- `FCk_Fragment_Inventory_ParamsData`           — internal-only ECS fragment (no `BlueprintType`); constructed exclusively from the typed structs via the two conversion ctors `FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_DataOnly_ParamsData&)` and `FCk_Fragment_Inventory_ParamsData(const FCk_Fragment_Inventory_Spatial_ParamsData&)`. Stored on the inventory entity by `CreateInventory`.
- `FCk_Fragment_Inventory_Spatial_ParamsData`   — Spatial public surface (BP / AS); adds `_Dimensions`.
- `FCk_Fragment_Inventory_DataOnly_ParamsData`  — DataOnly public surface (BP / AS); adds `_BoundMode` + `_BoundLimit`.

When adding / removing / renaming a shared field, **update all three in lockstep** — both typed structs *and* the conversion ctor bodies in `CkInventory_Fragment_Data.cpp` that copy the shared field set into the base struct. Each struct carries a comment block at its declaration pointing to the others.

The `Add()` forwarders on the typed Utils are one-liners over `CreateInventory(InOwnerEntity, FCk_Fragment_Inventory_ParamsData{InTypedParams}, ...)` — the field-by-field copy is owned by the conversion ctor, not duplicated at the call sites. The corresponding `AddMultiple` UFUNCTIONs accept `FCk_Fragment_MultipleInventory_DataOnly_ParamsData` / `FCk_Fragment_MultipleInventory_Spatial_ParamsData` — typed array wrappers (mirroring CkAttribute's `FCk_Fragment_MultipleIntegerAttribute_ParamsData` precedent).

## Anti-patterns

1. **Don't store item data in the inventory fragment** — item state lives on the item entity's fragments. The inventory only holds a Record of item handles.
2. **Don't transfer items by copying fragment data** — use `inventory_helpers::ExecuteTransfer` (or the base `Request_TransferItem_*` UFUNCTIONs that wrap it).
3. **Don't add non-UFUNCTION statics to `UCk_Utils_Inventory_*_UE`** — those classes are the public BP / AS surface. C++-only helpers go in `ck::inventory_helpers::`.
4. **Don't widen a processor's `HandleType` to `FCk_Handle_Inventory` (the base)** — that re-introduces the umbrella-era runtime branching the spatial/data-only split was created to eliminate. Per-request behavior lives in `inventory_handlers::TXxx::Handle` (templated, instantiated per concrete `(TInventoryHandle, TAddon)` pair via the per-shape Traits bundle); shared algorithmic bodies are templated functions in `Internal_Helpers` parameterized on the typed handle. Shape divergence inside a shared body uses typed-overload helpers in `Internal_Helpers`, not captured hook lambdas. **The Utils-boundary shape-branch in `DispatchEnqueue` is permitted** — public BP/AS surface only, dispatches *to* the typed enqueue path.
5. **Don't put a public base-request USTRUCT directly into a typed `std::variant` alternative** — derived USTRUCTs slice. Wrap public base requests in `TInventory_RequestEntry<TBaseRequest, TAddon>` (internal, non-reflected) so each typed shape's variant carries C++-distinct alternatives per template instantiation. The carrier preserves slicing-safety while letting the public surface share base USTRUCTs across shapes.

## Implementation notes

- **Multi-value results come back by value in a small USTRUCT**, never through pointer out-params
  (house convention, cf. `FCk_SpatialPlacementResult`) — e.g. `FCk_FillExistingStacksResult` bundles
  the filled count with the last stack that received quantity.

### ItemQuery

Item definitions are world-agnostic data assets, so `UCk_ItemQuery_Subsystem_UE` is an
**EngineSubsystem**: it builds the definition index once and shares it across worlds — this replaced
a per-call AssetRegistry scan + synchronous load. Editor invalidation is driven by AssetRegistry
add/remove/rename events plus `FCoreUObjectDelegates::OnObjectPropertyChanged` (a designer editing a
definition's traits). `FProcessor_ItemQuery_HandleRequests` caches the subsystem at construction
(resolved by the registration factory) rather than looking it up per entity — the same shape as
`FProcessor_Probe_*` caching the `JPH::PhysicsSystem`. A completed query broadcasts the result signal
and destroys the request entity.

`UCk_Utils_ItemQuery_UE::Request_QueryItemDefinitions` mirrors
`UCk_Utils_RenderStatus_UE::Request_QueryRenderedActors`: querying is ALWAYS deferred through the
request — there is no synchronous getter — so design-time / non-ECS callers cannot run it.

`FCk_ItemQuery_Filter`'s two custom-predicate flavours mirror the `CustomCanAcceptItem` pair above: a
native C++ delegate (not reflected, C++ callers only) and a BP/AS-bindable dynamic delegate that
reports through a `bool&` out-param because dynamic delegates cannot return a value. A definition
matches when it has ALL of `RequiredAll`, at least one of `RequiredAny`, NONE of `Excluded`, and
passes every bound predicate.

---

## See also

- `CkAttribute/Claude.md` — the canonical "homogeneous CkFoundation feature module" reference, including how typed handles, thin processor wrappers, and shared template logic compose.
- `CkGrid/Claude.md` — grid cell management; Spatial inventories use a `Ck2dGridSystem` per inventory entity.
- `CkTagSet/Claude.md` — item tag filtering.
- `CkRecord/Claude.md` — Record-of-entities pattern used for both `RecordOfInventories` (owner → inventories) and `RecordOfInventoryItems` (inventory → items).
