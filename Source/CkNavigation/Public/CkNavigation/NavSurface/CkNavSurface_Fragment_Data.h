#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkShapes/CkShapes_Common.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkNavSurface_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_NavSurfaceMarkup_HandleRequests;
    class FProcessor_NavSurface_LinkTraversal_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

// Every provider that can answer navigation-surface queries. Entries are added when the provider
// exists, never in advance.
UENUM(BlueprintType)
enum class ECk_NavSurface_Provider : uint8
{
    Recast,
    GroundNav
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_Provider);

// --------------------------------------------------------------------------------------------------------------------

// Whether a second provider answers alongside the one that installs. The shadowing provider's
// result is compared and discarded; the named provider is still the one whose result installs.
UENUM(BlueprintType)
enum class ECk_NavSurface_ShadowMode : uint8
{
    Off,
    GroundNavShadowsRecast
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_ShadowMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NavSurface_QueryStatus : uint8
{
    Success,
    // Nothing walkable qualified inside the search volume
    NoSurface,
    // The queried region is not built yet — NOT the same as NoSurface
    Unbuilt,
    // Walkable, but the filter or clearance rejected it
    Blocked,
    // No navigation provider resolved for this world
    NoProvider
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_QueryStatus);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NavSurface_ProjectionMode : uint8
{
    Down,
    Up,
    Closest
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_ProjectionMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NavSurface_Reachability : uint8
{
    Reachable,
    Unreachable,
    Unknown_ProviderNotReady
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_Reachability);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NavSurface_ProviderHealth : uint8
{
    Ready,
    Building,
    NoData,
    Error
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_ProviderHealth);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_ProjectionQuery
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_ProjectionQuery);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    // Zero opts into the project-wide projection extent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _SearchHalfExtents = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_ProjectionMode _Mode = ECk_NavSurface_ProjectionMode::Closest;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _QueryFilter;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY(_SearchHalfExtents);
    CK_PROPERTY(_Mode);
    CK_PROPERTY(_QueryFilter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_ProjectionQuery, _Location);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_ProjectionResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_ProjectionResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoProvider;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _SurfaceNormal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _AreaTags;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Location);
    CK_PROPERTY(_SurfaceNormal);
    CK_PROPERTY(_AreaTags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_MoveAlongSurfaceQuery
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_MoveAlongSurfaceQuery);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _End = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _QueryFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Nav_QueryFilterOverlay _QueryFilterOverlay;

public:
    CK_PROPERTY_GET(_Start);
    CK_PROPERTY_GET(_End);
    CK_PROPERTY(_QueryFilter);
    CK_PROPERTY(_QueryFilterOverlay);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_MoveAlongSurfaceQuery, _Start, _End);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_MoveAlongSurfaceResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_MoveAlongSurfaceResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoProvider;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _ReachedLocation = FVector::ZeroVector;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_ReachedLocation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_RaycastQuery
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_RaycastQuery);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _End = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _QueryFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Nav_QueryFilterOverlay _QueryFilterOverlay;

public:
    CK_PROPERTY_GET(_Start);
    CK_PROPERTY_GET(_End);
    CK_PROPERTY(_QueryFilter);
    CK_PROPERTY(_QueryFilterOverlay);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_RaycastQuery, _Start, _End);
};

// --------------------------------------------------------------------------------------------------------------------

// Success means the whole segment is walkable. Blocked means the ray hit a boundary or an excluded
// area, and _HitLocation carries where.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_RaycastResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_RaycastResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoProvider;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _HitLocation = FVector::ZeroVector;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_HitLocation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_BoundaryQuery
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_BoundaryQuery);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Radius = 0.0f;

    // Zero opts into the project-wide projection extent for the centre's poly lookup.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _SearchHalfExtents = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _QueryFilter;

public:
    CK_PROPERTY_GET(_Center);
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY(_SearchHalfExtents);
    CK_PROPERTY(_QueryFilter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_BoundaryQuery, _Center, _Radius);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_BoundarySegment
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_BoundarySegment);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _End = FVector::ZeroVector;

    // Points into walkable space.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _InwardNormal = FVector::ZeroVector;

public:
    CK_PROPERTY(_Start);
    CK_PROPERTY(_End);
    CK_PROPERTY(_InwardNormal);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_BoundaryResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_BoundaryResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoProvider;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_NavSurface_BoundarySegment> _Segments;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Segments);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_ReachabilityQuery
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_ReachabilityQuery);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _End = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _QueryFilter;

public:
    CK_PROPERTY_GET(_Start);
    CK_PROPERTY_GET(_End);
    CK_PROPERTY(_QueryFilter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_ReachabilityQuery, _Start, _End);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_ReachabilityResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_ReachabilityResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_Reachability _Reachability = ECk_NavSurface_Reachability::Unknown_ProviderNotReady;

public:
    CK_PROPERTY(_Reachability);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKNAVIGATION_API FCk_Handle_NavSurfaceMarkup : public FCk_Handle_TypeSafe
{ GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_NavSurfaceMarkup); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_NavSurfaceMarkup);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_NavSurface_AreaMarkup : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_NavSurface_AreaMarkup);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_NavSurface_AreaMarkup);

    friend class ck::FProcessor_NavSurfaceMarkup_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _AreaTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enable = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _WorldTransform = FTransform::Identity;

public:
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_AreaTag);
    CK_PROPERTY(_Enable);
    CK_PROPERTY(_WorldTransform);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_NavSurface_AreaMarkup, _Shape, _AreaTag);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * The world's navigation surface changed. InChangedBounds is the world-space region the change
 * covers, so a listener can test its own interest instead of invalidating everything.
 *
 * An INVALID box (FBox{ForceInit}) means the bounds are unknown and the whole surface must be
 * treated as changed. That is what a provider which reports a revision but never says WHERE it
 * moved delivers, and it is a contract rather than a gap: a listener must handle it.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_NavSurface_OnSurfaceRebuilt,
    FCk_Handle, InWorldEntity,
    FBox,       InChangedBounds);

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

// Which end of a link a body entered from. Neutral on purpose: a provider names its own ends, and a
// consumer driving the crossing only needs to know which way along the link it is going.
UENUM(BlueprintType)
enum class ECk_NavSurface_LinkEntryDirection : uint8
{
    Forward,
    Backward
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_LinkEntryDirection);

// --------------------------------------------------------------------------------------------------------------------

// One link at a time: a traverser is either crossing exactly one link or crossing none.
UENUM(BlueprintType)
enum class ECk_NavSurface_LinkTraversalState : uint8
{
    None,
    Traversing
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_LinkTraversalState);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Begins one crossing of one link, on the entity doing the crossing. Completing a crossing changes no
 * geometry, so no provider is consulted: this is consumer-observable state and nothing else.
 *
 * _CorrelatorId names THIS crossing rather than the link. The same body crosses the same link many
 * times, and only the correlator lets a later Complete say WHICH crossing it is completing. The caller
 * owns its uniqueness - nothing here mints one.
 */
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_NavSurface_BeginLinkTraversal : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_NavSurface_BeginLinkTraversal);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_NavSurface_BeginLinkTraversal);

    friend class ck::FProcessor_NavSurface_LinkTraversal_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _LinkId = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _CorrelatorId = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_LinkEntryDirection _EntryDirection = ECk_NavSurface_LinkEntryDirection::Forward;

public:
    CK_PROPERTY_GET(_LinkId);
    CK_PROPERTY_GET(_CorrelatorId);
    CK_PROPERTY(_EntryDirection);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_NavSurface_BeginLinkTraversal, _LinkId, _CorrelatorId);
};

// --------------------------------------------------------------------------------------------------------------------

// Ends the crossing the correlator names, normally. A correlator that is not the active one is not
// this traverser's crossing to end.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_NavSurface_CompleteLinkTraversal : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_NavSurface_CompleteLinkTraversal);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_NavSurface_CompleteLinkTraversal);

    friend class ck::FProcessor_NavSurface_LinkTraversal_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _CorrelatorId = INDEX_NONE;

public:
    CK_PROPERTY_GET(_CorrelatorId);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_NavSurface_CompleteLinkTraversal, _CorrelatorId);
};

// --------------------------------------------------------------------------------------------------------------------

// Abandons the crossing the correlator names. A correlator that is not the active one has nothing left
// to abandon, which is the caller's intent holding afterwards rather than a rejection.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_NavSurface_CancelLinkTraversal : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_NavSurface_CancelLinkTraversal);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_NavSurface_CancelLinkTraversal);

    friend class ck::FProcessor_NavSurface_LinkTraversal_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _CorrelatorId = INDEX_NONE;

public:
    CK_PROPERTY_GET(_CorrelatorId);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_NavSurface_CancelLinkTraversal, _CorrelatorId);
};

// --------------------------------------------------------------------------------------------------------------------

// What a traverser reads back about itself. A State of None carries no ids: an entity that has never
// begun a crossing and one that has finished its last are the same answer.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavSurface_LinkTraversal
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavSurface_LinkTraversal);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _LinkId = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _CorrelatorId = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_LinkEntryDirection _EntryDirection = ECk_NavSurface_LinkEntryDirection::Forward;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_NavSurface_LinkTraversalState _State = ECk_NavSurface_LinkTraversalState::None;

public:
    CK_PROPERTY(_LinkId);
    CK_PROPERTY(_CorrelatorId);
    CK_PROPERTY(_EntryDirection);
    CK_PROPERTY(_State);
};

// --------------------------------------------------------------------------------------------------------------------

// A crossing started. Fired once per admitted Begin, and never for a Begin that named the correlator
// already running.
DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_NavSurface_OnLinkTraversalBegun,
    FCk_Handle, InTraverser,
    int32,      InLinkId,
    int32,      InCorrelatorId);

// --------------------------------------------------------------------------------------------------------------------

/**
 * A crossing ended. InResult separates the two ways that happens: Succeeded for a Complete, and
 * Failed_Cancelled for a Cancel or a teardown that took the crossing with it. A listener that only
 * stops a ladder animation may ignore it; one that has to decide whether the body ARRIVED cannot.
 */
DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_NavSurface_OnLinkTraversalCompleted,
    FCk_Handle,                  InTraverser,
    int32,                       InLinkId,
    int32,                       InCorrelatorId,
    ECk_Request_OperationResult, InResult);

// --------------------------------------------------------------------------------------------------------------------
