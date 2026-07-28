#pragma once

#include "CkInteraction_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINTERACTION_API FProcessor_Interaction_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Interaction_HandleRequests,
            FCk_Handle_Interaction,
            TReadOnly<FFragment_Interaction_Params>,
            TReadWrite<FFragment_Interaction_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_Interaction_Requests;

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
            const FFragment_Interaction_Params& InParams,
            FFragment_Interaction_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams,
            const FCk_Request_Interaction_EndInteraction& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed interaction's
    // still-queued requests are never drained. This fires each pending request's completion delegate
    // with Failed_Cancelled so a caller awaiting completion terminates instead of hanging. This is
    // additive to FProcessor_Interaction_EndPlay below, which broadcasts Failed on the interaction's
    // own OnInteractionFinished signal (a different consumer — the interaction's participants, not the
    // caller of this specific Request_EndInteraction).
    class CKINTERACTION_API FProcessor_Interaction_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Interaction_CancelPendingRequests,
        FCk_Handle_Interaction,
        ck::TReadOnly<FFragment_Interaction_Requests>,
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
            const FFragment_Interaction_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_Interaction_EndPlay : public ck_exp::TProcessor<
            FProcessor_Interaction_EndPlay,
            FCk_Handle_Interaction,
            TReadOnly<FFragment_Interaction_Params>,
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
            const FFragment_Interaction_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------