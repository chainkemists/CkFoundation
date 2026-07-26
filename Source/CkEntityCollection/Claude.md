# CkEntityCollection

**Purpose:** Entity collection tracking — groups of entity handles stored in a fragment, with change detection (previous vs. current set). Used for systems that need to track a set of entities and react when the set changes.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** `CkObjective` (tracks entities involved in objectives).

---

## Key API

- `UCk_Utils_EntityCollection_UE` — add, remove, query entity handle sets on a parent entity.
- Processors fire signals when the collection changes (added/removed entries).

---

## Pattern

Attach an EntityCollection fragment to a 'group' entity; processors detect changes by comparing current vs. previous snapshots each tick.

---

## Anti-patterns

Don't use EntityCollection for ordered sequences — it's a set, not a list.

---

## Persistence and replication

The registered `FCk_RepData_EntityCollections` handler (`CkEntityCollection_Fragment.cpp`) is
`Register_NetAndSave_SplitApply`. The payload is **owner-keyed**: `Produce` walks the owner's record of
collections and emits one entry per collection, so array order is not load-bearing — every consumer looks
entries up by collection name.

- **`NetApply` never gates.** It only stashes the new/old snapshot into
  `FFragment_EntityCollection_SyncReplication`. All validity gating lives in
  `FProcessor_EntityCollection_SyncReplication`, which carries **no `MarkedDirtyBy`** on purpose: its two
  gates (the local child collection exists — i.e. the entity script's Construct has run — and every
  referenced entity's `EntityReplicationDriver` reports replication complete) routinely fail on
  first-rep-arrival while NetGuids are still resolving. With dirty-mark filtering, that early return would
  freeze the fragment forever, because a container rep does not re-deliver an unchanged snapshot. Without
  the filter the processor retries every tick, and on success removes the fragment itself to stop the loop.
- **`HydrationApply` is distinct from `NetApply`** because the ClientOnly drain processor never runs on a
  loading authority. Every `return NotReady` in it precedes the first mutation — a retry after a partial
  write would stack members.
- **Hydration is ADD-only, deliberately.** Reading the live set to REMOVE-then-ADD would stack
  construct-seeded members that are still enqueued and therefore invisible to a `Get_` read.
- **`FProcessor_EntityCollection_Replicate` publishes one projection.** It calls the registered owner-keyed
  `Produce` and full-replaces the owner container with the result; that is content-identical to the older
  per-child find-or-emplace, since each collection's live members are read the same way.

## Previous-snapshot lifetime

`Request_StorePreviousCollection` refreshes the `_Previous` record by **disconnecting every previously
snapshot'd entry and re-connecting the current set** — never by `Try_Remove<Previous>` + `AddIfMissing`,
and `FProcessor_EntityCollection_FireSignals::DoTick` deliberately does not clear it either.

`TUtils_RecordOfEntities::Request_Connect` stores a per-record disconnect lambda on each connected entry's
`FFragment_RecordEntry`. That lambda fires from `FProcessor_RecordEntry_Destructor` when the entry is
destroyed and does `Get<Previous>()` on the collection handle; removing the `_Previous` fragment while
prior entries still hold those lambdas ensure-fails during the entry's destruction cascade. Disconnecting
properly removes both the back-reference and the lambda, so the `_Previous` fragment's lifetime is bounded
by the collection entity rather than by the FireSignals tick.

Disconnect walks **valid entries only**: `Request_Disconnect` `Get<>`s the entry's `FFragment_RecordEntry`
with the default validity policy and would ensure-fail on pending-kill entries, which clean themselves up
through their own `FProcessor_RecordEntry_Destructor` pass.

---

## See also

- `CkRecord/Claude.md` — related concept for parent/child entity records.
- `CkObjective/Claude.md` — primary consumer.
