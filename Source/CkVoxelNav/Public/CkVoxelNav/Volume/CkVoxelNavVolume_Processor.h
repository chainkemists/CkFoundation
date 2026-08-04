#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Scheduling note for the two Jolt-touching processors below: FGroup_Transform, after
// FProcessor_JoltWorld_WaitForAsync and before FProcessor_JoltWorld_Step, is the ONLY window provably
// outside the async physics step — WaitForAsync consumes the previous frame's step and Step kicks the
// next. A processor issuing thousands of occupancy queries per frame anywhere after Step would run them
// concurrently with the task-graph update whenever jolt.EnableAsyncPhysicsUpdate is on.
//
// The build processor deliberately declares NO MarkedDirtyBy: the scheduler then runs it exactly once per
// main tick and never pumps it, which is the whole point of a per-tick budget. Its budget loop is also
// hand-rolled rather than built on ck::RunPacedSteps, because that primitive refills its budget inside
// pump passes whose position relative to FProcessor_JoltWorld_Step is not pinned.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Validates the authored params and, unless the volume opted out, arms the first build.
    class CKVOXELNAV_API FProcessor_VoxelNavVolume_Setup : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_Setup,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Params>,
        FTag_VoxelNavVolume_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_VoxelNavVolume_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVOXELNAV_API FProcessor_VoxelNavVolume_HandleRequests : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_HandleRequests,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Params>,
        ck::TReadWrite<FFragment_VoxelNavVolume_BuildState>,
        ck::TReadWrite<FFragment_VoxelNavVolume_Requests>,
        TExclude<FTag_VoxelNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_VoxelNavVolume_Setup>;
        using MarkedDirtyBy = FFragment_VoxelNavVolume_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            const FCk_Request_VoxelNavVolume_Build& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            const FCk_Request_VoxelNavVolume_CancelBuild& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Resolves the geometry backend and opens a build. Split from the build slice itself because it is the
    // one step that can fail outright, and it must fail before any octree state exists to half-fill.
    class CKVOXELNAV_API FProcessor_VoxelNavVolume_StartBuild : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_StartBuild,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Params>,
        ck::TReadWrite<FFragment_VoxelNavVolume_BuildState>,
        FTag_VoxelNavVolume_NeedsBuild,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;
        using MarkedDirtyBy = FTag_VoxelNavVolume_NeedsBuild;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // One budgeted slice of voxelization per tick, per volume.
    class CKVOXELNAV_API FProcessor_VoxelNavVolume_Build : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_Build,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Params>,
        ck::TReadWrite<FFragment_VoxelNavVolume_BuildState>,
        ck::TReadWrite<FFragment_VoxelNavVolume_BuiltOctree>,
        FTag_VoxelNavVolume_BuildInProgress,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync, FProcessor_VoxelNavVolume_StartBuild>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams,
            FFragment_VoxelNavVolume_BuildState& InBuildState,
            FFragment_VoxelNavVolume_BuiltOctree& InBuiltOctree) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, and a build in flight completes frames
    // after the request that started it. Both leave callers awaiting a completion that no longer has anyone
    // to report it — this fires Failed_Cancelled for the queue AND for the build's in-flight request.
    class CKVOXELNAV_API FProcessor_VoxelNavVolume_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_CancelPendingRequests,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Requests>,
        ck::TReadWrite<FFragment_VoxelNavVolume_BuildState>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Requests& InRequests,
            FFragment_VoxelNavVolume_BuildState& InBuildState) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
