#include "CkVoxelNavOccluder_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Settings/CkVoxelNav_ProjectSettings.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h"
#include "CkVoxelNav/Volume/CkVoxelNavVolume_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavOccluder_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavOccluder_Track);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_occluder_processor
{
    auto
        Get_ExtentsAreUsable(
            const ck::FFragment_VoxelNavOccluder_Params& InParams)
        -> bool
    {
        return InParams.Get_HalfExtentsUu().GetMin() > 0.0 &&
               InParams.Get_MovementThresholdUuOverride() >= 0.0f;
    }

    auto
        Get_MovementThresholdUu(
            const ck::FFragment_VoxelNavOccluder_Params& InParams)
        -> double
    {
        return static_cast<double>(
            InParams.Get_MovementThresholdOverride() == ECk_EnableDisable::Enable
                ? InParams.Get_MovementThresholdUuOverride()
                : UCk_Utils_VoxelNav_Settings_UE::Get_OccluderMovementThresholdUu());
    }

    /** The world AABB of the authored box under the entity's transform. Rotation and scale ride in through
     *  TransformBy, which is why the tracker needs no separate rotation test. */
    auto
        Get_WorldBounds(
            const FTransform& InTransform,
            const ck::FFragment_VoxelNavOccluder_Params& InParams)
        -> FBox
    {
        return FBox::BuildAABB(FVector::ZeroVector, InParams.Get_HalfExtentsUu()).TransformBy(InTransform);
    }

    auto
        Get_BoundsMovedBy(
            const FBox& InPrevious,
            const FBox& InCurrent)
        -> double
    {
        return FMath::Max(
            (InPrevious.Min - InCurrent.Min).GetAbsMax(),
            (InPrevious.Max - InCurrent.Max).GetAbsMax());
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoxelNavOccluder_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InOccluderEntity,
            const FFragment_VoxelNavOccluder_Params& InParams,
            FFragment_VoxelNavOccluder_Current& InCurrent)
        -> void
    {
        using namespace ck_voxelnav_occluder_processor;

        InOccluderEntity.Remove<MarkedDirtyBy>();

        const auto ExtentsAreUsable = Get_ExtentsAreUsable(InParams);

        CK_ENSURE_IF_NOT(ExtentsAreUsable,
            TEXT("VoxelNav Occluder [{}] cannot be tracked. Half-extents [{}] must be positive on every "
                 "axis and the movement threshold override [{}]uu cannot be negative"),
            InOccluderEntity, InParams.Get_HalfExtentsUu(), InParams.Get_MovementThresholdUuOverride())
        {
            // The occluder stays inert rather than emitting dirty regions of a degenerate box, which would cost
            // repairs without ever changing a voxel.
            return;
        }

        const auto TransformHandle = UCk_Utils_Transform_UE::Cast(InOccluderEntity);

        const auto TransformIsComposed = ck::IsValid(TransformHandle);

        CK_ENSURE_IF_NOT(TransformIsComposed,
            TEXT("VoxelNav Occluder [{}] has no Transform feature. The occluder tracks the entity's own "
                 "transform, so compose Transform onto it before the occluder"), InOccluderEntity)
        { return; }

        InCurrent._TrackedBounds = Get_WorldBounds(
            UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle), InParams);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoxelNavOccluder_Track::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InOccluderEntity,
            const FFragment_VoxelNavOccluder_Params& InParams,
            FFragment_VoxelNavOccluder_Current& InCurrent) const
        -> void
    {
        using namespace ck_voxelnav_occluder_processor;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Occluder_Track);

        const auto TransformHandle = UCk_Utils_Transform_UE::Cast(InOccluderEntity);

        // Setup already diagnosed a missing Transform. Repeating that diagnosis every tick would bury the
        // one that mattered.
        if (ck::Is_NOT_Valid(TransformHandle))
        { return; }

        const auto CurrentBounds = Get_WorldBounds(
            UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle), InParams);

        if (InCurrent._TrackedBounds.IsValid == 0)
        {
            InCurrent._TrackedBounds = CurrentBounds;
            return;
        }

        if (Get_BoundsMovedBy(InCurrent._TrackedBounds, CurrentBounds) < Get_MovementThresholdUu(InParams))
        { return; }

        // BOTH halves. The new bounds say where space became blocked; the old ones say where it became free
        // again, and a repair handed only the new bounds leaves the obstacle's departure point occupied
        // forever.
        const auto DirtyBounds = InCurrent._TrackedBounds + CurrentBounds;

        auto ReachedVolumeEntities = TArray<FCk_Entity>{};

        InOccluderEntity.View<
            FFragment_VoxelNavVolume_BuiltOctree,
            FTag_VoxelNavVolume_Built,
            ck::TExclude<FTag_DestroyEntity_Initiate>>().ForEach(
        [&](const auto InVolumeEntity, const FFragment_VoxelNavVolume_BuiltOctree& InBuiltOctree) -> void
        {
            const auto& PublishedOctree = InBuiltOctree.Get_Octree();

            if (NOT PublishedOctree.IsValid())
            { return; }

            // The NAVIGATION bounds, not the authored ones: the octree addresses a power-of-two cube snapped
            // up from what was authored, and cells outside the authored box are still baked and still real.
            if (NOT PublishedOctree->Get_NavigationBounds().Intersect(DirtyBounds))
            { return; }

            ReachedVolumeEntities.Add(InVolumeEntity);
        });

        // Collected first, acted on second: enqueuing a request adds a fragment, and mutating the registry
        // while iterating one of its views is how a view invalidates under you.
        for (const auto& VolumeEntity : ReachedVolumeEntities)
        {
            auto VolumeHandle = InOccluderEntity.Get_ValidHandle(VolumeEntity.Get_ID());
            auto Volume = UCk_Utils_VoxelNavVolume_UE::CastChecked(VolumeHandle);

            UCk_Utils_VoxelNavVolume_UE::Request_MarkDirty(
                Volume, FCk_Request_VoxelNavVolume_MarkDirty{DirtyBounds}, {});
        }

        InCurrent._TrackedBounds = CurrentBounds;
        ++InCurrent._TimesDirtied;

        voxelnav::VeryVerbose(TEXT("VoxelNav Occluder [{}] moved and dirtied [{}]"),
            InOccluderEntity, DirtyBounds);
    }
}

// --------------------------------------------------------------------------------------------------------------------
