#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include "CkGroundNavPath_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGROUNDNAV_API FCk_Handle_GroundNavPath : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GroundNavPath); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GroundNavPath);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-agent search tuning. Which field, from where and to where rides the request; these are
 * properties of the body doing the walking and they outlive any one route it asks for.
 *
 * The cost model is carried FIELD BY FIELD rather than as a FCk_GroundNav_PathCostParams, because
 * that type is a plain struct holding a TMap and is not reflected. The three constants below are the
 * whole of it an agent tunes; the per-plate multiplier table is markup the field carries, and an
 * agent has no business authoring one.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Fragment_GroundNavPath_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_GroundNavPath_ParamsData);

private:
    /** Tested against the field's per-cell clearance rather than baked into it, which is what lets one
     *  field serve every agent size. Zero admits every walkable cell. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _AgentRadiusUu = 0.0f;

    // How far above or below each end the field may look for the surface that end stands on.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _VerticalToleranceUu = 100.0f;

    // Penalty per unit of rise over run. Zero is parity with what a navmesh prices, which is nothing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _SlopePenaltyK = 0.0f;

    // Bias away from tight crossings, in cell widths of clearance. Zero is navmesh parity.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _ClearanceBiasK = 0.0f;

    // Inside-corner offset, as a multiple of the agent radius. Zero switches the pass off.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _CornerOffsetK = 1.0f;

    /** Greedy weight. One is admissible; above one trades optimality for expansions, bounded by
     *  (w - 1) on the answered length. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _GreedyWeightW = 1.0f;

    // Expansions one whole search may make, however many slices it takes. Zero means no limit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _MaxExpansions = 0;

    // Crossings the corridor may hold. Zero means no limit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _MaxCorridorLength = 0;

    /** Enabled by default, because the install boundary tells a Partial route from a Ready one and
     *  reacts to it - a search that answers Unreachable with nothing gives it nothing to react to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AllowPartialPath = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_AgentRadiusUu);
    CK_PROPERTY(_VerticalToleranceUu);
    CK_PROPERTY(_SlopePenaltyK);
    CK_PROPERTY(_ClearanceBiasK);
    CK_PROPERTY(_CornerOffsetK);
    CK_PROPERTY(_GreedyWeightW);
    CK_PROPERTY(_MaxExpansions);
    CK_PROPERTY(_MaxCorridorLength);
    CK_PROPERTY(_AllowPartialPath);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_GroundNavPath_ParamsData, _AgentRadiusUu);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Plan a route over whichever published field covers the start.
 *
 * The revision is opaque here: CkGroundNav never interprets it and only copies it onto the result, so
 * a consumer that advanced its episode counter can tell a result it is still waiting for from one
 * belonging to an episode it has already abandoned.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavPath_FindPath : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavPath_FindPath);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavPath_FindPath);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _From = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Goal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _RequestRevision = 0;

    /** A shadow route is searched and published exactly like any other and is never installed by the
     *  consumer that asked for it. The flag rides the answer so that consumer can tell which it got. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _IsShadow = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_From);
    CK_PROPERTY_GET(_Goal);
    CK_PROPERTY(_RequestRevision);
    CK_PROPERTY(_IsShadow);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNavPath_FindPath, _From, _Goal);
};

// --------------------------------------------------------------------------------------------------------------------

// Ends an in-flight ground path episode. Request_FindPath acquires; this releases.
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_Request_GroundNavPath_AbandonPath : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GroundNavPath_AbandonPath);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNavPath_AbandonPath);

private:
    // The episode counter AFTER the abandon, stamped onto the released slot for the same reason
    // FindPath stamps its own: a consumer must be able to date the slot it is reading.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _RequestRevision = 0;

public:
    CK_PROPERTY(_RequestRevision);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * What the agent's last finished episode answered.
 *
 * Reflected so a Blueprint or AngelScript consumer reads one value rather than six accessors, and
 * flattened to plain locations because that is what an install seam takes - the per-waypoint normals,
 * tags and running costs the plan carries are re-derivable from the field, and nothing downstream of
 * the install boundary has asked for them.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNavPath_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GroundNavPath_Result);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNav_PathStatus _Status = ECk_GroundNav_PathStatus::InProgress;

    // Empty unless the status is Ready or Partial.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _Waypoints;

    // Copied verbatim from the request this answers. Never interpreted here.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _RequestRevision = 0;

    // Copied verbatim from the request this answers, for the same reason the revision is.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _IsShadow = ECk_EnableDisable::Disable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    double _LengthUu = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _ExpansionCount = 0;

    /** What the search itself spent, summed over the slices it ran. The ticks it waited behind the
     *  per-frame cap and the ticks it sat parked on unbuilt ground are excluded, so this is comparable
     *  against a provider that answers the same route in one call. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _SearchDurationMs = 0.0f;

    /** The field epoch this plan was made against, as its bare counter: FCk_GroundNav_Epoch is a plain
     *  struct and is not reflected. Staleness is DERIVED by comparing this against the field's current
     *  epoch at the install boundary, and is never stored as a flag. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int64 _PlannedAgainstEpoch = 0;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Waypoints);
    CK_PROPERTY(_RequestRevision);
    CK_PROPERTY(_IsShadow);
    CK_PROPERTY(_LengthUu);
    CK_PROPERTY(_ExpansionCount);
    CK_PROPERTY(_SearchDurationMs);
    CK_PROPERTY(_PlannedAgainstEpoch);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_GroundNavPath_OnPathReady,
    FCk_Handle_GroundNavPath, InPath);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_GroundNavPath_OnPathFailed,
    FCk_Handle_GroundNavPath, InPath,
    ECk_GroundNav_PathStatus, InStatus);

// --------------------------------------------------------------------------------------------------------------------
