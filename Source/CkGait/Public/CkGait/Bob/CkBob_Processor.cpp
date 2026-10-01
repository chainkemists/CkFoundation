#include "CkBob_Processor.h"
#include "CkGait/Bob/CkBob_Kernel.h"
#include "CkGait/Gait/CkGait_Utils.h"
#include "CkGait/CkGait_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Bob_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Bob_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Bob_CancelPendingRequests);

auto
    ck::FProcessor_Bob_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Bob_Tunables& InTunables,
        FFragment_Bob& InBob,
        FFragment_Bob_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    algo::ForEachRequest(RequestsCopy, ck::Visitor([&](const auto& InRequest)
    {
        auto Result = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);
        if (InHandle.Has<FTag_DestroyEntity_Initiate>())
        {
            Result = ECk_Request_OperationResult::Failed_Cancelled;
            return;
        }

        gait::VeryVerbose(TEXT("Handling Bob request on [{}]"), InHandle);
        Result = DoHandleRequest(InHandle, InTunables, InBob, InRequest);
    }), policy::DontResetContainer{});
    if (InRequests._Requests.IsEmpty())
    { InHandle.Remove<MarkedDirtyBy>(); }
}

auto
    ck::FProcessor_Bob_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Bob_Tunables& InTunables,
        FFragment_Bob& InBob,
        const FCk_Request_Bob_UpdateSpec& InRequest)
    -> ECk_Request_OperationResult
{
    const auto IsValidSpec = bob::Get_IsSpecValid(InRequest.Get_Spec());
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Bob UpdateSpec rejected invalid spec on [{}]"), InHandle)
    { return ECk_Request_OperationResult::Failed; }

    const auto& NewGait = InRequest.Get_Spec().Get_Gait();
    const auto IsValidGait = ck::IsValid(NewGait) && UCk_Utils_Gait_UE::Has(NewGait);
    CK_ENSURE_IF_NOT(IsValidGait, TEXT("Bob UpdateSpec rejected invalid gait [{}] on [{}]"), NewGait, InHandle)
    { return ECk_Request_OperationResult::Failed; }

    // A re-pointed bob adopts the new gait's landing count, as Add does, so the new gait's past landings do not kick
    // the spring.
    if (NewGait != InTunables.Get_Gait())
    { InBob._ConsumedLandingCount = UCk_Utils_Gait_UE::Get_LandingCount(NewGait); }

    InTunables = InRequest.Get_Spec();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Bob_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Bob_Tunables& InTunables,
        FFragment_Bob& InBob,
        const FCk_Request_Bob_EnableDisable& InRequest)
    -> ECk_Request_OperationResult
{
    switch (InRequest.Get_EnableDisable())
    {
        case ECk_EnableDisable::Enable:
        {
            InHandle.Try_Remove<FTag_Bob_Disabled>();
            // A landing that happened while disabled must not kick the spring on re-enable.
            const auto& Gait = InTunables.Get_Gait();
            InBob._ConsumedLandingCount = (ck::IsValid(Gait) && UCk_Utils_Gait_UE::Has(Gait))
                ? UCk_Utils_Gait_UE::Get_LandingCount(Gait)
                : InBob._ConsumedLandingCount;
            break;
        }

        case ECk_EnableDisable::Disable:
        {
            InHandle.AddOrGet<FTag_Bob_Disabled>();
            break;
        }

        default:
        {
            CK_INVALID_ENUM(InRequest.Get_EnableDisable());
            return ECk_Request_OperationResult::Failed;
        }
    }

    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Bob_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Bob_Tunables& InTunables,
        FFragment_Bob& InBob,
        const FCk_Request_Bob_Reset& InRequest)
    -> ECk_Request_OperationResult
{
    // Does not publish and does not touch _LastWrittenOffset: Update republishes the rest offset through the normal path,
    // which keeps its foreign-write check free of a false positive. Re-seeding the consumed landing count swallows a
    // landing that arrived before the reset instead of kicking the freshly zeroed spring.
    InBob._Spring = {};
    InBob._Smoothed = {};
    const auto& Gait = InTunables.Get_Gait();
    const auto GaitIsLive = ck::IsValid(Gait) && UCk_Utils_Gait_UE::Has(Gait);
    InBob._ConsumedLandingCount = GaitIsLive ? UCk_Utils_Gait_UE::Get_LandingCount(Gait) : 0;
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Bob_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Bob_Tunables& InTunables,
        FFragment_Bob& InBob,
        const FCk_Request_Bob_SetRestOffset& InRequest)
    -> ECk_Request_OperationResult
{
    // Does not publish: Update composes BobOffset * Rest, sees it differ from the node's actual offset and republishes.
    InBob._RestOffset = InRequest.Get_RestOffset();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Bob_CancelPendingRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Bob_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    request::FireCancelledForPending(InHandle, RequestsCopy);
}

auto
    ck::FProcessor_Bob_Update::
    DoPublishOffset(
        HandleType InHandle,
        FFragment_Bob& InBob,
        const FTransform& InOffset)
    -> void
{
    auto Node = UCk_Utils_SceneNode_UE::Cast(InHandle);
    UCk_Utils_SceneNode_UE::Request_UpdateOffset(Node, FCk_Request_SceneNode_UpdateRelativeTransform{InOffset}, {});
    InBob._LastWrittenOffset = InOffset;
}

auto
    ck::FProcessor_Bob_Update::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Bob_Tunables& InTunables,
        FFragment_Bob& InBob)
    -> void
{
    auto Node = UCk_Utils_SceneNode_UE::Cast(InHandle);
    const auto IsSceneNode = ck::IsValid(Node);
    CK_ENSURE_IF_NOT(IsSceneNode, TEXT("Bob [{}] is no longer a SceneNode; it was detached or stripped and cannot publish an offset"), InHandle)
    { return; }

    const auto ActualOffset = UCk_Utils_SceneNode_UE::Get_Offset(Node);
    if (NOT ActualOffset.Equals(InBob.Get_LastWrittenOffset(), 1.0e-3) && NOT InHandle.Has<FTag_Bob_ForeignOffsetReported>())
    {
        CK_ENSURE_IF_NOT(false, TEXT("Bob node [{}] offset was written by something other than Bob; Bob owns this offset and will overwrite it"), InHandle)
        {}

        InHandle.AddOrGet<FTag_Bob_ForeignOffsetReported>();
    }

    auto Clock = gait::FClockState{};
    auto Motion = gait::Get_RestMotion();
    auto LandingCount = InBob._ConsumedLandingCount;
    auto Impact = 0.0f;

    // A destroyed gait relaxes the bob to rest without an ensure: the player entity and its bob nodes die in the same
    // pass, so an ensure here would fire at every pawn death.
    const auto& GaitHandle = InTunables.Get_Gait();
    const auto GaitIsLive = ck::IsValid(GaitHandle) && UCk_Utils_Gait_UE::Has(GaitHandle);
    if (GaitIsLive && NOT InHandle.Has<FTag_Bob_Disabled>())
    {
        const auto& Gait = GaitHandle.Get<FFragment_Gait>();
        Clock = Gait.Get_Clock();
        Motion = Gait.Get_LastMotion();
        LandingCount = Gait.Get_LandingCount();
        Impact = Gait.Get_LastLandImpactSpeed();
    }

    if (LandingCount != InBob._ConsumedLandingCount)
    {
        InBob._Spring._Velocity -= bob::Compute_LandKick(InTunables, Impact);
        InBob._ConsumedLandingCount = LandingCount;
    }

    const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());
    if (Dt <= 0.0f)
    { return; }

    bob::Step_Spring(InBob._Spring, bob::Compute_AirLiftTarget(InTunables, Motion), Dt, InTunables.Get_Air().Get_Spring());
    const auto Target = bob::Compute_Target(InTunables, Clock, InBob._Spring._Offset);
    InBob._Smoothed = bob::Smooth(InBob._Smoothed, Target, InTunables.Get_LagRate(), Dt);

    auto Clamped = InBob._Smoothed;
    Clamped._LocationCm = bob::Clamp_Location(Clamped._LocationCm, InTunables.Get_MaxOffsetCm());

    const auto NodeOffset = bob::Compose_NodeOffset(bob::Compose_Offset(Clamped), InBob.Get_RestOffset());
    // Compare with the node's ACTUAL offset, never _LastWrittenOffset: a foreign write that lands while the bob is at
    // rest must still be overwritten.
    if (NodeOffset.Equals(ActualOffset, 1.0e-4))
    { return; }

    DoPublishOffset(InHandle, InBob, NodeOffset);
}
