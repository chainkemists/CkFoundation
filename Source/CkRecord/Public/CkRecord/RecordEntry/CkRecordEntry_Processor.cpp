#include "CkRecordEntry_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_RecordEntry_Destructor);

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
            if (ck::Is_NOT_Valid(RecordEntity))
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
