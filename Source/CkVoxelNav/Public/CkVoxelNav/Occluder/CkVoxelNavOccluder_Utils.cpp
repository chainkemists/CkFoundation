#include "CkVoxelNavOccluder_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavOccluder_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VoxelNavOccluder_ParamsData& InParams)
    -> FCk_Handle_VoxelNavOccluder
{
    const auto HandleIsValid = ck::IsValid(InHandle);

    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid Handle [{}] supplied to UCk_Utils_VoxelNavOccluder_UE::Add"), InHandle)
    {}

    if (NOT HandleIsValid)
    { return {}; }

    const auto FeatureIsAbsent = NOT Has(InHandle);

    CK_ENSURE_IF_NOT(FeatureIsAbsent,
        TEXT("Entity [{}] already has the VoxelNavOccluder feature"), InHandle)
    {}

    if (NOT FeatureIsAbsent)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_VoxelNavOccluder_Params>(InParams);
    InHandle.Add<ck::FFragment_VoxelNavOccluder_Current>();
    InHandle.Add<ck::FTag_VoxelNavOccluder_NeedsSetup>();

    ck::voxelnav::Verbose(TEXT("VoxelNav Occluder added to [{}] (half-extents [{}])"),
        InHandle, InParams.Get_HalfExtentsUu());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavOccluder_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_VoxelNavOccluder_Params>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VoxelNavOccluder_UE::
    Get_TrackedBounds(
        const FCk_Handle_VoxelNavOccluder& InOccluder)
    -> FBox
{
    if (ck::Is_NOT_Valid(InOccluder))
    { return FBox{ForceInit}; }

    return InOccluder.Get<ck::FFragment_VoxelNavOccluder_Current>().Get_TrackedBounds();
}

auto
    UCk_Utils_VoxelNavOccluder_UE::
    Get_TimesDirtied(
        const FCk_Handle_VoxelNavOccluder& InOccluder)
    -> int32
{
    if (ck::Is_NOT_Valid(InOccluder))
    { return 0; }

    return InOccluder.Get<ck::FFragment_VoxelNavOccluder_Current>().Get_TimesDirtied();
}

// --------------------------------------------------------------------------------------------------------------------
