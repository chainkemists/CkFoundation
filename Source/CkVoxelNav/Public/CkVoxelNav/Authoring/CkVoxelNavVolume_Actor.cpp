#include "CkVoxelNavVolume_Actor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Utils.h"

#include <Components/BoxComponent.h>
#include <Engine/CollisionProfile.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_VoxelNavVolume_UE::
    ACk_VoxelNavVolume_UE()
{
    _BoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("VoxelNavBounds"));
    RootComponent = _BoundsComponent;

    _BoundsComponent->SetBoxExtent(FVector{1600.0f});
    _BoundsComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    _BoundsComponent->SetGenerateOverlapEvents(false);
    _BoundsComponent->SetHiddenInGame(true);
    _BoundsComponent->SetMobility(EComponentMobility::Static);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoxelNavVolume_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (NOT HasAuthority())
    { return; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());
    const auto TransientEntityIsValid = ck::IsValid(TransientEntity);

    CK_ENSURE_IF_NOT(TransientEntityIsValid,
        TEXT("VoxelNav volume actor [{}] could not resolve the TransientEntity for the current world"), this)
    { }

    if (NOT TransientEntityIsValid)
    { return; }

    _VolumeHandle = UCk_Utils_VoxelNavVolume_UE::Add(TransientEntity, Build_ParamsData());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoxelNavVolume_UE::
    EndPlay(
        const EEndPlayReason::Type InEndPlayReason)
    -> void
{
    if (ck::IsValid(_VolumeHandle))
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_VolumeHandle);
        _VolumeHandle = {};
    }

    Super::EndPlay(InEndPlayReason);
}

// --------------------------------------------------------------------------------------------------------------------

FBox
    ACk_VoxelNavVolume_UE::
    Get_WorldVolumeBounds() const
{
    if (_BoundsComponent == nullptr)
    { return FBox{ForceInit}; }

    const auto Extent = _BoundsComponent->GetUnscaledBoxExtent();
    return FBox{-Extent, Extent}.TransformBy(_BoundsComponent->GetComponentTransform());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_VoxelNavVolume_ParamsData
    ACk_VoxelNavVolume_UE::
    Build_ParamsData() const
{
    auto Params = FCk_Fragment_VoxelNavVolume_ParamsData{Get_WorldVolumeBounds(), _FinestCellSizeUu};
    Params.Set_ClearanceUu(_ClearanceUu);
    Params.Set_AutoBuildOnSetup(_AutoBuildOnSetup);
    Params.Set_BuildBudgetOverride(_BuildBudgetOverride);
    Params.Set_MaxOccupancyProbesPerTickOverride(_MaxOccupancyProbesPerTickOverride);
    Params.Set_ChunkPartitioning(_ChunkPartitioning);
    Params.Set_MaxChunkSizeOverride(_MaxChunkSizeOverride);
    Params.Set_MaxChunkSizeUuOverride(_MaxChunkSizeUuOverride);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_VoxelNavVolume
    ACk_VoxelNavVolume_UE::
    Get_VolumeHandle() const
{
    return _VolumeHandle;
}

// --------------------------------------------------------------------------------------------------------------------
