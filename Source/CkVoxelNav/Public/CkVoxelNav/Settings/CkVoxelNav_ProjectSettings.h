#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkVoxelNav_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "VoxelNav"))
class CKVOXELNAV_API UCk_VoxelNav_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VoxelNav_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Voxelization",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1,
            ToolTip = "Occupancy probes one volume may spend per tick. This is the PRIMARY budget: it is deterministic, so a bake costs the same number of ticks on every machine and a test can assert it."))
    int32 _MaxOccupancyProbesPerTick = 2048;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Voxelization",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Wall-clock GUARD on one volume's build slice, in milliseconds. 0 disables it. Checked between stages, so it bounds when a slice stops rather than how long one stage may run - keep the probe budget as the real limit."))
    float _MaxBuildMillisecondsPerTick = 4.0f;

public:
    CK_PROPERTY_GET(_MaxOccupancyProbesPerTick);
    CK_PROPERTY_GET(_MaxBuildMillisecondsPerTick);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKVOXELNAV_API UCk_Utils_VoxelNav_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoxelNav_Settings_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static int32 Get_MaxOccupancyProbesPerTick();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static float Get_MaxBuildMillisecondsPerTick();
};

// --------------------------------------------------------------------------------------------------------------------
