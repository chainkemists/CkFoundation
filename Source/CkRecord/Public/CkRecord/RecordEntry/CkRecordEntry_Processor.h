#pragma once
#include "CkRecordEntry_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRECORD_API FProcessor_RecordEntry_Destructor : public TProcessor<
        FProcessor_RecordEntry_Destructor,
        FFragment_RecordEntry,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_PreDestruction;
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RecordEntry& InRecordEntry) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
