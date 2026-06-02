#include "CkItem_Fragment.h"

#include "CkInventory/Item/CkItem_Definition.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------
// ck:: hoisted to an unqualified alias because CK_REGISTER_SNAPSHOTABLE token-pastes the type name.

using FSnap_InventoryItem = ck::FFragment_InventoryItem;
CK_REGISTER_SNAPSHOTABLE(FSnap_InventoryItem);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_InventoryItem::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    if (InAr.IsSaving())
    {
        auto* Raw = const_cast<UObject*>(static_cast<const UObject*>(_Definition.Get()));
        InAr << Raw;
    }
    else if (InAr.IsLoading())
    {
        UObject* Raw = nullptr;
        InAr << Raw;
        _Definition = Cast<UCk_InventoryItem_Definition>(Raw);
    }
}

// --------------------------------------------------------------------------------------------------------------------
