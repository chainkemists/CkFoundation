#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkQueue/Queue/CkQueue_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Queue_EndPlay;

    class CKQUEUE_API FProcessor_Queue_Setup : public ck_exp::TProcessor<
        FProcessor_Queue_Setup,
        FCk_Handle_Queue,
        ck::TReadOnly<FFragment_Queue_Params>,
        ck::TReadWrite<FFragment_Queue_Current>,
        FTag_Queue_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_Queue_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_Queue_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Queue_HandleRequests,
        FCk_Handle_Queue,
        ck::TReadOnly<FFragment_Queue_Params>,
        ck::TReadWrite<FFragment_Queue_Current>,
        ck::TReadWrite<FFragment_Queue_Requests>,
        TExclude<FTag_Queue_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Queue_Setup>;
        using MarkedDirtyBy = FFragment_Queue_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            FFragment_Queue_Requests& InRequests)
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_Join& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_Leave& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_AdvanceOrigin& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_SetOrigins& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_SetLayout& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_SetMovementSuppressed& InRequest)
            -> bool;

        static auto
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            const FCk_Request_Queue_ReportMovementOutcome& InRequest)
            -> bool;

        static auto
        FindMemberIndex(
            const FFragment_Queue_Current& InCurrent,
            const FCk_Handle& InMember)
            -> int32;

        static auto
        HasOriginCapacityForCount(
            const TArray<FCk_Queue_Origin>& InOrigins,
            int32 InMemberCount)
            -> bool;

        static auto
        InvalidateAssignmentsForReflow(
            HandleType InQueue,
            FFragment_Queue_Current& InCurrent)
            -> void;

        static auto
        RebuildRanks(
            HandleType InQueue,
            FFragment_Queue_Current& InCurrent,
            ECk_Queue_EventReason InReason)
            -> void;

        static auto
        RefreshPressure(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent)
            -> void;

        static auto
        BroadcastMemberEvent(
            HandleType InQueue,
            const FCk_Queue_MemberSnapshot& InSnapshot,
            ECk_Queue_MemberState InPreviousState,
            ECk_Queue_EventReason InReason,
            int32 InRevision)
            -> void;

        static auto
        MarkFormationDirty(
            HandleType InQueue,
            FFragment_Queue_Current& InCurrent,
            ECk_Queue_EventReason InReason)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_Queue_Reconcile : public ck_exp::TProcessor<
        FProcessor_Queue_Reconcile,
        FCk_Handle_Queue,
        ck::TReadOnly<FFragment_Queue_Params>,
        ck::TReadWrite<FFragment_Queue_Current>,
        TExclude<FTag_Queue_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Queue_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_Queue_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Queue_CancelPendingRequests,
        FCk_Handle_Queue,
        ck::TReadOnly<FFragment_Queue_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_Queue_EndPlay>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InQueue,
            const FFragment_Queue_Requests& InRequests)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKQUEUE_API FProcessor_Queue_EndPlay : public ck_exp::TProcessor<
        FProcessor_Queue_EndPlay,
        FCk_Handle_Queue,
        ck::TReadWrite<FFragment_Queue_Current>,
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
            HandleType InQueue,
            FFragment_Queue_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
