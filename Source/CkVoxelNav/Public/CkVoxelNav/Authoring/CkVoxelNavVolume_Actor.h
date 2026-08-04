#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment_Data.h"

#include <GameFramework/Info.h>

#include "CkVoxelNavVolume_Actor.generated.h"

class UBoxComponent;

// --------------------------------------------------------------------------------------------------------------------

/** Level-placed source of truth for a VoxelNav volume. The box transform and properties are available in an
 *  editor world, while BeginPlay composes the runtime ECS volume from the exact same values. Script-created
 *  volumes remain supported but have no truthful outside-PIE authoring source. */
UCLASS(Blueprintable, BlueprintType,
    DisplayName = "Ck Voxel Nav Volume",
    HideCategories = (Replication, Physics, Networking, Actor, Rendering, Input, LOD, HLOD, DataLayers,
        Cooking, "Level Instance", Advanced, Tags, ComponentReplication, ComponentTick, Events),
    meta = (DisplayName = "Ck Voxel Nav Volume"))
class CKVOXELNAV_API ACk_VoxelNavVolume_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_VoxelNavVolume_UE);

    ACk_VoxelNavVolume_UE();

protected:
    auto BeginPlay() -> void override;

    auto
    EndPlay(
        EEndPlayReason::Type InEndPlayReason) -> void override;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|VoxelNav|Authoring",
              DisplayName = "[Ck][VoxelNav] Get World Volume Bounds")
    FBox
    Get_WorldVolumeBounds() const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|VoxelNav|Authoring",
              DisplayName = "[Ck][VoxelNav] Build Volume Params")
    FCk_Fragment_VoxelNavVolume_ParamsData
    Build_ParamsData() const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|VoxelNav|Authoring",
              DisplayName = "[Ck][VoxelNav] Get Volume Handle")
    FCk_Handle_VoxelNavVolume
    Get_VolumeHandle() const;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Authoring",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UBoxComponent> _BoundsComponent;

    /** The finest navigable cell's edge length in uu. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Build",
              meta = (AllowPrivateAccess = true, ClampMin = "4.0"))
    float _FinestCellSizeUu = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Build",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _ClearanceUu = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Build",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoBuildOnSetup = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _BuildBudgetOverride = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Performance",
              meta = (AllowPrivateAccess = true, ClampMin = "1",
                      EditCondition = "_BuildBudgetOverride == ECk_EnableDisable::Enable",
                      EditConditionHides))
    int32 _MaxOccupancyProbesPerTickOverride = 2048;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Partitioning",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _ChunkPartitioning = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Partitioning",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MaxChunkSizeOverride = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|VoxelNav|Partitioning",
              meta = (AllowPrivateAccess = true, ClampMin = "1.0",
                      EditCondition = "_MaxChunkSizeOverride == ECk_EnableDisable::Enable",
                      EditConditionHides))
    float _MaxChunkSizeUuOverride = 12800.0f;

    UPROPERTY(Transient)
    FCk_Handle_VoxelNavVolume _VolumeHandle;
};

// --------------------------------------------------------------------------------------------------------------------
