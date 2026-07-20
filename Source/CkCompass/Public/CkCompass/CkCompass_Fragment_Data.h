#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkCompass_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Compass_HeadingSource : uint8
{
    // Camera view rotation when the observer has the Camera feature composed, otherwise the observer's Transform yaw
    Auto,
    CameraView,
    EntityTransform,
    // Heading pushed explicitly via Request_SetManualHeading
    Manual
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Compass_HeadingSource);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Compass_EntryArcState : uint8
{
    InsideArc,
    // The POI's bearing lies outside the compass arc and its OffscreenPolicy was ClampToEdge — the
    // normalized offset is pinned at -1 or +1
    ClampedToEdge
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Compass_EntryArcState);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCOMPASS_API FCk_Handle_Compass : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Compass); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Compass);

// --------------------------------------------------------------------------------------------------------------------

// A self-contained snapshot of one projected POI. Every field is safe to render even if the POI entity was
// destroyed after this entry was computed — only live derefs through _Poi (state tags, display asset) must be
// ck::IsValid-gated by the consumer.
USTRUCT(BlueprintType)
struct CKCOMPASS_API FCk_Compass_Entry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Compass_Entry);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Poi _Poi;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Category;

    // Signed shortest-path delta from the compass heading, degrees, [-180, 180]
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _BearingDegrees = 0.0f;

    // _BearingDegrees / (arc/2), clamped to [-1, 1]
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _NormalizedOffset = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Compass_EntryArcState _ArcState = ECk_Compass_EntryArcState::InsideArc;

    // 3D distance observer -> POI, cm
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Distance = 0.0f;

    // PoiZ - ObserverZ, cm (above/below indicators)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _ElevationDelta = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

public:
    CK_PROPERTY_GET(_Poi);
    CK_PROPERTY_GET(_Category);
    CK_PROPERTY_GET(_BearingDegrees);
    CK_PROPERTY_GET(_NormalizedOffset);
    CK_PROPERTY_GET(_ArcState);
    CK_PROPERTY_GET(_Distance);
    CK_PROPERTY_GET(_ElevationDelta);
    CK_PROPERTY_GET(_Priority);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Compass_Entry, _Poi, _Category, _BearingDegrees, _NormalizedOffset, _ArcState, _Distance, _ElevationDelta, _Priority);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCOMPASS_API FCk_Fragment_Compass_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Compass_ParamsData);

private:
    // Total visible arc, degrees (e.g. 180 shows POIs up to 90 degrees either side of the heading)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0", ClampMax = "360.0"))
    float _ArcDegrees = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _MaxEntries = 32;

    // Optional — an empty query accepts every POI category
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagQuery _CategoryFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Compass_HeadingSource _HeadingSource = ECk_Compass_HeadingSource::Auto;

    // Seconds between projection updates. 0 = every frame. The heading itself is ALWAYS refreshed every
    // frame regardless of this interval (a throttled heading visibly stutters during camera pans).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _UpdateInterval = 0.0f;

public:
    CK_PROPERTY_GET(_ArcDegrees);
    CK_PROPERTY(_MaxEntries);
    CK_PROPERTY(_CategoryFilter);
    CK_PROPERTY(_HeadingSource);
    CK_PROPERTY(_UpdateInterval);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Compass_ParamsData, _ArcDegrees);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCOMPASS_API FCk_Request_Compass_SetCategoryFilter : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Compass_SetCategoryFilter);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Compass_SetCategoryFilter);

private:
    // Empty query accepts every POI category
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagQuery _CategoryFilter;

public:
    CK_PROPERTY_GET(_CategoryFilter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Compass_SetCategoryFilter, _CategoryFilter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCOMPASS_API FCk_Request_Compass_SetManualHeading : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Compass_SetManualHeading);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Compass_SetManualHeading);

private:
    // Absolute world yaw, degrees. Only consumed when the compass' HeadingSource is Manual
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _HeadingDegrees = 0.0f;

public:
    CK_PROPERTY_GET(_HeadingDegrees);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Compass_SetManualHeading, _HeadingDegrees);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCOMPASS_API FCk_Request_Compass_SetObserver : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Compass_SetObserver);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Compass_SetObserver);

private:
    // The entity whose position (Transform) and view (Camera/Transform yaw) drive the projection.
    // Pass an invalid handle to reset to the default observer (the compass' own entity).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Observer;

public:
    CK_PROPERTY_GET(_Observer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Compass_SetObserver, _Observer);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Compass_EntryAppeared,
    FCk_Handle_Compass, InCompass,
    FCk_Compass_Entry, InEntry);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Compass_EntryDisappeared,
    FCk_Handle_Compass, InCompass,
    FCk_Handle_Poi, InPoi);

// --------------------------------------------------------------------------------------------------------------------