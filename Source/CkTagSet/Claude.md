# CkTagSet

**Purpose:** Entity-scoped tag sets — a replicated `FGameplayTagContainer` per entity with signal notifications on changes. Use when an entity needs multiple behavior tags that clients must see and that fire signals when added/removed.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`.
**Used by:** `CkInventory`, `CkInteraction`, ability conditions.

---

## Key API

- `UCk_Utils_TagSet_UE` — add tags, remove tags, query container.
- `FProcessor_TagSet_Replicate` — replicates tag set changes to clients.
- Signals: `OnTagAdded`, `OnTagRemoved`.

---

## Pattern

Use `CkTagSet` for **behavior tags** that live on the entity itself. Use `CkLabel` for the entity's **identity** tag (one, non-replicated). Use `CkEntityTag` for simple non-replicated tag storage.

---

## Persistence

**Container co-location.** `Add` puts the rep container on the SAME entity that holds
`FFragment_TagSet` — `InHandle`, never its LifetimeOwner (e.g. the Transient Entity of an
actor-bridged subject). `InHandle` is the one carrying the replication driver, and the client-side
`FProcessor_TagSet_SyncReplication` view requires both fragments together. `FProcessor_TagSet_Replicate`
consumes the registered `Produce` for the same reason; the value overload's `SetFragmentData`
find-or-adds the entry (the entry always exists post-`Add`), so the wire content is byte-identical
to the mutator path.

TagSet registers a `Register_NetAndSave_SplitApply<FCk_RepData_TagSet>` handler in
`CkTagSet_Fragment.cpp`, and the net and save paths deliberately do NOT share an applier:

- **`NetApply`** only stamps `FFragment_TagSet_SyncReplication`; `FProcessor_TagSet_SyncReplication`
  owns the actual diff/apply and does its own gating, so the apply always returns `Applied`.
- **`HydrationApply`** cannot reuse that stamp — the sync processor is ClientOnly, so on a loading
  AUTHORITY it would never drain and the server's tags would never be restored. It therefore
  replaces the authority tag set directly with `Set_Tags` (mirroring the client drain) and re-arms
  `FTag_TagSet_MayRequireReplication` so post-load clients converge.
- `Set_Tags`, never `Request_AddTags`: `Construct` re-seeds default tags, so an additive apply would
  resurrect a tag that was removed at runtime.

## Anti-patterns

Don't add/remove tags every tick — tag changes drive signals and replication, which have overhead.

---

## See also

- `CkLabel/Claude.md`, `CkEntityTag/Claude.md`, `CkCore/GameplayTag/README.md`.
