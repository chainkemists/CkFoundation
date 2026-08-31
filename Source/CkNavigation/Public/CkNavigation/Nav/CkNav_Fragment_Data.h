#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkNav_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Nav_Algorithm;
namespace ck
{
    class FProcessor_Nav_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Nav_PathStatus : uint8
{
    None,       // No query issued yet
    Pending,    // Query enqueued, not yet drained
    Ready,      // Valid full path available
    Failed,     // Last query failed or no path found
    Partial     // Destination unreachable; closest reachable point returned
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Nav_PathStatus);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Nav_PathFailReason : uint8
{
    None,                       // No failure recorded (status is Ready/Partial/None)
    NoNavSystem,                // UNavigationSystemV1 not present in this world
    NoNavData,                  // No ARecastNavMesh resolved (auto-create off, or not yet baked)
    NoDefaultFilter,            // NavData has a null DefaultQueryFilter (uninitialized navmesh)
    StartProjectFailed,         // Could not project start location onto the navmesh
    EndProjectFailed,           // Could not project requested target onto the navmesh
    FindPathError,              // ENavigationQueryResult::Error returned by Recast
    FindPathNoPath,             // ENavigationQueryResult::Fail returned (no route between polys)
    FindPathInvalid,            // ENavigationQueryResult::Invalid (degenerate input)
    EmptyPath,                  // Result=Success but Path had zero points (degenerate start≈end)
    NotAuthority,               // Client-side request was dropped (server-only model)
    BudgetExceeded,             // Per-frame budget hit; request will retry next frame
    PendingTimeout              // A provider parked the slot at Pending and never answered
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Nav_PathFailReason);

// --------------------------------------------------------------------------------------------------------------------

// Captured on every FindPathSync invocation, success or failure — the debugger reads these
// instead of the log.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Nav_PathDiagnostics
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Nav_PathDiagnostics);

    friend struct ::FCk_Nav_Algorithm;
    friend class  ck::FProcessor_Nav_HandleRequests;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    ECk_Nav_PathFailReason _LastFailReason = ECk_Nav_PathFailReason::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _LastTargetLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _LastAgentLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _LastProjectedStart = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _LastProjectedEnd = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _StartProjected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _EndProjected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _RawPathPointCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _ExtractedWaypointCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    double _LastQueryWallTime = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    float _LastQueryDurationMs = 0.0f;

public:
    CK_PROPERTY_GET(_LastFailReason);
    CK_PROPERTY_GET(_LastTargetLocation);
    CK_PROPERTY_GET(_LastAgentLocation);
    CK_PROPERTY_GET(_LastProjectedStart);
    CK_PROPERTY_GET(_LastProjectedEnd);
    CK_PROPERTY_GET(_StartProjected);
    CK_PROPERTY_GET(_EndProjected);
    CK_PROPERTY_GET(_RawPathPointCount);
    CK_PROPERTY_GET(_ExtractedWaypointCount);
    CK_PROPERTY_GET(_LastQueryWallTime);
    CK_PROPERTY_GET(_LastQueryDurationMs);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Nav_PathResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Nav_PathResult);

    friend struct ::FCk_Nav_Algorithm;
    friend class  ck::FProcessor_Nav_HandleRequests;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FVector> _Waypoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _DestinationLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    ECk_Nav_PathStatus _Status = ECk_Nav_PathStatus::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FCk_Nav_PathDiagnostics _Diagnostics;

    // Opaque caller-owned identity copied from the request. Navigation does not interpret it;
    // consumers use it to reject a result superseded while this request was deferred.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _RequestRevision = 0;

    // FPlatformTime::Seconds() when this slot was parked at Pending. A provider that never answers
    // has no other bound — CkPathNetwork and CkVoxelNav carry no timeout of their own — so this is
    // what lets a stalled episode be detected and reported instead of waiting forever.
    //
    // MUST NOT be persisted or replicated. It is a PROCESS-relative absolute timestamp: restored
    // into another process (or received from a peer) it reads as ancient and trips the timeout
    // instantly. If this fragment ever gains a persistence or replication handler, this field has
    // to be excluded or rebased, not carried.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    double _PendingSinceSeconds = 0.0;

public:
    CK_PROPERTY_GET(_Waypoints);
    CK_PROPERTY_GET(_DestinationLocation);
    CK_PROPERTY_GET(_Status);
    CK_PROPERTY_GET(_Diagnostics);
    CK_PROPERTY_GET(_RequestRevision);
    CK_PROPERTY_GET(_PendingSinceSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

// Value-only additions to a resolved navigation query filter. The resolver copies the base
// filter before applying these rules, so arbitrary host filter classes keep their own costs,
// flags, and exclusions.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Nav_QueryFilterOverlay
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Nav_QueryFilterOverlay);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    TArray<FGameplayTag> _ExcludedAreaTags;

public:
    CK_PROPERTY(_ExcludedAreaTags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_Nav_FindPath : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Nav_FindPath);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Nav_FindPath);

    friend class ck::FProcessor_Nav_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FVector _TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    bool _AllowPartialPath = true;

    // Maps via UCk_Nav_ProjectSettings_UE::_QueryFilters; empty/unmapped -> NavData's default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTag _QueryFilter;

    // Explicit filter tag for THIS query; outranks _QueryFilter when set. Exists for per-dispatch
    // filter swaps (CkCrowd's strict/permissive planning phases) — a phase is not a project
    // policy, so it cannot live in the settings table.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTag _QueryFilterOverride;

    // Value-only additions applied to a private copy of the resolved base filter for this query.
    // This never mutates the NavData cache shared by other callers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FCk_Nav_QueryFilterOverlay _QueryFilterOverlay;

    // Plan from _StartOverrideLocation instead of the entity's transform.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_EnableDisable _StartOverride = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FVector _StartOverrideLocation = FVector::ZeroVector;

    // Opaque caller-owned identity returned on FCk_Nav_PathResult. Zero preserves callers that
    // do not need stale-result protection.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    int32 _RequestRevision = 0;

public:
    CK_PROPERTY_GET(_TargetLocation);
    CK_PROPERTY(_AllowPartialPath);
    CK_PROPERTY(_QueryFilter);
    CK_PROPERTY(_QueryFilterOverride);
    CK_PROPERTY(_QueryFilterOverlay);
    CK_PROPERTY(_StartOverride);
    CK_PROPERTY(_StartOverrideLocation);
    CK_PROPERTY(_RequestRevision);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Nav_FindPath, _TargetLocation);
};

// --------------------------------------------------------------------------------------------------------------------

// Ends an in-flight path episode. Request_FindPath acquires; this releases. Without a release the
// result slot keeps whatever the acquire parked there — a caller that stops mid-query leaves it
// reading Pending for the entity's whole life, and every Get_PathStatus consumer is told a query
// is still in flight.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_Nav_AbandonPath : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Nav_AbandonPath);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Nav_AbandonPath);

private:
    // The caller's post-abandon revision, stamped onto the result so a query that drains AFTER
    // this abandon is recognised as superseded instead of applied. CkNavigation cannot read the
    // caller's revision counter (CkCrowd depends on CkNavigation, never the reverse), so it
    // arrives as payload.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    int32 _RequestRevision = 0;

public:
    CK_PROPERTY(_RequestRevision);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Nav_AbandonPath, _RequestRevision);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Nav_OnPathReady,
    FCk_Handle,         InHandle,
    FCk_Nav_PathResult, InResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Nav_OnPathFailed,
    FCk_Handle, InHandle);

// --------------------------------------------------------------------------------------------------------------------
