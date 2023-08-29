#pragma once
#include "CkRecordEntry_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRECORD_API FCk_Processor_RecordEntry_Destructor
        : public TProcessor<FCk_Processor_RecordEntry_Destructor, FCk_Fragment_RecordEntry, FCk_Tag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_RecordEntry& InRecordEntry) -> void;
    };
}