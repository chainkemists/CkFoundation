#include "CkCrowdAgent_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/Agent/CkCrowdAgent_DebugColor_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

#include "CkCore/Color/CkColor_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Add(
        FCk_Handle_Transform& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid owner handle [{}] passed to UCk_Utils_CrowdAgent_UE::Add"), InOwner)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InOwner),
        TEXT("Handle [{}] already has the CrowdAgent feature. An entity hosts at most ONE crowd agent"),
        InOwner)
    { return Cast(InOwner); }

    InOwner.Add<ck::FFragment_CrowdAgent_Params>(InParams);
    InOwner.Add<ck::FFragment_CrowdAgent_PathFollow>();
    InOwner.Add<ck::FFragment_CrowdAgent_DesiredVelocity>();
    InOwner.Add<ck::FFragment_CrowdAgent_FaceAngle>();
    InOwner.Add<ck::FFragment_CrowdAgent_NeighborCache>();
    InOwner.Add<ck::FFragment_CrowdAgent_SeparationForce>();
    InOwner.Add<ck::FFragment_CrowdAgent_ProbeRef>();
    InOwner.Add<ck::FFragment_CrowdAgent_BlockDetect>();
    InOwner.Add<ck::FFragment_CrowdAgent_PendingDisplacement>();
    InOwner.Add<ck::FFragment_CrowdAgent_NavMarkup>();
    InOwner.Add<ck::FFragment_CrowdAgent_PathTrouble>();
    InOwner.Add<ck::FTag_CrowdAgent_NeedsSetup>();
    InOwner.Add<ck::FTag_CrowdAgent_Idle>();

    if (InParams.Get_AgentMode() == ECk_CrowdAgent_Mode::Flying)
    { InOwner.Add<ck::FTag_CrowdAgent_Flying>(); }

    // Overridden per-MoveTo by _ArrivalRadiusOverride; otherwise Steering's final-stop branch reads this.
    InOwner.Get<ck::FFragment_CrowdAgent_PathFollow>()._ActiveArrivalRadius = InParams.Get_ArrivalRadius();

    ck::crowd::Verbose(TEXT("CrowdAgent added to [{}] (radius={}, height={})"),
        InOwner, InParams.Get_Radius(), InParams.Get_Height());

    return Cast(InOwner);
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
    Get_TargetPitchDegrees(
        const FCk_Handle_CrowdAgent& InHandle)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_TargetPitchDegrees"), InHandle)
    { return 0.0f; }

    return FMath::RadiansToDegrees(InHandle.Get<ck::FFragment_CrowdAgent_FaceAngle>().Get_TargetPitch());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_MoveTo(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_MoveTo& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_MoveTo"), InAgent)
    {}
    if (NOT AgentIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_MoveTo on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(InRequest);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_FollowTarget(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_FollowTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_FollowTarget"), InAgent)
    {}
    if (NOT AgentIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_FollowTarget on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto TargetPointIsValid = ck::IsValid(InRequest.Get_TargetPoint());
    CK_ENSURE_IF_NOT(TargetPointIsValid,
        TEXT("Request_FollowTarget on CrowdAgent [{}] dropped — the target point handle is invalid"), InAgent)
    {}
    if (NOT TargetPointIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(InRequest);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_Stop(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_Stop"), InAgent)
    {}
    if (NOT AgentIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Stop on CrowdAgent [{}] dropped — caller does not have authority."), InAgent)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto Request = FCk_Request_CrowdAgent_Stop{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(Request);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_SetMaxSpeed(
        FCk_Handle_CrowdAgent& InAgent,
        float InMaxSpeed,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_SetMaxSpeed"), InAgent)
    {}
    if (NOT AgentIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_SetMaxSpeed on CrowdAgent [{}] dropped — caller does not have authority. "
             "Steering is server-only."), InAgent)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto MaxSpeedIsValid = InMaxSpeed >= 1.0f;
    CK_ENSURE_IF_NOT(MaxSpeedIsValid,
        TEXT("Request_SetMaxSpeed on CrowdAgent [{}] dropped — MaxSpeed [{}] must be >= 1"),
        InAgent, InMaxSpeed)
    {}
    if (NOT MaxSpeedIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto Request = FCk_Request_CrowdAgent_SetMaxSpeed{InMaxSpeed};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(Request);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_SetNavQueryFilter(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_SetNavQueryFilter& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_SetNavQueryFilter"), InAgent)
    {}
    if (NOT AgentIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_SetNavQueryFilter on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
    {}
    if (NOT HasAuthority)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(InRequest);
    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_NavQueryFilter(
        const FCk_Handle_CrowdAgent& InAgent)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_NavQueryFilter"), InAgent)
    { return {}; }

    return InAgent.Get<ck::FFragment_CrowdAgent_Params>().Get_NavQueryFilter();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_MaxSpeed(
        const FCk_Handle_CrowdAgent& InAgent)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_MaxSpeed"), InAgent)
    { return 0.0f; }

    return InAgent.Get<ck::FFragment_CrowdAgent_Params>().Get_MaxSpeed();
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
    Get_MovementState(
        const FCk_Handle_CrowdAgent& InAgent)
    -> ECk_CrowdAgent_MovementState
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_MovementState"), InAgent)
    { return ECk_CrowdAgent_MovementState::None; }

    if (InAgent.Has<ck::FTag_CrowdAgent_Walking>())
    { return ECk_CrowdAgent_MovementState::Walking; }

    if (InAgent.Has<ck::FTag_CrowdAgent_PathPending>())
    { return ECk_CrowdAgent_MovementState::PathPending; }

    if (InAgent.Has<ck::FTag_CrowdAgent_Idle>())
    { return ECk_CrowdAgent_MovementState::Idle; }

    return ECk_CrowdAgent_MovementState::None;
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
    Get_HasReachedActiveGoal(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_HasReachedActiveGoal"), InAgent)
    {}
    if (NOT AgentIsValid)
    { return false; }

    // A queued MoveTo/Stop supersedes the retained result immediately from the caller's perspective, before the
    // request processor has replaced _ActiveGoal or reset the waypoint cursor.
    if (InAgent.Has<ck::FFragment_CrowdAgent_MoveRequests>())
    { return false; }

    const auto HasRequiredState =
        InAgent.Has<ck::FFragment_CrowdAgent_PathFollow>() &&
        InAgent.Has<ck::FFragment_Nav_PathResult>();
    if (NOT HasRequiredState || NOT InAgent.Has<ck::FTag_CrowdAgent_Idle>())
    { return false; }

    const auto& PathFollow = InAgent.Get<ck::FFragment_CrowdAgent_PathFollow>();
    const auto& PathResult = InAgent.Get<ck::FFragment_Nav_PathResult>();
    const auto& Waypoints = PathResult.Get_Waypoints();
    return PathResult.Get_Status() == ECk_Nav_PathStatus::Ready &&
           NOT Waypoints.IsEmpty() &&
           PathFollow.Get_WaypointIndex() >= Waypoints.Num() &&
           NOT PathFollow.Get_ActivePathEndsShortOfGoal();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_ActiveMoveEpisode(
        const FCk_Handle_CrowdAgent& InAgent)
    -> int32
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_ActiveMoveEpisode"), InAgent)
    {}
    if (NOT AgentIsValid || NOT InAgent.Has<ck::FFragment_CrowdAgent_PathFollow>())
    { return 0; }

    return InAgent.Get<ck::FFragment_CrowdAgent_PathFollow>().Get_ActiveMoveEpisode();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_ActiveMoveCorrelationId(
        const FCk_Handle_CrowdAgent& InAgent)
    -> int32
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_ActiveMoveCorrelationId"), InAgent)
    {}
    if (NOT AgentIsValid || NOT InAgent.Has<ck::FFragment_CrowdAgent_PathFollow>())
    { return 0; }

    return InAgent.Get<ck::FFragment_CrowdAgent_PathFollow>().Get_ActiveMoveCorrelationId();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_IsStationaryMarkupPainted(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_IsStationaryMarkupPainted"), InAgent)
    { return false; }

    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_NavMarkup>())
    { return false; }

    return InAgent.Get<ck::FFragment_CrowdAgent_NavMarkup>().Get_Markup().IsValid();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_IsStationaryMarkupConfirmed(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_IsStationaryMarkupConfirmed"), InAgent)
    { return false; }

    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_NavMarkup>())
    { return false; }

    const auto& Markup = InAgent.Get<ck::FFragment_CrowdAgent_NavMarkup>();
    return Markup.Get_Markup().IsValid() && Markup.Get_ConfirmedOnMesh();
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

    return UCk_Utils_LinearColor::Get_StableColorFromHash(static_cast<int32>(GetTypeHash(InAgent)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_SetDebugOverride(
        FCk_Handle_CrowdAgent& InAgent,
        bool InOverride,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_SetDebugOverride"), InAgent)
    {}
    if (NOT AgentIsValid)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    if (InOverride)
    { InAgent.AddOrGet<ck::FTag_CrowdAgent_DebugOverride>(); }
    else
    { InAgent.Try_Remove<ck::FTag_CrowdAgent_DebugOverride>(); }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Succeeded);

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
