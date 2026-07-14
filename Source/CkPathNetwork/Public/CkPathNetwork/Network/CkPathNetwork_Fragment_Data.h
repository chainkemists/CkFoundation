#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

#include "CkPathNetwork_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_PathNetwork_Setup;
    class FProcessor_PathNetwork_HandleRequests;
    class FProcessor_PathNetworkFollower_HandleRequests;
    class FProcessor_PathNetworkFollower_InvalidateOnRebuild;
}

class UCk_Utils_PathNetwork_UE;
class UCk_Utils_PathNetworkFollower_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPATHNETWORK_API FCk_Handle_PathNetwork : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_PathNetwork); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_PathNetwork);

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPATHNETWORK_API FCk_Handle_PathNetworkFollower : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_PathNetworkFollower); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_PathNetworkFollower);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PathNetwork_RouteStatus : uint8
{
    None,       // No route requested yet
    Pending,    // Request enqueued, not yet drained
    Ready,      // Corridor available
    Failed      // Last request failed (see fail reason)
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PathNetwork_RouteStatus);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PathNetwork_RouteFailReason : uint8
{
    None,
    NoNetwork,          // No network handle resolvable (request, corridor, and params all empty/invalid)
    NetworkNotBuilt,    // Network entity exists but its graph is empty (not built yet, or 0 edges)
    NoRouteFound,       // A* exhausted the search space (disconnected overlay)
    NotAuthority,       // Client-side request dropped (server-only model, mirrors CkNavigation)
    BudgetExceeded      // Per-frame budget hit; request retries next frame
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PathNetwork_RouteFailReason);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PathNetwork_CorridorLegType : uint8
{
    OnRibbon,   // Travels along a network edge between two distances-along
    OffPath     // Leaves the network; waypoints come from the navmesh (or straight-line fallback)
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PathNetwork_CorridorLegType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_CorridorLeg
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_CorridorLeg);

    friend class ck::FProcessor_PathNetworkFollower_HandleRequests;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    ECk_PathNetwork_CorridorLegType _LegType = ECk_PathNetwork_CorridorLegType::OffPath;

    // OnRibbon legs only: which edge and the [entry, exit] span traveled (cm along the edge).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _EdgeId = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    float _EntryDistAlong = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    float _ExitDistAlong = 0.0f;

    // OffPath legs only: the resolved waypoints (navmesh path or straight line).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FVector> _Waypoints;

public:
    CK_PROPERTY_GET(_LegType);
    CK_PROPERTY_GET(_EdgeId);
    CK_PROPERTY_GET(_EntryDistAlong);
    CK_PROPERTY_GET(_ExitDistAlong);
    CK_PROPERTY_GET(_Waypoints);
};

// --------------------------------------------------------------------------------------------------------------------

// The routing output stored on the follower entity. _CompiledWaypoints is the ready-to-walk
// polyline: on-ribbon legs sampled along the centerline WITH the side-keeping lateral offset
// already applied, off-path legs appended verbatim. Consumers that just want to move follow
// _CompiledWaypoints; consumers that want semantics read _Legs.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_RouteResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_RouteResult);

    friend class ck::FProcessor_PathNetworkFollower_HandleRequests;
    friend class ck::FProcessor_PathNetworkFollower_InvalidateOnRebuild;
    friend class ::UCk_Utils_PathNetworkFollower_UE;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    ECk_PathNetwork_RouteStatus _Status = ECk_PathNetwork_RouteStatus::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    ECk_PathNetwork_RouteFailReason _FailReason = ECk_PathNetwork_RouteFailReason::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetwork_CorridorLeg> _Legs;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FVector> _CompiledWaypoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    float _TotalCost = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _GoalLocation = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Status);
    CK_PROPERTY_GET(_FailReason);
    CK_PROPERTY_GET(_Legs);
    CK_PROPERTY_GET(_CompiledWaypoints);
    CK_PROPERTY_GET(_TotalCost);
    CK_PROPERTY_GET(_GoalLocation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_Fragment_PathNetwork_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_PathNetwork_ParamsData);

    friend class ck::FProcessor_PathNetwork_Setup;
    friend class ck::FProcessor_PathNetwork_HandleRequests;
    friend class ::UCk_Utils_PathNetwork_UE;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetwork_Ribbon> _Ribbons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_PathNetwork_BuildParams _BuildParams;

public:
    CK_PROPERTY(_Ribbons);
    CK_PROPERTY(_BuildParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_PathNetwork_ParamsData, _Ribbons);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_Fragment_PathNetworkFollower_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_PathNetworkFollower_ParamsData);

private:
    // The shortcut heuristic: off-network travel costs this many times its real distance.
    // Higher = agents cling to the network; 1.0 = the network is ignored (pure shortest path).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _OffPathCostMultiplier = 3.0f;

    // Lateral offset from the centerline as a fraction of the local half-width, signed by travel
    // direction (right-hand walking). Opposing streams pass cleanly because their signs differ.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0", ClampMax="0.9"))
    float _SideKeepingFraction = 0.5f;

    // Spacing of compiled corridor waypoints along on-ribbon legs.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="50.0"))
    float _CorridorWaypointSpacing = 250.0f;

    // The network this follower routes on. Assign at Add() time (spawn code holds the network
    // handle) or later via Request_SetNetwork; may also be supplied per-request.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_Handle_PathNetwork _Network;

public:
    CK_PROPERTY(_OffPathCostMultiplier);
    CK_PROPERTY(_SideKeepingFraction);
    CK_PROPERTY(_CorridorWaypointSpacing);
    CK_PROPERTY(_Network);
};

// --------------------------------------------------------------------------------------------------------------------

// Replace the network's ribbons and rebuild the graph. The runtime-rebuild entrypoint: a game
// re-runs its detector (or edits ribbons procedurally) and submits the new set. Every follower
// corridor on this network is invalidated (epoch bump) and replans.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_Request_PathNetwork_Rebuild : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_PathNetwork_Rebuild);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_PathNetwork_Rebuild);

    friend class ck::FProcessor_PathNetwork_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetwork_Ribbon> _NewRibbons;

public:
    CK_PROPERTY_GET(_NewRibbons);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_PathNetwork_Rebuild, _NewRibbons);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_Request_PathNetworkFollower_FindRoute : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_PathNetworkFollower_FindRoute);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_PathNetworkFollower_FindRoute);

    friend class ck::FProcessor_PathNetworkFollower_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FVector _GoalLocation = FVector::ZeroVector;

    // Optional override; empty -> the follower params' _Network.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_Handle_PathNetwork _Network;

public:
    CK_PROPERTY_GET(_GoalLocation);
    CK_PROPERTY(_Network);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_PathNetworkFollower_FindRoute, _GoalLocation);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_PathNetworkFollower_OnRouteReady,
    FCk_Handle_PathNetworkFollower, InFollower,
    FCk_PathNetwork_RouteResult, InResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_PathNetworkFollower_OnRouteFailed,
    FCk_Handle_PathNetworkFollower, InFollower);

// --------------------------------------------------------------------------------------------------------------------
