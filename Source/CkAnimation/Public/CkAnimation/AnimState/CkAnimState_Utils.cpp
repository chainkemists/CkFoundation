#include "CkAnimState_Utils.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkAnimation/AnimState/CkAnimState_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(AnimState, UCk_Utils_AnimState_UE, FCk_Handle_AnimState, ck::FFragment_AnimState_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AnimState_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AnimState_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_AnimState
{
    InHandle.Add<ck::FFragment_AnimState_Current>(InParams.Get_StartingAnimState());

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::animation::VeryVerbose
        (
            TEXT("Skipping creation of AnimState Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );
    }
    else
    {
        TryAddReplicatedFragment<UCk_Fragment_AnimState_Rep>(InHandle);
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_AnimState_UE::
    Get_AnimGoal(
        const FCk_Handle_AnimState& InHandle)
    -> FCk_AnimState_Goal
{
    return InHandle.Get<ck::FFragment_AnimState_Current>().Get_AnimGoal();
}

auto
    UCk_Utils_AnimState_UE::
    Get_AnimState(
        const FCk_Handle_AnimState& InHandle)
    -> FCk_AnimState_State
{
    return InHandle.Get<ck::FFragment_AnimState_Current>().Get_AnimState();
}

auto
    UCk_Utils_AnimState_UE::
    Get_AnimCluster(
        const FCk_Handle_AnimState& InHandle)
    -> FCk_AnimState_Cluster
{
    return InHandle.Get<ck::FFragment_AnimState_Current>().Get_AnimCluster();
}

auto
    UCk_Utils_AnimState_UE::
    Get_AnimOverlay(
        const FCk_Handle_AnimState& InHandle)
    -> FCk_AnimState_Overlay
{
    return InHandle.Get<ck::FFragment_AnimState_Current>().Get_AnimOverlay();
}

auto
    UCk_Utils_AnimState_UE::
    Request_SetAnimGoal(
        FCk_Handle_AnimState& InHandle,
        const FCk_Request_AnimState_SetGoal& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_AnimState_Requests>()._SetGoalRequest = InRequest;
}

auto
    UCk_Utils_AnimState_UE::
    Request_SetAnimState(
        FCk_Handle_AnimState& InHandle,
        const FCk_Request_AnimState_SetState& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_AnimState_Requests>()._SetStateRequest = InRequest;
}

auto
    UCk_Utils_AnimState_UE::
    Request_SetAnimCluster(
        FCk_Handle_AnimState& InHandle,
        const FCk_Request_AnimState_SetCluster& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_AnimState_Requests>()._SetClusterRequest = InRequest;
}

auto
    UCk_Utils_AnimState_UE::
    Request_SetAnimOverlay(
        FCk_Handle_AnimState& InHandle,
        const FCk_Request_AnimState_SetOverlay& InRequest)
    -> void
{
    InHandle.AddOrGet<ck::FFragment_AnimState_Requests>()._SetOverlayRequest = InRequest;
}

auto
    UCk_Utils_AnimState_UE::
    BindTo_OnGoalChanged(
        FCk_Handle_AnimState& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnGoalChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnGoalChanged::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_AnimState_UE::
    UnbindFrom_OnGoalChanged(
        FCk_Handle_AnimState& InHandle,
        const FCk_Delegate_AnimState_OnGoalChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnGoalChanged::Unbind(InHandle, InDelegate);
}

auto
    UCk_Utils_AnimState_UE::
    BindTo_OnStateChanged(
        FCk_Handle_AnimState& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnStateChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnStateChanged::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_AnimState_UE::
    UnbindFrom_OnStateChanged(
        FCk_Handle_AnimState& InHandle,
        const FCk_Delegate_AnimState_OnStateChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnStateChanged::Unbind(InHandle, InDelegate);
}

auto
    UCk_Utils_AnimState_UE::
    BindTo_OnClusterChanged(
        FCk_Handle_AnimState& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnClusterChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnClusterChanged::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_AnimState_UE::
    UnbindFrom_OnClusterChanged(
        FCk_Handle_AnimState& InHandle,
        const FCk_Delegate_AnimState_OnClusterChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnClusterChanged::Unbind(InHandle, InDelegate);
}

auto
    UCk_Utils_AnimState_UE::
    BindTo_OnOverlayChanged(
        FCk_Handle_AnimState& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AnimState_OnOverlayChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnOverlayChanged::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_AnimState_UE::
    UnbindFrom_OnOverlayChanged(
        FCk_Handle_AnimState& InHandle,
        const FCk_Delegate_AnimState_OnOverlayChanged& InDelegate)
    -> void
{
    ck::UUtils_Signal_AnimState_OnOverlayChanged::Unbind(InHandle, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------