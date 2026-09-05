#include "CkCrowdAgent_Neighbors_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Algorithm.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_NeighborSync);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::NeighborSync"), STAT_CkCrowd_NeighborSyncProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::NeighborSync (map overlaps)"), STAT_CkCrowd_NeighborSync_MapOverlaps, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Overlaps Processed"), STAT_CkCrowd_OverlapsProcessed, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Neighbors Kept"), STAT_CkCrowd_NeighborsKept, STATGROUP_CkCrowd);

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
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_AvoidanceVolumeCache& InAvoidanceVolumeCache) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_NeighborSyncProc);

        InNeighborCache._Neighbors.Reset();
        InAvoidanceVolumeCache._Obstacles.Reset();

        auto ProbeHandle = InProbeRef.Get_ProbeChild();
        if (ck::Is_NOT_Valid(ProbeHandle) || NOT ProbeHandle.Has<FFragment_Probe_Current>())
        { return; }

        // Runs on worker threads — reads only; the handle debug-info attach is thread-gated in FCk_Handle.
        const auto SelfHandle = ck::MakeHandle(InHandle.Get_Entity(), _TransientEntity);

        const auto SelfLoc = InTransform.Get_Transform().GetLocation();

        // Velocity is optional with a zero fallback, so it is deliberately NOT part of the view.
        auto SelfVelocity = UCk_Utils_Velocity_UE::Cast(SelfHandle);
        const auto SelfVel = ck::IsValid(SelfVelocity)
            ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(SelfVelocity)
            : FVector::ZeroVector;

        const auto& Overlaps = ProbeHandle.Get<FFragment_Probe_Current>().Get_CurrentOverlaps();
        if (Overlaps.Num() == 0)
        { return; }

        INC_DWORD_STAT_BY(STAT_CkCrowd_OverlapsProcessed, Overlaps.Num());

        InNeighborCache._Neighbors.Reserve(Overlaps.Num());

        {
            SCOPE_CYCLE_COUNTER(STAT_CkCrowd_NeighborSync_MapOverlaps);

            for (const auto& Overlap : Overlaps)
            {
                // A probe overlaps the OTHER PROBE, never the other agent — hence the lifetime-owner hop.
                auto OtherProbeChild = Overlap.Get_OtherEntity();
                if (ck::Is_NOT_Valid(OtherProbeChild))
                { continue; }

                auto OtherAgent = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(OtherProbeChild);
                if (ck::Is_NOT_Valid(OtherAgent))
                { continue; }

                if (OtherAgent == SelfHandle)
                { continue; }

                auto OtherTransform = UCk_Utils_Transform_UE::Cast(OtherAgent);
                if (ck::Is_NOT_Valid(OtherTransform))
                { continue; }

                if (UCk_Utils_CrowdAvoidanceVolume_UE::Has(OtherAgent))
                {
                    const auto HasRuntimeState =
                        OtherAgent.Has<FTag_CrowdAvoidanceVolume_HasRuntime>() &&
                        OtherAgent.Has<FFragment_CrowdAvoidanceVolume_ProbeRef>();
                    if (NOT HasRuntimeState)
                    { continue; }

                    const auto& Runtime = OtherAgent.Get<FFragment_CrowdAvoidanceVolume_ProbeRef>();
                    const auto& VolumeParams = OtherAgent.Get<FFragment_CrowdAvoidanceVolume_Params>();
                    const auto& Obb = Runtime.Get_AuthoredObb();
                    const auto RuntimeIsValid = ck::IsValid(Runtime.Get_Markup())
                        && Obb.IsFiniteAndPositive();
                    CK_ENSURE_IF_NOT(RuntimeIsValid,
                        TEXT("CrowdAvoidanceVolume [{}] has runtime state but no valid canonical OBB/markup"),
                        OtherAgent)
                    {}
                    if (NOT RuntimeIsValid)
                    { continue; }

                    const auto ExpandedObb = crowd_avoidance_volume::MakeEffectiveAgentObb(
                        Obb, Runtime.Get_PaintedObb(), InParams.Get_Radius());
                    const auto ExpandedObbIsValid = ExpandedObb.IsFiniteAndPositive();
                    CK_ENSURE_IF_NOT(ExpandedObbIsValid,
                        TEXT("CrowdAvoidanceVolume [{}] produced an invalid agent-expanded OBB"),
                        OtherAgent)
                    {}
                    if (NOT ExpandedObbIsValid)
                    { continue; }

                    const auto IsHardExclude = VolumeParams.Get_TraversalPolicy() ==
                        ECk_CrowdAvoidanceVolume_TraversalPolicy::HardExclude;
                    const auto IsVoxelRoute = InHandle.Has<FTag_CrowdAgent_Flying>() ||
                        InPathFollow.Get_ActiveProvider() ==
                            ECk_CrowdAgent_PathProvider::VoxelNav;
                    const auto PolicyAllowsInstalledPath =
                        VolumeParams.Get_TraversalPolicy() ==
                            ECk_CrowdAvoidanceVolume_TraversalPolicy::CostOnly ||
                        (VolumeParams.Get_TraversalPolicy() ==
                            ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible &&
                         InPathFollow.Get_PlanPhase() == ECk_CrowdAgent_PlanPhase::Permissive);
                    const auto HasInstalledTraversableWalk =
                        Runtime.Get_ConfirmedOnMesh() &&
                        InHandle.Has<FTag_CrowdAgent_Walking>() &&
                        NOT InHandle.Has<FTag_CrowdAgent_PathPending>() &&
                        NOT IsVoxelRoute &&
                        PolicyAllowsInstalledPath;

                    // A confirmed route that deliberately pays this volume's cost owns the motion
                    // decision, including while the body is inside the OBB. Every other state keeps
                    // the local wall: async paint/rebuild, idle or pushed-inside escape, an in-flight
                    // replacement path, VoxelNav (which cannot consume Recast area policy), and hard
                    // exclusion.
                    const auto KeepForRebuildOrEscape = IsHardExclude ||
                        NOT HasInstalledTraversableWalk;
                    if (KeepForRebuildOrEscape)
                    {
                        // The cached OBB is canonical: yaw-only scale-one transform plus world extents.
                        // The parallel sampler therefore cannot apply authored scale a second time.
                        InAvoidanceVolumeCache._Obstacles.Emplace(FCk_CrowdAvoidanceVolume_Obstacle{
                            Obb._YawTransform,
                            Obb._WorldHalfExtents});
                    }
                    continue;
                }

                if (NOT UCk_Utils_CrowdAgent_UE::Has(OtherAgent))
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
        }

        InNeighborCache._Neighbors.Sort([](const FCk_CrowdAgent_Neighbor& A, const FCk_CrowdAgent_Neighbor& B)
        {
            return A.Get_Distance() < B.Get_Distance();
        });

        const auto MaxN = FMath::Max(1, InParams.Get_MaxNeighborsForSteering());
        if (InNeighborCache._Neighbors.Num() > MaxN)
        {
            InNeighborCache._Neighbors.SetNum(MaxN, EAllowShrinking::No);
        }

        INC_DWORD_STAT_BY(STAT_CkCrowd_NeighborsKept, InNeighborCache._Neighbors.Num());
    }
}

// --------------------------------------------------------------------------------------------------------------------
