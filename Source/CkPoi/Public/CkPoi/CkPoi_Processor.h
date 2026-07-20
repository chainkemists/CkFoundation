#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPoi/CkPoi_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPOI_API FProcessor_Poi_Setup : public ck_exp::TProcessor<
        FProcessor_Poi_Setup,
        FCk_Handle_Poi,
        ck::TReadOnly<FFragment_Poi_Params>,
        ck::TReadWrite<FFragment_Poi_Current>,
        FTag_Poi_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_Poi_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPoiEntity,
            const FFragment_Poi_Params& InParams,
            FFragment_Poi_Current& InCurrentComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPOI_API FProcessor_Poi_HandleRequests
        : public ck_exp::TProcessor<FProcessor_Poi_HandleRequests, FCk_Handle_Poi,
            ck::TReadWrite<FFragment_Poi_Current>, ck::TReadOnly<FFragment_Poi_Params>, ck::TReadWrite<FFragment_Poi_Requests>,
            TExclude<FTag_Poi_NeedsSetup>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Poi_Setup>;
        using MarkedDirtyBy = FFragment_Poi_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPoiEntity,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            FFragment_Poi_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_EnableDisable& InRequest) -> void;

        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_AddStateTag& InRequest) -> void;

        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_RemoveStateTag& InRequest) -> void;

        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_SetStateTags& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
