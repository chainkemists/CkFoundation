#pragma once
#include "CkRecordEntry_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRECORD_API FProcessor_RecordEntry_Destructor
        : public TProcessor<FProcessor_RecordEntry_Destructor, FFragment_RecordEntry, FTag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RecordEntry& InRecordEntry) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
