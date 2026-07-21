#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPool/EntityPool/CkEntityPool_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPOOL_API FProcessor_EntityPool_Setup : public ck_exp::TProcessor<
        FProcessor_EntityPool_Setup,
        FCk_Handle_EntityPool,
        ck::TReadOnly<FFragment_EntityPool_Params>,
        ck::TReadWrite<FFragment_EntityPool_Current>,
        FTag_EntityPool_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_EntityPool_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPOOL_API FProcessor_EntityPool_Prewarm : public ck_exp::TProcessor<
        FProcessor_EntityPool_Prewarm,
        FCk_Handle_EntityPool,
        ck::TReadOnly<FFragment_EntityPool_Params>,
        ck::TReadWrite<FFragment_EntityPool_Current>,
        FTag_EntityPool_PrewarmInProgress,
        TExclude<FTag_EntityPool_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_EntityPool_Setup>;
        using MarkedDirtyBy = FTag_EntityPool_PrewarmInProgress;

        // Sticky marker + time-sliced budget: a pump pass would spend multiple budgets in one frame,
        // defeating the amortization the budget exists for
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent)
            -> void;

    public:
        // Shared with FProcessor_EntityPool_HandleRequests (grow-on-miss). Creates the pooled child entity,
        // kicks off its EntityScript construction, and inflates the live/in-flight counters
        static auto
        DoInitiate_InstanceSpawn(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPOOL_API FProcessor_EntityPool_HandleRequests
        : public ck_exp::TProcessor<FProcessor_EntityPool_HandleRequests, FCk_Handle_EntityPool,
            ck::TReadOnly<FFragment_EntityPool_Params>, ck::TReadWrite<FFragment_EntityPool_Current>,
            ck::TReadWrite<FFragment_EntityPool_Requests>,
            TExclude<FTag_EntityPool_NeedsSetup>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_EntityPool_Setup, FProcessor_EntityPool_Prewarm>;
        using MarkedDirtyBy = FFragment_EntityPool_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            FFragment_EntityPool_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_Acquire& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_Release& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_HandleConstructed& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_HandleDestroyed& InRequest) -> void;

    private:
        static auto
        DoDeliver(
            HandleType InPool,
            FFragment_EntityPool_Current& InCurrent,
            FCk_Handle InEntity,
            const FInstancedStruct& InPerUseParams,
            FCk_Handle InTicket) -> void;

        static auto
        DoTryDeliver_ToPendingAcquire(
            HandleType InPool,
            FFragment_EntityPool_Current& InCurrent,
            FCk_Handle InEntity) -> bool;

        static auto
        DoFulfill_Failed(
            HandleType InPool,
            FCk_Handle InTicket) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Reconciles pool bookkeeping when a POOLED entity is destroyed externally (dormant-list eviction, in-use
    // "steal" semantics). Cascade teardown (the pool itself dying) is filtered out by the owning-pool validity check
    class CKPOOL_API FProcessor_EntityPool_HandleDestroyedPooledEntity : public ck_exp::TProcessor<
        FProcessor_EntityPool_HandleDestroyedPooledEntity,
        FCk_Handle,
        ck::TReadOnly<FFragment_EntityPooled>,
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
            HandleType InEntity,
            const FFragment_EntityPooled& InPooled)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // When the POOL is destroyed mid-game, parked acquires are fulfilled with Failed so gameplay is not left
    // waiting on a promise that can never resolve. Dormant/in-use entities are lifetime children of the pool —
    // the destruction cascade already handles them
    class CKPOOL_API FProcessor_EntityPool_EndPlay : public ck_exp::TProcessor<
        FProcessor_EntityPool_EndPlay,
        FCk_Handle_EntityPool,
        ck::TReadWrite<FFragment_EntityPool_Current>,
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
            HandleType InPool,
            FFragment_EntityPool_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
