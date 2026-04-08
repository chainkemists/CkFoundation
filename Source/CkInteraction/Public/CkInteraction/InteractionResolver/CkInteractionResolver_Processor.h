#pragma once

#include "CkInteractionResolver_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

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
        using Group = FGroup_Gameplay;
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
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_InteractionResolver_HandleRequests>;

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

    class CKINTERACTION_API FProcessor_InteractionResolver_EndPlay
        : public ck_exp::TProcessor<
            FProcessor_InteractionResolver_EndPlay,
            FCk_Handle_InteractionResolver,
            FFragment_InteractionResolver_Params,
            FFragment_InteractionResolver_Current,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_PreDestruction;

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

}

// --------------------------------------------------------------------------------------------------------------------