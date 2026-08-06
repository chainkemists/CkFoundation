#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkVoxelNav/Volume/CkVoxelNavVolume_Fragment_Data.h"

#include "CkVoxelNavPath_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVOXELNAV_API FCk_Handle_VoxelNavPath : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VoxelNavPath); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VoxelNavPath);

// --------------------------------------------------------------------------------------------------------------------

/** Why a search produced no path. Every value names a distinct cause, because "the path failed" is not
 *  actionable: an unreachable goal asks for a different goal, an endpoint outside the bake asks for a
 *  bigger volume, and an iteration cap asks for a larger budget. */
UENUM(BlueprintType)
enum class ECk_VoxelNav_PathSearchOutcome : uint8
{
    Succeeded,

    // A NaN endpoint, or an iteration cap of zero or less.
    InvalidRequest,

    // The volume has published no octree, so there is no free space to search.
    NoBake,

    // An endpoint lies outside the baked cube, or resolves to no free cell inside it.
    EndpointUnresolvable,

    // Both endpoints resolved, but no sequence of free cells connects them.
    Unreachable,

    // The search spent its whole iteration budget without reaching the goal. A path may still exist.
    IterationCapReached
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoxelNav_PathSearchOutcome);

// --------------------------------------------------------------------------------------------------------------------

/** What the path entity currently holds.
 *
 *  `Stale` is never STORED: it is derived when read, by comparing the volume's current build epoch
 *  against the epoch the path was planned against. Staleness is a property of the volume, not of the
 *  path, so storing it would need every rebuild to walk every path that ever planned against it. */
UENUM(BlueprintType)
enum class ECk_VoxelNav_PathStatus : uint8
{
    // Nothing has been planned yet.
    None,

    Ready,

    Failed,

    // Planned against an older bake of the volume. The waypoints may cross space that is now blocked.
    Stale
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VoxelNav_PathStatus);

// --------------------------------------------------------------------------------------------------------------------

/** Per-agent search tuning. The query itself (which volume, from where, to where) rides the request:
 *  these are properties of the agent doing the flying, and they outlive any one path it asks for. */
USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_VoxelNavPath_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VoxelNavPath_Spec);

private:
    /** The agent's radius in uu. A cell whose half-extent is smaller than this is never traversed, so the
     *  same bake serves agents of different sizes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _AgentRadiusUu = 0.0f;

    /** Multiplies the remaining-distance estimate. 1 is admissible, so A* returns an optimal path; any
     *  value above 1 is a weighted search that reaches the goal sooner and may return a longer path. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _HeuristicScale = 1.0f;

    /** Discounts a step into a COARSE cell in proportion to how much coarser than the finest cell it is,
     *  which biases routes through wide-open space instead of hugging geometry. It makes the cost
     *  inadmissible against the Euclidean heuristic, so paths stop being provably shortest - hence off. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _NodeSizeCompensation = ECk_EnableDisable::Disable;

    /** The volume this agent plans through. Assign at Add() time (spawn code holds the volume handle) or
     *  later via Request_SetVolume. Request_FindPath still takes its own volume, so this binding is for
     *  consumers that plan repeatedly on the agent's behalf and have no volume of their own to pass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoxelNavVolume _Volume;

public:
    CK_PROPERTY(_AgentRadiusUu);
    CK_PROPERTY(_HeuristicScale);
    CK_PROPERTY(_NodeSizeCompensation);
    CK_PROPERTY(_Volume);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VoxelNavPath_Spec, _AgentRadiusUu);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOXELNAV_API FCk_Request_VoxelNavPath_FindPath : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoxelNavPath_FindPath);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoxelNavPath_FindPath);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoxelNavVolume _Volume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _From = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _To = FVector::ZeroVector;

    /** Drops every waypoint the octree proves redundant, turning the search's axis-by-axis staircase into
     *  the straight runs an agent can actually fly. On by default because the raw cell-centre path is a
     *  planning artefact, not a route: nothing that flies one wants its corners. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _VisibilityPruning = ECk_EnableDisable::Enable;

    /** Rounds the corners the pruning left, span by span, keeping a curve only where the octree agrees it
     *  stays in free space. Off by default: it multiplies the waypoint count, and a follower that already
     *  interpolates between waypoints gains nothing from it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Smoothing = ECk_EnableDisable::Disable;

    // Segments each span is divided into when smoothing. One or less introduces no point at all.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _SmoothingSubdivisions = 4;

public:
    CK_PROPERTY_GET(_Volume);
    CK_PROPERTY_GET(_From);
    CK_PROPERTY_GET(_To);
    CK_PROPERTY(_VisibilityPruning);
    CK_PROPERTY(_Smoothing);
    CK_PROPERTY(_SmoothingSubdivisions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoxelNavPath_FindPath, _Volume, _From, _To);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_VoxelNavPath_OnPathReady,
    FCk_Handle_VoxelNavPath, InPath);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VoxelNavPath_OnPathFailed,
    FCk_Handle_VoxelNavPath, InPath,
    ECk_VoxelNav_PathSearchOutcome, InOutcome);

// --------------------------------------------------------------------------------------------------------------------
