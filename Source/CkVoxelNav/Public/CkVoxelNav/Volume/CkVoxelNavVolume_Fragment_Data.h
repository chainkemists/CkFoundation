#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Build.h"

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

    /** Added to every probe half-extent, so it grows obstacles and shrinks free space. Set it to the
     *  agent's radius to bake a volume only that agent's clearance fits through. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _ClearanceUu = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoBuildOnSetup = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _BuildBudgetOverride = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _MaxOccupancyProbesPerTickOverride = 2048;

public:
    CK_PROPERTY_GET(_VolumeBounds);
    CK_PROPERTY_GET(_FinestCellSizeUu);
    CK_PROPERTY(_ClearanceUu);
    CK_PROPERTY(_AutoBuildOnSetup);
    CK_PROPERTY(_BuildBudgetOverride);
    CK_PROPERTY(_MaxOccupancyProbesPerTickOverride);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VoxelNavVolume_ParamsData, _VolumeBounds, _FinestCellSizeUu);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Request_VoxelNavVolume_Build : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoxelNavVolume_Build);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoxelNavVolume_Build);

private:
    /** Restart from scratch even when a build is already running. Left disabled, a request arriving mid-
     *  build is an idempotent no-op: the running build already satisfies the caller's intent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _ForceRestart = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY(_ForceRestart);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Request_VoxelNavVolume_CancelBuild : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoxelNavVolume_CancelBuild);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoxelNavVolume_CancelBuild);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_VoxelNavVolume_OnBuildComplete,
    FCk_Handle_VoxelNavVolume, InVolume,
    ECk_SucceededFailed, InResult,
    FCk_VoxelNav_BuildStats, InStats);

// --------------------------------------------------------------------------------------------------------------------
