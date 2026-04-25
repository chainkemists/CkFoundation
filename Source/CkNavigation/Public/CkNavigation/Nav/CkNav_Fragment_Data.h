#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkNav_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Forward declarations for friend declarations below — the actual processor classes live in
// namespace ck (per the codebase convention), so the friend lines need the namespace.
struct FCk_Nav_Algorithm;
namespace ck
{
    class FProcessor_Nav_HandleRequests;
    class FProcessor_Nav_CrowdReadVelocity;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKNAVIGATION_API FCk_Handle_NavAgent : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_NavAgent); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_NavAgent);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Nav_AvoidanceQuality : uint8
{
    Low    = 0,   // dtObstacleAvoidanceParams index 0 — far NPCs, ambient crowd
    Medium = 1,   // dtObstacleAvoidanceParams index 1 — mid-range NPCs
    High   = 2,   // dtObstacleAvoidanceParams index 2 — close-combat NPCs (default)
    Best   = 3    // dtObstacleAvoidanceParams index 3 — player / hero units
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Nav_AvoidanceQuality);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Nav_PathStatus : uint8
{
    None,       // No query issued yet
    Pending,    // Query in flight this frame
    Ready,      // Valid full path available
    Failed,     // Last query failed or no path found
    Partial     // Destination unreachable; closest reachable point returned
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Nav_PathStatus);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Nav_AgentParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Nav_AgentParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Radius = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Height = 192.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _MaxSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _MaxAcceleration = 2400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _SeparationWeight = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_Nav_AvoidanceQuality _AvoidanceQuality = ECk_Nav_AvoidanceQuality::High;

public:
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_MaxAcceleration);
    CK_PROPERTY(_SeparationWeight);
    CK_PROPERTY(_AvoidanceQuality);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Nav_AgentParams, _Radius, _Height);
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

public:
    CK_PROPERTY_GET(_Waypoints);
    CK_PROPERTY_GET(_DestinationLocation);
    CK_PROPERTY_GET(_Status);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Nav_CrowdVelocity
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Nav_CrowdVelocity);

    friend struct ::FCk_Nav_Algorithm;
    friend class  ck::FProcessor_Nav_CrowdReadVelocity;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _Velocity = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _DesiredVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Velocity);
    CK_PROPERTY_GET(_DesiredVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_Request_Nav_FindPath : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_Nav_FindPath);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Nav_FindPath);

    friend class FProcessor_Nav_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FVector _TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    bool _AllowPartialPath = true;

    // v1: reserved, not read. Will map to UNavigationQueryFilter subclasses in Phase 2.
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
    FCk_Handle_NavAgent, InHandle,
    FCk_Nav_PathResult,  InResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Nav_OnPathFailed,
    FCk_Handle_NavAgent, InHandle);

// --------------------------------------------------------------------------------------------------------------------
