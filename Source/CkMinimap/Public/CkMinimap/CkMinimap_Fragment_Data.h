#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkMinimap/CkFogOfWar_Fragment_Data.h"
#include "CkMinimap/CkMinimap_WorldBounds.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkMinimap_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Minimap_ProjectionMode : uint8
{
    // Frame is centered on the observer and spans ViewExtent world-cm to each edge (the HUD minimap)
    ObserverCentric,
    // Frame spans the FixedBounds rectangle, always north-up (the fullscreen world map). Immutable
    // post-Add — a world map and a minimap are separate Minimap children on the same owner
    FixedBounds
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Minimap_ProjectionMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Minimap_RotationMode : uint8
{
    // North (+X world) is always screen-up
    NorthLocked,
    // The observer's view yaw is screen-up (the map rotates around the player). Ignored by FixedBounds
    RotateWithObserver
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Minimap_RotationMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Minimap_FrameShape : uint8
{
    Rectangle,
    Circle
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Minimap_FrameShape);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Minimap_EntryEdgeState : uint8
{
    InsideFrame,
    // The POI lies outside the frame and its OffscreenPolicy was ClampToEdge — the frame position is
    // pinned onto the frame boundary (rect border or circle rim)
    ClampedToEdge
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Minimap_EntryEdgeState);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKMINIMAP_API FCk_Handle_Minimap : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Minimap); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Minimap);

// --------------------------------------------------------------------------------------------------------------------

// A self-contained snapshot of one projected POI. Every field is safe to render even if the POI entity was
// destroyed after this entry was computed — only live derefs through _Poi (state tags, display asset) must be
// ck::IsValid-gated by the consumer.
USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Minimap_Entry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Minimap_Entry);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Poi _Poi;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Category;

    // Frame space: center-origin, +X = screen right, +Y = screen DOWN (UMG), visible frame is [-1, 1]².
    // Widgets place a blip at _MapPosition x half-widget-size
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector2D _MapPosition = FVector2D::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Minimap_EntryEdgeState _EdgeState = ECk_Minimap_EntryEdgeState::InsideFrame;

    // World yaw (degrees) of the POI OWNER's transform — rotates directional blips (players, vehicles)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _WorldYawDegrees = 0.0f;

    // 3D distance observer -> POI, cm
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Distance = 0.0f;

    // PoiZ - ObserverZ, cm (above/below indicators, interior floors later)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _ElevationDelta = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

public:
    CK_PROPERTY_GET(_Poi);
    CK_PROPERTY_GET(_Category);
    CK_PROPERTY_GET(_MapPosition);
    CK_PROPERTY_GET(_EdgeState);
    CK_PROPERTY_GET(_WorldYawDegrees);
    CK_PROPERTY_GET(_Distance);
    CK_PROPERTY_GET(_ElevationDelta);
    CK_PROPERTY_GET(_Priority);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Minimap_Entry, _Poi, _Category, _MapPosition, _EdgeState, _WorldYawDegrees, _Distance, _ElevationDelta, _Priority);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Fragment_Minimap_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Minimap_ParamsData);

private:
    // World cm from the frame center to the frame edge (the zoom). Runtime changes go through
    // Request_SetViewExtent — this is the STARTING value
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _ViewExtent = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Minimap_ProjectionMode _ProjectionMode = ECk_Minimap_ProjectionMode::ObserverCentric;

    // Starting value; runtime changes go through Request_SetRotationMode
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Minimap_RotationMode _RotationMode = ECk_Minimap_RotationMode::NorthLocked;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Minimap_FrameShape _FrameShape = ECk_Minimap_FrameShape::Rectangle;

    // REQUIRED valid (half-extents > 0) when _ProjectionMode is FixedBounds; ignored otherwise.
    // Immutable post-Add (destroy + re-Add to change maps)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Minimap_WorldBounds _FixedBounds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _MaxEntries = 64;

    // Optional — an empty query accepts every POI category
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagQuery _CategoryFilter;

    // Seconds between projection updates. 0 = every frame. UNLIKE the compass there is no unthrottled
    // channel: view origin/yaw AND all entry positions go stale together between updates (> 0 trades
    // blip smoothness for cost)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _UpdateInterval = 0.0f;

public:
    CK_PROPERTY_GET(_ViewExtent);
    CK_PROPERTY(_ProjectionMode);
    CK_PROPERTY(_RotationMode);
    CK_PROPERTY(_FrameShape);
    CK_PROPERTY(_FixedBounds);
    CK_PROPERTY(_MaxEntries);
    CK_PROPERTY(_CategoryFilter);
    CK_PROPERTY(_UpdateInterval);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Minimap_ParamsData, _ViewExtent);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_Minimap_SetViewExtent : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Minimap_SetViewExtent);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Minimap_SetViewExtent);

private:
    // World cm from the frame center to the frame edge. Must be > 0 — the request is rejected otherwise
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _ViewExtent = 5000.0f;

public:
    CK_PROPERTY_GET(_ViewExtent);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Minimap_SetViewExtent, _ViewExtent);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_Minimap_SetCategoryFilter : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Minimap_SetCategoryFilter);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Minimap_SetCategoryFilter);

private:
    // Empty query accepts every POI category
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagQuery _CategoryFilter;

public:
    CK_PROPERTY_GET(_CategoryFilter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Minimap_SetCategoryFilter, _CategoryFilter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_Minimap_SetObserver : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Minimap_SetObserver);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Minimap_SetObserver);

private:
    // The entity whose position (Transform) and view (Camera/Transform yaw) drive the projection.
    // Pass an invalid handle to reset to the default observer (the minimap child's LIFETIME OWNER —
    // the entity Add was called on)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Observer;

public:
    CK_PROPERTY_GET(_Observer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Minimap_SetObserver, _Observer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_Minimap_SetRotationMode : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Minimap_SetRotationMode);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Minimap_SetRotationMode);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Minimap_RotationMode _RotationMode = ECk_Minimap_RotationMode::NorthLocked;

public:
    CK_PROPERTY_GET(_RotationMode);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Minimap_SetRotationMode, _RotationMode);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_Minimap_SetFogOfWar : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Minimap_SetFogOfWar);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Minimap_SetFogOfWar);

private:
    // POIs on unexplored fog cells are culled from the projection. Pass an invalid handle to disable
    // fog culling
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_FogOfWar _FogOfWar;

public:
    CK_PROPERTY_GET(_FogOfWar);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Minimap_SetFogOfWar, _FogOfWar);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Minimap_EntryAppeared,
    FCk_Handle_Minimap, InMinimap,
    FCk_Minimap_Entry, InEntry);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Minimap_EntryDisappeared,
    FCk_Handle_Minimap, InMinimap,
    FCk_Handle_Poi, InPoi);

// --------------------------------------------------------------------------------------------------------------------
