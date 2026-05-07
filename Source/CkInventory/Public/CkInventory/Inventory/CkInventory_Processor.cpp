#include "CkInventory_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_FireSignals);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Inventory_FireSignals::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_Inventory_Params& /*InParams*/,
            FFragment_Inventory_PreviousItems& InPreviousItems) -> void
    {
        const auto CurrentItems = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Get_ValidEntries(InHandle);
        const auto& PreviousItems = InPreviousItems.Get_Items();

        const auto ItemsAdded   = algo::Except(CurrentItems, PreviousItems);
        const auto ItemsRemoved = algo::Except(PreviousItems, CurrentItems);

        if (NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty())
        {
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InHandle);

            UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, ItemsAdded, ItemsRemoved));
        }

        InPreviousItems._Items = CurrentItems;
        InHandle.Remove<FTag_Inventory_MayHaveChanged>();
    }
}

// --------------------------------------------------------------------------------------------------------------------