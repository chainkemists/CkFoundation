#pragma once

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

namespace ck
{
    class CKCROWD_API FProcessor_CrowdAvoidanceVolume_Setup : public ck_exp::TProcessor<
        FProcessor_CrowdAvoidanceVolume_Setup,
        FCk_Handle_CrowdAvoidanceVolume,
        FTag_CrowdAvoidanceVolume_NeedsSetup,
        TReadOnly<FFragment_CrowdAvoidanceVolume_Params>,
        TReadOnly<FFragment_Transform>,
        TReadWrite<FFragment_CrowdAvoidanceVolume_ProbeRef>,
        TExclude<FTag_CrowdAvoidanceVolume_HasRuntime>,
        TExclude<FTag_CrowdAvoidanceVolume_Invalid>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using TProcessor::TProcessor;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAvoidanceVolume_Params& InParams,
            const FFragment_Transform& InTransform,
            FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) const -> void;
    };

    class CKCROWD_API FProcessor_CrowdAvoidanceVolume_Monitor : public ck_exp::TProcessor<
        FProcessor_CrowdAvoidanceVolume_Monitor,
        FCk_Handle_CrowdAvoidanceVolume,
        FTag_CrowdAvoidanceVolume_HasRuntime,
        TReadOnly<FFragment_Transform>,
        TReadWrite<FFragment_CrowdAvoidanceVolume_ProbeRef>,
        TExclude<FTag_CrowdAvoidanceVolume_Invalid>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using TProcessor::TProcessor;

        static auto Release_Runtime(FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) const -> void;
    };

    class CKCROWD_API FProcessor_CrowdAvoidanceVolume_EndPlay : public ck_exp::TProcessor<
        FProcessor_CrowdAvoidanceVolume_EndPlay,
        FCk_Handle_CrowdAvoidanceVolume,
        TReadWrite<FFragment_CrowdAvoidanceVolume_ProbeRef>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime) -> void;

    private:
        TArray<FCk_CrowdAvoidanceVolume_Retirement> _PendingRetirements;
    };
}

// --------------------------------------------------------------------------------------------------------------------
