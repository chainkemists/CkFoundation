#pragma once

#include "CkEntityTag/Query/CkEntityTagQuery_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYTAG_API FProcessor_EntityTagQuery_HandleRequests : public ck_exp::TProcessor<
            FProcessor_EntityTagQuery_HandleRequests,
            FCk_Handle_EntityTagQuery,
            ck::TReadWrite<FFragment_EntityTagQuery_Current>,
            ck::TReadWrite<FFragment_EntityTagQuery_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityTagQuery_Requests;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityTagQuery_Current& InCurrent,
            FFragment_EntityTagQuery_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            FFragment_EntityTagQuery_Current& InCurrent,
            const FCk_Request_EntityTagQuery_AddRequirement& InRequest) -> void;

        static auto
        DoHandleRequest(
            FFragment_EntityTagQuery_Current& InCurrent,
            const FCk_Request_EntityTagQuery_RemoveRequirement& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKENTITYTAG_API FProcessor_EntityTagQuery_Evaluate : public ck_exp::TProcessor<
            FProcessor_EntityTagQuery_Evaluate,
            FCk_Handle_EntityTagQuery,
            ck::TReadWrite<FFragment_EntityTagQuery_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using RunAfter = TDepList<FProcessor_EntityTagQuery_HandleRequests>;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityTagQuery_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // When a tagged entity dies, scrub it from every tracked query's _ResultsPerRequirement.
    class CKENTITYTAG_API FProcessor_EntityTagQuery_TrackedEntity_Destructor : public ck_exp::TProcessor<
            FProcessor_EntityTagQuery_TrackedEntity_Destructor,
            FCk_Handle,
            ck::TReadOnly<FFragment_EntityTagQuery_TrackedByQueries>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityTagQuery_TrackedByQueries& InTracked) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // When a query is destroyed, scrub it from every tracked tagged entity's _Queries list.
    // Symmetric cleanup so dangling entries don't accumulate.
    class CKENTITYTAG_API FProcessor_EntityTagQuery_Query_Destructor : public ck_exp::TProcessor<
            FProcessor_EntityTagQuery_Query_Destructor,
            FCk_Handle_EntityTagQuery,
            ck::TReadOnly<FFragment_EntityTagQuery_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityTagQuery_Current& InCurrent) const -> void;
    };
}
