#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment.h"

namespace ck
{
    class CKUNREAL_API FCk_Processor_UnrealEntity_HandleRequests
        : public TProcessor<FCk_Processor_UnrealEntity_HandleRequests, FCk_Fragment_UnrealEntity_Requests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(FTimeType,
            FCk_Handle InHandle,
            FCk_Fragment_UnrealEntity_Requests& InRequests) const -> void;
    };
}
