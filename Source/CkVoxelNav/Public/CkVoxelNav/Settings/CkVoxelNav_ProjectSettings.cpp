#include "CkVoxelNav_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_MaxOccupancyProbesPerTick()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 2048; }

    return Settings->Get_MaxOccupancyProbesPerTick();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_MaxBuildMillisecondsPerTick()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 4.0f; }

    return Settings->Get_MaxBuildMillisecondsPerTick();
}

// --------------------------------------------------------------------------------------------------------------------
