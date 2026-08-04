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

    UPROPERTY(Config, EditDefaultsOnly, Category = "Voxelization",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1,
            ToolTip = "Occupancy probes one volume may spend per tick on a LOCAL repair. Separate from the bake budget because a repair runs during gameplay rather than at load, so what a frame can afford is a different question."))
    int32 _MaxRepairProbesPerTick = 512;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Dynamic Occluders",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "How far an occluder's world bounds must move before it dirties the volumes it overlaps. Sub-threshold drift is ignored: a repair that re-probes hundreds of cells to change no voxel is pure cost, and upstream's tick-time transform comparison used a float epsilon, so any jitter at all rebuilt."))
    float _OccluderMovementThresholdUu = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Chunking",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0,
            ToolTip = "Longest edge one chunk may have before a volume splits along that axis. Upstream Nav3D's figure was 2.5 km, sized for an offline bake; ours is sized for what a budgeted game-thread bake chews through, and for an octree that pads to a power-of-two cube on the volume's LONGEST axis."))
    float _MaxChunkSizeUu = 12800.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Chunking",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1,
            ToolTip = "Hard cap on chunk divisions per axis, so a volume authored far larger than the chunk size yields a bounded number of chunks rather than thousands. At the cap, chunks are simply larger than the chunk size."))
    int32 _MaxChunksPerAxis = 8;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1,
            ToolTip = "Cell expansions one path search may spend. The search runs to completion inside the tick that requested it, so this cap is the only thing bounding a pathological query - a search that reaches it reports IterationCapReached rather than a path."))
    int32 _MaxPathSearchIterations = 200000;

public:
    CK_PROPERTY_GET(_MaxOccupancyProbesPerTick);
    CK_PROPERTY_GET(_MaxRepairProbesPerTick);
    CK_PROPERTY_GET(_OccluderMovementThresholdUu);
    CK_PROPERTY_GET(_MaxBuildMillisecondsPerTick);
    CK_PROPERTY_GET(_MaxPathSearchIterations);
    CK_PROPERTY_GET(_MaxChunkSizeUu);
    CK_PROPERTY_GET(_MaxChunksPerAxis);
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
    static int32 Get_MaxRepairProbesPerTick();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static float Get_OccluderMovementThresholdUu();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static float Get_MaxBuildMillisecondsPerTick();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static int32 Get_MaxPathSearchIterations();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static float Get_MaxChunkSizeUu();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|VoxelNav|Settings")
    static int32 Get_MaxChunksPerAxis();
};

// --------------------------------------------------------------------------------------------------------------------
