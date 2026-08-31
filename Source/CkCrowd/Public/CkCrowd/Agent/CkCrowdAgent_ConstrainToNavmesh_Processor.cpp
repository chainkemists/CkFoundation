#include "CkCrowdAgent_ConstrainToNavmesh_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_ConstrainToNavmesh_Algorithm.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_ConstrainToNavmesh);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::ConstrainToNavmesh"), STAT_CkCrowd_ConstrainToNavmeshProc, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Grounding Verifies"), STAT_CkCrowd_GroundingVerifies, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Agents Off Navmesh"), STAT_CkCrowd_AgentsOffNavmesh, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_constrain_to_navmesh
{
    // An off-mesh agent (spawned off-mesh, or a pre-existing excursion) is snapped back if the
    // mesh is within this multiple of its radius horizontally and its normal body-height extent
    // vertically. Widening only the horizontal search self-heals a corner leak without treating a
    // deliberately elevated or deeply displaced agent as ordinary navmesh drift.
    constexpr auto RECOVERY_EXTENT_RADIUS_MULTIPLIER = 4.0f;

    // A stranded agent re-reports on this cadence so a long session's log names it without
    // needing to have caught the transition edge.
    constexpr auto OFF_MESH_REPORT_PERIOD_SECONDS = 30.0f;
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
            FFragment_CrowdAgent_PendingDisplacement& InPending,
            FFragment_CrowdAgent_Grounding& InGrounding)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_ConstrainToNavmeshProc);

        using namespace ck_crowd_agent_constrain_to_navmesh_algorithm;

        InGrounding._SecondsSinceVerified += static_cast<float>(InDeltaT.Get_Seconds());
        const auto SecondsSinceLastPass = InGrounding.Get_SecondsSinceVerified();

        const auto Displacement = InPending.Get_Displacement();
        const auto IsDisplacing = NOT Displacement.IsNearlyZero();
        const auto IsVerifyDue = Get_ShouldVerifyGrounding(
            SecondsSinceLastPass, UCk_Utils_Crowd_Settings_UE::Get_GroundingVerifyIntervalSeconds());

        if (NOT IsDisplacing && NOT IsVerifyDue)
        { return; }

        InPending._Displacement = FVector::ZeroVector;
        InGrounding._SecondsSinceVerified = 0.0f;

        if (NOT IsDisplacing)
        { INC_DWORD_STAT(STAT_CkCrowd_GroundingVerifies); }

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);

        const auto EnqueueOffset = [&](const FVector& InOffset) -> void
        {
            if (InOffset.IsNearlyZero())
            { return; }

            UCk_Utils_Transform_UE::Request_AddLocationOffset(
                SelfTransform,
                FCk_Request_Transform_AddLocationOffset{InOffset}
                    .Set_LocalWorld(ECk_LocalWorld::World), {});
        };

        const auto MarkOnMesh = [&]() -> void
        {
            InGrounding._IsOffNavmesh = false;
            InGrounding._SecondsOffNavmesh = 0.0f;
        };

        if (UCk_Utils_Crowd_Settings_UE::Get_NavmeshConstraintMode() == ECk_CrowdNavmeshConstraintMode::Disabled)
        {
            EnqueueOffset(Displacement);
            return;
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        {
            EnqueueOffset(Displacement);
            return;
        }

        const auto SurfaceHealth = UCk_Utils_NavSurface_UE::Get_ProviderHealth(World);

        // A world without nav data has nothing to constrain against — legitimate absence, not an error.
        if (SurfaceHealth == ECk_NavSurface_ProviderHealth::NoData ||
            SurfaceHealth == ECk_NavSurface_ProviderHealth::Error)
        {
            EnqueueOffset(Displacement);
            return;
        }

        const auto From = InTransform.Get_Transform().GetLocation();

        const auto HorizontalExtent = InParams.Get_Radius();
        const auto VerticalExtent = InParams.Get_Height();
        const auto ProjectionExtent = FVector{HorizontalExtent, HorizontalExtent, VerticalExtent};

        const auto StartProjection = UCk_Utils_NavSurface_UE::Try_ProjectPoint(
            World, FCk_NavSurface_ProjectionQuery{From}.Set_SearchHalfExtents(ProjectionExtent));

        if (StartProjection.Get_Status() != ECk_NavSurface_QueryStatus::Success)
        {
            using namespace ck_crowd_agent_constrain_to_navmesh;

            const auto RecoveryExtent = FVector
            {
                HorizontalExtent * RECOVERY_EXTENT_RADIUS_MULTIPLIER,
                HorizontalExtent * RECOVERY_EXTENT_RADIUS_MULTIPLIER,
                VerticalExtent
            };

            auto RecoveryAccepted = false;
            auto RecoveryRejectedForLift = false;
            auto RecoveryOffset = FVector::ZeroVector;

            const auto RecoveryProjection = UCk_Utils_NavSurface_UE::Try_ProjectPoint(
                World, FCk_NavSurface_ProjectionQuery{From}.Set_SearchHalfExtents(RecoveryExtent));

            if (RecoveryProjection.Get_Status() == ECk_NavSurface_QueryStatus::Success)
            {
                RecoveryOffset = ResolveSurfaceOffset(From, RecoveryProjection.Get_Location());
                RecoveryRejectedForLift = Get_RecoveryExceedsStepUp(
                    static_cast<float>(RecoveryOffset.Z),
                    UCk_Utils_Crowd_Settings_UE::Get_GroundingRecoveryMaxStepUpCm());
                RecoveryAccepted = NOT RecoveryRejectedForLift;
            }

            if (RecoveryAccepted)
            {
                if (RecoveryOffset.Size() > InParams.Get_Radius())
                {
                    ck::crowd::Log(
                        TEXT("CrowdAgent [{}] recovered onto the navmesh at [{}] — correction [{}]uu (dz [{}]uu)"),
                        InHandle, From, RecoveryOffset.Size(), RecoveryOffset.Z);
                }

                MarkOnMesh();
                EnqueueOffset(RecoveryOffset);
                return;
            }

            INC_DWORD_STAT(STAT_CkCrowd_AgentsOffNavmesh);

            const auto IsHolding =
                IsDisplacing &&
                UCk_Utils_Crowd_Settings_UE::Get_OffMeshDisplacementMode() == ECk_CrowdOffMeshDisplacementMode::Hold;

            if (NOT InGrounding.Get_IsOffNavmesh())
            {
                InGrounding._IsOffNavmesh = true;
                InGrounding._SecondsOffNavmesh = 0.0f;

                if (RecoveryRejectedForLift)
                {
                    ck::crowd::Log(
                        TEXT("[RECOVERY-REJECT] CrowdAgent [{}] at [{}]: nearest mesh is [{}]uu ABOVE — refusing to lift beyond step height ({})"),
                        InHandle, From, RecoveryOffset.Z,
                        IsHolding ? TEXT("holding") : TEXT("reported"));
                }
                else if (IsHolding)
                {
                    ck::crowd::Log(
                        TEXT("[GLIDE-HOLD] CrowdAgent [{}] went OFF the navmesh at [{}] — displacement held, not applied in free space"),
                        InHandle, From);
                }
                else
                {
                    ck::crowd::Verbose(
                        TEXT("CrowdAgent [{}] is OFF the navmesh beyond recovery at [{}] — reported, not moved"),
                        InHandle, From);
                }
            }
            else
            {
                const auto PrevReportBucket =
                    FMath::FloorToInt32(InGrounding._SecondsOffNavmesh / OFF_MESH_REPORT_PERIOD_SECONDS);
                InGrounding._SecondsOffNavmesh += SecondsSinceLastPass;
                const auto NewReportBucket =
                    FMath::FloorToInt32(InGrounding._SecondsOffNavmesh / OFF_MESH_REPORT_PERIOD_SECONDS);

                if (NewReportBucket > PrevReportBucket)
                {
                    ck::crowd::Log(
                        TEXT("CrowdAgent [{}] still OFF the navmesh after [{}]s at [{}]"),
                        InHandle, InGrounding.Get_SecondsOffNavmesh(), From);
                }
            }

            if (NOT IsHolding)
            { EnqueueOffset(Displacement); }
            return;
        }

        MarkOnMesh();

        if (NOT IsDisplacing)
        {
            const auto VerticalOffset = ResolveVerticalDriftOffset(
                From, StartProjection.Get_Location(), UCk_Utils_Crowd_Settings_UE::Get_GroundingVerifyMinCorrectionCm());

            if (FMath::Abs(VerticalOffset.Z) > InParams.Get_Radius())
            {
                ck::crowd::Log(
                    TEXT("CrowdAgent [{}] grounding verify corrected dz [{}]uu at [{}]"),
                    InHandle, VerticalOffset.Z, From);
            }

            EnqueueOffset(VerticalOffset);
            return;
        }

        // Walk the requested planar displacement from the projected feet location. The result's Z
        // is the authoritative surface height reached by that walk.
        const auto DesiredTarget = StartProjection.Get_Location() + FVector{Displacement.X, Displacement.Y, 0.0f};

        const auto SurfaceWalk = UCk_Utils_NavSurface_UE::Try_MoveAlongSurface(
            World, FCk_NavSurface_MoveAlongSurfaceQuery{StartProjection.Get_Location(), DesiredTarget});

        if (SurfaceWalk.Get_Status() != ECk_NavSurface_QueryStatus::Success)
        {
            // Valid on-mesh start but the surface walk failed: hold position rather than risk
            // stepping off — dtCrowd's corridor simply doesn't advance in this case either.
            return;
        }

        // Delta against the agent's ACTUAL feet location (not the projected start), so planar
        // drift and vertical drift both fold into this frame's surface correction. Using the
        // integrator's raw Z here lets downward momentum overshoot the surface and eventually
        // escape the projection extent.
        const auto SurfaceOffset = ResolveSurfaceOffset(From, SurfaceWalk.Get_ReachedLocation());

        if (FMath::Abs(SurfaceOffset.Z) > InParams.Get_Radius())
        {
            ck::crowd::Log(
                TEXT("CrowdAgent [{}] surface walk folded dz [{}]uu at [{}]"),
                InHandle, SurfaceOffset.Z, From);
        }

        EnqueueOffset(SurfaceOffset);
    }
}

// --------------------------------------------------------------------------------------------------------------------
