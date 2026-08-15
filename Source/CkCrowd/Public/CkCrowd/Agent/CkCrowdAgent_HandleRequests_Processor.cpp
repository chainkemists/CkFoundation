#include "CkCrowdAgent_HandleRequests_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment.h"
#include "CkVoxelNav/Path/CkVoxelNavPath_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::HandleRequests"), STAT_CkCrowd_HandleRequestsProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_MoveRequests& InRequests) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_HandleRequestsProc);

        InHandle.CopyAndRemove(InRequests, [&](const auto& InSnapshot)
        {
            // Policy is setup, not a sequential movement command: choose the final value before
            // dispatching this batch so SetPolicy + MoveTo is order-independent within one frame.
            const auto* LastPolicyRequest = static_cast<const FCk_Request_CrowdAgent_SetNavQueryFilter*>(nullptr);
            for (const auto& Request : InSnapshot._Requests)
            {
                if (const auto* Policy = std::get_if<FCk_Request_CrowdAgent_SetNavQueryFilter>(&Request))
                { LastPolicyRequest = Policy; }
            }
            if (LastPolicyRequest != nullptr)
            { InParams._NavQueryFilter = LastPolicyRequest->Get_NavQueryFilter(); }
            const auto NavigationRevisionBeforeCommands = InPathFollow.Get_ActiveNavigationRequestRevision();

            algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
            [&](const auto& InRequest)
            {
                // Every DoHandleRequest overload below is void and has no rejection path, so reaching
                // the line after the call IS the success condition.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InParams, InPathFollow, InDesired, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;
            }), policy::DontResetContainer{});

            // A MoveTo/FollowTarget that actually dispatched a route advanced this revision using
            // the resolved policy. If it was a same-goal no-op (or no movement command appeared),
            // force-replan the current episode without changing its goal/correlation/follow ownership.
            if (LastPolicyRequest != nullptr
                && LastPolicyRequest->Get_ForceReplan() == ECk_EnableDisable::Enable
                && InPathFollow.Get_ActiveNavigationRequestRevision() == NavigationRevisionBeforeCommands)
            {
                DoForceReplan(InHandle, InParams, InPathFollow, InDesired);
            }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_MoveTo& InRequest)
        -> void
    {
        // A plain MoveTo takes over from any follow in flight; the FollowTarget handler delegates
        // here and re-adds its own state afterwards.
        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();

        const auto Goal = InRequest.Get_Target();

        // Re-issuing the goal we are already walking to resets the waypoint cursor, so a noisy
        // re-issuer would stop the final-stop ever latching and the agent would orbit its goal.
        constexpr auto SameGoalEpsilonCm = 20.0f;
        if (NOT InRequest.Get_ForceRepath() &&
            InHandle.Has<FTag_CrowdAgent_Walking>() &&
            FVector::Dist(Goal, InPathFollow.Get_ActiveGoal()) <= SameGoalEpsilonCm &&
            (InRequest.Get_CorrelationId() == 0 ||
             InRequest.Get_CorrelationId() == InPathFollow.Get_ActiveMoveCorrelationId()))
        {
            // Log, not Verbose: this silently swallows a caller's recovery attempt, and a caller
            // that meant it has _ForceRepath.
            ck::crowd::Log(
                TEXT("CrowdAgent [{}] MoveTo {} ignored (same goal without a new nonzero correlation {} or ForceRepath, already walking)"),
                InHandle, Goal, InRequest.Get_CorrelationId());
            return;
        }

        InPathFollow._ActiveMoveEpisode = InPathFollow._ActiveMoveEpisode == MAX_int32
            ? 1
            : InPathFollow._ActiveMoveEpisode + 1;
        InPathFollow._ActiveMoveCorrelationId = InRequest.Get_CorrelationId();

        const auto ArrivalRadius = InRequest.Get_ArrivalRadiusOverrideMode() == ECk_Override::Override
            ? InRequest.Get_ArrivalRadiusOverrideValue()
            : InParams.Get_ArrivalRadius();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ProtectedLeadingWaypointCount = 0;
        InPathFollow._ActiveArrivalRadius = ArrivalRadius;
        InPathFollow._ActiveGoal = Goal;

        // The same-goal walking no-op returned above, so this is a genuinely new movement episode.
        InHandle.Get<FFragment_CrowdAgent_PathTrouble>() = FFragment_CrowdAgent_PathTrouble{};

        // Intentionally do NOT zero _DesiredVelocity: re-targeting mid-walk preserves momentum and
        // the acceleration ramp reconciles direction once the new path resolves. Zeroing made
        // back-to-back MoveTos stop dead and re-accelerate. Stop DOES zero — that is its semantic.

        InHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
        InHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        // An external MoveTo starts a NEW episode, so OnGoalBlocked may fire again for the new goal.
        DoClearBlockedState(InHandle);

        RequestPathForActiveGoal(InHandle, InParams, InPathFollow);

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] MoveTo {} (arrival={})"),
            InHandle, Goal, ArrivalRadius);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        Request_NavigationPath(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FVector& InGoal)
        -> void
    {
        // Park the slot at Pending BEFORE enqueueing: OnPathResolved runs after this processor
        // in the same frame and would otherwise consume a previous move's Ready result as if
        // it answered THIS MoveTo, walking the agent down the stale corridor.
        FCk_Nav_Algorithm::MarkPathPending(
            InHandle, InPathFollow.Get_ActiveNavigationRequestRevision());

        auto Request = FCk_Request_Nav_FindPath{InGoal};
        Request.Set_QueryFilter(InParams.Get_NavQueryFilter());
        Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());

        // A MoveTo issued while the agent stands inside painted stationary markup would plan
        // "through" the band — see Get_EscapedQueryStart.
        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::IsValid(TransformHandle))
        {
            const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                InHandle,
                InHandle.Get_Entity(),
                UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle),
                InGoal,
                InParams.Get_Radius());
            if (Escaped.IsSet())
            {
                Request.Set_StartOverride(ECk_EnableDisable::Enable)
                       .Set_StartOverrideLocation(*Escaped);
            }
        }

        UCk_Utils_Nav_UE::Request_FindPath(InHandle, Request, {});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        AdvanceNavigationRequestRevision(FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> int32
    {
        InPathFollow._ActiveNavigationRequestRevision =
            InPathFollow._ActiveNavigationRequestRevision == MAX_int32
                ? 1
                : InPathFollow._ActiveNavigationRequestRevision + 1;
        return InPathFollow._ActiveNavigationRequestRevision;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        RequestPathForActiveGoal(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow)
        -> void
    {
        const auto Goal = InPathFollow.Get_ActiveGoal();

        // Advance even for a non-CkNavigation provider: an already queued CkNavigation result must
        // not replace this newer route after the provider decision changes.
        AdvanceNavigationRequestRevision(InPathFollow);

        const auto HasValidVoxelVolume = UCk_Utils_VoxelNavPath_UE::Has(InHandle)
            && ck::IsValid(InHandle.Get<FFragment_VoxelNavPath_Params>().Get_Volume());
        const auto IsActiveVoxelProvider = HasValidVoxelVolume
            && (InHandle.Has<FFragment_CrowdAgent_InstalledVoxelPath>()
                || (InHandle.Has<FTag_CrowdAgent_PathPending>()
                    && NOT InHandle.Has<FTag_CrowdAgent_VoxelPathFallbackPending>()));
        if (IsActiveVoxelProvider)
        {
            FCk_Nav_Algorithm::MarkPathPending(
                InHandle, InPathFollow.Get_ActiveNavigationRequestRevision());
            InHandle.Try_Remove<FFragment_CrowdAgent_InstalledVoxelPath>();

            auto Path = UCk_Utils_VoxelNavPath_UE::CastChecked(InHandle);
            const auto From = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
                UCk_Utils_Transform_UE::CastChecked(InHandle));
            UCk_Utils_VoxelNavPath_UE::Request_FindPath(
                Path,
                FCk_Request_VoxelNavPath_FindPath{
                    InHandle.Get<FFragment_VoxelNavPath_Params>().Get_Volume(), From, Goal},
                {});
            return;
        }

        if (UCk_Utils_PathNetworkFollower_UE::Has(InHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(
                InHandle, InPathFollow.Get_ActiveNavigationRequestRevision());
            InHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(InHandle);
            auto Request = FCk_Request_PathNetworkFollower_FindRoute{Goal};
            Request.Set_NavQueryFilter(InParams.Get_NavQueryFilter());
            Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower, Request, {});
            return;
        }

        Request_NavigationPath(InHandle, InParams, InPathFollow, Goal);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_FollowTarget& InRequest)
        -> void
    {
        const auto& TargetPoint = InRequest.Get_TargetPoint();
        if (ck::Is_NOT_Valid(TargetPoint))
        {
            ck::crowd::Warning(TEXT("CrowdAgent [{}] FollowTarget ignored — the target point handle is invalid"),
                InHandle);
            return;
        }
        const auto LiveGoal = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TargetPoint);

        // Delegate the path-begin to the MoveTo handler with the goal resolved LIVE...
        auto MoveTo = FCk_Request_CrowdAgent_MoveTo{LiveGoal};
        MoveTo.Set_ArrivalRadiusOverrideMode(InRequest.Get_ArrivalRadiusOverrideMode());
        MoveTo.Set_ArrivalRadiusOverrideValue(InRequest.Get_ArrivalRadiusOverrideValue());
        DoHandleRequest(InHandle, InParams, InPathFollow, InDesired, MoveTo);

        // ...then arm the follow AFTER the delegation, because a plain MoveTo clears it.
        auto& Follow = InHandle.AddOrGet<FFragment_CrowdAgent_FollowTarget>();
        Follow._Request = InRequest;
        Follow._RepathAccumulatorSec = 0.0f;

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] FollowTarget (goal={})"), InHandle, LiveGoal);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_Stop& InRequest)
        -> void
    {
        InDesired._Velocity = FVector::ZeroVector;
        InPathFollow._WaypointIndex = 0;
        InPathFollow._ProtectedLeadingWaypointCount = 0;
        InHandle.Get<FFragment_CrowdAgent_PathTrouble>() = FFragment_CrowdAgent_PathTrouble{};

        InHandle.Try_Remove<FFragment_CrowdAgent_FollowTarget>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
        InHandle.AddOrGet<FTag_CrowdAgent_Idle>();

        // Stop abandons the goal entirely — BlockedRecheck must never resume it.
        DoClearBlockedState(InHandle);

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] Stop"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoClearBlockedState(
            FCk_Handle_CrowdAgent& InAgent)
        -> void
    {
        InAgent.Try_Remove<FTag_CrowdAgent_GoalBlocked>();

        if (NOT InAgent.Has<FFragment_CrowdAgent_BlockDetect>())
        { return; }

        auto& BlockDetect = InAgent.AddOrGet<FFragment_CrowdAgent_BlockDetect>();
        BlockDetect._BlockedBy = FCk_Handle{};
        BlockDetect._BlockedCause = ECk_CrowdAgent_BlockedReason::GoalOccupied;
        BlockDetect._RecheckAccumulatorSec = 0.0f;
        BlockDetect._BlockedSignalSent = false;
        BlockDetect._StallRepathCount = 0;
        BlockDetect._BlockedRetryCount = 0;
        BlockDetect.DoResetProgressWindow();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoForceReplan(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity)
        -> void
    {
        const auto HasActiveGoal = InHandle.Has<FTag_CrowdAgent_Walking>()
            || InHandle.Has<FTag_CrowdAgent_PathPending>()
            || InHandle.Has<FTag_CrowdAgent_GoalBlocked>();
        if (NOT HasActiveGoal)
        { return; }

        // Query filters are a Recast policy. A Voxel route cannot apply one,
        // and replacing its in-flight job without a provider-owned revision
        // would create a false stale-result guarantee. Persist the policy for
        // future Recast fallback but leave the current volumetric route intact.
        if (UCk_Utils_VoxelNavPath_UE::Has(InHandle)
            && ck::IsValid(InHandle.Get<FFragment_VoxelNavPath_Params>().Get_Volume()))
        {
            ck::crowd::Verbose(TEXT("CrowdAgent [{}] stored nav-query policy without replanning its Voxel route"),
                InHandle);
            return;
        }

        InHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        InHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        InHandle.Try_Remove<FTag_CrowdAgent_PathNetworkFallbackPending>();
        InHandle.Try_Remove<FTag_CrowdAgent_VoxelPathFallbackPending>();
        InHandle.AddOrGet<FTag_CrowdAgent_PathPending>();
        InHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();
        InHandle.Try_Remove<FFragment_CrowdAgent_InstalledVoxelPath>();

        InPathFollow._WaypointIndex = 0;
        InPathFollow._ActivePathEndsShortOfGoal = false;
        DoClearBlockedState(InHandle);

        // Preserve _ActiveGoal, _ActiveArrivalRadius, move episode/correlation, FollowTarget and
        // desired velocity. This is a route replacement, not a terminal movement transition.
        RequestPathForActiveGoal(InHandle, InParams, InPathFollow);
        ck::crowd::Verbose(TEXT("CrowdAgent [{}] nav-query policy forced replan to {}"),
            InHandle, InPathFollow.Get_ActiveGoal());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetNavQueryFilter& InRequest)
        -> void
    {
        // The batch prepass wrote the last policy before any movement command was dispatched.
        // Keep an overload so this request participates in ordinary completion/cancellation flow.
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            const FCk_Request_CrowdAgent_SetMaxSpeed& InRequest)
        -> void
    {
        InParams._MaxSpeed = InRequest.Get_MaxSpeed();

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] SetMaxSpeed {}"), InHandle, InRequest.Get_MaxSpeed());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_MoveRequests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
