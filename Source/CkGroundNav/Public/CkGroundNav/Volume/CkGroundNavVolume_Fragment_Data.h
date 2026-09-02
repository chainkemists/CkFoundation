#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"

#include "CkGroundNavVolume_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGROUNDNAV_API FCk_Handle_GroundNavVolume : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GroundNavVolume); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GroundNavVolume);

// --------------------------------------------------------------------------------------------------------------------

/**
 * What one ground-nav volume bakes, and how.
 *
 * The authored shape is a world-space box; the tile lattice is derived from it rather than authored
 * beside it, so a volume cannot be configured with an origin and a division count that disagree about
 * the ground they cover.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Fragment_GroundNavVolume_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_GroundNavVolume_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FBox _VolumeBounds = FBox{ForceInit};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_GroundNav_BakeConfig _Config;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_GroundNav_AgentProfile _Profile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_GroundNav_MergeTunables _MergeTunables;

    /** The clearance ceiling, and therefore the halo each tile bakes with. A query for a radius above
     *  this cannot be answered on clearance alone, because every cell that open reads exactly this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MaxClearanceUu = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoBuildOnSetup = ECk_EnableDisable::Enable;

    /** Probes one tick of building may spend. The budget gates whether the next TILE starts, so a slice
     *  can overshoot it by one tile — a tile is never split, which is what keeps the total the same
     *  however the build was sliced.
     *
     *  A probe is one innermost cell or span read (see FCk_GroundNav_BakeStageResult). Measured on the
     *  reference scene — 800uu tiles at 25uu cells, so a 48x48 halo lattice per tile, two layers under
     *  the deck — one tile costs about 92,000 probes, so this default admits roughly sixteen such tiles
     *  per tick. Bigger tiles, finer cells or deeper columns cost more per tile and get fewer of them. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _ProbeBudgetPerTick = 1500000;

public:
    CK_PROPERTY_GET(_VolumeBounds);
    CK_PROPERTY_GET(_Config);
    CK_PROPERTY_GET(_Profile);
    CK_PROPERTY(_MergeTunables);
    CK_PROPERTY(_MaxClearanceUu);
    CK_PROPERTY(_AutoBuildOnSetup);
    CK_PROPERTY(_ProbeBudgetPerTick);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_GroundNavVolume_ParamsData, _VolumeBounds, _Config, _Profile);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavVolume_Build : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavVolume_Build);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavVolume_Build);

private:
    /** Start over even when a build is already running. Left disabled, a request arriving mid-build is
     *  an idempotent no-op: the running build already satisfies the caller's intent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _ForceRestart = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY(_ForceRestart);
};

// --------------------------------------------------------------------------------------------------------------------
