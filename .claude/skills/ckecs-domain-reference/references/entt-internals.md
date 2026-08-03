# EnTT as vendored (3.16.0)

Reference for `ckecs-domain-reference`. What the vendored ECS library actually does, cited against `Source/CkThirdParty/Public/CkThirdParty/entt-3.16.0/` (read 2026-07-02).

### 1.1 Entity = index + version

`entt::entity` is `enum class entity : uint32` (`entity/fwd.hpp:14`; `ENTT_ID_TYPE` = `std::uint32_t`,
`config/config.h:39-41`). The 32 bits split into a **20-bit index** (`entity_mask 0xFFFFF`) and a
**12-bit version** (`version_mask 0xFFF`) (`entity/entity.hpp:31-40`). The version is the staleness
detector: destroying an entity bumps its version, so an old copy of the id — same index, old
version — no longer matches what the registry stores. `to_entity`/`to_version` split an id
(`entity.hpp:94-109`); `next()` increments the version, skipping the reserved all-ones value
(`entity.hpp:116-119`).

Two sentinels (`entity.hpp:203-384`): `entt::null` compares by **index** part, `entt::tombstone`
compares by **version** part — both encode as all-ones. `FCk_Entity` (the Ck wrapper,
`CkEcs/Entity/CkEntity.h:12-67`) default-constructs to `entt::tombstone` (`CkEntity.h:27`) and is
statically asserted to be nothing more than the id in non-editor builds (`CkEntity.h:86-92`).
12 bits of version ⇒ a slot's version wraps after 4095 destroys; a wrapped id can false-match. Low
practical risk, but it is why handles are re-validated, never trusted long-term.

### 1.2 Registry = per-type sparse-set pools

`entt::basic_registry` holds a `dense_map<id_type, std::shared_ptr<basic_sparse_set>>` — one
**pool per fragment type**, keyed by compile-time type hash (`entity/registry.hpp:243`), plus a
dedicated entity storage. `assure<Type>()` lazily creates a pool on first touch
(`registry.hpp:248-277`). There is no "archetype" anywhere: an entity is just an index; which
fragments it "has" is answered per-pool ("is this index in that pool?").

A **sparse set** is two arrays (`entity/sparse_set.hpp:138-165`): a paged *sparse* array indexed by
entity index (pages of `ENTT_SPARSE_PAGE` = 4096, `config.h:46-47`) holding positions into a dense
*packed* array of entity ids. Membership test = one indexed load; iteration = walk the packed array
(contiguous). `basic_storage` extends the sparse set with paged payload storage — pages of
`ENTT_PACKED_PAGE` = 1024 elements (`config.h:50-51`), allocated page-by-page
(`entity/storage.hpp:241-261`). Because payload lives in **independently allocated pages**, growing
a pool never relocates existing fragments: a `FFragment_X&` stays valid across *adds* of the same
type. Whether *deletion* moves fragments depends on the deletion policy — and CkEcs pins that
globally (§1.3).

`create()` recycles destroyed indices with bumped versions (`registry.hpp:503-505`).
`destroy(entt)` walks every pool in reverse pool-creation order, removing the entity from each,
then releases the id (`registry.hpp:544-551`). `valid(entt)` checks the id (index **and** version)
is live in the entity storage (`registry.hpp:485-487`); `current(entt)` reports the live version
for an index (`registry.hpp:495-497`). `emplace<T>` on an entity that already has `T` is UB
(assert) (`registry.hpp:597-610`) — the Ck wrapper turns this into an ensure (§2.1).

### 1.3 Deletion policies: swap-and-pop vs in_place (tombstones)

Enum `entt::deletion_policy` (`entity/fwd.hpp:17-26`): `swap_and_pop` (default), `in_place`,
`swap_only` (entity storage only).

- **swap_and_pop** — EnTT's upstream default, NOT what CkEcs pools use (see below)
  (`sparse_set.hpp:264-276`; payload side `storage.hpp:347-352`): the last packed element is moved
  into the erased slot. The packed array stays **dense** — iteration touches only live elements —
  but the moved fragment's address changes and iteration order is not stable.
- **in_place** (`sparse_set.hpp:282-286`; payload destroyed in place `storage.hpp:343-345`): the
  slot becomes a **tombstone** threaded into a free list (`head`). Fragment addresses are stable
  ("pointer stability") and erasing moves nothing, but the packed array accumulates holes that
  iteration must skip — density and cache locality degrade until slots are reused or the pool is
  compacted.
- **swap_only**: used by the entity storage itself (`storage.hpp:978-1042`) — destroyed ids stay in
  the packed array past the free-list boundary with a bumped version; that boundary is what
  `registry.valid()` tests.

Which policy a type gets: `component_traits<T>::in_place_delete` (`entity/component.hpp:42-55`) —
a per-type `static constexpr bool in_place_delete = true;` member (`component.hpp:20-22`),
**automatically** if the type is not move-constructible-and-assignable (`component.hpp:14-15`), or
a `component_traits` specialization.

**CkEcs forces in_place for EVERY fragment type.** `CkHandle.h:71-77` specializes
`entt::component_traits<Type>` for all types with `in_place_delete = true` (keeping the standard
page-size formula). Consequences:

- Fragment addresses are stable for the fragment's lifetime; erases move nothing.
- Every pool carries tombstones after churn. Views skip them silently — single-storage views via an
  explicit check (`view.hpp:23`, applied `view.hpp:459`), multi-storage views because a tombstone
  id matches no other pool. Tombstone slots are recycled by later emplaces through the free list;
  nothing in `Source/` ever calls `compact()` (rg-verified 2026-07-02).
- `FCk_Registry::Sort` on a pool that currently has tombstones trips EnTT's "Sorting with
  tombstones not allowed" assert (`sparse_set.hpp:1000,1054`) — sort only pools without pending
  in-place holes.
- The unconditional form is deliberate design, established by git (DECISIONS.md §45): introduced
  debug-gated (`745507381`, 2024-03-07), deliberately ungated in `06938bba3` (2026-02-17, "feat:
  fragments are always pointer stable"). The mechanical motive, INFERRED from the code shape:
  pointer stability for fragments whose members are referenced from outside the pool — signal
  fragments' bound-delegate state (`CkSignal_Fragment.h:44,99`) and the handle-debug mapper's raw
  fragment pointers (`CkHandle_Debugging.h:67`) both ALSO opt in with the per-type member form,
  which holds even in a TU where the CkHandle.h specialization isn't visible — plus erase-safety
  during processor iteration.

### 1.4 Empty types (tags) cost nothing

`component_traits<T>::page_size` is 0 for empty classes (`component.hpp:24-25`, via
`ENTT_ETO_TYPE`, `config.h:71-75`); Ck's global `component_traits` specialization reproduces the
same formula directly (`!std::is_empty_v<Type> * ENTT_PACKED_PAGE`, `CkHandle.h:76`), so the result
holds either way. A `page_size == 0` storage specialization stores **no payload at all** — the pool
is just the sparse set of member entities (`storage.hpp:789-819`; warning at `storage.hpp:220-222`).
This is why Ck tags (`CK_DEFINE_ECS_TAG`) are free-ish to add/remove, why `FCk_Registry::Get` on an
empty type returns a shared static instead of touching storage (`CkRegistry.h:737-741`), and why
tags never appear as `ForEachEntity` parameters (§2.4).

### 1.5 The storage_type specialization mechanism

`registry.storage<T>()` resolves the pool type through `storage_for` → `storage_type`
(`entity/fwd.hpp:225-262`): by default `sigh_mixin<basic_storage<T>>` — a mixin that adds
`on_construct`/`on_update`/`on_destroy` signals, surfaced on the registry at
`registry.hpp:1013-1071`. Customize a type's storage by specializing `storage_type`/
`component_traits` — CkEcs's blanket `component_traits` specialization (§1.3) is exactly this
mechanism in production. Two more facts about this codebase:

- **Nothing in CkFoundation subscribes to those storage signals or creates groups** (rg over
  `Source/` minus ThirdParty: zero `on_construct()`/`.group<` call sites, 2026-07-02). Reactivity
  is done with tag fragments + processors instead.
- **Named storages ARE used**: `registry.storage<T>(id)` creates a *separate pool of the same C++
  type per runtime id* (`registry.hpp:449-456`). CkEcs exposes this as
  `FCk_Registry::Storage<T>(Hash)` (`CkRegistry.h:298-302`, friend-gated) and CkEntityTag builds
  its per-FName tag pools on it (`CkEntityTag/CkEntityTag_Utils.cpp:24`).

### 1.6 Views — lazy intersection

`registry.view<A, B>(entt::exclude<C>)` builds a lightweight object holding **pool pointers only**
— nothing is computed or cached at construction (`registry.hpp:1081-1094`). At iteration time the
view picks the **smallest** pool as the *leading* pool (`view.hpp:244-254`, doc `view.hpp:419-425`
— NOT the first template argument; `use<T>()` overrides, `view.hpp:516-525`), walks its packed
array, and for each entity membership-checks every other pool and the exclude list
(`view.hpp:456-467`). Cost per step = one packed load + (N-1) sparse lookups. Views are cheap to
create every frame — that is exactly what processors do (§2.4).

The const overload of `registry.view` does **not** create missing pools — a never-touched fragment
type yields an empty view (`registry.hpp:1081-1087`); the non-const overload `assure`s pools into
existence (`registry.hpp:1089-1094`).

**Iteration invalidation contract** (verbatim rules, `view.hpp:203-219`): view iterators survive
(a) new entities/fragments being added, (b) the *currently returned* entity being modified or
destroyed. **Everything else invalidates** — in particular, removing the viewed fragment types
from *other* entities mid-loop. This is the mechanical reason for house non-negotiable #5
("requests are deferred; processors mutate"): a request-drain processor touches only the current
entity, which the contract explicitly permits. Deferred destruction (§3.4) exists for the same
reason. For cross-entity mutation from parallel code, `ck::FDeferredCommandBuffer` queues
Add/Remove/Replace closures per worker thread and flushes them single-threaded
(`CkEcs/Processor/CkDeferredCommands.h:13-137`).

### 1.7 Groups — owning, reordered storage (available, unused here)

`registry.group<Owned...>(get<...>, exclude<...>)` is the eager alternative: a `group_handler`
subscribes to `on_construct`/`on_destroy` of every involved pool (`entity/group.hpp:144-149`) and
**physically reorders the owned pools** so all group members sit contiguously at the front
(`group.hpp:103-152`) — iteration is then a straight walk of aligned arrays with no per-entity
checks. The prices: each pool can be owned by at most one group ("Conflicting groups" assert,
`registry.hpp:1119`), owned pools can no longer be user-sorted ("Cannot sort owned storage",
`registry.hpp:1173-1186`), and every add/remove of an involved type pays the bookkeeping.
CkFoundation uses **views only** (§1.5); know groups exist so you read upstream EnTT docs
correctly, but do not reach for them here without a maintainer conversation. Owning groups are in
fact statically forbidden today — the global `in_place_delete` trait (§1.3) trips `group.hpp:697`'s
static_assert. The adoption question (measure the tombstone cost, then decide) is
`ck-feature-frontier` candidate 5 — the decision procedure and constraints live there.

