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
        // stripped to a single depth iteration with N×R samples on concentric rings, plus three
        // explicit "anchor" samples: zero, the path-follow desired velocity, and the agent's
        // current velocity. The anchors guarantee that the most natural choices (stop / continue
        // as planned / hold current motion) are always in the set with their exact magnitude and
        // direction — without them the ring grid only covers off-axis approximations and the
        // sampler oscillates between similar-but-not-equal candidates as the desired direction
        // tilts frame-to-frame.
        auto BuildSamplePattern(
            const FVector& InDesiredVelocity,
            const FVector& InCurrentVelocity,
            float InMaxSpeed,
            int32 InAngularDivs,
            int32 InRings) -> TArray<FVector>
        {
            TArray<FVector> Samples;
            Samples.Reserve(InAngularDivs * InRings + 3);

            // Anchor 1: the do-nothing zero candidate. Always present so "stop" is on the table.
            Samples.Add(FVector::ZeroVector);

            // Anchor 2: path-follow desired velocity exactly. Without this, the sampler can never
            // pick "go where I planned at full speed" — only off-axis ring approximations of it.
            if (NOT InDesiredVelocity.IsNearlyZero())
            {
                Samples.Add(InDesiredVelocity.GetClampedToMaxSize(InMaxSpeed));
            }

            // Anchor 3: current velocity exactly. Supports the wCurVel inertia bias so the agent
            // can "keep going as I am" without the ring grid forcing a CurPen tax.
            if (NOT InCurrentVelocity.IsNearlyZero()
                && NOT InCurrentVelocity.Equals(InDesiredVelocity, 1.0f))
            {
                Samples.Add(InCurrentVelocity.GetClampedToMaxSize(InMaxSpeed));
            }

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

        // Time-to-collision between self at velocity vCand and neighbor moving at its absolute
        // velocity. Returns FLT_MAX if no collision in the time horizon, else seconds until first
        // contact.
        //
        // The neighbor cache stores relative velocity (NbrVel - SelfVel_at_sample). To get the
        // correct closing rate for self-at-CANDIDATE-velocity, we need:
        //   Dv = CandidateVel - NbrVel
        //      = CandidateVel - (RelativeVelocity + SelfVel_at_sample)
        //      = CandidateVel - RelativeVelocity - SelfVel_at_sample
        //
        // Without subtracting SelfVel here, TOIs come out too short (forward candidates look more
        // dangerous than they are) and the sampler over-corrects.
        //
        // Math mirrors DetourObstacleAvoidance.cpp sweepCircleCircle: solves for t such that
        // |P + Dv*t| == combinedRadius, where P = -NbrRelPos.
        auto TimeToCollision(
            const FVector& InCandidateVel,
            const FVector& InCurrentSelfVel,
            const FVector& InNeighborRelPos,
            const FVector& InNeighborRelVel,
            float InCombinedRadius,
            float InHorizon) -> float
        {
            const auto Dv = FVector2D(InCandidateVel - InNeighborRelVel - InCurrentSelfVel);
            const auto P  = FVector2D(-InNeighborRelPos.X, -InNeighborRelPos.Y);
            const auto A = static_cast<float>(FVector2D::DotProduct(Dv, Dv));
            const auto B = static_cast<float>(FVector2D::DotProduct(P, Dv));
            const auto C = static_cast<float>(FVector2D::DotProduct(P, P)) - InCombinedRadius * InCombinedRadius;

            if (C < 0.0f) { return 0.0f; }
            if (A < KINDA_SMALL_NUMBER || B >= 0.0f) { return FLT_MAX; }

            const auto Disc = B * B - A * C;
            if (Disc < 0.0f) { return FLT_MAX; }
            const auto T = (-B - FMath::Sqrt(Disc)) / A;
            return T > InHorizon ? FLT_MAX : T;
        }

        // Side "rightness" — how much a candidate velocity goes RIGHT relative to current motion.
        // Independent of neighbor position so head-on cases (where neighbor lies along the motion
        // axis with cross-product = 0) still produce a meaningful preference. Caller flips sign
        // based on PassLeft vs PassRight.
        //
        // The simpler dtCrowd-equivalent semantics: "always swerve LEFT (or RIGHT) when avoiding"
        // — the side preference is the agent's own choice, not a function of where the neighbor
        // is. Two head-on agents both swerving LEFT necessarily diverge (their lefts point in
        // opposite world directions because they face opposite ways).
        auto SideRightness(
            const FVector& InCandidateVel,
            const FVector& InMotionDir) -> float
        {
            // Left perpendicular of motion direction (90° CCW around +Z).
            const auto LeftPerp = FVector(-InMotionDir.Y, InMotionDir.X, 0.0);
            // Dot of candidate with leftPerp: positive = candidate goes LEFT, negative = RIGHT.
            const auto Leftness = static_cast<float>(FVector::DotProduct(InCandidateVel, LeftPerp));
            return -Leftness;  // flip so positive = going RIGHT
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

        const auto Samples = BuildSamplePattern(DesiredVel, CurrentVel, MaxSpeed, AngularDivs, Rings);

        // Motion direction for side preference. Falls back to desired direction if current is
        // near-zero (e.g. agent at start of motion).
        const auto MotionDir = CurrentVel.IsNearlyZero()
            ? (DesiredVel.IsNearlyZero() ? FVector::ForwardVector : DesiredVel.GetSafeNormal())
            : CurrentVel.GetSafeNormal();

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
            for (const auto& Nbr : Neighbors)
            {
                const auto NbrRad = AgentRad + AgentRad;  // approximation: neighbors share radius
                const auto Ttc = TimeToCollision(Cand, CurrentVel, Nbr.Get_RelativeOffset(), Nbr.Get_RelativeVelocity(), NbrRad, Horizon);
                if (Ttc < TMin) { TMin = Ttc; }
            }
            const auto ToiPen = (TMin >= FLT_MAX)
                ? 0.0f
                : (WToi * (1.0f / (0.1f + TMin * InvHorizon)));

            // wSide: penalty for going against the chosen side preference. Normalised by MaxSpeed
            // so it's comparable to the other weighted terms.
            auto SidePen = 0.0f;
            if (SideEnabled)
            {
                const auto Rightness = SideRightness(Cand, MotionDir) * InvVMax;
                // SideSign = +1 (PassLeft) penalizes positive rightness; -1 (PassRight) penalizes
                // negative rightness. Negative penalty would be a bonus — clamp to zero.
                SidePen = WSide * FMath::Max(0.0f, Rightness * SideSign);
            }

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
