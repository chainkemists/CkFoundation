#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkVoxelNavVolume_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVOXELNAV_API FCk_Handle_VoxelNavVolume : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VoxelNavVolume); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VoxelNavVolume);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Fragment_VoxelNavVolume_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VoxelNavVolume_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FBox _VolumeBounds = FBox{ForceInit};

    /** The finest navigable cell's EDGE length in uu — not a half-extent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "4.0"))
    float _FinestCellSizeUu = 50.0f;

public:
    CK_PROPERTY_GET(_VolumeBounds);
    CK_PROPERTY_GET(_FinestCellSizeUu);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VoxelNavVolume_ParamsData, _VolumeBounds, _FinestCellSizeUu);
};

// --------------------------------------------------------------------------------------------------------------------
