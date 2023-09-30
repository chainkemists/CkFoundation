#include "CkRecordEntry_Processor.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

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
            const auto& RecordEntityHandle = MakeHandle(RecordEntity, InHandle);

            if (ck::Is_NOT_Valid(RecordEntityHandle))
            { continue; }

            const auto& DestructionCleanupFunc = Kvp.Value;

            DestructionCleanupFunc(RecordEntityHandle, InHandle);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
