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

    // Arms the owner's evaluation pacer to Interval + a per-entity jitter, staggering the fleet's evaluation cohorts.
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
    // Drains owner-level requests: AddThreat (O(1) map lookup; create-on-miss per CreatePolicy; routes the threat to
    // the target's request queue), RemoveTarget / ClearAllTargets (stamp PendingForget), and the taunt path
    // SetActiveTarget / ClearActiveTarget (flip IsActive + Current, broadcast OnAggroActiveTargetChanged).

    class CKAGGRO_API FProcessor_Aggro_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Aggro_HandleRequests,
        FCk_Handle_Aggro,
        TReadWrite<FFragment_Aggro_Requests>,
        TReadWrite<FFragment_Aggro_TargetMap>,
        TReadWrite<FFragment_Aggro_Current>,
        TExclude<FTag_Aggro_NeedsSetup>,
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
    // The one always-ticking processor (no MarkedDirtyBy) — empty-view-skipped when no aggro owners exist. Ticks the
    // pacer chrono; on fire, rearms with fresh jitter, bumps the debug eval counter, and stamps EvaluationPending on
    // every tracked target (the record's one hot use, at eval cadence) + SelectionPending on itself.

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
    // Argmax over the tracked-target map (eligibility + hysteresis gates); flips IsActive + Current and broadcasts
    // OnAggroActiveTargetChanged on a switch. Cost is O(dirty-owners x cap), not O(all targets).

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
