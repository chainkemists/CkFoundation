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
    Get_MaxRepairProbesPerTick()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 512; }

    return Settings->Get_MaxRepairProbesPerTick();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_OccluderMovementThresholdUu()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 10.0f; }

    return Settings->Get_OccluderMovementThresholdUu();
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

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_MaxPathSearchIterations()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 200000; }

    return Settings->Get_MaxPathSearchIterations();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_MaxChunkSizeUu()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 12800.0f; }

    return Settings->Get_MaxChunkSizeUu();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_MaxChunksPerAxis()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 8; }

    return Settings->Get_MaxChunksPerAxis();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNav_Settings_UE::
    Get_CellMerging()
    -> ECk_EnableDisable
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_VoxelNav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_EnableDisable::Disable; }

    return Settings->Get_CellMerging();
}

// --------------------------------------------------------------------------------------------------------------------
