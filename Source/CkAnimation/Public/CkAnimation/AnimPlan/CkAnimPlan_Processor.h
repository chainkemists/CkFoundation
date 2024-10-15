#pragma once

#include "CkAnimPlan_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANIMATION_API FProcessor_AnimPlan_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AnimPlan_HandleRequests,
            FCk_Handle_AnimPlan,
            FFragment_AnimPlan_Params,
            FFragment_AnimPlan_Current,
            FFragment_AnimPlan_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AnimPlan_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AnimPlan_Params& InParams,
            FFragment_AnimPlan_Current& InCurrent,
            FFragment_AnimPlan_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimPlan_Current& InCurrent,
            const FCk_Request_AnimPlan_UpdateAnimCluster& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimPlan_Current& InCurrent,
            const FCk_Request_AnimPlan_UpdateAnimState& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANIMATION_API FProcessor_AnimPlan_Replicate : public ck_exp::TProcessor<
            FProcessor_AnimPlan_Replicate,
            FCk_Handle_AnimPlan,
            FFragment_AnimPlan_Params,
            FFragment_AnimPlan_Current,
            FTag_AnimPlan_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AnimPlan_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AnimPlan_Params& InParams,
            FFragment_AnimPlan_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
