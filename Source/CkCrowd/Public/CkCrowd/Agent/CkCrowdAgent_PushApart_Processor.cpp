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

        // Exact center match: a SHARED axis (the xor is symmetric, so both agents pick the same
        // line) with OPPOSITE signs, otherwise coincident agents translate together forever
        // instead of separating.
        auto Get_CoincidentSeparationAxis(uint32 InSelfHash, uint32 InNeighborHash) -> FVector
        {
            constexpr auto AngleBucketCount = 3600;
            constexpr auto RadiansPerAngleBucket = PI / 1800.0f;

            const auto PairAngle = static_cast<float>((InSelfHash ^ InNeighborHash) % AngleBucketCount) * RadiansPerAngleBucket;
            const auto Axis      = FVector{FMath::Cos(PairAngle), FMath::Sin(PairAngle), 0.0f};
            const auto Sign      = InSelfHash > InNeighborHash ? 1.0f : -1.0f;

            return Axis * Sign;
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

        // Overlap below the slop is left alone. COLLISION_RESOLVE_FACTOR removes a FRACTION of the
        // penetration per iteration, so without a floor the correction decays geometrically and
        // never terminates -- a settled pile keeps issuing sub-millimetre pushes, and because each
        // agent resolves against a one-frame-old neighbour cache those pushes chase each other and
        // the formation creeps forever instead of coming to rest.
        const auto SlopCm = FMath::Max(0.0f, UCk_Utils_Crowd_Settings_UE::Get_PushApartSlopCm());

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        const auto SelfRadius = InParams.Get_Radius();
        const auto NeighborRadius = SelfRadius;  // approximation -- neighbors share radius
        const auto CombinedRadius = SelfRadius + NeighborRadius;

        // The resting contact distance, not the geometric one: an overlap the solver has already
        // brought inside the slop is DONE, and re-correcting it is what denies the pile a fixed
        // point.
        const auto ContactDistance = CombinedRadius - SlopCm;

        // Idle is part of the predicate, not decoration: markup confirmation measures PHYSICAL
        // stillness, so a walker pressing against a body it cannot pass confirms too after a few
        // seconds. Without the Idle term that presser reads as a second hard body, the pair falls
        // back to the damped model, and the presser -- which still drives forward every frame --
        // shoves the parked body it was supposed to be stopped by.
        const auto HardBodiesEnabled = UCk_Utils_Crowd_Settings_UE::Get_StationaryHardBodyMode()
            == ECk_CrowdStationaryHardBodyMode::Enabled;
        // GoalBlocked is excluded for the same reason: that agent is stationary by frustration,
        // still wants the goal, and keeps the soft model.
        const auto IsSelfConfirmedStationary = HardBodiesEnabled
            && InHandle.Has<FTag_CrowdAgent_StationaryMarkupConfirmed>()
            && InHandle.Has<FTag_CrowdAgent_Idle>()
            && NOT InHandle.Has<FTag_CrowdAgent_GoalBlocked>();

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

                    const auto IsNeighborConfirmedStationary = HardBodiesEnabled
                        && ck::IsValid(NeighborHandle)
                        && NeighborHandle.Has<FTag_CrowdAgent_StationaryMarkupConfirmed>()
                        && NeighborHandle.Has<FTag_CrowdAgent_Idle>()
                        && NOT NeighborHandle.Has<FTag_CrowdAgent_GoalBlocked>();

                    // EXACTLY one confirmed side makes the pair hard. Two confirmed bodies resting
                    // in contact must keep the damped model below: a symmetric zero yield leaves
                    // any overlap between them permanently unresolvable.
                    if (IsSelfConfirmedStationary != IsNeighborConfirmedStationary)
                    {
                        if (IsSelfConfirmedStationary)
                        { continue; }

                        // Predictive on purpose: folding in the displacement ApplyOffset already
                        // staged this frame cancels the inbound component BEFORE it lands, so the
                        // mover stops AT contact instead of penetrating and being ejected back
                        // out. A post-hoc ejection whose outbound half the navmesh walk eats is
                        // what accumulates penetration frame over frame.
                        auto ToSelf = -Nbr.Get_RelativeOffset() + InPending.Get_Displacement() + Displacement;
                        ToSelf.Z = 0.0f;

                        const auto HardDist = static_cast<float>(ToSelf.Size());
                        if (HardDist >= ContactDistance)
                        { continue; }

                        const auto HardAxis = HardDist < KINDA_SMALL_NUMBER
                            ? ck_crowd_agent_push_apart_processor::Get_CoincidentSeparationAxis(GetTypeHash(InHandle), GetTypeHash(NeighborHandle))
                            : ToSelf / HardDist;

                        // Share 1, factor 1, yield 1: the constraint is exact, so one pass converges.
                        Displacement += HardAxis * (ContactDistance - HardDist);
                        continue;
                    }

                    // Points FROM the neighbor TO already-displaced self, which is the push direction.
                    auto PushFromNeighbor = -Nbr.Get_RelativeOffset() + Displacement;

                    // Must stay planar: a Z delta gets amplified each iteration and shoves agents
                    // through the floor. Z is owned by path-follow and the integrator.
                    PushFromNeighbor.Z = 0.0f;
                    const auto Dist = static_cast<float>(PushFromNeighbor.Size());

                    if (Dist >= ContactDistance)
                    { continue; }

                    if (Dist < KINDA_SMALL_NUMBER)
                    {
                        const auto Axis = ck_crowd_agent_push_apart_processor::Get_CoincidentSeparationAxis(GetTypeHash(InHandle), GetTypeHash(NeighborHandle));
                        Displacement += Axis * (PairShare * CombinedRadius * SelfYield);
                        continue;
                    }

                    // Resolve only the part of the overlap that exceeds the slop, so an agent
                    // arriving at the resting distance is not then pushed past it.
                    const auto Penetration = ContactDistance - Dist;
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
