#include "CkCrowdAgent_PushApart_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

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
    namespace
    {
        // Per dtCrowd's COLLISION_RESOLVE_FACTOR (DetourCrowd.cpp:1599). Sub-1 so we don't
        // overshoot the resolution; iterations converge on the right answer.
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
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_PushApartProc);

        const auto Iterations = Get_IterationCount(UCk_Utils_Crowd_Settings_UE::Get_PushApartMode());
        if (Iterations <= 0)
        { return; }

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SelfTransform))
        { return; }

        const auto SelfRadius = InParams.Get_Radius();

        // An IDLE agent has ARRIVED. It has no restoring drive to walk back to where it was, so any
        // displacement it absorbs is permanent — which is how a newcomer pressing into an agent that
        // already reached its goal was body-checking it clean off the spot (an NPC standing at a shelf,
        // evicted by the next customer). Idle agents therefore absorb only a fraction of the shared
        // correction, and the still-walking party — which CAN steer and re-approach — takes the rest.
        //
        // This is a bound on the damage, not the cure. The real fix for a newcomer that presses forever
        // is for it to notice its goal is unreachable and stop pressing; this only limits how far a
        // stander is shoved in the meantime (and when a player barges through a crowd).
        //
        // Deliberately not zero: PushApart must still resolve idle-vs-idle overlap (a cluster settling
        // at a shared goal), which requires both parties to yield something.
        const auto SelfYield = InHandle.Has<FTag_CrowdAgent_Idle>()
            ? InParams.Get_PushApartIdleYield()
            : 1.0f;

        auto Displacement = FVector::ZeroVector;

        {
            // O(Iterations × Neighbors) relaxation — the per-iteration re-scan is the hidden cost.
            SCOPE_CYCLE_COUNTER(STAT_CkCrowd_PushApart);

            for (auto Iter = 0; Iter < Iterations; ++Iter)
            {
                for (const auto& Nbr : Neighbors)
                {
                    // Use the cache's RelativeOffset directly — it's NbrLoc - SelfLoc as of NeighborSync.
                    // After self has displaced by Displacement, the offset from new-self to neighbor is:
                    //   NbrLoc - (SelfLoc + Displacement) = RelOffset - Displacement
                    // We want the vector pointing FROM neighbor TO new-self (the push direction):
                    //   (SelfLoc + Displacement) - NbrLoc = -RelOffset + Displacement
                    auto Diff = -Nbr.Get_RelativeOffset() + Displacement;
                    // Crowd push is planar — agents walk on the navmesh, Z is owned by path-follow
                    // and the integrator. Without this, any Z delta between agents gets amplified
                    // each iteration and shoves capsules through the floor.
                    Diff.Z = 0.0f;
                    const auto Dist = static_cast<float>(Diff.Size());
                    const auto NeighborRadius = SelfRadius;  // approximation — neighbors share radius
                    const auto CombinedRadius = SelfRadius + NeighborRadius;

                    if (Dist >= CombinedRadius)
                    { continue; }

                    if (Dist < KINDA_SMALL_NUMBER)
                    {
                        // Degenerate overlap (exact center match). The old code pushed EVERY
                        // coincident agent along a shared +X, so two stacked agents translated
                        // together forever (the "slide off in +X" bug) instead of separating.
                        // Derive a deterministic axis from the unordered pair — both agents agree
                        // on the LINE — and give each the OPPOSITE sign along it so they move APART.
                        // Once they are a hair apart, the penetration resolution below takes over.
                        const auto SelfHash  = GetTypeHash(static_cast<const FCk_Handle&>(InHandle));
                        const auto NbrHash   = GetTypeHash(Nbr.Get_Handle());
                        // (SelfHash ^ NbrHash) is symmetric → same axis for both; % 3600 * (PI/1800)
                        // maps to [0, 2π) at 0.1° resolution.
                        const auto PairAngle = static_cast<float>((SelfHash ^ NbrHash) % 3600) * (PI / 1800.0f);
                        const auto Axis      = FVector(FMath::Cos(PairAngle), FMath::Sin(PairAngle), 0.0f);
                        const auto Sign      = SelfHash > NbrHash ? 1.0f : -1.0f;
                        Displacement += Axis * (Sign * 0.5f * CombinedRadius * SelfYield);
                        continue;
                    }

                    const auto Penetration = CombinedRadius - Dist;
                    const auto Pen = (Penetration * 0.5f) * COLLISION_RESOLVE_FACTOR * SelfYield / Dist;
                    Displacement += Diff * Pen;
                }
            }
        }

        if (NOT Displacement.IsNearlyZero())
        {
            UCk_Utils_Transform_UE::Request_AddLocationOffset(
                SelfTransform,
                FCk_Request_Transform_AddLocationOffset{Displacement});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
