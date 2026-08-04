#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoxelNavVolume_NeedsBuild);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoxelNavVolume_Params = FCk_Fragment_VoxelNavVolume_ParamsData;
}

// --------------------------------------------------------------------------------------------------------------------
