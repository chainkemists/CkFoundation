#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVOXELNAV_API FProcessor_VoxelNavVolume_Setup : public ck_exp::TProcessor<
        FProcessor_VoxelNavVolume_Setup,
        FCk_Handle_VoxelNavVolume,
        ck::TReadOnly<FFragment_VoxelNavVolume_Params>,
        FTag_VoxelNavVolume_NeedsBuild,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_VoxelNavVolume_NeedsBuild;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
