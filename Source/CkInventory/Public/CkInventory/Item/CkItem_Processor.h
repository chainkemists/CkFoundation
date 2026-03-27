#pragma once

#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // EndPlay: when an item is destroyed directly, enqueue a remove request on
    // its owning inventory so signals fire and UI can react.
    class CKINVENTORY_API FProcessor_InventoryItem_EndPlay : public ck_exp::TProcessor<
            FProcessor_InventoryItem_EndPlay,
            FCk_Handle_Item,
            FFragment_InventoryItem,
            CK_IF_END_PLAY>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_InventoryItem&) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
