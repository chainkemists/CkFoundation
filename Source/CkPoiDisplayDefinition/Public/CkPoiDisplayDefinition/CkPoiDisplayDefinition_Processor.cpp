#include "CkPoiDisplayDefinition_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Log.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_PoiDisplayDefinition_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_PoiDisplayDefinition_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_PoiDisplayDefinition_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PoiDisplayDefinition_Current& InCurrent,
            FFragment_PoiDisplayDefinition_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        auto DisplayChanged = false;

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            DisplayChanged |= DoHandleRequest(InHandle, InCurrent, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        // One broadcast per drain, not per request: a caller that sets tint AND size hint this frame wakes
        // its consumers once, and a request that re-asserts the value already in Current wakes nobody.
        if (DisplayChanged)
        {
            UUtils_Signal_OnPoiDisplayDefinition_DisplayChanged::Broadcast(InHandle, MakePayload(InHandle));
        }

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_PoiDisplayDefinition_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_PoiDisplayDefinition_Current& InCurrent,
            const FCk_Request_PoiDisplayDefinition_SetTint& InRequest)
        -> bool
    {
        if (InRequest.Get_Tint().Equals(InCurrent._Tint))
        { return false; }

        poidisplaydefinition::VeryVerbose(TEXT("PoiDisplayDefinition [{}] Tint changed to [{}]"),
            InHandle, InRequest.Get_Tint().ToString());

        InCurrent._Tint = InRequest.Get_Tint();

        return true;
    }

    auto
        FProcessor_PoiDisplayDefinition_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_PoiDisplayDefinition_Current& InCurrent,
            const FCk_Request_PoiDisplayDefinition_SetSizeHint& InRequest)
        -> bool
    {
        if (InRequest.Get_SizeHint().Equals(InCurrent._SizeHint))
        { return false; }

        poidisplaydefinition::VeryVerbose(TEXT("PoiDisplayDefinition [{}] SizeHint changed to [{}]"),
            InHandle, InRequest.Get_SizeHint().ToString());

        InCurrent._SizeHint = InRequest.Get_SizeHint();

        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PoiDisplayDefinition_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PoiDisplayDefinition_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
