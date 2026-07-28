#include "CkCrowdAgent_ConstrainToNavmesh_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_ConstrainToNavmesh_Algorithm.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "NavigationSystem.h"
#include "NavigationData.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_ConstrainToNavmesh);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::ConstrainToNavmesh"), STAT_CkCrowd_ConstrainToNavmeshProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_constrain_to_navmesh
{
    // An off-mesh agent (spawned off-mesh, or a pre-existing excursion) is snapped back if the
    // mesh is within this multiple of its radius horizontally and its normal body-height extent
    // vertically. Widening only the horizontal search self-heals a corner leak without treating a
    // deliberately elevated or deeply displaced agent as ordinary navmesh drift.
    constexpr auto RECOVERY_EXTENT_RADIUS_MULTIPLIER = 4.0f;
}

namespace ck
{
    auto
        FProcessor_CrowdAgent_ConstrainToNavmesh::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PendingDisplacement& InPending)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_ConstrainToNavmeshProc);

        const auto Displacement = InPending.Get_Displacement();
        if (Displacement.IsNearlyZero())
        { return; }

        InPending._Displacement = FVector::ZeroVector;

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);

        const auto EnqueueOffset = [&](const FVector& InOffset) -> void
        {
            UCk_Utils_Transform_UE::Request_AddLocationOffset(
                SelfTransform,
                FCk_Request_Transform_AddLocationOffset{InOffset}
                    .Set_LocalWorld(ECk_LocalWorld::World), {});
        };

        if (UCk_Utils_Crowd_Settings_UE::Get_NavmeshConstraintMode() == ECk_CrowdNavmeshConstraintMode::Disabled)
        {
            EnqueueOffset(Displacement);
            return;
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
        {
            EnqueueOffset(Displacement);
            return;
        }

        const auto NavSys = UNavigationSystemV1::GetCurrent(World);
        const auto NavData = ck::IsValid(NavSys, ck::IsValid_Policy_NullptrOnly{})
            ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)
            : static_cast<ANavigationData*>(nullptr);

        // A world without nav data has nothing to constrain against — legitimate absence, not an error.
        if (NOT ck::IsValid(NavData, ck::IsValid_Policy_NullptrOnly{}))
        {
            EnqueueOffset(Displacement);
            return;
        }

        const auto From = InTransform.Get_Transform().GetLocation();
        using ck_crowd_agent_constrain_to_navmesh_algorithm::ResolveSurfaceOffset;

        const auto HorizontalExtent = InParams.Get_Radius();
        const auto VerticalExtent = InParams.Get_Height();
        const auto ProjectionExtent = FVector{HorizontalExtent, HorizontalExtent, VerticalExtent};

        auto StartOnMesh = FNavLocation{};
        if (NOT NavSys->ProjectPointToNavigation(From, StartOnMesh, ProjectionExtent))
        {
            using namespace ck_crowd_agent_constrain_to_navmesh;

            const auto RecoveryExtent = FVector
            {
                HorizontalExtent * RECOVERY_EXTENT_RADIUS_MULTIPLIER,
                HorizontalExtent * RECOVERY_EXTENT_RADIUS_MULTIPLIER,
                VerticalExtent
            };

            auto Recovered = FNavLocation{};
            if (NavSys->ProjectPointToNavigation(From, Recovered, RecoveryExtent))
            {
                EnqueueOffset(ResolveSurfaceOffset(From, Recovered.Location));
                return;
            }

            EnqueueOffset(Displacement);
            return;
        }

        // Walk the requested planar displacement from the projected feet location. The result's Z
        // is the authoritative surface height reached by that walk.
        const auto DesiredTarget = StartOnMesh.Location + FVector{Displacement.X, Displacement.Y, 0.0f};

        auto Constrained = FNavLocation{};
        if (NOT NavData->FindMoveAlongSurface(StartOnMesh, DesiredTarget, Constrained))
        {
            // Valid on-mesh start but the surface walk failed: hold position rather than risk
            // stepping off — dtCrowd's corridor simply doesn't advance in this case either.
            return;
        }

        // Delta against the agent's ACTUAL feet location (not the projected start), so planar
        // drift and vertical drift both fold into this frame's surface correction. Using the
        // integrator's raw Z here lets downward momentum overshoot the surface and eventually
        // escape the projection extent.
        EnqueueOffset(ResolveSurfaceOffset(From, Constrained.Location));
    }
}

// --------------------------------------------------------------------------------------------------------------------
