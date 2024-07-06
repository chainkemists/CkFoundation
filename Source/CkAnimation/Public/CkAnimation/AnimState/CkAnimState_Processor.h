#pragma once

#include "CkAnimState_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANIMATION_API FProcessor_AnimState_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AnimState_HandleRequests,
            FCk_Handle_AnimState,
            FFragment_AnimState_Current,
            FFragment_AnimState_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AnimState_Requests;

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
            FFragment_AnimState_Current& InComp,
            FFragment_AnimState_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetGoal& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetState& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetCluster& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetOverlay& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANIMATION_API FProcessor_AnimState_Replicate : public ck_exp::TProcessor<
            FProcessor_AnimState_Replicate,
            FCk_Handle_AnimState,
            FFragment_AnimState_Current,
            TObjectPtr<UCk_Fragment_AnimState_Rep>,
            FTag_AnimState_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AnimState_Updated;

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
            FFragment_AnimState_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_AnimState_Rep>& InComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
