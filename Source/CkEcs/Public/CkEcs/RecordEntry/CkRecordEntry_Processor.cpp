#include "CkRecordEntry_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcs/Record/CkRecord_Fragment.h"
#include "CkEcs/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RecordEntry_Destructor::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_RecordEntry& InRecordEntry) const
        -> void
    {
        for (const auto& Kvp : InRecordEntry._DisconnectionFuncs)
        {
            const auto& RecordEntity = Kvp.Key;

            // If our Record is in the process of getting destroyed, ignore
            if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(RecordEntity))
            { continue; }

            const auto& RecordEntityHandle = MakeHandle(RecordEntity, InHandle);

            if (ck::Is_NOT_Valid(RecordEntityHandle))
            { continue; }

            const auto& DestructionCleanupFunc = Kvp.Value;

            DestructionCleanupFunc(RecordEntityHandle, InHandle);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
