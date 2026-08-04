#include "CkVoxelNavVolume_Processor.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VoxelNavVolume_Setup);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoxelNavVolume_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_VoxelNavVolume_Params& InParams)
        -> void
    {
        InVolumeEntity.Remove<MarkedDirtyBy>();

        voxelnav::Verbose(TEXT("VoxelNav Volume [{}] setup with finest cell size [{}]uu"),
            InVolumeEntity, InParams.Get_FinestCellSizeUu());
    }
}

// --------------------------------------------------------------------------------------------------------------------
