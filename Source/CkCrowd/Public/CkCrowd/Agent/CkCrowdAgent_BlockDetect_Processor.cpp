#include "CkCrowdAgent_BlockDetect_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_PathRefresh_Processor.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_BlockDetect);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_BlockedRecheck);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::BlockDetect"), STAT_CkCrowd_BlockDetectProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::BlockedRecheck"), STAT_CkCrowd_BlockedRecheckProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_crowdagent_blockdetect
    {
        // Is the agent's FINAL destination occupied by a neighbour that has settled on it?
        //
        // The closest an agent can physically get to a point occupied by another agent is
        // (SelfRadius + NbrRadius) from that neighbour's centre. If that exceeds the agent's arrival
        // tolerance, the goal is UNREACHABLE — not "hard", not "slow": unreachable. This is geometry,
        // not a heuristic, which is why it needs no timeout and never has to guess.
        //
        // Returns the blocking neighbour, or an invalid handle when the goal is clear.
        auto Get_GoalBlocker(
            const FVector& InSelfLoc,
            const FVector& InSelfVel,
            const FVector& InFinalWaypoint,
            float InSelfRadius,
            float InArrivalRadius,
            float InStationarySpeedThreshold,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> FCk_Handle
        {
            for (const auto& Nbr : InNeighborCache.Get_Neighbors())
            {
                // A neighbour merely PASSING THROUGH the goal is not blocking it — it will be gone by
                // the time we arrive. Only one that has settled there is an obstruction.
                const auto NbrAbsVel = Nbr.Get_RelativeVelocity() + InSelfVel;
                if (NbrAbsVel.Size2D() >= InStationarySpeedThreshold)
                { continue; }

                const auto NbrCentre = InSelfLoc + Nbr.Get_RelativeOffset();

                // Neighbours share our radius (the same approximation the rest of the module makes).
                const auto CombinedRadius = InSelfRadius + InSelfRadius;
                const auto ClosestApproach = CombinedRadius - InArrivalRadius;

                if (ClosestApproach <= 0.0f)
                { continue; }  // arrival tolerance is wide enough to swallow the blocker — not blocked

                if (FVector::Dist2D(NbrCentre, InFinalWaypoint) < ClosestApproach)
                { return Nbr.Get_Handle(); }
            }

            return FCk_Handle{};
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_BlockDetect::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_BlockDetectProc);

        if (UCk_Utils_Crowd_Settings_UE::Get_BlockDetectionMode() == ECk_CrowdBlockDetectionMode::Disabled)
        { return; }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.IsEmpty())
        { return; }

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        { return; }

        const auto SelfLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        // The point Steering actually latches on. NOT _ActiveGoal: for a Partial path the two differ,
        // and it is the path's end the agent is really trying to stand on.
        const auto& FinalWaypoint = Waypoints.Last();
        const auto DistanceToFinal = static_cast<float>(FVector::Dist(SelfLoc, FinalWaypoint));

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto SelfRadius = InParams.Get_Radius();
        const auto ArrivalRadius = InPathFollow.Get_ActiveArrivalRadius();

        // ---- Geometric detector (primary) --------------------------------------------------------
        //
        // Only meaningful near the destination: a neighbour parked on a goal 30m away is not blocking
        // anything yet, and it may well have moved by the time we get there. Engage once the agent is
        // within its own braking distance of the end — the last moment it could still stop cleanly —
        // plus the body it would have to fit through. Derived from existing constants; no new knob.
        const auto BrakingDistance = MaxAccel > 0.0f
            ? (MaxSpeed * MaxSpeed) / (2.0f * MaxAccel)
            : 0.0f;
        constexpr auto EngagementSlackCm = 20.0f;
        const auto EngagementRange =
            (2.0f * SelfRadius) + ArrivalRadius + BrakingDistance + EngagementSlackCm;

        if (DistanceToFinal <= EngagementRange)
        {
            auto SelfVelocity = UCk_Utils_Velocity_UE::Cast(InHandle);
            const auto SelfVel = ck::IsValid(SelfVelocity)
                ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(SelfVelocity)
                : FVector::ZeroVector;

            const auto Blocker = ck_crowdagent_blockdetect::Get_GoalBlocker(
                SelfLoc,
                SelfVel,
                FinalWaypoint,
                SelfRadius,
                ArrivalRadius,
                Settings->Get_BlockedStationarySpeedThreshold(),
                InNeighborCache);

            if (ck::IsValid(Blocker))
            {
                DoBlock(InHandle, InParams, InBlockDetect,
                    ECk_CrowdAgent_BlockedReason::GoalOccupied, Blocker, DistanceToFinal);
                return;
            }
        }

        // ---- No-progress detector (safety net) ---------------------------------------------------
        //
        // Everything the geometric test cannot see: a plug of several agents, a dynamic prop, a
        // corridor the agent physically cannot walk. Mirrors UPathFollowingComponent's block detection
        // (PathFollowingComponent.cpp:1556-1608).
        const auto SampleCount = Settings->Get_BlockDetectionSampleCount();
        const auto SampleInterval = Settings->Get_BlockDetectionInterval();

        InBlockDetect._SampleAccumulatorSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InBlockDetect._SampleAccumulatorSec < SampleInterval)
        { return; }

        InBlockDetect._SampleAccumulatorSec = 0.0f;

        if (InBlockDetect._FeetSamples.Num() < SampleCount)
        {
            InBlockDetect._FeetSamples.Add(SelfLoc);
            return;
        }

        InBlockDetect._FeetSamples[InBlockDetect._NextSampleIdx] = SelfLoc;
        InBlockDetect._NextSampleIdx = (InBlockDetect._NextSampleIdx + 1) % SampleCount;

        auto Centroid = FVector::ZeroVector;
        for (const auto& Sample : InBlockDetect._FeetSamples)
        { Centroid += Sample; }
        Centroid /= static_cast<float>(InBlockDetect._FeetSamples.Num());

        const auto BlockRadius = Settings->Get_BlockDetectionDistance();
        const auto GoingNowhere = ck::algo::AllOf(InBlockDetect._FeetSamples,
            [&](const FVector& InSample) -> bool
            {
                return FVector::Dist(InSample, Centroid) <= BlockRadius;
            });

        if (GoingNowhere)
        {
            DoBlock(InHandle, InParams, InBlockDetect,
                ECk_CrowdAgent_BlockedReason::NoProgress, FCk_Handle{}, DistanceToFinal);
        }
    }

    auto
        FProcessor_CrowdAgent_BlockDetect::
        DoBlock(
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            ECk_CrowdAgent_BlockedReason InReason,
            FCk_Handle InBlocker,
            float InDistanceToGoal) const
        -> void
    {
        auto NonConstHandle = InHandle;

        // Stop. Walking → Idle + GoalBlocked. Idle is what makes the agent actually halt (AccelClamp's
        // Idle branch ramps the velocity down, so it decelerates rather than snapping to a stop), and
        // it preserves the documented Idle/PathPending/Walking exclusivity. GoalBlocked records that
        // the agent still WANTS the goal — which is what lets BlockedRecheck resume it later.
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_GoalBlocked>();

        InBlockDetect._BlockedBy = InBlocker;
        InBlockDetect._FeetSamples.Reset();
        InBlockDetect._NextSampleIdx = 0;
        InBlockDetect._SampleAccumulatorSec = 0.0f;
        InBlockDetect._RecheckAccumulatorSec = 0.0f;

        // Once per blocked EPISODE, not once per re-check — otherwise an agent that holds, retries and
        // re-blocks would spam its listener every cadence.
        if (NOT InBlockDetect._BlockedSignalSent)
        {
            InBlockDetect._BlockedSignalSent = true;

            UUtils_Signal_CrowdAgent_OnGoalBlocked::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle,
                    FCk_CrowdAgent_GoalBlockedInfo{InReason, InBlocker, InDistanceToGoal}));

            ck::crowd::Verbose(TEXT("CrowdAgent [{}] goal BLOCKED ({}) at {}cm — blocker [{}]"),
                InHandle, InReason, InDistanceToGoal, InBlocker);
        }

        // FailMove hands recovery to the caller, UE-style: the move is over, and gameplay decides what
        // to do. HoldAndRetry (the default) leaves GoalBlocked in place for BlockedRecheck to resume.
        if (InParams.Get_BlockedPolicy() == ECk_CrowdAgent_BlockedPolicy::FailMove)
        {
            NonConstHandle.Try_Remove<FTag_CrowdAgent_GoalBlocked>();

            UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_BlockedRecheck::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_BlockedRecheckProc);

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        InBlockDetect._RecheckAccumulatorSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InBlockDetect._RecheckAccumulatorSec < Settings->Get_BlockedRecheckInterval())
        { return; }

        InBlockDetect._RecheckAccumulatorSec = 0.0f;

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        { return; }

        const auto SelfLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        // Is the goal still taken? A held agent is stationary, so its own velocity is ~zero and the
        // neighbour's relative velocity IS its absolute velocity.
        const auto Blocker = ck_crowdagent_blockdetect::Get_GoalBlocker(
            SelfLoc,
            FVector::ZeroVector,
            InPathFollow.Get_ActiveGoal(),
            InParams.Get_Radius(),
            InPathFollow.Get_ActiveArrivalRadius(),
            Settings->Get_BlockedStationarySpeedThreshold(),
            InNeighborCache);

        if (ck::IsValid(Blocker))
        { return; }  // still taken — keep holding

        // The goal is free. Resume.
        //
        // A FULL RE-PATH, not a resumption of the stale cursor: the agent may have been shoved around
        // while it held, and a no-progress-blocked agent wants a fresh chance at a different corridor
        // anyway. This is the mechanism that makes "blocked" recoverable rather than terminal — the
        // reason we refused to fake an arrival at a widened radius.
        auto NonConstHandle = InHandle;

        NonConstHandle.Try_Remove<FTag_CrowdAgent_GoalBlocked>();
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        InPathFollow._WaypointIndex = 0;

        InBlockDetect._BlockedBy = FCk_Handle{};
        InBlockDetect._FeetSamples.Reset();
        InBlockDetect._NextSampleIdx = 0;
        InBlockDetect._SampleAccumulatorSec = 0.0f;

        // NOT resetting _BlockedSignalSent: the agent is still working on the SAME goal, so a
        // re-block is the same episode and must not re-fire the signal. Only an external MoveTo/Stop
        // starts a new episode.

        const auto Goal = InPathFollow.Get_ActiveGoal();

        if (UCk_Utils_PathNetworkFollower_UE::Has(NonConstHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(NonConstHandle);
            NonConstHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(NonConstHandle);
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower,
                FCk_Request_PathNetworkFollower_FindRoute{Goal});
        }
        else
        {
            auto Request = FCk_Request_Nav_FindPath{Goal};
            Request.Set_QueryFilter(InParams.Get_NavQueryFilter());

            // A held agent may have been shoved inside painted stationary markup while it waited;
            // resuming from inside the band would pick "through" — see Get_EscapedQueryStart.
            const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                NonConstHandle, InHandle.Get_Entity(), SelfLoc, Goal, InParams.Get_Radius());
            if (Escaped.IsSet())
            {
                Request.Set_StartOverride(ECk_EnableDisable::Enable)
                       .Set_StartOverrideLocation(*Escaped);
            }

            UCk_Utils_Nav_UE::Request_FindPath(NonConstHandle, Request);
        }

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] goal CLEARED — resuming to {}"), InHandle, Goal);
    }
}

// --------------------------------------------------------------------------------------------------------------------
