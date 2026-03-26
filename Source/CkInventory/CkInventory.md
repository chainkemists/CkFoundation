# CkInventory

Data-driven inventory system with two modes: Spatial (grid-based with x,y coordinates) and DataOnly (flat list). Handles item stacking, splitting, transferring, and replication through deferred requests.

## Key Concepts

- **Two Inventory Types** — `Spatial` (grid with dimensions, items occupy cells) and `DataOnly` (flat list, optionally bounded).
- **Item** — An ECS entity in an inventory. Can have extensible fragments: Stackable, Dimensions, Tags, or custom.
- **Item Definition** — Data asset defining defaults (max stack size, grid dimensions, etc.).
- **Stacking/Splitting** — Items with Stackable fragments can be merged or split into separate stacks.
- **Transfer** — Move items between inventories with validation.
- **Signals** — `OnItemsChanged` (added/removed lists), `OnInventoryFull`, `OnSpaceAvailable`.
- **Replication** — Server processes requests, syncs results to clients.

## Example: Player Picks Up Items

```mermaid
flowchart LR
    A["Player picks up<br/>3 healing potions"] -->|"Request_AddItem ×3"| B["Stack count<br/>increases to 3"]
    B -->|"OnItemsChanged"| C["Inventory UI<br/>updates"]
```

## Usage Examples

### Create an inventory

```cpp
auto Params = UCk_Utils_Inventory_UE::Make_InventoryParams_Spatial(Dimensions);
UCk_Utils_Inventory_UE::Add(OwnerEntity, Params);
```

### Add an item

```cpp
UCk_Utils_Inventory_UE::Request_AddItem(InventoryHandle, AddItemRequest, OnResultDelegate);
```

### Remove an item

```cpp
UCk_Utils_Inventory_UE::Request_RemoveItem(InventoryHandle, RemoveItemRequest, OnResultDelegate);
```

### Stack items together

```cpp
UCk_Utils_Inventory_UE::Request_StackItems(InventoryHandle, StackRequest);
```

### Transfer between inventories

```cpp
UCk_Utils_Inventory_UE::Request_TransferItem(SourceInventory, TransferRequest);
```

### Check if an item fits

```cpp
bool CanAccept = UCk_Utils_Inventory_UE::Get_CanAcceptItem(InventoryHandle, Item);
```

## Tests

No tests found for this module in CkTest.
