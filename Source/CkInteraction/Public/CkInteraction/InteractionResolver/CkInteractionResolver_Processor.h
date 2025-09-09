#pragma once

#include "CkInteractionResolver_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractionResolver_HandleRequests
        : public ck_exp::TProcessor<
            FProcessor_InteractionResolver_HandleRequests,
            FCk_Handle_InteractionResolver,
            FFragment_InteractionResolver_Params,
            FFragment_InteractionResolver_Current,
            FFragment_InteractionResolver_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_InteractionResolver_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT)
            -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FFragment_InteractionResolver_Requests& InRequestsComp) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_StartIntent& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_StopIntent& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_AddInteractTarget& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_RemoveInteractTarget& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent,
            const FCk_Request_InteractionResolver_RemoveAllTargetsByChannel& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractionResolver_Persistent
        : public ck_exp::TProcessor<
            FProcessor_InteractionResolver_Persistent,
            FCk_Handle_InteractionResolver,
            FFragment_InteractionResolver_Params,
            FFragment_InteractionResolver_Current,
            FTag_InteractionResolver_IntentUpdated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent)
            -> void;

    private:
        static auto
        DoUpdateCachedTargets(
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractionResolver_Teardown
        : public ck_exp::TProcessor<
            FProcessor_InteractionResolver_Teardown,
            FCk_Handle_InteractionResolver,
            FFragment_InteractionResolver_Params,
            FFragment_InteractionResolver_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractionResolver_Params& InParams,
            FFragment_InteractionResolver_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractionResolver_Replicate
        : public ck_exp::TProcessor<
            FProcessor_InteractionResolver_Replicate,
            FCk_Handle_InteractionResolver,
            FFragment_InteractionResolver_Current,
            TObjectPtr<UCk_Fragment_InteractionResolver_Rep>,
            FTag_InteractionResolver_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_InteractionResolver_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_InteractionResolver_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_InteractionResolver_Rep>& InComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------