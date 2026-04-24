#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_EntityLifetime_DestructionPhase_Endplay; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FProcessor_EntityScript_SpawnEntity_HandleRequests : public TProcessor<
            FProcessor_EntityScript_SpawnEntity_HandleRequests,
            ck::TReadOnly<FFragment_EntityScript_RequestSpawnEntity>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using MarkedDirtyBy = FFragment_EntityScript_RequestSpawnEntity;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_RequestSpawnEntity& InRequestFragment) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityScript_SpawnEntity& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_ContinueConstruction : public ck_exp::TProcessor<
            FProcessor_EntityScript_ContinueConstruction,
            FCk_Handle_EntityScript,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            FTag_EntityScript_ContinueConstruction,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_SpawnEntity_HandleRequests>;
        using MarkedDirtyBy = FTag_EntityScript_ContinueConstruction;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_Replicate : public ck_exp::TProcessor<
            FProcessor_EntityScript_Replicate,
            FCk_Handle_EntityScript,
            ck::TReadOnly<FRequest_EntityScript_Replicate>,
            FTag_EntityScript_FinishConstruction,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_ContinueConstruction>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;
        using MarkedDirtyBy = FRequest_EntityScript_Replicate;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FRequest_EntityScript_Replicate& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_FinishConstruction : public ck_exp::TProcessor<
            FProcessor_EntityScript_FinishConstruction,
            FCk_Handle_EntityScript,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            FTag_EntityScript_FinishConstruction,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_Replicate>;
        using MarkedDirtyBy = FTag_EntityScript_FinishConstruction;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_PendingReplicationRetry : public ck_exp::TProcessor<
            FProcessor_EntityScript_PendingReplicationRetry,
            FCk_Handle_EntityScript,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            FTag_EntityScript_PendingReplicationRetry,
            ck::TReadOnly<FFragment_EntityScript_PendingReplicationRetryTimestamp>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_FinishConstruction>;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent,
            const FFragment_EntityScript_PendingReplicationRetryTimestamp& InTimestamp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_BeginPlay : public ck_exp::TProcessor<
            FProcessor_EntityScript_BeginPlay,
            FCk_Handle_EntityScript,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            FTag_EntityScript_BeginPlay,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;
        using RunAfter = TDepList<FProcessor_EntityScript_FinishConstruction>;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;
        using MarkedDirtyBy = FTag_EntityScript_BeginPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityScript_EndPlay : public ck_exp::TProcessor<
            FProcessor_EntityScript_EndPlay,
            FCk_Handle_EntityScript,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            FTag_EntityScript_HasBegunPlay,
            CK_IF_END_PLAY>
    {
    public:
        // EntityScript_EndPlay lives alongside the other feature *_EndPlay processors in FGroup_EndPlay.
        // The cross-group RunAfter on DestructionPhase_Endplay is superseded by the group-level ordering
        // (FGroup_EndPlay::RunAfter = FGroup_EntityLifecycle).
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
