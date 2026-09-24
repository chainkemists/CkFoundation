#include "CkVisualLod_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedUtils.h"

#include "CkVisualLod/CkVisualLod_Log.h"
#include "CkVisualLod/CkVisualLod_Utils.h"
#include "CkVisualLod/CkVisualLodArbiter_Processor.h"
#include "CkVisualLod/CkVisualLodArbiter_Utils.h"

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
            FFragment_VisualLod& InVisualLod)
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
            FFragment_VisualLod& InVisualLod,
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

            DoHandleRequest(InHandle, InVisualLod, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod& InVisualLod,
            const FCk_Request_VisualLod_SetArbiter& InRequest)
        -> void
    {
        InVisualLod._Arbiter = InRequest.Get_Arbiter();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod& InVisualLod,
            const FCk_Request_VisualLod_SetVisibility& InRequest)
        -> void
    {
        // The latch is the contract; the arbiter's next update applies it to whichever
        // representation is live (hide member / hide proxy / release the slot)
        InVisualLod._Hidden = InRequest.Get_ShowHide() == ECk_VisualLod_ShowHide::Hide;
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod& InVisualLod,
            const FCk_Request_VisualLod_SetFarAnim& InRequest)
        -> void
    {
        InVisualLod._FarAnim = InRequest.Get_FarAnim();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod& InVisualLod,
            const FCk_Request_VisualLod_SetRenderer& InRequest)
        -> void
    {
        InVisualLod._RendererOverride = InRequest.Get_Renderer();

        // The rooted batch pins the PREVIOUS renderer; the next promote must load the new one
        InVisualLod._LoadedAssets = {};
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod& InVisualLod,
            const FCk_Request_VisualLod_Suspend& InRequest)
        -> void
    {
        InHandle.AddOrGet<FTag_VisualLod_Suspended>();
    }

    auto
        FProcessor_VisualLod_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod& InVisualLod,
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
            FFragment_VisualLod& InVisualLod)
        -> void
    {
        const auto Crowd = InVisualLod._Crowd.Get();
        if (ck::IsValid(Crowd) && InVisualLod._MemberIndex != INDEX_NONE)
        {
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, InVisualLod._MemberIndex, false);
            UCk_Utils_IskmBatched_UE::Clear_CrowdMemberCosmetics(Crowd, InVisualLod._MemberIndex);
        }

        auto Arbiter = InVisualLod._Arbiter;
        if (ck::IsValid(Arbiter) && UCk_Utils_VisualLodArbiter_UE::Has(Arbiter))
        {
            auto& ArbiterCurrent = Arbiter.Get<FFragment_VisualLodArbiter>();

            if (InVisualLod._MemberIndex != INDEX_NONE)
            {
                FProcessor_VisualLodArbiter_Update::DoRecycle_Slot(ArbiterCurrent,
                    InHandle.Get<FFragment_VisualLod_Params>().Get_CrowdIndex(),
                    InHandle, InVisualLod._MemberIndex);
            }

            if (InVisualLod._Promoted)
            { FProcessor_VisualLodArbiter_Update::DoRefund_Charge(ArbiterCurrent, InHandle, InVisualLod); }
        }

        // The node is a lifetime descendant and would cascade anyway; the explicit request makes
        // the pooled-SKMC release deterministic rather than cascade-ordered
        if (ck::IsValid(InVisualLod._VisualNode))
        {
            auto Node = InVisualLod._VisualNode;
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Node);
        }

        InVisualLod._MemberIndex = INDEX_NONE;
        InVisualLod._Crowd       = nullptr;
        InVisualLod._Promoted    = false;
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
