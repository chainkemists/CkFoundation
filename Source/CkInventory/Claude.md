# CkInventory

**Purpose:** Inventory system — items in a grid-based inventory. Inventory is a Record of item entities, each occupying one or more grid cells. Supports item stacking, filtering by tag set, replication.

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkGrid`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`, `CkTagSet`.
**Used by:** Player inventory, loot containers, shops.

---

## Key API

- `UCk_Utils_Inventory_UE` — add items, query items by tag, transfer between inventories.
- `FCk_Handle_InventoryItem` — typed item handle.
- Replication via processor; items replicate through the entity replication system.

---

## Pattern

Inventory entity → Record of item entities → each item occupies GridCell entities in a `CkGrid`. Items carry `CkTagSet` tags for filtering.

---

## Anti-patterns

1. Don't store item data in the Inventory fragment directly — item state lives on the item entity's fragments.
2. Don't transfer items by copying fragment data; use the provided transfer utilities.

---

## See also

- `CkGrid/Claude.md` — grid cell management.
- `CkTagSet/Claude.md` — item tag filtering.
- `CkRecord/Claude.md` — inventory uses Record pattern.
