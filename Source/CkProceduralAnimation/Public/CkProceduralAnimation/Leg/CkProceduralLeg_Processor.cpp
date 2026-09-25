#include "CkProceduralAnimation/Leg/CkProceduralLeg_Processor.h"

#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralLeg_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralLeg_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_leg_processor
{
    auto
        Get_IsPartLive(
            const FCk_Handle_Transform& InPart)
        -> bool
    {
        return ck::IsValid(InPart) && NOT InPart.Has<ck::FTag_DestroyEntity_Initiate>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralLeg_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ProceduralLeg_Requests& InRequestsComp)
        -> void
    {
        auto Requests = MoveTemp(InRequestsComp._Requests);
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(Requests, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed_Cancelled;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            if (InHandle.Has<FTag_DestroyEntity_Initiate>())
            { return; }

            DoHandleRequest(InHandle, InRequest);
            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_ProceduralLeg_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ProceduralLeg_EnableDisable& InRequest)
        -> void
    {
        switch (InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                InHandle.Try_Remove<FTag_ProceduralLeg_Disabled>();
                break;
            }
            case ECk_EnableDisable::Disable:
            {
                InHandle.AddOrGet<FTag_ProceduralLeg_Disabled>();
                break;
            }
        }
    }

    auto
        FProcessor_ProceduralLeg_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ProceduralLeg_Detach& InRequest)
        -> void
    {
        auto Released = FCk_ProceduralLeg_ReleasedParts{};
        if (UCk_Utils_ProceduralRig_UE::Has(InHandle))
        {
            const auto& Chain = InHandle.Get<FFragment_ProceduralRig_Params>();
            auto Parts = algo::Filter(Chain.Get_Segments(), [](const FCk_Handle_Transform& InPart) -> bool
            {
                return ck_procedural_leg_processor::Get_IsPartLive(InPart);
            });

            if (ck_procedural_leg_processor::Get_IsPartLive(Chain.Get_Foot()))
            { Parts.Add(Chain.Get_Foot()); }

            Released.Set_Parts(Parts);
        }

        if (InRequest.Get_PartsOwnership() == ECk_ProceduralLeg_ReleasedPartsOwnership::TransferToWorld)
        {
            for (auto Part : Released.Get_Parts())
            {
                UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(Part, UCk_Utils_EntityLifetime_UE::Get_TransientEntity(Part));
            }
        }

        UUtils_Signal_OnProceduralLeg_Detached::Broadcast(InHandle, MakePayload(InHandle, Released));

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralLeg_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralLeg_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
