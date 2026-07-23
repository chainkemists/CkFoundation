#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
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
    // Resolves initial threat (owner default or per-target override), stamps the analytic-decay anchor + creation time,
    // applies the param-derived exception tags, and announces the acquisition.

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
    // Drains per-target requests: threat add/set (analytic-decay-to-now then apply, clamp, fire OnAggroThreatChanged),
    // perception counted-tag inc/dec/clear, and explicit forget. Stamps EvaluationPending (self) + SelectionPending
    // (owner) so a reactive burst retargets same-frame via the pump.

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
    // Per-target evaluation: analytic decay -> distance/score -> retention-band tag -> forget checks. Runs only on
    // targets stamped EvaluationPending (staggered clock fire or threat-request dirty). SERIAL for now — the plan's
    // primary path is TParallelProcessor; parallelization is the perf follow-up once the model is proven (Claude.md).

    class CKAGGRO_API FProcessor_AggroTarget_Evaluate : public ck_exp::TProcessor<
        FProcessor_AggroTarget_Evaluate,
        FCk_Handle_AggroTarget,
        TReadOnly<FFragment_AggroTarget_ThreatParams>,
        TReadOnly<FFragment_AggroTarget_ScoreParams>,
        TReadOnly<FFragment_AggroTarget_LifetimeParams>,
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
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_ScoreParams& InScoreParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            const FFragment_AggroTarget_Perception& InPerception,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Score& InScore) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Completes a forget: derive the reason, broadcast OnAggroTargetForgotten, prune the owner's map, clear the active
    // target if it was us, then destroy the target entity (deferred).

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
    // Safety net for externally-destroyed AggroTargets: prune the owner's map and clear the active target if it was us.

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
