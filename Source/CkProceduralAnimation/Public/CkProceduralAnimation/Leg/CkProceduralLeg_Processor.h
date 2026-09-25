#pragma once

#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralLeg_HandleRequests : public ck_exp::TProcessor<
        FProcessor_ProceduralLeg_HandleRequests,
        FCk_Handle_ProceduralLeg,
        TReadWrite<FFragment_ProceduralLeg_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using MarkedDirtyBy = FFragment_ProceduralLeg_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ProceduralLeg_Requests& InRequests)
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ProceduralLeg_EnableDisable& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ProceduralLeg_Detach& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProcessor_ProceduralLeg_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_ProceduralLeg_CancelPendingRequests,
        FCk_Handle_ProceduralLeg,
        TReadOnly<FFragment_ProceduralLeg_Requests>,
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
            const FFragment_ProceduralLeg_Requests& InRequests)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
