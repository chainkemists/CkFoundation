#include "CkCrowdAgent_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/Agent/CkCrowdAgent_DebugColor_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

#include "CkCore/Color/CkColor_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid owner handle [{}] passed to UCk_Utils_CrowdAgent_UE::Add"), InOwner)
    { return {}; }

    auto NewAgentEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_CrowdAgent>(InOwner);

    NewAgentEntity.Add<ck::FFragment_CrowdAgent_Params>(InParams);
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_PathFollow>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_DesiredVelocity>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_FaceAngle>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_NeighborCache>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_SeparationForce>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_ProbeRef>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_BlockDetect>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_PendingDisplacement>();
    NewAgentEntity.Add<ck::FFragment_CrowdAgent_NavMarkup>();
    NewAgentEntity.Add<ck::FTag_CrowdAgent_NeedsSetup>();
    NewAgentEntity.Add<ck::FTag_CrowdAgent_Idle>();

    // Seed _ActiveArrivalRadius from the agent's params default. Updated per-MoveTo if the request
    // overrides it; otherwise this is what Steering's final-stop branch uses.
    NewAgentEntity.Get<ck::FFragment_CrowdAgent_PathFollow>()._ActiveArrivalRadius = InParams.Get_ArrivalRadius();

    ck::crowd::Verbose(TEXT("CrowdAgent added to [{}] -> [{}] (radius={}, height={})"),
        InOwner, NewAgentEntity, InParams.Get_Radius(), InParams.Get_Height());

    return NewAgentEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_DesiredVelocity(
        const FCk_Handle_CrowdAgent& InHandle)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_DesiredVelocity"), InHandle)
    { return FVector::ZeroVector; }

    return InHandle.Get<ck::FFragment_CrowdAgent_DesiredVelocity>().Get_Velocity();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_SeparationForce(
        const FCk_Handle_CrowdAgent& InHandle)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_SeparationForce"), InHandle)
    { return FVector::ZeroVector; }

    return InHandle.Get<ck::FFragment_CrowdAgent_SeparationForce>().Get_Force();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_CurrentWaypointIndex(
        const FCk_Handle_CrowdAgent& InHandle)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_CurrentWaypointIndex"), InHandle)
    { return INDEX_NONE; }

    return InHandle.Get<ck::FFragment_CrowdAgent_PathFollow>().Get_WaypointIndex();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_TargetYawDegrees(
        const FCk_Handle_CrowdAgent& InHandle)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_TargetYawDegrees"), InHandle)
    { return 0.0f; }

    return FMath::RadiansToDegrees(InHandle.Get<ck::FFragment_CrowdAgent_FaceAngle>().Get_TargetYaw());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_MoveTo(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_MoveTo& InRequest)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_MoveTo"), InAgent)
    { return InAgent; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InAgent),
        TEXT("Request_MoveTo on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
    { return InAgent; }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(InRequest);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_FollowTarget(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_FollowTarget& InRequest)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_FollowTarget"), InAgent)
    { return InAgent; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InAgent),
        TEXT("Request_FollowTarget on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
    { return InAgent; }

    CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_TargetPoint()),
        TEXT("Request_FollowTarget on CrowdAgent [{}] dropped — the target point handle is invalid"), InAgent)
    { return InAgent; }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(InRequest);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_Stop(
        FCk_Handle_CrowdAgent& InAgent)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_Stop"), InAgent)
    { return InAgent; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InAgent),
        TEXT("Request_Stop on CrowdAgent [{}] dropped — caller does not have authority."), InAgent)
    { return InAgent; }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(FCk_Request_CrowdAgent_Stop{});
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    BindTo_OnGoalReached(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalReached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_CrowdAgent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_CrowdAgent_OnGoalReached, InAgent, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAgent;
}

auto
    UCk_Utils_CrowdAgent_UE::
    UnbindFrom_OnGoalReached(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalReached& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_CrowdAgent_OnGoalReached, InAgent, InDelegate);
    return InAgent;
}

auto
    UCk_Utils_CrowdAgent_UE::
    BindTo_OnGoalFailed(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_CrowdAgent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_CrowdAgent_OnGoalFailed, InAgent, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAgent;
}

auto
    UCk_Utils_CrowdAgent_UE::
    UnbindFrom_OnGoalFailed(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalFailed& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_CrowdAgent_OnGoalFailed, InAgent, InDelegate);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    BindTo_OnGoalBlocked(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalBlocked& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_CrowdAgent
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_CrowdAgent_OnGoalBlocked, InAgent, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    UnbindFrom_OnGoalBlocked(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalBlocked& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_CrowdAgent_OnGoalBlocked, InAgent, InDelegate);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_IsGoalBlocked(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_IsGoalBlocked"), InAgent)
    { return false; }

    return InAgent.Has<ck::FTag_CrowdAgent_GoalBlocked>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_ActiveGoal(
        const FCk_Handle_CrowdAgent& InAgent)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_ActiveGoal"), InAgent)
    { return FVector::ZeroVector; }

    return InAgent.Get<ck::FFragment_CrowdAgent_PathFollow>().Get_ActiveGoal();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Set_DebugColor(
        FCk_Handle_CrowdAgent& InAgent,
        FLinearColor InColor)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Set_DebugColor"), InAgent)
    { return InAgent; }

    auto& DebugColor = InAgent.AddOrGet<ck::FFragment_CrowdAgent_DebugColor>();
    DebugColor._Color = InColor;
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_DebugColor(
        const FCk_Handle_CrowdAgent& InAgent)
    -> FLinearColor
{
    if (NOT ck::IsValid(InAgent))
    { return FLinearColor::White; }

    if (InAgent.Has<ck::FFragment_CrowdAgent_DebugColor>())
    { return InAgent.Get<ck::FFragment_CrowdAgent_DebugColor>().Get_Color(); }

    // Hash-derived fallback. Production agents that never opt in still get a stable distinct
    // colour when an in-world overlay or debugger swatch happens to render them. Delegates to
    // the framework helper so any other debug subsystem (DrawBody processor, breadcrumb,
    // planned path, etc.) reaches the same color for the same entity.
    return UCk_Utils_LinearColor::Get_StableColorFromHash(static_cast<int32>(GetTypeHash(InAgent)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_SetDebugOverride(
        FCk_Handle_CrowdAgent& InAgent,
        bool InOverride)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_SetDebugOverride"), InAgent)
    { return InAgent; }

    if (InOverride)
    { InAgent.AddOrGet<ck::FTag_CrowdAgent_DebugOverride>(); }
    else
    { InAgent.Try_Remove<ck::FTag_CrowdAgent_DebugOverride>(); }

    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_HasDebugOverride(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    if (NOT ck::IsValid(InAgent))
    { return false; }

    return InAgent.Has<ck::FTag_CrowdAgent_DebugOverride>();
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CrowdAgent_UE, FCk_Handle_CrowdAgent, ck::FFragment_CrowdAgent_Params);

// --------------------------------------------------------------------------------------------------------------------
