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
                const auto NbrAbsVel = Nbr.Get_RelativeVelocity() + InSelfVel;
                const auto NeighbourHasSettled = NbrAbsVel.Size2D() < InStationarySpeedThreshold;
                if (NOT NeighbourHasSettled)
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
            const FFragment_Transform& InTransform,
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

        const auto SelfLoc = InTransform.Get_Transform().GetLocation();

        // The path's END, not _ActiveGoal — for a Partial path the two differ.
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
        // Only meaningful near the destination — a neighbour parked on a goal 30m away blocks nothing yet.
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

        // Idle is what actually halts the agent (AccelClamp's Idle branch ramps its velocity down);
        // GoalBlocked records that it still WANTS the goal, which is what lets BlockedRecheck resume it.
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_GoalBlocked>();

        InBlockDetect._BlockedBy = InBlocker;
        InBlockDetect._FeetSamples.Reset();
        InBlockDetect._NextSampleIdx = 0;
        InBlockDetect._SampleAccumulatorSec = 0.0f;
        InBlockDetect._RecheckAccumulatorSec = 0.0f;

        // Once per blocked EPISODE, not once per re-check — a holding agent re-blocks every cadence.
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
            const FFragment_Transform& InTransform,
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

        const auto SelfLoc = InTransform.Get_Transform().GetLocation();

        // A held agent is stationary, so neighbour relative velocity IS absolute velocity — hence
        // the zero self-velocity argument.
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

        // The goal is free: a FULL re-path, not a resumed cursor — the agent may have been shoved
        // around while it held.
        auto NonConstHandle = InHandle;

        NonConstHandle.Try_Remove<FTag_CrowdAgent_GoalBlocked>();
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Idle>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        InPathFollow._WaypointIndex = 0;

        InBlockDetect._BlockedBy = FCk_Handle{};
        InBlockDetect._FeetSamples.Reset();
        InBlockDetect._NextSampleIdx = 0;
        InBlockDetect._SampleAccumulatorSec = 0.0f;

        // _BlockedSignalSent is deliberately NOT reset: same goal means same episode. Only an
        // external MoveTo/Stop starts a new one.

        const auto Goal = InPathFollow.Get_ActiveGoal();

        if (UCk_Utils_PathNetworkFollower_UE::Has(NonConstHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(NonConstHandle);
            NonConstHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(NonConstHandle);
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower,
                FCk_Request_PathNetworkFollower_FindRoute{Goal}, {});
        }
        else
        {
            // Park the slot at Pending so OnPathResolved can't consume the PREVIOUS Ready corridor.
            FCk_Nav_Algorithm::MarkPathPending(NonConstHandle);

            auto Request = FCk_Request_Nav_FindPath{Goal};
            Request.Set_QueryFilter(InParams.Get_NavQueryFilter());

            // A held agent may have been shoved inside painted stationary markup — resuming from
            // inside the band would pick "through". See Get_EscapedQueryStart.
            const auto Escaped = FProcessor_CrowdAgent_PathRefresh::Get_EscapedQueryStart(
                NonConstHandle, InHandle.Get_Entity(), SelfLoc, Goal, InParams.Get_Radius());
            if (Escaped.IsSet())
            {
                Request.Set_StartOverride(ECk_EnableDisable::Enable)
                       .Set_StartOverrideLocation(*Escaped);
            }

            UCk_Utils_Nav_UE::Request_FindPath(NonConstHandle, Request, {});
        }

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] goal CLEARED — resuming to {}"), InHandle, Goal);
    }
}

// --------------------------------------------------------------------------------------------------------------------
