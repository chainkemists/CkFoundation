#pragma once

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

// Why the last path query failed. Captured for every failure branch so the debugger
// (and any listener) can pinpoint root cause without needing diagnostic logs.
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
    BudgetExceeded              // Per-frame budget hit; request will retry next frame
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Nav_PathFailReason);

// --------------------------------------------------------------------------------------------------------------------

// Per-query diagnostic context. Captured on every FindPathSync invocation (success or failure)
// so the debugger can show a complete picture of the last attempt without round-tripping
// through logs.
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

public:
    CK_PROPERTY_GET(_Waypoints);
    CK_PROPERTY_GET(_DestinationLocation);
    CK_PROPERTY_GET(_Status);
    CK_PROPERTY_GET(_Diagnostics);
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

    // Maps to a UNavigationQueryFilter class via UCk_Nav_ProjectSettings_UE::_QueryFilters.
    // Empty/unmapped -> NavData's default filter.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTag _QueryFilter;

public:
    CK_PROPERTY_GET(_TargetLocation);
    CK_PROPERTY(_AllowPartialPath);
    CK_PROPERTY(_QueryFilter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Nav_FindPath, _TargetLocation);
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
