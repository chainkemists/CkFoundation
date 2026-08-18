# CkDynamic

**Purpose:** Dynamic behavior system — runtime-composable behaviors attached to entities via data assets. Provides a type-erased struct dispatch mechanism (`FScriptStructWildcard`) so behavior definitions can be content-authored.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** `CkStateMachine` (state behaviors), `CkAngelscriptGenerator`, `CkK2Nodes`, `CkDynamicEditor`.

---

## Key API

- `UCk_Utils_Dynamic_UE` — add, remove, query dynamic behaviors on entities.
- Behaviors are defined as `UDataAsset`-derived types and referenced by `FScriptStructWildcard`.

---

## Pattern

Designers create behavior data assets; the dynamic system applies them to entities at runtime. State machines use this to swap behaviors on state transitions.

---

## Anti-patterns

1. Don't hardcode behavior selection in C++ where a data asset would suffice.
2. Don't mix Dynamic behavior data with Fragment data — they're different abstraction layers.

---

## Implementation notes

Rationale relocated out of the source during the 2026-07-25 comment sweep. The code is deliberately
comment-light; the *why* lives here.

### Handle-type registry (`CkDynamic_AngelScript.cpp`)

- **Three independent registry sources, none fatal.** A missing or unparsable canonical
  `DynamicHandleTypes.json` continues the load: the sibling stub file and the per-plugin TESTONLY
  files are merged on top and either can carry types on its own.
- **Stub recovery** is a sibling `_StubRecovery_<canonical>.json`, written by the AS-failure
  dispatcher so the canonical file stays byte-clean from HEAD, and deleted by the PostCompile hook.
- **TESTONLY merge iterates plugin directories rather than asking `IPluginManager`** — AS PreCompile
  runs off the game thread in packaged builds, where the plugin manager is not safe. The
  `!UE_BUILD_SHIPPING && !UE_BUILD_TEST` gate is load-bearing: test plugins are disabled in those
  configs, so their TESTONLY types have no C++ backing and registering them yields `asINVALID_TYPE`
  across every handle mixin.
- **F-prefix candidate lookup:** the registry stores the F-stripped `UScriptStruct` name while the
  AngelScript type database is keyed on the F-prefixed script name, so a bare lookup misses by
  construction for every AS-declared fragment.

### Dynamic-pool cache (`CkDynamic_Utils.cpp`)

- **`Get_AllFragments` sweeps a registry-ctx cache of the dynamic pools**
  (`ck_dynamic_utils::FCtx_DynamicFragmentPools`), not the registry's full pool list — the save capture's
  blanket `Produce` calls it once per entity, and the full walk measured as the dominant produce cost.
  Staleness is the registry's pool COUNT: pools are only ever appended, so an unchanged count proves the
  cached list is current, and the count check is one iterator subtraction. If pool DISCARDING is ever
  introduced (`registry.storage().discard`-style), the count key still catches it unless a discard and a
  create land in the same interval — re-key the cache before adopting such an API. A first attempt cached
  global storage IDs and probed them per entity; the per-id hash lookups measured SLOWER than the linear
  walk they replaced (produce 37ms → 45ms) — don't resurrect that shape.

### Save / hydration (`CkDynamic_Fragment.cpp`, `CkDynamic_Fragment_Data.h`)

- **`FCk_SaveData_DynamicFragments` exists because the net path is invisible to the save census.**
  Dynamic fragments replicate through a single runtime-typed fallback handler, which
  `Get_SaveHandlerTypes` (per-type handlers only) structurally cannot see; this wrapper is the
  per-type handler that makes them participate in the save.
- **`HydrationApply`-only, never the net `Apply` slot** — applying on a client would race
  construct-time composition. Dynamic fragments have no structural composition step (they exist iff
  they hold data), so hydration has nothing to wait on: always `Applied`, never `NotReady`.
- The post-write broadcast is **`Hydration_OnTypeHydrated`, NOT `OnRepNotify`** (`CkEcs/Persistence/CkPersistenceHydration.h`).
  It mirrors the net fallback's shape — one edge per committed type, after every value in the entity's
  payload set is written — but names the event that occurred. A load is not replication, and while both
  used the same signal a consumer binding `OnRepNotify` for wire updates silently received load edges
  too. `CkDynamic_Module.cpp`'s net broadcast keeps `OnRepNotify`; `BindTo_OnRepNotify` therefore binds
  the NET edge only. In Gate 01 `Hydration_OnTypeHydrated` is a C++ signal with no `BindTo_*` UFUNCTION —
  nothing binds it yet, and the per-ENTITY `Promise_OnHydrated` is the hook a consumer wants in almost
  every case.
- The re-arm payload **carries no per-type replication flag**, so a fragment the rebuild did not
  re-register as replicated stays local-only.
- **Posture decides what is captured, and it is resolved from the TYPE, once.** `Produce` and
  `HydrationApply` both call `ck::Get_FragmentPosture` (`CkEcs/Snapshot/CkSnapshot_Posture.h`) and
  branch three ways: `Session` is skipped on both sides; `Durable` is captured whole and assigned
  whole; `Undeclared` keeps the pre-posture behaviour — captured, and copied back field-wise through
  the live-session guard below. The markers (`FCk_Snapshot_Durable` / `FCk_Snapshot_Session`) live in
  CkEcs, which CkDynamic already depends on, so the registered-handler path reads the same
  declaration; `FCk_DynamicFragment_SnapshotTransient` remains as the deprecated Session spelling for
  the script sites that still carry it. Structs *derive* a marker in C++ and carry it as a FIELD in
  AngelScript (script structs cannot inherit) — the resolver accepts either. Do not use USTRUCT
  metadata for this contract: Game and cooked targets strip metadata behind `WITH_METADATA`.
- **`Get_IsLiveSessionField` — the `Undeclared` path, and only that path.** For a fragment nobody has
  declared, the field-wise copy preserves every `CPF_Transient` field and every top-level delegate /
  multicast-delegate field, with no opt-in. A delegate binds UObject + FunctionName, so across a
  rebuild+hydrate its saved form is stale or empty *by construction* (it named objects in the
  torn-down world) while the live value is the set of subscribers that bound during the rebuild.
  Copying the saved list over them silently unsubscribes everyone, and the feature then keeps correct
  state and correct one-shot reads while never notifying again — **"correct initial value, then
  frozen"** (BusterBlock 2026-08-16: a HUD wallet showing the right balance that never moved;
  interact prompts and key hints that never re-appeared after a load). Found across 46 unmarked BB
  `_Signals` fragments. **Limit — top-level fields only.** A delegate nested in a struct- or
  array-typed member is copied wholesale, because "preserve the live one" is undefined once the saved
  and live arrays differ in length. That limit is why the posture resolver's delegate derivation
  walks the type at ANY depth instead: a delegate-carrying fragment is `Session` and is never copied
  at all, so the guard is the transitional backstop for undeclared fragments rather than the rule.
  It dies with the last `Undeclared` fragment.
- **`Restore_UnresolvedHandles` — hydration never DOWNGRADES a live handle.** Reached on the
  `Undeclared` path only, for the same reason. A fragment
  field naming a construct-derived child (SceneNode, probe, Interactable, UnrealComponent, Tween) is
  written fresh by the owner's replayed construction, but the SAVED value cannot resolve: those
  children are unlabeled `FTag_ConstructSpawned` entities, which capture rule 3 classifies
  save-transient, so their saved id remaps to a tombstone. The whole-fragment copy then replaced a
  working handle with a dead one and the feature came back structurally complete but functionally
  inert — only destroying and respawning the owner fixed it (BusterBlock 2026-08-16: a poster that
  focused but never changed texture; a checkout counter whose settle offered no cash/credit option).
  After the copy, any destination handle that is now invalid where the pre-copy value was valid is
  restored. It stands down entirely when the pre-copy and post-copy walks visit different numbers of
  handle slots, since a saved array of a different length shifts every later slot and positional
  correspondence no longer holds. **Known trade:** a field the save deliberately CLEARED is
  indistinguishable from one whose target failed to resolve (both arrive invalid), so a cleared field
  keeps its construction-fresh value — holding a live handle is recoverable, holding a dead one is the
  inert-feature bug. A field whose emptiness is load-bearing must persist that fact explicitly rather
  than encode it as an invalid handle. Both diagnostics log at **Warning**, not Verbose: every
  firing is a fragment whose posture has not been declared, so it is a work item rather than trace
  noise, and a backstop that engages silently tells nobody. The whole function is scheduled for
  deletion once no fragment resolves `Undeclared` any more — it exists to carry the undeclared ones
  across, not to be a permanent safety net.

### Cooked display schema (`CkDynamic_FragmentDisplaySchema.*`)

- **Debugger-facing labels cannot depend on reflected metadata in Game targets.** USTRUCT,
  UPROPERTY, and enum `DisplayName` metadata is stripped with editor-only data, so the runtime
  registry owns fragment, property, and enum labels as values keyed by stable type paths, authored
  property names, and numeric enum values.
- **AngelScript descriptors are the source of truth for script-authored labels.** First-use
  observation captures only fragments that actually enter dynamic storage; PostCompile atomically
  replaces the AngelScript-owned generation so stale hot-reload objects are never retained.
- **The initial PostCompile is the startup publication point.** CkDynamic loads before the
  PostDefault AngelScript loader initializes its manager, so module startup subscribes without
  reading the manager. A late module reload may refresh immediately only when the manager already
  exists.
- **Native producers own native custom labels.** They register an explicit schema during startup and
  unregister it during shutdown. Native entries take precedence and are not erased by an
  AngelScript refresh.
- **Display metadata is diagnostic, never gameplay admission.** Failure to observe a display schema
  emits an ensure but must not prevent `Get_StorageId` from computing and caching the fragment's
  storage ID.

### Script-processor host (`CkDynamic_ScriptProcessor_Host.*`)

- **Lives in CkDynamic, not CkEcs**, because descriptor construction needs CkDynamic's
  `UScriptStruct` → storage bridge and CkEcs must never depend on CkDynamic. Same reason
  `UCk_Utils_DynamicFragment_UE::Has_AnyEntityWith_Fragment` exists: it lets the scheduler wrapper
  gate on `MarkedDirtyBy` from the CkDynamic side.
- **Ordering edges are filled in `DoFillCommon`, on both the typed and direct paths**, so a processor
  keeps identical scheduler edges through the two-pass window where its driver is not yet generated.
- **A partially admitted query never publishes hashes.** `Configure` is an imperative sequence; if any
  call failed, the surviving slots do not describe the author's intent, so publishing them would
  advertise a weakened scheduler contract. The factory is still installed — the hosted processor
  repeats validation and disables itself.
- The descriptor's dirty-marker hash **must be the same key the mutation paths bump**
  (`Get_DirtyMarkerHash`); a divergent domain leaves the node pump-deaf.
- **"No driver yet" is a one-compile window, not a loss.** A driver authored this compile is generated
  in the same PostCompile, whose file-write triggers the recompile that finds it.

### Hosted processor and batch (`CkDynamic_ScriptQuery*`)

- **One native N-way join per tick, then ONE `ForEachBatch` call** — O(1) native→script crossings per
  tick instead of one per visited entity. That is the whole reason the hosted wrapper exists.
- **Batch lifetime:** a process-lifetime resolver maps (opaque state pointer + generation) to the
  native state for the duration of `ForEachBatch` and drops the mapping before the call returns, so a
  stashed batch can neither dereference freed state nor alias a later state at the same address.
  Generations are process-unique; the resolver is intentionally never freed so it takes no part in
  world teardown or static destruction ordering.
- **Query mixin mutators** reject a duplicate fragment type across all slots (keep-first + ensure) and
  an invalid type; `NoEntities` requires an empty slot list.

---

## See also

- `CkStateMachine/Claude.md` — primary consumer.
- `CkEcsExt/Claude.md` — Meta Fragment infrastructure.
