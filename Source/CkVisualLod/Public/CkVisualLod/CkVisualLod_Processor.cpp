#include "CkVisualLod_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVisualLod/CkVisualLod_Log.h"
#include "CkVisualLod/CkVisualLod_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLod_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLod_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLod_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLod_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VisualLod_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLod_Params& InParams,
            FFragment_VisualLod_Current& InCurrent)
        -> void
    {
        // Arbiter resolution (tag -> live arbiter entity) is the arbiter update's job — it owns
        // the domain census. Setup only consumes the marker; an entity with no resolved arbiter
        // stays unmanaged
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLod_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            FFragment_VisualLod_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            DoHandleRequest(InHandle, InCurrent, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetArbiter& InRequest)
        -> void
    {
        InCurrent._Arbiter = InRequest.Get_Arbiter();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetVisibility& InRequest)
        -> void
    {
        // The latch is the contract; the arbiter's next update applies it to whichever
        // representation is live (hide member / hide proxy / release the slot)
        InCurrent._Hidden = InRequest.Get_ShowHide() == ECk_VisualLod_ShowHide::Hide;
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetFarAnim& InRequest)
        -> void
    {
        InCurrent._FarAnim = InRequest.Get_FarAnim();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetRenderer& InRequest)
        -> void
    {
        InCurrent._RendererOverride = InRequest.Get_Renderer();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_Suspend& InRequest)
        -> void
    {
        InHandle.AddOrGet<FTag_VisualLod_Suspended>();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_Resume& InRequest)
        -> void
    {
        InHandle.Try_Remove<FTag_VisualLod_Suspended>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLod_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent)
        -> void
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLod_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLod_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
