#include "CkCrowdAgent_PushApart_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_PushApart);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::PushApart"), STAT_CkCrowd_PushApartProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::PushApart (relax)"), STAT_CkCrowd_PushApart, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_crowd_agent_push_apart_processor
    {
        // Sub-1 on purpose: the iterations converge rather than overshooting in one step.
        constexpr auto COLLISION_RESOLVE_FACTOR = 0.7f;

        auto Get_IterationCount(ECk_PushApartMode InMode) -> int32
        {
            switch (InMode)
            {
                case ECk_PushApartMode::Disabled: return 0;
                case ECk_PushApartMode::Single:   return 1;
                case ECk_PushApartMode::Standard: return 4;
                default:                          return 4;
            }
        }
    }

    auto
        FProcessor_CrowdAgent_PushApart::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_PendingDisplacement& InPending)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_PushApartProc);

        const auto Iterations = ck_crowd_agent_push_apart_processor::Get_IterationCount(UCk_Utils_Crowd_Settings_UE::Get_PushApartMode());
        if (Iterations <= 0)
        { return; }

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        const auto SelfRadius = InParams.Get_Radius();

        // A terminal failed-goal hold is a physical anchor until an explicit wake. Ordinary Idle
        // and GoalBlocked agents retain their existing reduced-yield behavior.
        const auto IsSelfFailedHeld = InHandle.Has<FTag_CrowdAgent_GoalFailedHold>();
        const auto SelfYield = IsSelfFailedHeld
            ? 0.0f
            : InHandle.Has<FTag_CrowdAgent_Idle>()
                ? InParams.Get_PushApartIdleYield()
                : 1.0f;

        auto Displacement = FVector::ZeroVector;

        {
            SCOPE_CYCLE_COUNTER(STAT_CkCrowd_PushApart);

            for (auto Iter = 0; Iter < Iterations; ++Iter)
            {
                for (const auto& Nbr : Neighbors)
                {
                    const auto& NeighborHandle = Nbr.Get_Handle();
                    const auto IsNeighborFailedHeld = ck::IsValid(NeighborHandle)
                        && NeighborHandle.Has<FTag_CrowdAgent_GoalFailedHold>();
                    // Normally each side owns half of the overlap. A non-held agent opposite a
                    // failed-held anchor owns the whole correction; two held agents both yield
                    // zero and remain stably overlapped until one is explicitly woken.
                    const auto PairShare = IsNeighborFailedHeld && NOT IsSelfFailedHeld
                        ? 1.0f
                        : 0.5f;

                    // Points FROM the neighbor TO already-displaced self, which is the push direction.
                    auto PushFromNeighbor = -Nbr.Get_RelativeOffset() + Displacement;

                    // Must stay planar: a Z delta gets amplified each iteration and shoves agents
                    // through the floor. Z is owned by path-follow and the integrator.
                    PushFromNeighbor.Z = 0.0f;
                    const auto Dist = static_cast<float>(PushFromNeighbor.Size());
                    const auto NeighborRadius = SelfRadius;  // approximation — neighbors share radius
                    const auto CombinedRadius = SelfRadius + NeighborRadius;

                    if (Dist >= CombinedRadius)
                    { continue; }

                    if (Dist < KINDA_SMALL_NUMBER)
                    {
                        // Exact center match: a SHARED axis (the xor is symmetric, so both agents
                        // pick the same line) with OPPOSITE signs, otherwise coincident agents
                        // translate together forever instead of separating.
                        constexpr auto AngleBucketCount = 3600;
                        constexpr auto RadiansPerAngleBucket = PI / 1800.0f;

                        const auto SelfHash  = GetTypeHash(InHandle);
                        const auto NbrHash   = GetTypeHash(Nbr.Get_Handle());
                        const auto PairAngle = static_cast<float>((SelfHash ^ NbrHash) % AngleBucketCount) * RadiansPerAngleBucket;
                        const auto Axis      = FVector(FMath::Cos(PairAngle), FMath::Sin(PairAngle), 0.0f);
                        const auto Sign      = SelfHash > NbrHash ? 1.0f : -1.0f;
                        Displacement += Axis * (Sign * PairShare * CombinedRadius * SelfYield);
                        continue;
                    }

                    const auto Penetration = CombinedRadius - Dist;
                    const auto PenetrationScale = (Penetration * PairShare) * ck_crowd_agent_push_apart_processor::COLLISION_RESOLVE_FACTOR * SelfYield / Dist;
                    Displacement += PushFromNeighbor * PenetrationScale;
                }
            }
        }

        if (NOT Displacement.IsNearlyZero())
        {
            InPending._Displacement += Displacement;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
