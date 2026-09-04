#include "CkCrowdAgent_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_ConstrainToNavmesh_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_DebugColor_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

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
    const auto HasValidCloseGoalStrafe =
        (InParams.Get_CloseGoalStrafe() == ECk_EnableDisable::Enable
            || InParams.Get_CloseGoalStrafe() == ECk_EnableDisable::Disable)
        && FMath::IsFinite(InParams.Get_CloseGoalStrafeDistanceUu())
        && InParams.Get_CloseGoalStrafeDistanceUu() >= 0.0f;
    CK_ENSURE_IF_NOT(HasValidCloseGoalStrafe,
        TEXT("Invalid CrowdAgent close-goal strafe params (mode [{}], distance [{}])"),
        InParams.Get_CloseGoalStrafe(), InParams.Get_CloseGoalStrafeDistanceUu())
    { return {}; }

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
    InOwner.Add<ck::FFragment_CrowdAgent_AvoidanceVolumeCache>();
    InOwner.Add<ck::FFragment_CrowdAgent_LocalBoundary>();
    InOwner.Add<ck::FFragment_CrowdAgent_SeparationForce>();
    InOwner.Add<ck::FFragment_CrowdAgent_ProbeRef>();
    InOwner.Add<ck::FFragment_CrowdAgent_BlockDetect>();
    InOwner.Add<ck::FFragment_CrowdAgent_PendingDisplacement>();

    // Phase-seed the grounding lease so a crowd composed on one frame does not verify on one frame forever.
    auto GroundingFragment = ck::FFragment_CrowdAgent_Grounding{};
    GroundingFragment._SecondsSinceVerified =
        ck::ck_crowd_agent_constrain_to_navmesh_algorithm::Get_GroundingVerifyPhaseSeconds(
            GetTypeHash(InOwner), UCk_Utils_Crowd_Settings_UE::Get_GroundingVerifyIntervalSeconds());
    InOwner.Add<ck::FFragment_CrowdAgent_Grounding>(GroundingFragment);

    InOwner.Add<ck::FFragment_CrowdAgent_NavMarkup>();
    InOwner.Add<ck::FFragment_CrowdAgent_PathTrouble>();
    InOwner.Add<ck::FFragment_CrowdAgent_TransientPersonalSpace>();
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
    Request_SetTransientPersonalSpaceScale(
        FCk_Handle_CrowdAgent& InAgent,
        float InScale,
        float InDurationSeconds,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    constexpr auto MinScale = 0.25f;
    constexpr auto MaxScale = 1.0f;
    constexpr auto MaxDurationSeconds = 10.0f;
    const auto IsValidAgent = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(IsValidAgent, TEXT("Invalid CrowdAgent handle [{}] passed to Request_SetTransientPersonalSpaceScale"), InAgent)
    { InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued); return InAgent; }
    if (NOT IsValidAgent)
    { InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued); return InAgent; }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority, TEXT("Request_SetTransientPersonalSpaceScale on CrowdAgent [{}] dropped — caller does not have authority."), InAgent)
    { InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued); return InAgent; }
    if (NOT HasAuthority)
    { InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued); return InAgent; }

    const auto InputsAreValid = FMath::IsFinite(InScale) && FMath::IsFinite(InDurationSeconds)
        && InScale >= MinScale && InScale <= MaxScale
        && InDurationSeconds > 0.0f && InDurationSeconds <= MaxDurationSeconds;
    CK_ENSURE_IF_NOT(InputsAreValid, TEXT("Request_SetTransientPersonalSpaceScale dropped for [{}]: scale [{}] must be [{}, {}], duration [{}] must be (0, {}]"), InAgent, InScale, MinScale, MaxScale, InDurationSeconds, MaxDurationSeconds)
    { InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued); return InAgent; }
    if (NOT InputsAreValid)
    { InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued); return InAgent; }

    auto Request = FCk_Request_CrowdAgent_SetTransientPersonalSpaceScale{InScale, InDurationSeconds};
    if (InDelegate.IsBound()) { Request.Set_CompletionDelegate(InDelegate); }
    InAgent.AddOrGet<ck::FFragment_CrowdAgent_MoveRequests>()._Requests.Emplace(Request);
    return InAgent;
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
    Get_TransientPersonalSpaceScale(
        const FCk_Handle_CrowdAgent& InAgent)
    -> float
{
    const auto IsValidAgent = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(IsValidAgent,
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_TransientPersonalSpaceScale"), InAgent)
    { return 1.0f; }
    if (NOT IsValidAgent)
    { return 1.0f; }

    return InAgent.Has<ck::FFragment_CrowdAgent_TransientPersonalSpace>()
        ? InAgent.Get<ck::FFragment_CrowdAgent_TransientPersonalSpace>().Get_Scale()
        : 1.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_TransientPersonalSpaceRemainingSeconds(
        const FCk_Handle_CrowdAgent& InAgent)
    -> float
{
    const auto IsValidAgent = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(IsValidAgent,
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_TransientPersonalSpaceRemainingSeconds"), InAgent)
    { return 0.0f; }
    if (NOT IsValidAgent)
    { return 0.0f; }

    return InAgent.Has<ck::FFragment_CrowdAgent_TransientPersonalSpace>()
        ? InAgent.Get<ck::FFragment_CrowdAgent_TransientPersonalSpace>().Get_RemainingSeconds()
        : 0.0f;
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
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_MoveTo on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
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
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_FollowTarget on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto TargetPointIsValid = ck::IsValid(InRequest.Get_TargetPoint());
    CK_ENSURE_IF_NOT(TargetPointIsValid,
        TEXT("Request_FollowTarget on CrowdAgent [{}] dropped — the target point handle is invalid"), InAgent)
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
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Stop on CrowdAgent [{}] dropped — caller does not have authority."), InAgent)
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
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_SetMaxSpeed on CrowdAgent [{}] dropped — caller does not have authority. "
             "Steering is server-only."), InAgent)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto MaxSpeedIsValid = InMaxSpeed >= 1.0f;
    CK_ENSURE_IF_NOT(MaxSpeedIsValid,
        TEXT("Request_SetMaxSpeed on CrowdAgent [{}] dropped — MaxSpeed [{}] must be >= 1"),
        InAgent, InMaxSpeed)
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
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_SetNavQueryFilter on CrowdAgent [{}] dropped — caller does not have authority. "
             "Pathfinding is server-only."), InAgent)
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
    Get_IsGoalFailedHold(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    const auto IsValidAgent = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(IsValidAgent,
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_IsGoalFailedHold"), InAgent)
    { return false; }
    if (NOT IsValidAgent)
    { return false; }

    return InAgent.Has<ck::FTag_CrowdAgent_GoalFailedHold>();
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
    Get_IsOffNavmesh(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_IsOffNavmesh"), InAgent)
    { return false; }

    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_Grounding>())
    { return false; }

    return InAgent.Get<ck::FFragment_CrowdAgent_Grounding>().Get_IsOffNavmesh();
}

auto
    UCk_Utils_CrowdAgent_UE::
    Get_SecondsOffNavmesh(
        const FCk_Handle_CrowdAgent& InAgent)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Get_SecondsOffNavmesh"), InAgent)
    { return 0.0f; }

    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_Grounding>())
    { return 0.0f; }

    return InAgent.Get<ck::FFragment_CrowdAgent_Grounding>().Get_SecondsOffNavmesh();
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
    if (ck::Is_NOT_Valid(InAgent))
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
    if (ck::Is_NOT_Valid(InAgent))
    { return false; }

    return InAgent.Has<ck::FTag_CrowdAgent_DebugOverride>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Request_SetPermeable(
        FCk_Handle_CrowdAgent& InAgent,
        ECk_EnableDisable InPermeable,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(AgentIsValid,
        TEXT("Invalid CrowdAgent handle [{}] passed to Request_SetPermeable"), InAgent)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    if (InPermeable == ECk_EnableDisable::Enable)
    { InAgent.AddOrGet<ck::FTag_CrowdAgent_Permeable>(); }
    else
    { InAgent.Try_Remove<ck::FTag_CrowdAgent_Permeable>(); }

    // NOTE: the stale-separation-force hazard is handled in FProcessor_CrowdAgent_Separation, not
    // here. Separation is the only writer of that fragment and only zeroes it on a frame it runs;
    // Steering reads and adds it unconditionally. So the processor keeps the agent in its view and
    // zeroes-then-returns on the tag, rather than being TExclude'd — which also means the tag works
    // when set by any path, not only through this function.

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Succeeded);

    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Get_IsPermeable(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    if (ck::Is_NOT_Valid(InAgent))
    { return false; }

    return InAgent.Has<ck::FTag_CrowdAgent_Permeable>();
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CrowdAgent_UE, FCk_Handle_CrowdAgent, ck::FFragment_CrowdAgent_Params);

// --------------------------------------------------------------------------------------------------------------------
