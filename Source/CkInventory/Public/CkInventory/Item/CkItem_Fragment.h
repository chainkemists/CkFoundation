#pragma once

#include "CkItem_Fragment_Data.h"

#include "CkEcs/Concepts/CkSnapshot_Concepts.h" // forward-declares ck::FSnapshotContext for the SerializeSnapshot decl

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;
class FArchive;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKINVENTORY_API FFragment_InventoryItem
    {
        CK_GENERATED_BODY(FFragment_InventoryItem);
        using IsSnapshotable = void;

    private:
        TWeakObjectPtr<const UCk_InventoryItem_Definition> _Definition;

    public:
        CK_PROPERTY_GET(_Definition);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InventoryItem, _Definition);

    public:
        // Tier-C (non-template): serializes the item-definition pointer by path via the proxy archive's
        // UObject-by-string handling. Does NOT use FSnapshotContext (no entity-handle refs). Body in the .cpp.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
