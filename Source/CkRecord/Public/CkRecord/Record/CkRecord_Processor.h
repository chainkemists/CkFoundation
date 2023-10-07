#pragma once
#include "CkRecord_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRECORD_API FProcessor_RecordOfEntities_Destructor
        : public TProcessor<FProcessor_RecordOfEntities_Destructor, FFragment_RecordOfEntities, FTag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType                          InDeltaT,
            const HandleType&                 InHandle,
            const FFragment_RecordOfEntities& InRecord) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
