#include "CkCrowdAgent_Separation_Processor.h"

#include "CkCrowd/CkCrowd_Stats.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Separation);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::Separation"), STAT_CkCrowd_SeparationProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::Separation (accum)"), STAT_CkCrowd_Separation, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Active Agents"), STAT_CkCrowd_ActiveAgents, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_Separation::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            const FFragment_CrowdAgent_TransientPersonalSpace& InPersonalSpace,
            FFragment_CrowdAgent_SeparationForce& InSeparationForce)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_SeparationProc);

        INC_DWORD_STAT(STAT_CkCrowd_ActiveAgents);

        const auto SeparationRadius = InParams.Get_SeparationRadius() * InPersonalSpace.Get_Scale();
        const auto SeparationWeight = InParams.Get_SeparationWeight();
        const auto MaxSpeed = InParams.Get_MaxSpeed();

        // Zeroing any of these in params is the documented way to opt out of separation.
        if (SeparationRadius <= 0.0f || SeparationWeight <= 0.0f || MaxSpeed <= 0.0f)
        {
            InSeparationForce._Force = FVector::ZeroVector;
            return;
        }

        auto Force = FVector::ZeroVector;

        {
            SCOPE_CYCLE_COUNTER(STAT_CkCrowd_Separation);

            for (const auto& Nbr : InNeighborCache.Get_Neighbors())
            {
                const auto Distance = Nbr.Get_Distance();
                if (Distance >= SeparationRadius)
                { continue; }

                // Must stay planar, and must normalize by the PLANAR distance: an unprojected Z
                // delta joins the push direction, gets amplified by the falloff, and shoves agents
                // through the floor. Z belongs to path-follow and the integrator.
                auto OffsetPlanar = Nbr.Get_RelativeOffset();
                OffsetPlanar.Z = 0.0f;
                const auto SafeDistance = FMath::Max(static_cast<float>(OffsetPlanar.Size()), 0.01f);
                const auto PushAwayFromNeighbor = -OffsetPlanar / SafeDistance;

                const auto NormalizedCloseness = 1.0f - (Distance / SeparationRadius);
                const auto QuadraticFalloff = NormalizedCloseness * NormalizedCloseness;

                Force += PushAwayFromNeighbor * QuadraticFalloff;
            }
        }

        // Returning early keeps far-apart agents from drifting on the tie-breaker jitter alone.
        if (Force.IsNearlyZero())
        {
            InSeparationForce._Force = FVector::ZeroVector;
            return;
        }

        // Two perfectly mirrored agents on a head-on path produce equal-and-opposite forces and
        // freeze; the per-entity phase makes them disagree, deterministically.
        constexpr auto JitterPhasePerIndex = 0.123f;
        constexpr auto JitterFractionOfForce = 0.05f;

        const auto EntityIndex = static_cast<float>(GetTypeHash(InHandle));
        const auto JitterPhase = EntityIndex * JitterPhasePerIndex;
        const auto Jitter = FVector{
            FMath::Sin(JitterPhase),
            FMath::Cos(JitterPhase),
            0.0};

        // SeparationWeight is "fraction of MaxSpeed per full-overlap neighbor", so this lands in
        // cm/s and stacks directly with the path-follow velocity in Steering.
        const auto NewForce = (Force + Jitter * JitterFractionOfForce) * SeparationWeight * MaxSpeed;

        // Blending toward last frame's force kills the frame-to-frame flicker that drove
        // vibration in head-on encounters.
        const auto LastForce = InSeparationForce.Get_Force();
        const auto Inertia = FMath::Clamp(InParams.Get_SeparationInertia(), 0.0f, 1.0f);
        InSeparationForce._Force = FMath::Lerp(NewForce, LastForce, Inertia);
    }
}

// --------------------------------------------------------------------------------------------------------------------
