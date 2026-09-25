#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralGait_Setup : public ck_exp::TProcessor<
        FProcessor_ProceduralGait_Setup,
        FCk_Handle_ProceduralGait,
        TReadOnly<FFragment_ProceduralGait_Tunables>,
        TReadWrite<FFragment_ProceduralGait>,
        TReadOnly<FFragment_Transform>,
        FTag_ProceduralGait_NeedsSetup,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralLeg_HandleRequests>;
        using MarkedDirtyBy = FTag_ProceduralGait_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProcessor_ProceduralGait_HandleRequests : public ck_exp::TProcessor<
        FProcessor_ProceduralGait_HandleRequests,
        FCk_Handle_ProceduralGait,
        TReadWrite<FFragment_ProceduralGait_Tunables>,
        TReadWrite<FFragment_ProceduralGait>,
        TReadWrite<FFragment_ProceduralGait_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralLeg_HandleRequests>;
        using MarkedDirtyBy = FFragment_ProceduralGait_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Requests& InRequestsComp)
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            const FCk_Request_ProceduralGait_ApplyPreset& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProcessor_ProceduralGait_Update : public ck_exp::TProcessor<
        FProcessor_ProceduralGait_Update,
        FCk_Handle_ProceduralGait,
        TReadOnly<FFragment_ProceduralGait_Tunables>,
        TReadWrite<FFragment_ProceduralGait>,
        TReadWrite<FFragment_ProceduralGait_Debug>,
        TReadOnly<FFragment_Transform>,
        TExclude<FTag_ProceduralGait_NeedsSetup>,
        TExclude<FFragment_ProceduralGait_Failure>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralLeg_HandleRequests, FProcessor_ProceduralGait_Setup, FProcessor_ProceduralGait_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProcessor_ProceduralGait_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_ProceduralGait_CancelPendingRequests,
        FCk_Handle_ProceduralGait,
        TReadOnly<FFragment_ProceduralGait_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
