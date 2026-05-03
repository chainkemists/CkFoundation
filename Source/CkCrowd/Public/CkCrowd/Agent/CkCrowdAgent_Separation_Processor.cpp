#include "CkCrowdAgent_Separation_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Separation);

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
            FFragment_CrowdAgent_SeparationForce& InSeparationForce)
        -> void
    {
        const auto SeparationRadius = InParams.Get_SeparationRadius();
        const auto SeparationWeight = InParams.Get_SeparationWeight();
        const auto MaxSpeed = InParams.Get_MaxSpeed();

        // Cheap early-out: zero radius or zero weight disables the system without spending
        // per-neighbor cycles. Setting either to 0 in params is a documented way to opt out.
        if (SeparationRadius <= 0.0f || SeparationWeight <= 0.0f || MaxSpeed <= 0.0f)
        {
            InSeparationForce._Force = FVector::ZeroVector;
            return;
        }

        auto Force = FVector::ZeroVector;

        for (const auto& Nbr : InNeighborCache.Get_Neighbors())
        {
            const auto Distance = Nbr.Get_Distance();
            if (Distance >= SeparationRadius)
            { continue; }

            // Push direction: away from the neighbor. _RelativeOffset is (NbrLoc - SelfLoc),
            // so negate to push self away. Divide by max(D, 0.01) to normalize without a
            // singularity at exact-overlap (two agents at identical positions).
            const auto SafeDistance = FMath::Max(Distance, 0.01f);
            const auto Push = -Nbr.Get_RelativeOffset() / SafeDistance;

            // Quadratic falloff: contribution is full at distance 0, zero at SeparationRadius.
            // Pow-2 makes nearby neighbors dominate, which matches the "everybody breaks the
            // tie by stepping back from whoever's closest" heuristic crowds expect.
            const auto Normalized = 1.0f - (Distance / SeparationRadius);
            const auto Falloff = Normalized * Normalized;

            Force += Push * Falloff;
        }

        // No active force → no output. Skipping the jitter when there are no neighbors keeps
        // far-apart agents from drifting on the deterministic tie-breaker alone.
        if (Force.IsNearlyZero())
        {
            InSeparationForce._Force = FVector::ZeroVector;
            return;
        }

        // Stalemate-breaking jitter: two perfectly mirrored agents on a head-on path produce
        // equal-and-opposite forces and freeze. A tiny deterministic per-agent offset based on
        // the entity's index breaks symmetry. 5% of the weighted force magnitude is below the
        // visual noise floor at typical agent speeds but enough for the steering ramp to pick
        // a consistent side. Keyed off entity index so two distinct agents always disagree on
        // the tie-breaker direction.
        const auto EntityIndex = static_cast<float>(GetTypeHash(InHandle));
        const auto JitterPhase = EntityIndex * 0.123f;
        const auto Jitter = FVector{
            FMath::Sin(JitterPhase),
            FMath::Cos(JitterPhase),
            0.0};

        // Scale to cm/s: SeparationWeight is "fraction of MaxSpeed per full-overlap neighbor",
        // so the dimensionless Force sum × Weight × MaxSpeed lands in cm/s and stacks with the
        // path-follow velocity directly in Steering's combination.
        const auto NewForce = (Force + Jitter * 0.05f) * SeparationWeight * MaxSpeed;

        // Phase 1.1 — inertia lerp toward last frame's force kills frame-to-frame flicker that
        // drove vibration in head-on encounters. Mirrors dtCrowd's weightCurVel penalty concept
        // (DetourObstacleAvoidance.cpp:472), applied here as a force-blend factor since this
        // solver doesn't sample-and-score. 0.5 default ≈ dtCrowd's wCurVel/wDesVel = 0.375 ratio.
        const auto LastForce = InSeparationForce.Get_Force();
        const auto Inertia = FMath::Clamp(InParams.Get_SeparationInertia(), 0.0f, 1.0f);
        InSeparationForce._Force = FMath::Lerp(NewForce, LastForce, Inertia);
    }
}

// --------------------------------------------------------------------------------------------------------------------
