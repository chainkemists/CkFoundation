#include "CkPoi_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkPoi/CkPoi_Log.h"
#include "CkPoi/CkPoi_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Poi_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Poi_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Poi_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPoiEntity,
            const FFragment_Poi_Params& InParams,
            FFragment_Poi_Current& InCurrentComp)
        -> void
    {
        InPoiEntity.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Poi_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPoiEntity,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            FFragment_Poi_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InDeltaT, InPoiEntity, InCurrentComp, InParamsComp, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        {
            InPoiEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_Poi_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_EnableDisable& InRequest)
        -> void
    {
        const auto CurrentState = InHandle.Has<FTag_Poi_Disabled>()
            ? ECk_EnableDisable::Disable
            : ECk_EnableDisable::Enable;

        // No-op requests must NOT broadcast.
        if (CurrentState == InRequest.Get_EnableDisable())
        { return; }

        poi::VeryVerbose(TEXT("Handling EnableDisable [{}] Request for Poi with Entity [{}]"),
            InRequest.Get_EnableDisable(), InHandle);

        switch (InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                InHandle.Remove<FTag_Poi_Disabled>();
                break;
            }
            case ECk_EnableDisable::Disable:
            {
                InHandle.Add<FTag_Poi_Disabled>();
                break;
            }
        }

        UUtils_Signal_OnPoiEnableDisable::Broadcast(InHandle,
            MakePayload(InHandle, InRequest.Get_EnableDisable()));
    }

    auto
        FProcessor_Poi_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_AddStateTag& InRequest)
        -> void
    {
        if (InCurrentComp._StateTags.HasTagExact(InRequest.Get_Tag()))
        { return; }

        poi::VeryVerbose(TEXT("Handling AddStateTag [{}] Request for Poi with Entity [{}]"),
            InRequest.Get_Tag(), InHandle);

        InCurrentComp._StateTags.AddTag(InRequest.Get_Tag());

        UUtils_Signal_OnPoiStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, InCurrentComp._StateTags));
    }

    auto
        FProcessor_Poi_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_RemoveStateTag& InRequest)
        -> void
    {
        if (NOT InCurrentComp._StateTags.HasTagExact(InRequest.Get_Tag()))
        { return; }

        poi::VeryVerbose(TEXT("Handling RemoveStateTag [{}] Request for Poi with Entity [{}]"),
            InRequest.Get_Tag(), InHandle);

        InCurrentComp._StateTags.RemoveTag(InRequest.Get_Tag());

        UUtils_Signal_OnPoiStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, InCurrentComp._StateTags));
    }

    auto
        FProcessor_Poi_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Poi_Current& InCurrentComp,
            const FFragment_Poi_Params& InParamsComp,
            const FCk_Request_Poi_SetStateTags& InRequest)
        -> void
    {
        if (InCurrentComp._StateTags == InRequest.Get_Tags())
        { return; }

        poi::VeryVerbose(TEXT("Handling SetStateTags Request for Poi with Entity [{}]"), InHandle);

        InCurrentComp._StateTags = InRequest.Get_Tags();

        UUtils_Signal_OnPoiStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, InCurrentComp._StateTags));
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
