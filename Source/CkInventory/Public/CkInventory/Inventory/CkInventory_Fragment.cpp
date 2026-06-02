#include "CkInventory_Fragment.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration for the inventory record-of-entities fragments. ck:: types are hoisted to
// unqualified file-scope aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the type name (the `::` cannot be pasted).

using FSnap_RecordOfInventories    = ck::FFragment_RecordOfInventories;
using FSnap_RecordOfInventoryItems = ck::FFragment_RecordOfInventoryItems;

CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfInventories);
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfInventoryItems);

// Tier-C TFragment_EntityHolder specializations stored on item / slot entities.
using FSnap_Item_ParentInventory  = ck::FFragment_Item_ParentInventory;
using FSnap_InventorySlot_ItemRef = ck::FFragment_InventorySlot_ItemRef;

CK_REGISTER_SNAPSHOTABLE(FSnap_Item_ParentInventory);
CK_REGISTER_SNAPSHOTABLE(FSnap_InventorySlot_ItemRef);

// --------------------------------------------------------------------------------------------------------------------
