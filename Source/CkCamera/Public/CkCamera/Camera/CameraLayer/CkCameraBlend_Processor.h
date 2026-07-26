#pragma once

#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------
// Auto-blend, per-operation effective-delta formulas and the group-ordering contract: CkCamera/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCAMERA_API FProcessor_CameraLayer_Blend : public ck_exp::TProcessor<
            FProcessor_CameraLayer_Blend,
            FCk_Handle_CameraLayer,
            TReadWrite<FFragment_CameraLayer_Blend>,
            TReadWrite<FFragment_CameraLayer_AcquiredModifiers>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InLayer,
            FFragment_CameraLayer_Blend& InBlend,
            FFragment_CameraLayer_AcquiredModifiers& InAcquired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
