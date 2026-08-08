# O11 — Does CkRecord guarantee iteration order?

Read-only investigation, 2026-08-08. No design decisions, no code changed.
Every claim below is labelled **VERIFIED** (code read at the cited line) or **INFERRED** (reasoned,
not traced to source).

---

## Verdict

### `ORDERED-TODAY-BUT-INCIDENTAL`

A Record's entries live in a plain `TArray` inside one fragment, insertion appends to the tail, and
both explicit removal paths are order-preserving — so a record that is only ever appended to and
explicitly disconnected from *does* iterate in insertion order today. But the guarantee is not
contractual and not total: the **shared iteration path itself contains an order-destroying
`RemoveAtSwap` lazy prune** (`CkRecord_Utils.h:825`, `:870`) that fires whenever an entry's entity
has been destroyed from the registry without its disconnection callback having run for that record.
That prune is a hidden side effect of a *read*, its trigger is invisible to the caller, and it
permutes the array by moving the tail element into the hole. The word "order" appears **zero times**
in the entire CkRecord module (VERIFIED: `rg -i order Source/CkRecord/` → no matches), there is no
comment, doc line, or test asserting the property, and `TUtils_RecordOfEntities::Sort`
(`CkRecord_Utils.h:1110`) exists precisely because ordering is treated as something callers impose,
not something the record maintains. Two consumers already lean on insertion order de facto
(CkStateMachine explicitly, CkInventory transitively), but they do so on their own authority — nothing
in CkRecord backs them.

**Entt pool traits are irrelevant to this question.** `CkHandle.h:69-75`'s global
`entt::component_traits<Type>::in_place_delete = true` governs how the *record fragment* is stored in
its pool. Record *entries* are a `TArray<EntityType>` member inside that fragment
(`CkRecord_Fragment.h:41,47`), iterated by index (`CkRecord_Utils.h:817`, `:861`) — no pool, no view,
no entt iteration is involved in entry ordering at all. **VERIFIED.**

---

## Mechanism walk-through

### Storage shape — a `TArray` in one fragment

`Source/CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h:41,47` — **VERIFIED**

```cpp
using RecordEntriesType = TArray<EntityType>;   // :41   EntityType = the (maybe typesafe) handle
...
// mutable because we lazily remove entries when performing a ForEach
mutable RecordEntriesType _RecordEntries;       // :47
```

`EntityType` is `T_HandleType` (`:39-40`), i.e. `FCk_Handle` or an `FCk_Handle_TypeSafe` subclass —
entity id + registry handle by value. A parallel `TArray<TPair<FName, EntityType>>
_RecordEntriesTagNamePairs` (`:43,48`) is the label index; it is a *separate* array maintained
imperatively and is not the ordering authority.

The `mutable` on `_RecordEntries` is load-bearing and is the tell: the const iteration overload
mutates the array through it (`CkRecord_Utils.h:870`).

There is exactly **one** record storage variant. `CK_DEFINE_RECORD_OF_ENTITIES`,
`_ROUNDTRIP`, and `_TRANSIENT` (`CkRecord_Fragment.h:88-98`) all expand to the same
`ck::TFragment_RecordOfEntities<T>` base; the latter two are documented inert aliases
(`Source/CkRecord/CLAUDE.md`, "Implementation notes"). **No STOP condition — the question is
well-posed.** **VERIFIED.**

### Insertion — append to tail

`CkRecord_Utils.h:1007` — **VERIFIED**

```cpp
RecordFragment._RecordEntries.Emplace(InRecordEntry);
```

`Request_Connect` is the only append site. It is preceded by a duplicate guard
(`:948-957`, `Contains` + ensure + early `Succeeded`), so an entry can appear at most once per record.
The label-pair array is appended separately at `:982`. Insertion is therefore strictly
chronological: index N = the N-th successful `Request_Connect`. **VERIFIED.**

### Removal — three paths, two order-preserving, one not

| # | Path | Call | Order? |
|---|---|---|---|
| 1 | Explicit disconnect | `CkRecord_Utils.h:1059` — `_RecordEntries.RemoveSingle(InRecordEntry)` | **preserves** |
| 2 | Entry-destroy cleanup | `CkRecord_Utils.h:1018` — `_RecordEntries.Remove(...)` inside the disconnection lambda registered at `:1015-1019`, invoked by `FProcessor_RecordEntry_Destructor::ForEachEntity` (`CkRecordEntry_Processor.cpp:24-40`, `FGroup_EndPlay`) | **preserves** |
| 3 | **Lazy prune during iteration** | `CkRecord_Utils.h:825` (mutable overload) and `:870` (const overload) — `RemoveAtSwap(Index); --Index;` | **DESTROYS** |

Paths 1 and 2 use UE's shift-down removals; path 3 swaps the last element into the hole.
`RemoveSingle`/`Remove` being order-preserving and `RemoveAtSwap` being swap-with-last is the
documented UE `TArray` contract — **INFERRED** (I did not open `Engine/Source/Runtime/Core/Public/
Containers/Array.h`; per project convention I do not grep the engine tree uninstructed). The same
prune idiom with the same intent appears elsewhere in the codebase, e.g.
`CkIskmRenderer/.../CkIskm_BatchedCrowd_Actor.cpp:437` (`Cosmetics.RemoveAtSwap(Idx); continue;
// cosmetic destroyed — prune`). **VERIFIED** that the CkRecord sites are `RemoveAtSwap`.

### Iteration — index walk, with the prune inline

`CkRecord_Utils.h:817-842` (mutable) and `:861-888` (const) — **VERIFIED**

```cpp
for (auto Index = 0; Index < RecordEntries.Num(); ++Index)
{
    const auto RecordEntryHandle = ck::MakeHandle(RecordEntries[Index], InHandle);

    if (ck::Is_NOT_Valid(RecordEntryHandle, T_ValidationPolicy{}))
    {
        if (ck::Is_NOT_Valid(RecordEntryHandle, ck::IsValid_Policy_IncludePendingKill{}))
        {
            RecordEntries.RemoveAtSwap(Index);   // :825  ← order destroyed here
            --Index;
        }
        continue;
    }
    ...
}
```

`DoForEach_Entry` is the single iteration primitive. **Every** public read funnels through it or
through a direct copy of `_RecordEntries`:

- `ForEach_Entry` / `ForEach_ValidEntry` / `*_If` (`:645-797`) → `DoForEach_Entry`.
- `Get_Entries` (`:358-359`) copies `Fragment.Get_RecordEntries()` in array order, then appends each
  EntityExtension sub-record's entries (`:361-365`) — own entries first, extensions after.
- `Get_ValidEntries` (`:378-383`) / `Get_ValidEntries_If` (`:404-409`) build on `Get_Entries` and
  filter with `ck::algo::Filter` (order-preserving) — so their output order is `Get_Entries`' order.
- `Get_ValidEntry_If` (`:555-565`) and `Get_ValidEntry_ByTag` (`:605-617`) walk the arrays directly
  and return the **first** match — first-in-array-order, i.e. these are already order-sensitive APIs.

Note the asymmetry: the *copy-out* readers (`Get_Entries` and friends) do **not** prune, so they
neither destroy nor repair order; only the `ForEach_*` family mutates. **VERIFIED.**

---

## Removal / slot-reuse behaviour specifically

**When the swap-prune fires.** The prune is gated on `Is_NOT_Valid(handle,
IsValid_Policy_IncludePendingKill{})`, which resolves to "registry slot gone, **or** `registry.valid(entity)`
is false" (`CkHandle.cpp:215-226`, VERIFIED). That is strictly stronger than the default policy
(`CkHandle.cpp:204-211`), which additionally excludes entities in the Teardown/Destroyed destruction
phases. So a pending-destroy entry is *skipped* but *kept* (order intact); only an entry whose entity
has actually left the registry is swap-pruned.

**Ordinary destruction does not trip it.** `FProcessor_RecordEntry_Destructor` runs in
`FGroup_EndPlay` (`CkRecordEntry_Processor.h:18`), two ticks before `FGroup_DestructionPipeline`
finalizes the entity (`CkEcs/CLAUDE.md`, "Destruction pipeline, tick by tick"). So in the normal
case the order-preserving `Remove` at `:1018` has already unlinked the entry before it could ever
read as fully invalid. The prune is a safety net. **VERIFIED** (group + tick timeline read; the
combined timing claim is **INFERRED**).

**But the net is reachable.** Three routes, in descending confidence:

1. **`_DisconnectionFuncs` is keyed by record ENTITY, not by record TYPE.**
   `CkRecordEntry_Fragment.h:44,49` — `TMap<EntityType, DestructionCleanupFuncType> _DisconnectionFuncs`,
   written at `CkRecord_Utils.h:1015` with `.Add(InRecordHandle, lambda)`. The lambda closes over
   `T_DerivedRecord`. If one entry entity is connected to **two different record types on the same
   parent entity**, the second `Add` overwrites the first lambda (TMap::Add is replace-on-existing-key),
   so on destroy only one of the two records gets the order-preserving `Remove` — the other keeps a
   stale entry that the next `ForEach_*` swap-prunes. Note `_Records` (`:48`) is a `TArray` that can
   hold the same record twice while the func map cannot. **VERIFIED** (the shape); **INFERRED** (that a
   live call site exercises it — I did not find one).
2. **Cross-registry / restore.** After a CkSnapshot load, an entry handle can resolve against a
   registry slot that no longer holds that entity.  **INFERRED.**
3. **Record destroyed first.** `FProcessor_RecordEntry_Destructor` skips records that are themselves
   invalid (`CkRecordEntry_Processor.cpp:29-35`) — harmless, the record is dying too. **VERIFIED.**

**Slot reuse does not alias, it triggers the prune.** `_RecordEntries` stores handles, and
`FCk_Handle::operator==` compares the full entity (id **and** version) plus the registry handle
(`CkHandle.cpp:142-152`, VERIFIED). entt recycles an index with a bumped version, and
`registry.valid()` is version-checked, so a stale entry never silently becomes a *different* live
entity — it reads as invalid and gets swap-pruned on the next `ForEach_*`. Identity is safe; **order
is not**. The `registry.valid()` version-check claim is **INFERRED** (entt contract; I did not open
`entt-3.16.0` — the answer does not depend on pool iteration, so per the brief I stayed out).

**Removal then later insertion.** Re-inserting a previously-removed entry always appends to the tail
(`:1007` is the only append), so a removed-then-readded entry loses its original position under every
removal path — including the two order-preserving ones. LIFO push/pop is safe here; "restore to
original slot" is not expressible. **VERIFIED.**

**Snapshot round-trip is order-faithful.** `TFragment_RecordOfEntities::SerializeSnapshot`
(`CkRecord_Fragment.h:60-84`) writes `Num` then walks `_RecordEntries` in index order and reads back
into a `SetNum`'d array in the same order. Whatever order was live at capture is the order at
restore. **VERIFIED.**

---

## Is it contractual?

**No. Incidental.** Evidence:

- **Zero mentions of "order" in the module.** `rg -i order Source/CkRecord/` returns nothing — not in
  the code, not in `Claude.md`/`CLAUDE.md`, not in `CkRecord.md`. **VERIFIED.**
- **`Sort` exists as a public API.** `CkRecord_Utils.h:1106-1118` sorts `_RecordEntries` in place with
  a caller-supplied predicate. Its existence is the framework's own statement that entry order is a
  *mutable caller-owned property*, not a maintained invariant. **VERIFIED.**
- **The iteration path mutates order as a side effect of reading** (`:825`, `:870`), which no
  contract-bearing container would do. **VERIFIED.**
- **No test asserts it.** No ordering assertion found in the module. **INFERRED** (I searched
  CkFoundation `Source/`, not the CkTests submodule).

---

## Existing consumers — who relies on order

### 1. CkStateMachine — **explicit reliance, strongest evidence of a de-facto contract**

`Source/CkStateMachine/Public/CkStateMachine/State/CkSmState_Processor.cpp:123` — **VERIFIED**

```cpp
// ---- Walk transitions in record order (insertion order = priority order) ----
```

followed at `:128-160` by `RecordOfSmTransitions_Utils::ForEach_ValidEntry(...)` returning
`ECk_Record_ForEachIterationResult::Break` on the first `Undetermined` and on the first `Pass`.
First-match-wins over record order **is** the transition priority mechanism. `CkStateMachine/CLAUDE.md`
restates it independently ("walks transitions in declaration order and `Break`s on the first
`Undetermined` it finds") and builds the event-driven-condition resting-state design on top of it.

This reliance is safe **only** because SM transition entries are created once at graph construction
and are destroyed together with their owning state — nothing disconnects a transition mid-life, so
the swap-prune never fires on these records. The safety is circumstantial, not enforced. **VERIFIED**
(the reliance); **INFERRED** (the reason it holds).

### 2. CkInventory — **relies on order, and mutates it deliberately**

- `Inventory/CkInventory_RequestHandlers.cpp:432` and
  `Inventory/Spatial/CkInventory_Spatial_RequestTraits.cpp:320` — `FInventoryItemRecord::Sort(Base, ...)`
  is how `Request_Sort` is implemented: it reorders `_RecordEntries` and nothing else. **VERIFIED.**
- `Inventory/CkInventory_Utils.cpp:627-631` — `Get_Items` → `RecordOfInventoryItems_Utils::Get_ValidEntries`,
  i.e. record order. **VERIFIED.**
- `UI/CkInventoryUI_DataOnlyPanel.cpp:65` and `UI/CkInventoryUI_InventoryList.cpp:48` — the UI
  refreshes straight from `Get_Items`, so the sorted order is what the player sees. **VERIFIED.**

Sort-then-display is a full round trip through record order. This is the module that would visibly
break if the order property were dropped.

### 3. CkCamera — **the framework's existing layer stack does NOT use record order**

Directly relevant to the campaign's fork. `CkCamera` owns a Record of layer entities
(`CameraLayer/CkCameraLayer_Fragment.h:121`) and resolves the dominant layer with an **explicit
`int32 _Priority`** field (`CkCameraLayer_Fragment.h:38,45`):

`Camera/CkCamera_Processor.cpp:210-216` — **VERIFIED**

```cpp
const auto Priority = InLayer.Get<FFragment_CameraLayer_Params>().Get_Priority();
if (Priority > DominantPriority || (Priority == DominantPriority && Alpha >= DominantAlpha))
{ DominantPriority = Priority; DominantAlpha = Alpha; Dominant = InLayer; }
```

Same for eviction (`:87-102`, `OneOnly` stacking evicts same-`Priority` siblings) and blend-out
(`:135-147`, matched by `LayerClass`). The iteration is `ForEach_ValidEntry` — order-agnostic by
construction, since every decision is a fold over an explicit key. Camera layers are also destroyed
mid-life (`:186-190` destroys alpha-0 layers), i.e. exactly the population that would trip the
swap-prune — and they are immune to it because they never read order. **VERIFIED.**

### 4. Order-agnostic majority

`CkGoap`, `CkAttribute`, `CkTimer`, `CkAudio`, `CkVfx`, and the rest use `ForEach_ValidEntry` /
`Get_ValidEntry_If` as unordered set traversals or label lookups. **INFERRED** (spot-checked, not
exhaustively audited).

---

## Where a guarantee would have to be enforced

Location only — no proposal, no design.

| Site | File:line | Why it is in scope |
|---|---|---|
| Lazy prune, mutable overload | `CkRecord_Utils.h:825` | The only order-destroying operation in the module |
| Lazy prune, const overload | `CkRecord_Utils.h:870` | Same, through the `mutable` member |
| Sole append | `CkRecord_Utils.h:1007` | Any positional-insert semantic lands here |
| Destroy-path unlink lambda | `CkRecord_Utils.h:1018` | Registered per record entity; see the TMap-key hazard above |
| Explicit disconnect | `CkRecord_Utils.h:1059` | The other removal path |
| Caller-driven reorder | `CkRecord_Utils.h:1106-1118` (`Sort`) | Any invariant must survive it |
| Extension-boundary append order | `CkRecord_Utils.h:361-365` (`Get_Entries`), `:385-389` (`Get_ValidEntries`) | Defines cross-record total order when EntityExtensions are involved |
| Snapshot | `CkRecord_Fragment.h:60-84` | Already order-faithful; nothing needed |

---

## Adjacent findings (not part of the question — flagged, not acted on)

1. **`Get_ValidEntries` / `Get_ValidEntries_If` double-count EntityExtension entries.**
   `CkRecord_Utils.h:378` calls `Get_Entries`, which *already* recursed into extensions (`:361-365`),
   then `:385-389` recurses into extensions **again** and appends. Any record on an entity with
   EntityExtensions returns extension entries twice. Same shape at `:404` / `:411-415`. **VERIFIED**
   by reading; not reproduced at runtime.
2. **`Get_ValidEntries_ByTag` (`:420-451`) looks non-instantiable / dead.** It declares an unused
   `T_Predicate` template parameter, computes an unused `Entries` local (`:430`), assigns a
   `TArray<TPair<FName, RecordEntityType>>` filter result into a
   `TArray<RecordEntryMaybeTypeSafeHandle>` (`:438-442`), and calls `.Get<RecordType>()` at `:436`
   with no `Has` guard. Templates only compile when instantiated — this one apparently never is.
   **VERIFIED** (the code); **INFERRED** (that it is uninstantiated).
3. **`FProcessor_RecordOfEntities_Destructor` does not exist.** Friend-declared at
   `CkRecord_Fragment.h:31` and `CkRecordEntry_Fragment.h:32`; no definition or registration anywhere
   in `Source/`. Stale declaration. **VERIFIED.**
4. **`_DisconnectionFuncs` keyed by record entity, not (entity, record type)** — see removal
   section, route 1. **VERIFIED** (the shape).
