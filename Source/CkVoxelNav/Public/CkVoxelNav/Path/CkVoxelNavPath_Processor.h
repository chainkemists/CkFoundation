#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Scheduling note: unlike the volume's build processors, nothing here touches Jolt. A search reads only
// the immutable octree the volume already published, so it needs no window around the physics step and
// sits in the ordinary gameplay group with every other request drain.
//
// The search runs to completion INSIDE the drain, bounded by an iteration cap rather than a time slice,
// so a request made on one tick is answered on the next.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVOXELNAV_API FProcessor_VoxelNavPath_HandleRequests : public ck_exp::TProcessor<
        FProcessor_VoxelNavPath_HandleRequests,
        FCk_Handle_VoxelNavPath,
        ck::TReadOnly<FFragment_VoxelNavPath_Params>,
        ck::TReadWrite<FFragment_VoxelNavPath_Result>,
        ck::TReadWrite<FFragment_VoxelNavPath_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FFragment_VoxelNavPath_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_VoxelNavPath_Params& InParams,
            FFragment_VoxelNavPath_Result& InResult,
            FFragment_VoxelNavPath_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_VoxelNavPath_Params& InParams,
            FFragment_VoxelNavPath_Result& InResult,
            const FCk_Request_VoxelNavPath_FindPath& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // A path request queued on an entity that is torn down before the drain reaches it would otherwise
    // leave its caller waiting on a completion nobody is left to report.
    class CKVOXELNAV_API FProcessor_VoxelNavPath_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_VoxelNavPath_CancelPendingRequests,
        FCk_Handle_VoxelNavPath,
        ck::TReadOnly<FFragment_VoxelNavPath_Requests>,
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
            HandleType InPathEntity,
            const FFragment_VoxelNavPath_Requests& InRequests) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
