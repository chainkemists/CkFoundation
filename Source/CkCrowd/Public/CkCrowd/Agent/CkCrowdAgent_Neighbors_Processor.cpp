#include "CkCrowdAgent_Neighbors_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_NeighborSync);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_NeighborSync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_ProbeRef& InProbeRef,
            FFragment_CrowdAgent_NeighborCache& InNeighborCache)
        -> void
    {
        InNeighborCache._Neighbors.Reset();

        auto ProbeHandle = InProbeRef.Get_ProbeChild();
        if (ck::Is_NOT_Valid(ProbeHandle) || NOT ProbeHandle.Has<FFragment_Probe_Current>())
        { return; }

        // Self transform / velocity for delta computation. Agents created by the gym always have
        // both features; if either is missing we skip rather than ensure-spam.
        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SelfTransform))
        { return; }
        const auto SelfLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(SelfTransform);

        auto SelfVelocity = UCk_Utils_Velocity_UE::Cast(InHandle);
        const auto SelfVel = ck::IsValid(SelfVelocity)
            ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(SelfVelocity)
            : FVector::ZeroVector;

        const auto& Overlaps = ProbeHandle.Get<FFragment_Probe_Current>().Get_CurrentOverlaps();
        if (Overlaps.Num() == 0)
        { return; }

        InNeighborCache._Neighbors.Reserve(Overlaps.Num());

        for (const auto& Overlap : Overlaps)
        {
            // The probe's _OtherEntity is the *other probe child*, not the other agent. Walk one
            // lifetime-owner hop to map back to the agent entity. ExcludePendingKill is the default
            // and what we want — neighbors that are mid-destroy aren't useful for steering.
            auto OtherProbeChild = Overlap.Get_OtherEntity();
            if (ck::Is_NOT_Valid(OtherProbeChild))
            { continue; }

            auto OtherAgent = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(OtherProbeChild);
            if (ck::Is_NOT_Valid(OtherAgent))
            { continue; }

            // Defensive: in theory the probe's same-context filter prevents an agent from overlapping
            // its own probe — but with policy Any, the agent's probe could conceivably show up.
            if (OtherAgent == static_cast<const FCk_Handle&>(InHandle))
            { continue; }

            auto OtherTransform = UCk_Utils_Transform_UE::Cast(OtherAgent);
            if (ck::Is_NOT_Valid(OtherTransform))
            { continue; }
            const auto OtherLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(OtherTransform);

            auto OtherVelocity = UCk_Utils_Velocity_UE::Cast(OtherAgent);
            const auto OtherVel = ck::IsValid(OtherVelocity)
                ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(OtherVelocity)
                : FVector::ZeroVector;

            const auto RelativeOffset = OtherLoc - SelfLoc;
            const auto Distance = static_cast<float>(RelativeOffset.Size());
            const auto RelativeVelocity = OtherVel - SelfVel;

            InNeighborCache._Neighbors.Emplace(FCk_CrowdAgent_Neighbor{
                OtherAgent,
                RelativeOffset,
                RelativeVelocity,
                Distance});
        }

        // Sort ascending by distance — the closest neighbors dominate the separation force, and
        // trimming after sort keeps the sort cap meaningful.
        InNeighborCache._Neighbors.Sort([](const FCk_CrowdAgent_Neighbor& A, const FCk_CrowdAgent_Neighbor& B)
        {
            return A.Get_Distance() < B.Get_Distance();
        });

        // Trim to the per-agent perf cap. _MaxNeighborsForSteering is the documented stress-perf knob.
        const auto MaxN = FMath::Max(1, InParams.Get_MaxNeighborsForSteering());
        if (InNeighborCache._Neighbors.Num() > MaxN)
        {
            InNeighborCache._Neighbors.SetNum(MaxN, EAllowShrinking::No);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
