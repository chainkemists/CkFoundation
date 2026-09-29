#include "CkSway_Processor.h"
#include "CkSway/CkSway_Kernel.h"
#include "CkSway/CkSway_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Sway_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sway_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sway_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sway_CancelPendingRequests);

auto
    ck::FProcessor_Sway_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway)
    -> void
{
    const auto Node = UCk_Utils_SceneNode_UE::Cast(InHandle);
    const auto IsSceneNode = ck::IsValid(Node);
    // No bail-out: a node that is not a SceneNode still leaves setup (Update then ensures and stays inert).
    CK_ENSURE_IF_NOT(IsSceneNode, TEXT("Sway [{}] is not a SceneNode"), InHandle)
    {}
    if (IsSceneNode)
    {
        // Adopt the current offset so the first Update raises no foreign-write ensure.
        InSway._LastWrittenOffset = UCk_Utils_SceneNode_UE::Get_Offset(Node);
        InSway._LastDriverWorld = UCk_Utils_SceneNode_UE::Get_DriverWorldTransform(Node);
    }
    if (InTunables.Get_StartingState() == ECk_EnableDisable::Disable)
    { InHandle.AddOrGet<FTag_Sway_Disabled>(); }
    InHandle.Remove<MarkedDirtyBy>();
}

auto
    ck::FProcessor_Sway_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway,
        FFragment_Sway_Requests& InRequests)
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
        sway::VeryVerbose(TEXT("Handling Sway request on [{}]"), InHandle);
        Result = DoHandleRequest(InHandle, InTunables, InSway, InRequest);
    }), policy::DontResetContainer{});
    if (InRequests._Requests.IsEmpty())
    { InHandle.Remove<MarkedDirtyBy>(); }
}

auto
    ck::FProcessor_Sway_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway,
        const FCk_Request_Sway_UpdateSpec& InRequest)
    -> ECk_Request_OperationResult
{
    const auto IsValidSpec = sway::Get_IsSpecValid(InRequest.Get_Spec());
    CK_ENSURE_IF_NOT(IsValidSpec, TEXT("Sway UpdateSpec rejected invalid spec on [{}]"), InHandle)
    { return ECk_Request_OperationResult::Failed; }
    InTunables = InRequest.Get_Spec();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Sway_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway,
        const FCk_Request_Sway_EnableDisable& InRequest)
    -> ECk_Request_OperationResult
{
    switch (InRequest.Get_EnableDisable())
    {
        case ECk_EnableDisable::Enable:
        {
            InHandle.Try_Remove<FTag_Sway_Disabled>();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InHandle.AddOrGet<FTag_Sway_Disabled>();
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
    ck::FProcessor_Sway_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway,
        const FCk_Request_Sway_Reset& InRequest)
    -> ECk_Request_OperationResult
{
    // Does not publish and does not touch _LastWrittenOffset: Update reseeds the driver pose (unset here) and the
    // rest offset is published through the normal path, which keeps its foreign-write check free of a false positive.
    InSway._Location = {};
    InSway._Rotation = {};
    InSway._LastDriverWorld.Reset();
    InSway._LastStimulus = {};
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Sway_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway,
        const FCk_Request_Sway_SetRestOffset& InRequest)
    -> ECk_Request_OperationResult
{
    // Does not publish: Update composes SwayOffset * Rest, sees it differ from the node's actual offset and republishes.
    InSway._RestOffset = InRequest.Get_RestOffset();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Sway_CancelPendingRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Sway_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    request::FireCancelledForPending(InHandle, RequestsCopy);
}

auto
    ck::FProcessor_Sway_Update::
    DoPublishOffset(
        HandleType InHandle,
        FFragment_Sway& InSway,
        const FTransform& InOffset)
    -> void
{
    auto Node = UCk_Utils_SceneNode_UE::Cast(InHandle);
    UCk_Utils_SceneNode_UE::Request_UpdateOffset(Node, FCk_Request_SceneNode_UpdateRelativeTransform{InOffset}, {});
    InSway._LastWrittenOffset = InOffset;
}

auto
    ck::FProcessor_Sway_Update::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Sway_Tunables& InTunables,
        FFragment_Sway& InSway)
    -> void
{
    auto Node = UCk_Utils_SceneNode_UE::Cast(InHandle);
    const auto IsSceneNode = ck::IsValid(Node);
    CK_ENSURE_IF_NOT(IsSceneNode, TEXT("Sway [{}] is no longer a SceneNode; it was detached or stripped and cannot publish an offset"), InHandle)
    { return; }

    const auto ActualOffset = UCk_Utils_SceneNode_UE::Get_Offset(Node);
    if (NOT ActualOffset.Equals(InSway.Get_LastWrittenOffset(), 1.0e-3) && NOT InHandle.Has<FTag_Sway_ForeignOffsetReported>())
    {
        CK_ENSURE_IF_NOT(false, TEXT("Sway node [{}] offset was written by something other than Sway; Sway owns this offset and will overwrite it"), InHandle)
        {}
        InHandle.AddOrGet<FTag_Sway_ForeignOffsetReported>();
    }

    const auto DriverWorld = UCk_Utils_SceneNode_UE::Get_DriverWorldTransform(Node);
    const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());

    if (NOT InSway._LastDriverWorld.IsSet())
    {
        InSway._LastDriverWorld = DriverWorld;
        return;
    }

    const auto MaybeStimulus = sway::Compute_Stimulus(InSway._LastDriverWorld.GetValue(), DriverWorld, Dt,
        InTunables.Get_TeleportDistanceCm(), InTunables.Get_TeleportAngleDeg());
    InSway._LastDriverWorld = DriverWorld;

    const auto Stimulus = (InHandle.Has<FTag_Sway_Disabled>() || NOT MaybeStimulus.IsSet())
        ? FCk_Sway_Stimulus{} : MaybeStimulus.GetValue();
    InSway._LastStimulus = Stimulus;

    if (Dt <= 0.0f)
    { return; }

    sway::Step_Channel(InSway._Location, sway::Compute_LocationTarget(InTunables, Stimulus), Dt, InTunables.Get_Location());
    sway::Step_Channel(InSway._Rotation, sway::Compute_RotationTarget(InTunables, Stimulus), Dt, InTunables.Get_Rotation());

    const auto NodeOffset = sway::Compose_NodeOffset(
        sway::Compose_Offset(InSway._Location._Value, InSway._Rotation._Value), InSway.Get_RestOffset());
    // Compare with the node's ACTUAL offset, never _LastWrittenOffset: a foreign write that lands while the spring is
    // at rest must still be overwritten.
    if (NodeOffset.Equals(ActualOffset, 1.0e-4))
    { return; }
    DoPublishOffset(InHandle, InSway, NodeOffset);
}
