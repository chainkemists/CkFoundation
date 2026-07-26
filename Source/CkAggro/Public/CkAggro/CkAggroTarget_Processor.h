#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkAggro/CkAggroTarget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Aggro_Setup;
    class FProcessor_Aggro_HandleRequests;
    class FProcessor_Aggro_EvaluationPacer;

    // ----------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_AggroTarget_Setup : public ck_exp::TProcessor<
        FProcessor_AggroTarget_Setup,
        FCk_Handle_AggroTarget,
        TReadOnly<FFragment_AggroTarget_ThreatParams>,
        TReadOnly<FFragment_AggroTarget_LifetimeParams>,
        TReadWrite<FFragment_AggroTarget_TargetInfo>,
        TReadWrite<FFragment_AggroTarget_Threat>,
        TReadWrite<FFragment_AggroTarget_Perception>,
        FTag_AggroTarget_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Aggro_Setup>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_AggroTarget_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_AggroTarget_HandleRequests : public ck_exp::TProcessor<
        FProcessor_AggroTarget_HandleRequests,
        FCk_Handle_AggroTarget,
        TReadOnly<FFragment_AggroTarget_ThreatParams>,
        TReadOnly<FFragment_AggroTarget_TargetInfo>,
        TReadWrite<FFragment_AggroTarget_Threat>,
        TReadWrite<FFragment_AggroTarget_Perception>,
        TReadWrite<FFragment_AggroTarget_Requests>,
        TExclude<FTag_AggroTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Aggro_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FFragment_AggroTarget_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            FFragment_AggroTarget_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(HandleType InTarget, const FFragment_AggroTarget_ThreatParams& InThreatParams, const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat, FFragment_AggroTarget_Perception& InPerception, const FCk_Request_AggroTarget_AddThreat& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InTarget, const FFragment_AggroTarget_ThreatParams& InThreatParams, const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat, FFragment_AggroTarget_Perception& InPerception, const FCk_Request_AggroTarget_SetThreat& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InTarget, const FFragment_AggroTarget_ThreatParams& InThreatParams, const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat, FFragment_AggroTarget_Perception& InPerception, const FCk_Request_AggroTarget_MarkPerceived& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InTarget, const FFragment_AggroTarget_ThreatParams& InThreatParams, const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat, FFragment_AggroTarget_Perception& InPerception, const FCk_Request_AggroTarget_MarkUnperceived& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InTarget, const FFragment_AggroTarget_ThreatParams& InThreatParams, const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat, FFragment_AggroTarget_Perception& InPerception, const FCk_Request_AggroTarget_ResetPerception& InRequest) -> void;
        static auto
        DoHandleRequest(HandleType InTarget, const FFragment_AggroTarget_ThreatParams& InThreatParams, const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat, FFragment_AggroTarget_Perception& InPerception, const FCk_Request_AggroTarget_Forget& InRequest) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // PARALLEL: the per-target body does registry READS plus writes to the target's OWN Threat/Score fragments; every
    // structural tag flip is DEFERRED through the read-only handle's per-task command buffer and flushed
    // single-threaded. 'Now' is hoisted to the game thread once per tick (DoTick). Contract: CkAggro/CLAUDE.md.

    class CKAGGRO_API FProcessor_AggroTarget_Evaluate : public TParallelProcessor<
        FProcessor_AggroTarget_Evaluate,
        FCk_Handle_AggroTarget,
        TReadOnly<FFragment_AggroTarget_ThreatParams>,
        TReadOnly<FFragment_AggroTarget_ScoreParams>,
        TReadOnly<FFragment_AggroTarget_LifetimeParams>,
        TReadOnly<FFragment_AggroTarget_SpatialParams>,
        TReadOnly<FFragment_AggroTarget_ForgetParams>,
        TReadOnly<FFragment_AggroTarget_TargetInfo>,
        TReadOnly<FFragment_AggroTarget_Perception>,
        TReadWrite<FFragment_AggroTarget_Threat>,
        TReadWrite<FFragment_AggroTarget_Score>,
        FTag_AggroTarget_EvaluationPending,
        TExclude<FTag_AggroTarget_NeedsSetup>,
        TExclude<FTag_AggroTarget_PendingForget>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Aggro_EvaluationPacer>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_AggroTarget_EvaluationPending;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_ScoreParams& InScoreParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            const FFragment_AggroTarget_SpatialParams& InSpatialParams,
            const FFragment_AggroTarget_ForgetParams& InForgetParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            const FFragment_AggroTarget_Perception& InPerception,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Score& InScore) const -> void;

    private:
        // Set on the game thread by DoTick before dispatch; read-only during the parallel phase.
        FCk_Time _Now;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_AggroTarget_Forget : public ck_exp::TProcessor<
        FProcessor_AggroTarget_Forget,
        FCk_Handle_AggroTarget,
        TReadOnly<FFragment_AggroTarget_ThreatParams>,
        TReadOnly<FFragment_AggroTarget_LifetimeParams>,
        TReadOnly<FFragment_AggroTarget_TargetInfo>,
        TReadOnly<FFragment_AggroTarget_Perception>,
        TReadOnly<FFragment_AggroTarget_Threat>,
        FTag_AggroTarget_PendingForget,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_AggroTarget_Evaluate>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_AggroTarget_PendingForget;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            const FFragment_AggroTarget_Perception& InPerception,
            const FFragment_AggroTarget_Threat& InThreat) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Safety net for AggroTargets destroyed from OUTSIDE the Forget path (which does its own owner cleanup).

    class CKAGGRO_API FProcessor_AggroTarget_EndPlay : public ck_exp::TProcessor<
        FProcessor_AggroTarget_EndPlay,
        FCk_Handle_AggroTarget,
        TReadOnly<FFragment_AggroTarget_TargetInfo>,
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
            HandleType InTarget,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
