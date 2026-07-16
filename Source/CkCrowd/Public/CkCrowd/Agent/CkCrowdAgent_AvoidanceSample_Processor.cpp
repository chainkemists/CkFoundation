#include "CkCrowdAgent_AvoidanceSample_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AvoidanceSample);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample"), STAT_CkCrowd_AvoidanceSampleProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample (scan)"), STAT_CkCrowd_AvoidanceSample_Scan, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Agents Sampled"), STAT_CkCrowd_AgentsSampled, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_crowd_agent_avoidance_sample_processor
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

        // Build the candidate-velocity cloud. Mirrors DetourObstacleAvoidance.cpp:538-572
        // (sampleVelocityAdaptive, single depth iteration): ring samples of radius
        // MaxSpeed * (1 - VelBias), centred on DesiredVelocity * VelBias — NOT on the origin, and
        // NOT on the desired velocity itself.
        //
        // The bias centre IS Detour's "always add sample at zero" (:551-554): that is a zero
        // PATTERN OFFSET, so the centre candidate is VelBias of the desired velocity ("slow down"),
        // never a true stop. This port previously centred the rings on the ORIGIN and added an
        // explicit zero-velocity anchor "so stop is on the table". That anchor is a freeze fixed
        // point: two stopped agents predict no collision between them, so the stop candidate pays
        // NO time-to-impact penalty while every forward candidate pays the full one — and once
        // stopped, the inertia term makes staying stopped cheaper still. Both agents pick it and
        // never move again. Detour has no such candidate, and neither do we now.
        //
        // The desired- and current-velocity anchors go with it: they predate the bias centre, and
        // with the cloud centred on DesiredVelocity * VelBias the outer ring already contains the
        // desired velocity whenever the agent is at full speed. Off-grid anchors also distort the
        // uniform-coverage assumption the penalty comparison rests on. Detour has neither.
        //
        // Single depth iteration (no adaptiveDepth refinement) is deliberate and is a shipped
        // engine configuration: UE's ECrowdAvoidanceQuality::Low is 5 divs x 2 rings + centre at
        // VelBias 0.5 (CrowdManager.cpp:182-187). Our 8 x 2 + centre is denser than that.
        //
        // Inline capacity covers the default 8x2 pattern + the bias centre without heap traffic —
        // this allocates per sampling agent per frame, and under parallel dispatch each task
        // builds its own.
        using FSamplePattern = TArray<FVector, TInlineAllocator<32>>;

        auto BuildSamplePattern(
            const FVector& InDesiredVelocity,
            float InMaxSpeed,
            float InVelBias,
            int32 InAngularDivs,
            int32 InRings) -> FSamplePattern
        {
            FSamplePattern Samples;
            Samples.Reserve(InAngularDivs * InRings + 1);

            const auto BiasCentre = InDesiredVelocity * InVelBias;
            const auto CloudRadius = InMaxSpeed * (1.0f - InVelBias);

            Samples.Add(BiasCentre);

            // Pattern heading follows the desired velocity. The world +X fallback matches Detour's
            // dtAtan2(0, 0) == 0 when the agent has no desired direction.
            const auto DesiredDir = InDesiredVelocity.IsNearlyZero()
                ? FVector::ForwardVector
                : InDesiredVelocity.GetSafeNormal();
            const auto BaseAngle = FMath::Atan2(DesiredDir.Y, DesiredDir.X);

            const auto AngularStep = (2.0f * PI) / static_cast<float>(InAngularDivs);
            for (int32 Ring = 1; Ring <= InRings; ++Ring)
            {
                const auto Radius = (static_cast<float>(Ring) / static_cast<float>(InRings)) * CloudRadius;
                // Stagger alternating rings by half-step for better angular coverage (mirrors dtCrowd).
                const auto AngleOffset = (Ring % 2 == 0) ? 0.5f * AngularStep : 0.0f;
                for (int32 Div = 0; Div < InAngularDivs; ++Div)
                {
                    const auto Angle = BaseAngle + AngleOffset + AngularStep * static_cast<float>(Div);
                    const auto Candidate = BiasCentre +
                        FVector{FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0};

                    // Detour rejects candidates beyond vmax (DetourObstacleAvoidance.cpp:589). At
                    // VelBias 0.5 nothing can exceed it (0.5*|dvel| + 0.5*vmax <= vmax); the guard
                    // keeps tuned VelBias values well-formed.
                    if (Candidate.SizeSquared2D() > FMath::Square(InMaxSpeed + KINDA_SMALL_NUMBER))
                    { continue; }

                    Samples.Emplace(Candidate);
                }
            }

            return Samples;
        }

        // Time-to-collision between self at velocity vCand and neighbor moving at its absolute
        // velocity. Returns FLT_MAX if no collision in the time horizon, else seconds until first
        // contact. When the pair ALREADY overlaps, returns an overlap-severity score in the same
        // units (see below) rather than a true time.
        //
        // RVO RECIPROCITY, per DetourObstacleAvoidance.cpp:337-341 (vab = 2*vcand - vel - nei.vel).
        // Every agent runs this same sampler, so each may assume the neighbour will shoulder HALF
        // the avoidance. Doubling the candidate's weight in relative-velocity space means half the
        // deviation resolves the same predicted collision, and mutual avoidance CONVERGES instead
        // of both agents over-correcting into a reciprocal dance. This port previously used plain
        // VO (Dv = Cand - NbrVel), i.e. each agent solved as if the other would not move at all.
        //
        // The neighbour cache stores relative velocity (NbrVel - SelfVel_at_sample), so
        // NbrVel = RelativeVelocity + SelfVel, and the RVO relative velocity expands to:
        //   vab = 2*Cand - SelfVel - NbrVel
        //       = 2*Cand - SelfVel - (RelativeVelocity + SelfVel)
        //       = 2*(Cand - SelfVel) - RelativeVelocity
        //
        // Math mirrors DetourObstacleAvoidance.cpp sweepCircleCircle: solves for t such that
        // |P + Dv*t| == combinedRadius, where P = -NbrRelPos.
        //
        // THE OVERLAP CASE (C < 0) IS NOT "TIME ZERO". Returning a flat 0.0 here — as this did — is
        // the bug that made two touching agents refuse to avoid each other at all. C is a function of
        // the relative POSITION and the combined radius ONLY; it does not depend on the candidate. So
        // a constant return makes the caller's ToI penalty (wToi / (0.1 + tmin*invHorizon)) evaluate
        // IDENTICALLY for every candidate in the pattern. The collision term stops discriminating,
        // the score collapses to the desired/current/side terms, and the winner is always the
        // path-follow anchor — i.e. the sampler's considered choice becomes "drive exactly into the
        // neighbour I am meant to be avoiding". Two agents spawned co-located start inside this blind
        // zone and never leave it under their own power.
        //
        // The quadratic still has real roots while overlapping (tmin < 0 < tmax), and tmin DOES vary
        // with the candidate: one that separates fast enters deeply negative, one that drives deeper
        // sits near zero. So mirror dtCrowd's overlap branch ("avoid more when overlapped") and
        // return -tmin*0.5 — separating candidates score LARGER, hence a LOWER penalty, and the
        // sampler steers out of the overlap instead of ignoring it.
        auto TimeToCollision(
            const FVector& InCandidateVel,
            const FVector& InCurrentSelfVel,
            const FVector& InNeighborRelPos,
            const FVector& InNeighborRelVel,
            float InCombinedRadius,
            float InHorizon) -> float
        {
            const auto Dv = FVector2D(2.0f * (InCandidateVel - InCurrentSelfVel) - InNeighborRelVel);
            const auto P  = FVector2D(-InNeighborRelPos.X, -InNeighborRelPos.Y);
            const auto A = static_cast<float>(FVector2D::DotProduct(Dv, Dv));
            const auto B = static_cast<float>(FVector2D::DotProduct(P, Dv));
            const auto C = static_cast<float>(FVector2D::DotProduct(P, P)) - InCombinedRadius * InCombinedRadius;

            const auto Disc = B * B - A * C;

            if (C < 0.0f)
            {
                // A ~ 0 means this candidate exactly matches the neighbour's velocity: the pair is
                // locked in overlap and it never resolves. That is the worst available choice, so
                // score it 0 — maximum ToI penalty.
                if (A < KINDA_SMALL_NUMBER)
                { return 0.0f; }

                // C < 0 and A > 0 => Disc = B^2 + A*|C| > 0, and this root is always negative.
                const auto TEnter = (-B - FMath::Sqrt(Disc)) / A;
                return -TEnter * 0.5f;
            }

            if (A < KINDA_SMALL_NUMBER || B >= 0.0f) { return FLT_MAX; }

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
            FFragment_CrowdAgent_DesiredVelocity& InDesired) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSampleProc);

        // WORKER THREAD: reconstruct the full typed handle for the gate reads below — reads only;
        // the debug-info attach is region/thread-gated in FCk_Handle.
        const auto SelfAgent = UCk_Utils_CrowdAgent_UE::Cast(ck::MakeHandle(InHandle.Get_Entity(), _TransientEntity));

        // Round-robin gate FIRST — it's a modulo on the frame counter, while ShouldSample walks
        // fragments and gameplay tags. On (Stride-1)/Stride of frames the cheap gate rejects
        // before the expensive one runs. Both are pure predicates, so the order is behavior-neutral.
        if (NOT ck_crowd_agent_avoidance_sample_processor::IsSamplingFrame(SelfAgent))
        { return; }

        // Trigger gate. Off-path leaves the force solver's _Velocity in place (which will then be
        // ramped by AccelClamp downstream).
        if (NOT ck_crowd_agent_avoidance_sample_processor::ShouldSample(SelfAgent, InNeighborCache))
        { return; }

        // Counted only past both gates — shows how many agents the round-robin stride actually let through.
        INC_DWORD_STAT(STAT_CkCrowd_AgentsSampled);

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
        const auto VelBias     = Settings->Get_AvoidanceVelBias();
        const auto InvVMax     = (MaxSpeed > KINDA_SMALL_NUMBER) ? 1.0f / MaxSpeed : 0.0f;
        const auto InvHorizon  = (Horizon  > KINDA_SMALL_NUMBER) ? 1.0f / Horizon  : 0.0f;

        // Cache the path-follow desired velocity (what the force solver wrote) BEFORE we overwrite.
        const auto DesiredVel = InDesired.Get_Velocity();
        // Current velocity = last frame's _LastVelocity (set by AccelClamp). Falls back to current
        // _Velocity for the first frame an agent samples (LastVelocity may still be zero).
        const auto CurrentVel = InDesired.Get_LastVelocity().IsNearlyZero()
            ? DesiredVel
            : InDesired.Get_LastVelocity();

        const auto Samples = ck_crowd_agent_avoidance_sample_processor::BuildSamplePattern(DesiredVel, MaxSpeed, VelBias, AngularDivs, Rings);

        // Motion direction for side preference. Falls back to desired direction if current is
        // near-zero (e.g. agent at start of motion).
        const auto MotionDir = CurrentVel.IsNearlyZero()
            ? (DesiredVel.IsNearlyZero() ? FVector::ForwardVector : DesiredVel.GetSafeNormal())
            : CurrentVel.GetSafeNormal();

        // Score each candidate, track best.
        auto BestPenalty = TNumericLimits<float>::Max();
        auto BestVel     = DesiredVel;

        {
            // The only nested-loop processor in the module: Samples × Neighbors per sampling agent.
            SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSample_Scan);

            for (const auto& Cand : Samples)
            {
                // wDesVel: deviation from the path-follow heading (normalised by MaxSpeed).
                const auto DesPen = WDes * static_cast<float>(FVector::Dist2D(Cand, DesiredVel)) * InvVMax;
                // wCurVel: deviation from current velocity — the inertia bias.
                const auto CurPen = WCur * static_cast<float>(FVector::Dist2D(Cand, CurrentVel)) * InvVMax;

                // wToi: minimum time-to-collision across neighbors, FLOORED AT THE HORIZON.
                // Detour initialises tmin to horizTime (DetourObstacleAvoidance.cpp:329) and only
                // ever lowers it, so a collision-free candidate STILL pays wToi / (0.1 + 1) — the
                // ToI term never reaches zero. This port previously special-cased "no predicted
                // collision" to ToiPen = 0, which made stopping collision-free and therefore FREE.
                // That is half of the freeze fixed point: a stationary pair predicts no collision
                // between them, so "stop" paid nothing while every candidate that actually made
                // progress paid the full reciprocal penalty for closing on the neighbour.
                auto TMin = Horizon;
                for (const auto& Nbr : Neighbors)
                {
                    const auto NbrRad = AgentRad + AgentRad;  // approximation: neighbors share radius
                    const auto Ttc = ck_crowd_agent_avoidance_sample_processor::TimeToCollision(Cand, CurrentVel, Nbr.Get_RelativeOffset(), Nbr.Get_RelativeVelocity(), NbrRad, Horizon);
                    if (Ttc < TMin) { TMin = Ttc; }
                }
                constexpr auto ToiPenaltySoftening = 0.1f;  // per dtCrowd (DetourObstacleAvoidance.cpp:417)
                const auto ToiPen = WToi * (1.0f / (ToiPenaltySoftening + TMin * InvHorizon));

                // wSide: penalty for going against the chosen side preference. Normalised by MaxSpeed
                // so it's comparable to the other weighted terms.
                auto SidePen = 0.0f;
                if (SideEnabled)
                {
                    const auto Rightness = ck_crowd_agent_avoidance_sample_processor::SideRightness(Cand, MotionDir) * InvVMax;
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
        }

        InDesired._Velocity = BestVel;
    }
}

// --------------------------------------------------------------------------------------------------------------------
