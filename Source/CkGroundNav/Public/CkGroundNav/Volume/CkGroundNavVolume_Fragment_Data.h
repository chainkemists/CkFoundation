#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"

#include "CkShapes/CkShapes_Common.h"

#include <GameplayTagContainer.h>

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

USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavVolume_Repair : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavVolume_Repair);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavVolume_Repair);

private:
    /** World-space box whose ground is no longer trustworthy. For a MOVED body this is the UNION of
     *  where it was and where it is: the NEW half closes the ground it arrived on, and the OLD half
     *  reopens the ground it left. A request carrying only the new half leaves the body's old
     *  footprint blocked for as long as the field lives, because nothing else will ever revisit it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FBox _DirtyBounds = FBox{ForceInit};

public:
    CK_PROPERTY_GET(_DirtyBounds);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNavVolume_Repair, _DirtyBounds);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Paints an authored area tag onto the ground a shape covers.
 *
 * The MARKUP ENTITY is the identity, not the shape and not the tag: a second request naming the same
 * entity updates the volume's record in place rather than adding a second one, which is what lets a
 * caller move, retag or disable a volume it already placed. Disabling is a state the record keeps
 * carrying — a disabled markup is not a released one, and only the release request removes anything.
 *
 * What the tag MEANS is not carried here. The volume resolves it through the neutral area-policy
 * registry, so an unregistered tag is rejected at admission rather than baked as a record nothing
 * downstream knows how to apply.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavVolume_AreaMarkup : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavVolume_AreaMarkup);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavVolume_AreaMarkup);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _MarkupEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _WorldTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AreaTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_MarkupEntity);
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_WorldTransform);
    CK_PROPERTY_GET(_AreaTag);

    CK_PROPERTY(_Enable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNavVolume_AreaMarkup,
        _MarkupEntity, _Shape, _WorldTransform, _AreaTag);
};

// --------------------------------------------------------------------------------------------------------------------

/** Drops the record the markup entity owns. Releasing a markup the volume does not hold is an
 *  idempotent no-op: the caller's intent — this volume holds no record for that entity — already
 *  holds afterwards. */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavVolume_ReleaseAreaMarkup : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavVolume_ReleaseAreaMarkup);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavVolume_ReleaseAreaMarkup);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _MarkupEntity;

public:
    CK_PROPERTY_GET(_MarkupEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNavVolume_ReleaseAreaMarkup, _MarkupEntity);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Authors a navigation link between two world points on the volume.
 *
 * The LINK ENTITY is the identity, not the two points it names: a second request naming the same
 * entity updates the volume's record in place rather than adding a second one, which is what lets a
 * caller move, re-price or disable a link it already placed. Disabling is a state the record keeps
 * carrying - a disabled link is not a released one, and only the release request removes anything.
 *
 * The record's id and the epoch it was submitted against are the volume's to assign and not the
 * caller's: an id is handed out monotonically and never reused, and the epoch is the one the field was
 * already published at when the request was admitted.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavVolume_Link : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavVolume_Link);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavVolume_Link);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _LinkEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_GroundNav_LinkRecord _Record;

public:
    CK_PROPERTY_GET(_LinkEntity);

    CK_PROPERTY(_Record);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNavVolume_Link, _LinkEntity, _Record);
};

// --------------------------------------------------------------------------------------------------------------------

/** Drops the record the link entity owns. Releasing a link the volume does not hold is an idempotent
 *  no-op: the caller's intent - this volume holds no record for that entity - already holds
 *  afterwards. */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavVolume_ReleaseLink : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavVolume_ReleaseLink);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavVolume_ReleaseLink);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _LinkEntity;

public:
    CK_PROPERTY_GET(_LinkEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNavVolume_ReleaseLink, _LinkEntity);
};

// --------------------------------------------------------------------------------------------------------------------
