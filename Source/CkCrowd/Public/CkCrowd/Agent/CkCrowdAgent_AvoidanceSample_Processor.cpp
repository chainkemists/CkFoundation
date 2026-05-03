#include "CkCrowdAgent_AvoidanceSample_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AvoidanceSample);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace
    {
        // Tag check — agent itself plus its lifetime owner (typical: gym station / zone volume).
        // Walks one hop only; designer puts the tag on the immediate parent that defines the zone.
        auto HasAvoidanceTag(const FCk_Handle& InAgent, const FGameplayTag& InTag) -> bool
        {
            // Direct check — the agent may carry it on its Params._Tags.
            if (InAgent.Has<FFragment_CrowdAgent_Params>())
            {
                const auto& Params = InAgent.Get<FFragment_CrowdAgent_Params>();
                if (Params.Get_Tags().HasTagExact(InTag))
                { return true; }
            }

            const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAgent);
            if (ck::Is_NOT_Valid(Owner))
            { return false; }

            // Owner-side fallback for cases where designers tag the parent station instead.
            if (Owner.Has<FFragment_CrowdAgent_Params>())
            {
                return Owner.Get<FFragment_CrowdAgent_Params>().Get_Tags().HasTagExact(InTag);
            }
            return false;
        }

        // Resolve "should this agent sample this frame?" — combines project mode, per-agent
        // override, zone tags, neighbor count, and round-robin stride.
        auto ShouldSample(
            const FCk_Handle_CrowdAgent& InAgent,
            const FFragment_CrowdAgent_NeighborCache& InCache) -> bool
        {
            // Per-agent override has the highest priority (and short-circuits the rest).
            if (InAgent.Has<FFragment_CrowdAgent_AvoidancePolicy>())
            {
                switch (InAgent.Get<FFragment_CrowdAgent_AvoidancePolicy>().Get_Policy())
                {
                    case ECk_AvoidancePolicy::ForceOnly:        return false;
                    case ECk_AvoidancePolicy::SamplingAlways:   return true;
                    case ECk_AvoidancePolicy::UseProjectDefault: break;  // fall through
                }
            }

            // Zone-tag NeverSample wins over project default.
            if (HasAvoidanceTag(InAgent, TAG_CrowdAvoidance_NeverSample))
            { return false; }

            const auto Trigger = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleTrigger();
            if (Trigger == ECk_AvoidanceSampleTrigger::Disabled)
            { return false; }

            const auto AlwaysTagSet = HasAvoidanceTag(InAgent, TAG_CrowdAvoidance_AlwaysSample);
            const auto Threshold = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleNeighborThreshold();
            const auto NeighborGate = InCache.Get_Neighbors().Num() >= Threshold;

            switch (Trigger)
            {
                case ECk_AvoidanceSampleTrigger::NeighborCountOnly:        return NeighborGate;
                case ECk_AvoidanceSampleTrigger::ZoneTagOnly:              return AlwaysTagSet;
                case ECk_AvoidanceSampleTrigger::NeighborCountAndZoneTag:  return NeighborGate || AlwaysTagSet;
                default:                                                   return false;
            }
        }

        // Round-robin: agent fires only on frames where (Frame + EntityHash) % Stride == 0.
        // Distributes cost evenly regardless of which agents are clumped.
        auto IsSamplingFrame(const FCk_Handle_CrowdAgent& InAgent) -> bool
        {
            const auto Stride = FMath::Max(1, UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleStride());
            const auto FrameIdx = static_cast<int32>(GFrameCounter);
            const auto AgentIdx = static_cast<int32>(GetTypeHash(InAgent));
            return ((FrameIdx + AgentIdx) % Stride) == 0;
        }

        // Build the candidate-velocity set. Pattern mirrors DetourObstacleAvoidance.cpp:548-567
        // but stripped to a single depth iteration with N×R samples on concentric rings.
        // Returns up to AngularDivs * Rings + 1 candidates (+1 for the zero-vector "stop" sample).
        auto BuildSamplePattern(
            const FVector& InDesiredVelocity,
            float InMaxSpeed,
            int32 InAngularDivs,
            int32 InRings) -> TArray<FVector>
        {
            TArray<FVector> Samples;
            Samples.Reserve(InAngularDivs * InRings + 1);

            // Ring 0: the do-nothing candidate (zero velocity). Always present so "stop" is on
            // the table when a strong neighbor force says we should yield entirely.
            Samples.Add(FVector::ZeroVector);

            // Use the desired velocity's heading as the pattern center. If it's near-zero (agent
            // braked at goal), use world +X as a fallback.
            const auto DesiredDir = InDesiredVelocity.IsNearlyZero()
                ? FVector::ForwardVector
                : InDesiredVelocity.GetSafeNormal();
            const auto BaseAngle = FMath::Atan2(DesiredDir.Y, DesiredDir.X);

            const auto AngularStep = (2.0f * PI) / static_cast<float>(InAngularDivs);
            for (int32 Ring = 1; Ring <= InRings; ++Ring)
            {
                const auto Radius = (static_cast<float>(Ring) / static_cast<float>(InRings)) * InMaxSpeed;
                // Stagger alternating rings by half-step for better angular coverage (mirrors dtCrowd).
                const auto AngleOffset = (Ring % 2 == 0) ? 0.5f * AngularStep : 0.0f;
                for (int32 Div = 0; Div < InAngularDivs; ++Div)
                {
                    const auto Angle = BaseAngle + AngleOffset + AngularStep * static_cast<float>(Div);
                    Samples.Emplace(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
                }
            }

            return Samples;
        }

        // Time-to-collision between self at velocity vCand and neighbor at relative position relPos
        // with relative velocity relVel (both in 2D, Z dropped). Returns FLT_MAX if no collision in
        // the time horizon, else seconds until first contact. Math mirrors DetourObstacleAvoidance.cpp
        // sweepCircleCircle: solves for t such that |relPos + (vCand-relVel)*t| == combinedRadius.
        auto TimeToCollision(
            const FVector& InCandidateVel,
            const FVector& InNeighborRelPos,
            const FVector& InNeighborRelVel,
            float InCombinedRadius,
            float InHorizon) -> float
        {
            // Translate to neighbor's frame: dv = candidate moving relative to neighbor.
            const auto Dv = FVector2D(InCandidateVel - InNeighborRelVel);
            const auto P  = FVector2D(-InNeighborRelPos.X, -InNeighborRelPos.Y);
            // (P + Dv*t)·(P + Dv*t) = R² → quadratic in t.
            const auto A = static_cast<float>(FVector2D::DotProduct(Dv, Dv));
            const auto B = static_cast<float>(FVector2D::DotProduct(P, Dv));
            const auto C = static_cast<float>(FVector2D::DotProduct(P, P)) - InCombinedRadius * InCombinedRadius;

            // Already inside the radius? Treat as immediate collision.
            if (C < 0.0f) { return 0.0f; }
            // Moving in a direction with no component toward neighbor? No collision.
            if (A < KINDA_SMALL_NUMBER || B >= 0.0f) { return FLT_MAX; }

            const auto Disc = B * B - A * C;
            if (Disc < 0.0f) { return FLT_MAX; }
            const auto T = (-B - FMath::Sqrt(Disc)) / A;
            return T > InHorizon ? FLT_MAX : T;
        }

        // Side score per neighbor — mirrors dtCrowd's wSide concept. >0 = candidate passes neighbor
        // on the same side they're already on relative to current motion (= candidate is "going toward"
        // neighbor's side, the bad direction); 0 if candidate is going to the opposite side.
        auto SideScore(
            const FVector& InCandidateVel,
            const FVector& InCurrentVel,
            const FVector& InNeighborRelPos) -> float
        {
            // 2D cross of current-velocity and neighbor-relpos tells us which side neighbor is on.
            const auto CrossNeighbor = static_cast<float>(InCurrentVel.X * InNeighborRelPos.Y - InCurrentVel.Y * InNeighborRelPos.X);
            // 2D cross of current-velocity and (candidate-velocity) tells us which side the candidate
            // is taking us toward.
            const auto CrossCand = static_cast<float>(InCurrentVel.X * InCandidateVel.Y - InCurrentVel.Y * InCandidateVel.X);
            // Same sign = candidate goes toward neighbor's side = bad.
            return (CrossNeighbor * CrossCand >= 0.0f) ? FMath::Abs(CrossCand) : 0.0f;
        }
    }

    auto
        FProcessor_CrowdAgent_AvoidanceSample::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        // Trigger gate. Off-path leaves the force solver's _Velocity in place (which will then be
        // ramped by AccelClamp downstream).
        if (NOT ShouldSample(InHandle, InNeighborCache))
        { return; }

        if (NOT IsSamplingFrame(InHandle))
        { return; }

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        // Read project settings once per agent (cheap; settings are CDO-backed).
        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        const auto MaxSpeed   = InParams.Get_MaxSpeed();
        const auto AgentRad   = InParams.Get_Radius();
        const auto Horizon    = Settings->Get_AvoidanceHorizonTime();
        const auto WDes       = Settings->Get_AvoidanceWeightDesVel();
        const auto WCur       = Settings->Get_AvoidanceWeightCurVel();
        const auto WSide      = Settings->Get_AvoidanceWeightSide();
        const auto WToi       = Settings->Get_AvoidanceWeightToi();
        const auto SidePref   = Settings->Get_AvoidanceSidePreference();
        const auto SideEnabled = (SidePref != ECk_AvoidanceSidePreference::Disabled);
        const auto SideSign   = (SidePref == ECk_AvoidanceSidePreference::PassRight) ? -1.0f : 1.0f;
        const auto AngularDivs = Settings->Get_AvoidanceSampleAngularDivs();
        const auto Rings       = Settings->Get_AvoidanceSampleRings();
        const auto InvVMax     = (MaxSpeed > KINDA_SMALL_NUMBER) ? 1.0f / MaxSpeed : 0.0f;
        const auto InvHorizon  = (Horizon  > KINDA_SMALL_NUMBER) ? 1.0f / Horizon  : 0.0f;

        // Cache the path-follow desired velocity (what the force solver wrote) BEFORE we overwrite.
        const auto DesiredVel = InDesired.Get_Velocity();
        // Current velocity = last frame's _LastVelocity (set by AccelClamp). Falls back to current
        // _Velocity for the first frame an agent samples (LastVelocity may still be zero).
        const auto CurrentVel = InDesired.Get_LastVelocity().IsNearlyZero()
            ? DesiredVel
            : InDesired.Get_LastVelocity();

        const auto Samples = BuildSamplePattern(DesiredVel, MaxSpeed, AngularDivs, Rings);

        // Score each candidate, track best.
        auto BestPenalty = TNumericLimits<float>::Max();
        auto BestVel     = DesiredVel;

        for (const auto& Cand : Samples)
        {
            // wDesVel: deviation from the path-follow heading (normalised by MaxSpeed).
            const auto DesPen = WDes * static_cast<float>(FVector::Dist2D(Cand, DesiredVel)) * InvVMax;
            // wCurVel: deviation from current velocity — the inertia bias.
            const auto CurPen = WCur * static_cast<float>(FVector::Dist2D(Cand, CurrentVel)) * InvVMax;

            // wToi: minimum time-to-collision across neighbors. Reciprocal-scaled so short TTCs
            // dominate (the formula is tpen = wToi * 1/(0.1 + tmin*invHorizTime), per dtCrowd).
            auto TMin = FLT_MAX;
            auto SideAccum = 0.0f;
            for (const auto& Nbr : Neighbors)
            {
                const auto NbrRad = AgentRad + AgentRad;  // approximation: neighbors share radius
                const auto Ttc = TimeToCollision(Cand, Nbr.Get_RelativeOffset(), Nbr.Get_RelativeVelocity(), NbrRad, Horizon);
                if (Ttc < TMin) { TMin = Ttc; }

                if (SideEnabled)
                {
                    SideAccum += SideScore(Cand, CurrentVel, Nbr.Get_RelativeOffset());
                }
            }
            const auto ToiPen = (TMin >= FLT_MAX)
                ? 0.0f
                : (WToi * (1.0f / (0.1f + TMin * InvHorizon)));
            const auto SidePen = SideEnabled
                ? (WSide * SideAccum * SideSign)
                : 0.0f;

            const auto Penalty = DesPen + CurPen + ToiPen + SidePen;
            if (Penalty < BestPenalty)
            {
                BestPenalty = Penalty;
                BestVel     = Cand;
            }
        }

        InDesired._Velocity = BestVel;
    }
}

// --------------------------------------------------------------------------------------------------------------------
