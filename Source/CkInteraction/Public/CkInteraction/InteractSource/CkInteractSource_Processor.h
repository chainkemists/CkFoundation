#pragma once

#include "CkInteractSource_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINTERACTION_API FProcessor_InteractSource_Setup : public ck_exp::TProcessor<
            FProcessor_InteractSource_Setup,
            FCk_Handle_InteractSource,
            TReadOnly<FFragment_InteractSource_Params>,
            TReadWrite<FFragment_InteractSource_Current>,
            FTag_InteractSource_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_InteractSource_RequiresSetup;

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
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp) const -> void;
    };

    class CKINTERACTION_API FProcessor_InteractSource_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InteractSource_HandleRequests,
            FCk_Handle_InteractSource,
            TReadOnly<FFragment_InteractSource_Params>,
            TReadWrite<FFragment_InteractSource_Current>,
            TReadWrite<FFragment_InteractSource_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_InteractSource_Setup>;
        using MarkedDirtyBy = FFragment_InteractSource_Requests;

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
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp,
            FFragment_InteractSource_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InCurrent,
            const FCk_Request_InteractSource_StartInteraction& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InCurrent,
            const FCk_Request_InteractSource_CancelInteraction& InRequest) const -> void;

    private:
        auto
        OnInteractionFinished(
            FCk_Handle_Interaction InteractionHandle,
            ECk_SucceededFailed SucceededFailed) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed source's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKINTERACTION_API FProcessor_InteractSource_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_InteractSource_CancelPendingRequests,
        FCk_Handle_InteractSource,
        ck::TReadOnly<FFragment_InteractSource_Requests>,
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
            const FFragment_InteractSource_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractSource_EndPlay : public ck_exp::TProcessor<
            FProcessor_InteractSource_EndPlay,
            FCk_Handle_InteractSource,
            TReadOnly<FFragment_InteractSource_Params>,
            TReadWrite<FFragment_InteractSource_Current>,
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
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp) -> void;
    };

}

// --------------------------------------------------------------------------------------------------------------------