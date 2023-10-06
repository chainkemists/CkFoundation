#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/Entity/CkUnrealEntity_Fragment.h"

namespace ck
{
    class CKNET_API FProcessor_UnrealEntity_HandleRequests
        : public TProcessor<FProcessor_UnrealEntity_HandleRequests, FCk_Fragment_UnrealEntity_Requests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType,
            HandleType InHandle,
            FCk_Fragment_UnrealEntity_Requests& InRequests) const -> void;
    };
}
