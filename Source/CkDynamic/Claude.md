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

### Save / hydration (`CkDynamic_Fragment.cpp`, `CkDynamic_Fragment_Data.h`)

- **`FCk_SaveData_DynamicFragments` exists because the net path is invisible to the save census.**
  Dynamic fragments replicate through a single runtime-typed fallback handler, which
  `Get_SaveHandlerTypes` (per-type handlers only) structurally cannot see; this wrapper is the
  per-type handler that makes them participate in the save.
- **`HydrationApply`-only, never the net `Apply` slot** — applying on a client would race
  construct-time composition. Dynamic fragments have no structural composition step (they exist iff
  they hold data), so hydration has nothing to wait on: always `Applied`, never `NotReady`.
- The post-write RepNotify broadcast **mirrors the net fallback's RepNotify** (`CkDynamic_Module.cpp`)
  so a bound `OnRepNotify` handler re-runs against the restored value.
- The re-arm payload **carries no per-type replication flag**, so a fragment the rebuild did not
  re-register as replicated stays local-only.
- Snapshot-transient dynamic structs derive from `FCk_DynamicFragment_SnapshotTransient` (C++) or
  carry a field of that type (AngelScript — script structs cannot inherit); capture and hydration
  identify them through runtime `IsChildOf` / marker-typed-property scan. Do not use USTRUCT
  metadata for this contract: Game and cooked targets strip metadata behind `WITH_METADATA`.

### Cooked display schema (`CkDynamic_FragmentDisplaySchema.*`)

- **Debugger-facing labels cannot depend on reflected metadata in Game targets.** USTRUCT,
  UPROPERTY, and enum `DisplayName` metadata is stripped with editor-only data, so the runtime
  registry owns fragment, property, and enum labels as values keyed by stable type paths, authored
  property names, and numeric enum values.
- **AngelScript descriptors are the source of truth for script-authored labels.** First-use
  observation captures only fragments that actually enter dynamic storage; PostCompile atomically
  replaces the AngelScript-owned generation so stale hot-reload objects are never retained.
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
