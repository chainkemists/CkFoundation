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

// Routing output on the follower. _CompiledWaypoints is the ready-to-walk polyline (on-ribbon legs
// sampled along the centerline WITH the side-keeping offset applied, off-path legs verbatim):
// consumers that only need to move follow it, consumers that need semantics read _Legs.
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

    // Monotonic follower-tuning revision that produced this route. Consumers use it
    // with the network epoch to distinguish a same-goal live tuning replan.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _TuningRevision = 0;

public:
    CK_PROPERTY_GET(_Status);
    CK_PROPERTY_GET(_FailReason);
    CK_PROPERTY_GET(_Legs);
    CK_PROPERTY_GET(_CompiledWaypoints);
    CK_PROPERTY_GET(_TotalCost);
    CK_PROPERTY_GET(_GoalLocation);
    CK_PROPERTY_GET(_TuningRevision);
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

    friend class ck::FProcessor_PathNetworkFollower_HandleRequests;
    friend class ::UCk_Utils_PathNetworkFollower_UE;

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

    // Maximum distance trimmed from each side of an eligible corner before inserting a bounded
    // fillet. Zero preserves authored corners without smoothing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _CornerSmoothingDistance = 150.0f;

    // Preferred distance from the already agent-radius-eroded Recast boundary. This is a soft
    // on-ribbon compile-time preference: zero disables it and insufficient room keeps the
    // original valid corridor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _DesiredNavmeshClearance = 75.0f;

    // The network this follower routes on. Assign at Add() time (spawn code holds the network
    // handle) or later via Request_SetNetwork; may also be supplied per-request.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_Handle_PathNetwork _Network;

    // Optional cooperative provenance token for composition owners that must
    // distinguish their follower from a replacement installed by another system
    // on the same entity. This is a namespaced ownership contract, not a security
    // boundary; independent writers must use distinct tokens.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FName _OwnerToken = NAME_None;

    // Runtime-only invalidation identity. It is deliberately not exposed as an
    // authoring value: Request_UpdateTuningAndReplan owns every increment.
    int32 _TuningRevision = 0;

public:
    CK_PROPERTY(_OffPathCostMultiplier);
    CK_PROPERTY(_SideKeepingFraction);
    CK_PROPERTY(_CorridorWaypointSpacing);
    CK_PROPERTY(_CornerSmoothingDistance);
    CK_PROPERTY(_DesiredNavmeshClearance);
    CK_PROPERTY(_Network);
    CK_PROPERTY(_OwnerToken);
    CK_PROPERTY_GET(_TuningRevision);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetworkFollower_Tuning
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetworkFollower_Tuning);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _OffPathCostMultiplier = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0", ClampMax="0.9"))
    float _SideKeepingFraction = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="50.0"))
    float _CorridorWaypointSpacing = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _CornerSmoothingDistance = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _DesiredNavmeshClearance = 75.0f;

public:
    CK_PROPERTY(_OffPathCostMultiplier);
    CK_PROPERTY(_SideKeepingFraction);
    CK_PROPERTY(_CorridorWaypointSpacing);
    CK_PROPERTY(_CornerSmoothingDistance);
    CK_PROPERTY(_DesiredNavmeshClearance);
};

// --------------------------------------------------------------------------------------------------------------------

// Replace the network's ribbons and rebuild the graph — the runtime-rebuild entrypoint. Every
// follower corridor on this network is invalidated (epoch bump) and replans.
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

    int32 _TuningRevision = 0;

public:
    CK_PROPERTY_GET(_GoalLocation);
    CK_PROPERTY(_Network);
    CK_PROPERTY(_TuningRevision);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_PathNetworkFollower_FindRoute, _GoalLocation);
};

// Deferred so tuning changes are serialized with route planning on the follower's entity.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_Request_PathNetworkFollower_UpdateTuning : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_PathNetworkFollower_UpdateTuning);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_PathNetworkFollower_UpdateTuning);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_PathNetworkFollower_Tuning _Tuning;

public:
    CK_PROPERTY_GET(_Tuning);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_PathNetworkFollower_UpdateTuning, _Tuning);
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
