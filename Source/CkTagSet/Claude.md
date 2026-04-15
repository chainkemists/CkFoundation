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

## Anti-patterns

Don't add/remove tags every tick — tag changes drive signals and replication, which have overhead.

---

## See also

- `CkLabel/Claude.md`, `CkEntityTag/Claude.md`, `CkCore/GameplayTag/README.md`.
