#include "CkVoxelNavVolume_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_VoxelNavVolume_ParamsData& InParams)
    -> FCk_Handle_VoxelNavVolume
{
    const auto OwnerIsValid = ck::IsValid(InOwner);

    CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("Invalid owner Handle [{}] supplied to UCk_Utils_VoxelNavVolume_UE::Add"), InOwner)
    {}

    if (NOT OwnerIsValid)
    { return {}; }

    auto NewVolumeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_VoxelNavVolume>(InOwner);

    NewVolumeEntity.Add<ck::FFragment_VoxelNavVolume_Params>(InParams);
    NewVolumeEntity.Add<ck::FTag_VoxelNavVolume_NeedsBuild>();

    ck::voxelnav::Verbose(TEXT("VoxelNav Volume added to [{}] -> [{}]"), InOwner, NewVolumeEntity);

    return NewVolumeEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavVolume_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_VoxelNavVolume_Params>();
}

// --------------------------------------------------------------------------------------------------------------------
