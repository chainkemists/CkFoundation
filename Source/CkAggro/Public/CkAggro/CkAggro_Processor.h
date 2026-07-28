#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkAggro/CkAggro_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_AggroTarget_Setup;
    class FProcessor_AggroTarget_HandleRequests;
    class FProcessor_AggroTarget_Forget;

    class CKAGGRO_API FProcessor_Aggro_Setup : public ck_exp::TProcessor<
        FProcessor_Aggro_Setup,
        FCk_Handle_Aggro,
        TReadOnly<FFragment_Aggro_EvaluationParams>,
        TReadWrite<FFragment_Aggro_EvaluationClock>,
        FTag_Aggro_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_Aggro_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Aggro_EvaluationParams& InEvaluationParams,
            FFragment_Aggro_EvaluationClock& InEvaluationClock) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_Aggro_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Aggro_HandleRequests,
        FCk_Handle_Aggro,
        TReadWrite<FFragment_Aggro_Requests>,
        TReadWrite<FFragment_Aggro_TargetMap>,
        TReadWrite<FFragment_Aggro_Current>,
        TExclude<FTag_Aggro_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_AggroTarget_Setup>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FFragment_Aggro_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            FFragment_Aggro_Requests& InRequests,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent) const -> void;

    private:
        static auto
        DoHandleRequest(HandleType InAggro, FFragment_Aggro_TargetMap& InTargetMap, FFragment_Aggro_Current& InCurrent, const FCk_Request_Aggro_AddThreat& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InAggro, FFragment_Aggro_TargetMap& InTargetMap, FFragment_Aggro_Current& InCurrent, const FCk_Request_Aggro_RemoveTarget& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InAggro, FFragment_Aggro_TargetMap& InTargetMap, FFragment_Aggro_Current& InCurrent, const FCk_Request_Aggro_ClearAllTargets& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InAggro, FFragment_Aggro_TargetMap& InTargetMap, FFragment_Aggro_Current& InCurrent, const FCk_Request_Aggro_SetActiveTarget& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InAggro, FFragment_Aggro_TargetMap& InTargetMap, FFragment_Aggro_Current& InCurrent, const FCk_Request_Aggro_ClearActiveTarget& InRequest) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // HandleRequests excludes owners already tagged for destruction, so a destroyed Aggro's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.

    class CKAGGRO_API FProcessor_Aggro_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Aggro_CancelPendingRequests,
        FCk_Handle_Aggro,
        TReadOnly<FFragment_Aggro_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            const FFragment_Aggro_Requests& InRequestsComp)
            -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // The one always-ticking processor: no MarkedDirtyBy on purpose (it is empty-view-skipped when no owners exist).

    class CKAGGRO_API FProcessor_Aggro_EvaluationPacer : public ck_exp::TProcessor<
        FProcessor_Aggro_EvaluationPacer,
        FCk_Handle_Aggro,
        TReadOnly<FFragment_Aggro_EvaluationParams>,
        TReadWrite<FFragment_Aggro_EvaluationClock>,
        TExclude<FTag_Aggro_NeedsSetup>,
        TExclude<FTag_Aggro_Disabled>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_AggroTarget_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            const FFragment_Aggro_EvaluationParams& InEvaluationParams,
            FFragment_Aggro_EvaluationClock& InEvaluationClock) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_Aggro_SelectActiveTarget : public ck_exp::TProcessor<
        FProcessor_Aggro_SelectActiveTarget,
        FCk_Handle_Aggro,
        TReadWrite<FFragment_Aggro_Current>,
        TReadOnly<FFragment_Aggro_SelectionParams>,
        TReadOnly<FFragment_Aggro_TargetMap>,
        FTag_Aggro_SelectionPending,
        TExclude<FTag_Aggro_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_AggroTarget_Forget>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_Aggro_SelectionPending;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            FFragment_Aggro_Current& InCurrent,
            const FFragment_Aggro_SelectionParams& InSelectionParams,
            const FFragment_Aggro_TargetMap& InTargetMap) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
