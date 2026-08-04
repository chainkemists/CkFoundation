#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

#include "CkVoxelNav/Occluder/CkVoxelNavOccluder_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// The tracker replaces upstream's ticking UActorComponent outright. There is no component, no actor and no
// per-frame GetComponentsBoundingBox: the occluder is a fragment on whatever entity already carries the
// transform, and one processor walks every occluder in the world.
//
// It spends NO occupancy probe and touches no octree - it only compares bounds and enqueues a request - so
// it does not need the Jolt window that the bake and the repair do. It sits in FGroup_Transform after the
// transform drain purely so it reads a pose that is settled for the frame rather than one mid-update.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Validates the authored extents and seeds the tracked pose. It deliberately does NOT dirty anything:
    // whatever bake exists already saw the occluder where it is, and a volume with no bake has nothing to
    // repair locally.
    class CKVOXELNAV_API FProcessor_VoxelNavOccluder_Setup : public ck_exp::TProcessor<
        FProcessor_VoxelNavOccluder_Setup,
        FCk_Handle_VoxelNavOccluder,
        ck::TReadOnly<FFragment_VoxelNavOccluder_Params>,
        ck::TReadWrite<FFragment_VoxelNavOccluder_Current>,
        FTag_VoxelNavOccluder_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_VoxelNavOccluder_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InOccluderEntity,
            const FFragment_VoxelNavOccluder_Params& InParams,
            FFragment_VoxelNavOccluder_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // One bounds comparison per occluder per tick. On a real move it hands the union of the old and new
    // bounds to every BUILT volume the union reaches, and to nothing else.
    class CKVOXELNAV_API FProcessor_VoxelNavOccluder_Track : public ck_exp::TProcessor<
        FProcessor_VoxelNavOccluder_Track,
        FCk_Handle_VoxelNavOccluder,
        ck::TReadOnly<FFragment_VoxelNavOccluder_Params>,
        ck::TReadWrite<FFragment_VoxelNavOccluder_Current>,
        TExclude<FTag_VoxelNavOccluder_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InOccluderEntity,
            const FFragment_VoxelNavOccluder_Params& InParams,
            FFragment_VoxelNavOccluder_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
