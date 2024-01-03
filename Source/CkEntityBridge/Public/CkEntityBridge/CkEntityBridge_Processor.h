#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEntityBridge/CkEntityBridge_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYBRIDGE_API FProcessor_EntityBridge_HandleRequests
        : public TProcessor<FProcessor_EntityBridge_HandleRequests, FFragment_EntityBridge_Requests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityBridge_Requests& InRequests) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
