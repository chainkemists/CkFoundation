#pragma once
#include "CkRecord_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRECORD_API FCk_Processor_Record_Destructor
        : public TProcessor<FCk_Processor_Record_Destructor, FFragment_Record, FCk_Tag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Record& InRecord) -> void;
    };
}
