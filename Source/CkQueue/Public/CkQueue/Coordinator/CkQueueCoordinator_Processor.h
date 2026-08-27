#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkQueue/Coordinator/CkQueueCoordinator_Fragment.h"
#include "CkQueue/Queue/CkQueue_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_QueueCoordinator_EndPlay;

    class CKQUEUE_API FProcessor_QueueCoordinator_Setup : public ck_exp::TProcessor<
        FProcessor_QueueCoordinator_Setup,
        FCk_Handle_QueueCoordinator,
        ck::TReadWrite<FFragment_QueueCoordinator_Current>,
        FTag_QueueCoordinator_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_QueueCoordinator_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCoordinator,
            FFragment_QueueCoordinator_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_QueueCoordinator_HandleRequests : public ck_exp::TProcessor<
        FProcessor_QueueCoordinator_HandleRequests,
        FCk_Handle_QueueCoordinator,
        ck::TReadOnly<FFragment_QueueCoordinator_Params>,
        ck::TReadWrite<FFragment_QueueCoordinator_Current>,
        ck::TReadWrite<FFragment_QueueCoordinator_Requests>,
        TExclude<FTag_QueueCoordinator_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_QueueCoordinator_Setup>;
        using RunBefore = TDepList<FProcessor_Queue_HandleRequests>;
        using MarkedDirtyBy = FFragment_QueueCoordinator_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            FFragment_QueueCoordinator_Requests& InRequests)
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            TMap<FCk_Handle_Queue, int32>& InOutProjectedAdmissions,
            const FCk_Request_QueueCoordinator_RegisterQueue& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            TMap<FCk_Handle_Queue, int32>& InOutProjectedAdmissions,
            const FCk_Request_QueueCoordinator_UnregisterQueue& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            TMap<FCk_Handle_Queue, int32>& InOutProjectedAdmissions,
            const FCk_Request_QueueCoordinator_SelectQueue& InRequest)
            -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_QueueCoordinator_Reconcile : public ck_exp::TProcessor<
        FProcessor_QueueCoordinator_Reconcile,
        FCk_Handle_QueueCoordinator,
        ck::TReadWrite<FFragment_QueueCoordinator_Current>,
        FTag_QueueCoordinator_NeedsReconcile,
        TExclude<FTag_QueueCoordinator_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_QueueCoordinator_HandleRequests>;
        using MarkedDirtyBy = FTag_QueueCoordinator_NeedsReconcile;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCoordinator,
            FFragment_QueueCoordinator_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_QueueCoordinator_EndPlay : public ck_exp::TProcessor<
        FProcessor_QueueCoordinator_EndPlay,
        FCk_Handle_QueueCoordinator,
        ck::TReadWrite<FFragment_QueueCoordinator_Current>,
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
            HandleType InCoordinator,
            FFragment_QueueCoordinator_Current& InCurrent)
            -> void;
    };

    class CKQUEUE_API FProcessor_QueueCoordinator_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_QueueCoordinator_CancelPendingRequests,
        FCk_Handle_QueueCoordinator,
        ck::TReadOnly<FFragment_QueueCoordinator_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_QueueCoordinator_EndPlay>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Requests& InRequests)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
