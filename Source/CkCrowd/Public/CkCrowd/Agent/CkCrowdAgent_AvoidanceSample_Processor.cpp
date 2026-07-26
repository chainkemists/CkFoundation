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
        auto HasAvoidanceTagOnSelfOrLifetimeOwner(const FCk_Handle& InAgent, const FGameplayTag& InTag) -> bool
        {
            if (InAgent.Has<FFragment_CrowdAgent_Params>())
            {
                const auto& Params = InAgent.Get<FFragment_CrowdAgent_Params>();
                if (Params.Get_Tags().HasTagExact(InTag))
                { return true; }
            }

            const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAgent);
            if (ck::Is_NOT_Valid(Owner))
            { return false; }

            if (Owner.Has<FFragment_CrowdAgent_Params>())
            {
                return Owner.Get<FFragment_CrowdAgent_Params>().Get_Tags().HasTagExact(InTag);
            }
            return false;
        }

        auto ShouldSample(
            const FCk_Handle_CrowdAgent& InAgent,
            const FFragment_CrowdAgent_NeighborCache& InCache) -> bool
        {
            if (InAgent.Has<FFragment_CrowdAgent_AvoidancePolicy>())
            {
                switch (InAgent.Get<FFragment_CrowdAgent_AvoidancePolicy>().Get_Policy())
                {
                    case ECk_AvoidancePolicy::ForceOnly:        return false;
                    case ECk_AvoidancePolicy::SamplingAlways:   return true;
                    case ECk_AvoidancePolicy::UseProjectDefault: break;
                }
            }

            if (HasAvoidanceTagOnSelfOrLifetimeOwner(InAgent, TAG_CrowdAvoidance_NeverSample))
            { return false; }

            const auto Trigger = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleTrigger();
            if (Trigger == ECk_AvoidanceSampleTrigger::Disabled)
            { return false; }

            const auto ZoneForcesSampling = HasAvoidanceTagOnSelfOrLifetimeOwner(InAgent, TAG_CrowdAvoidance_AlwaysSample);
            const auto NeighborThreshold  = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleNeighborThreshold();
            const auto EnoughNeighbors    = InCache.Get_Neighbors().Num() >= NeighborThreshold;

            switch (Trigger)
            {
                case ECk_AvoidanceSampleTrigger::NeighborCountOnly:        return EnoughNeighbors;
                case ECk_AvoidanceSampleTrigger::ZoneTagOnly:              return ZoneForcesSampling;
                case ECk_AvoidanceSampleTrigger::NeighborCountAndZoneTag:  return EnoughNeighbors || ZoneForcesSampling;
                default:                                                   return false;
            }
        }

        auto IsSamplingFrame(const FCk_Handle_CrowdAgent& InAgent) -> bool
        {
            const auto Stride = FMath::Max(1, UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleStride());
            const auto FrameIdx = static_cast<int32>(GFrameCounter);
            const auto AgentRoundRobinPhase = static_cast<int32>(GetTypeHash(InAgent));
            return ((FrameIdx + AgentRoundRobinPhase) % Stride) == 0;
        }

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

            const auto DesiredDir = InDesiredVelocity.IsNearlyZero()
                ? FVector::ForwardVector
                : InDesiredVelocity.GetSafeNormal();
            const auto BaseAngle = FMath::Atan2(DesiredDir.Y, DesiredDir.X);

            const auto AngularStep = (2.0f * PI) / static_cast<float>(InAngularDivs);
            for (int32 Ring = 1; Ring <= InRings; ++Ring)
            {
                const auto Radius = (static_cast<float>(Ring) / static_cast<float>(InRings)) * CloudRadius;
                const auto RingStagger = (Ring % 2 == 0) ? 0.5f * AngularStep : 0.0f;
                for (int32 Div = 0; Div < InAngularDivs; ++Div)
                {
                    const auto Angle = BaseAngle + RingStagger + AngularStep * static_cast<float>(Div);
                    const auto Candidate = BiasCentre +
                        FVector{FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0};

                    const auto ExceedsMaxSpeed = Candidate.SizeSquared2D() > FMath::Square(InMaxSpeed + KINDA_SMALL_NUMBER);
                    if (ExceedsMaxSpeed)
                    { continue; }

                    Samples.Emplace(Candidate);
                }
            }

            return Samples;
        }

        // Seconds until first contact between self at InCandidateVel and the neighbor; FLT_MAX when
        // nothing within InHorizon. An already-overlapping pair returns a separation score in the
        // same units instead: candidates that separate faster score larger, hence a lower ToI penalty.
        auto TimeToCollision(
            const FVector& InCandidateVel,
            const FVector& InCurrentSelfVel,
            const FVector& InNeighborRelPos,
            const FVector& InNeighborRelVel,
            float InCombinedRadius,
            float InHorizon) -> float
        {
            // RVO reciprocity (vab = 2*vcand - vel - nei.vel), expanded for a cache that stores
            // neighbor velocity RELATIVE to self.
            const auto RvoRelativeVel = FVector2D(2.0f * (InCandidateVel - InCurrentSelfVel) - InNeighborRelVel);
            const auto NeighborToSelf = FVector2D(-InNeighborRelPos.X, -InNeighborRelPos.Y);
            const auto A = static_cast<float>(FVector2D::DotProduct(RvoRelativeVel, RvoRelativeVel));
            const auto B = static_cast<float>(FVector2D::DotProduct(NeighborToSelf, RvoRelativeVel));
            const auto C = static_cast<float>(FVector2D::DotProduct(NeighborToSelf, NeighborToSelf)) - InCombinedRadius * InCombinedRadius;

            const auto Disc = B * B - A * C;

            const auto AlreadyOverlapping = C < 0.0f;
            if (AlreadyOverlapping)
            {
                const auto CandidateNeverSeparates = A < KINDA_SMALL_NUMBER;
                if (CandidateNeverSeparates)
                { return 0.0f; }

                const auto OverlapEntryTime = (-B - FMath::Sqrt(Disc)) / A;
                return -OverlapEntryTime * 0.5f;
            }

            if (A < KINDA_SMALL_NUMBER || B >= 0.0f) { return FLT_MAX; }

            if (Disc < 0.0f) { return FLT_MAX; }
            const auto T = (-B - FMath::Sqrt(Disc)) / A;
            return T > InHorizon ? FLT_MAX : T;
        }

        auto SideRightness(
            const FVector& InCandidateVel,
            const FVector& InMotionDir) -> float
        {
            const auto MotionLeftPerp = FVector(-InMotionDir.Y, InMotionDir.X, 0.0);
            const auto Leftness = static_cast<float>(FVector::DotProduct(InCandidateVel, MotionLeftPerp));
            return -Leftness;
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

        // Runs on worker threads — reads only; the handle debug-info attach is thread-gated in FCk_Handle.
        const auto SelfAgent = UCk_Utils_CrowdAgent_UE::Cast(ck::MakeHandle(InHandle.Get_Entity(), _TransientEntity));

        if (NOT ck_crowd_agent_avoidance_sample_processor::IsSamplingFrame(SelfAgent))
        { return; }

        if (NOT ck_crowd_agent_avoidance_sample_processor::ShouldSample(SelfAgent, InNeighborCache))
        { return; }

        INC_DWORD_STAT(STAT_CkCrowd_AgentsSampled);

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

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

        const auto DesiredVel = InDesired.Get_Velocity();
        const auto CurrentVel = InDesired.Get_LastVelocity().IsNearlyZero()
            ? DesiredVel
            : InDesired.Get_LastVelocity();

        const auto Samples = ck_crowd_agent_avoidance_sample_processor::BuildSamplePattern(DesiredVel, MaxSpeed, VelBias, AngularDivs, Rings);

        const auto MotionDir = CurrentVel.IsNearlyZero()
            ? (DesiredVel.IsNearlyZero() ? FVector::ForwardVector : DesiredVel.GetSafeNormal())
            : CurrentVel.GetSafeNormal();

        const auto AssumedNeighborRadius = AgentRad;
        const auto CombinedRadius = AgentRad + AssumedNeighborRadius;

        const auto Get_MinTimeToImpact = [&](const FVector& InCandidate) -> float
        {
            // Floored at the horizon so a collision-free candidate still pays the base ToI cost —
            // "Invariants this port must preserve" in CkCrowd/CLAUDE.md.
            auto TMin = Horizon;
            for (const auto& Nbr : Neighbors)
            {
                const auto Ttc = ck_crowd_agent_avoidance_sample_processor::TimeToCollision(
                    InCandidate, CurrentVel, Nbr.Get_RelativeOffset(), Nbr.Get_RelativeVelocity(), CombinedRadius, Horizon);
                if (Ttc < TMin) { TMin = Ttc; }
            }
            return TMin;
        };

        constexpr auto ToiPenaltySoftening = 0.1f;
        const auto Get_CandidatePenalty = [&](const FVector& InCandidate) -> float
        {
            const auto PathDeviationPen = WDes * static_cast<float>(FVector::Dist2D(InCandidate, DesiredVel)) * InvVMax;
            const auto InertiaPen       = WCur * static_cast<float>(FVector::Dist2D(InCandidate, CurrentVel)) * InvVMax;
            const auto ToiPen           = WToi * (1.0f / (ToiPenaltySoftening + Get_MinTimeToImpact(InCandidate) * InvHorizon));

            auto WrongSidePen = 0.0f;
            if (SideEnabled)
            {
                const auto Rightness = ck_crowd_agent_avoidance_sample_processor::SideRightness(InCandidate, MotionDir) * InvVMax;
                WrongSidePen = WSide * FMath::Max(0.0f, Rightness * SideSign);
            }

            return PathDeviationPen + InertiaPen + ToiPen + WrongSidePen;
        };

        auto BestPenalty = TNumericLimits<float>::Max();
        auto BestVel     = DesiredVel;

        {
            SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSample_Scan);

            for (const auto& Cand : Samples)
            {
                const auto Penalty = Get_CandidatePenalty(Cand);
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
