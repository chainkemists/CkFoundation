#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FCk_Processor_Acceleration_Setup : public TProcessor<
            FCk_Processor_Acceleration_Setup,
            FCk_Tag_Acceleration_Setup,
            FCk_Fragment_Acceleration_Params,
            FCk_Fragment_Acceleration_Current>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_Acceleration_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Acceleration_Params& InParams,
            FCk_Fragment_Acceleration_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
