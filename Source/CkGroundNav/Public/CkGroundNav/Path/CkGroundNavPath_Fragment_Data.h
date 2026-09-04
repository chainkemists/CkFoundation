#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGroundNav/Search/CkGroundNav_PathSearch.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkGroundNavPath_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_GroundNavPath_Diagnostics;
}

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
 * Whether a plan starts from nothing or from the corridor the agent is already walking.
 *
 * Cold is the default because a caller that has never planned has nothing to repair. Repair names an
 * INTENT and never a guarantee: what a corridor survives is the search's decision, reported as the
 * result's repair verdict, and an agent holding no corridor plans cold.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_PlanMode : uint8
{
    // Open the search at the start plate and nowhere else.
    Cold,

    /** Re-walk the corridor of this agent's last published plan first, and search only from wherever
     *  that walk stops holding. */
    Repair
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_PlanMode);

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

    /** Which start the search is given. Repair is honoured only where the agent still holds a corridor
     *  from an earlier plan; with none held it plans Cold, because a repair of nothing IS a cold plan.
     *  What was actually paid for is on the result's repair verdict, never inferred from this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNav_PlanMode _PlanMode = ECk_GroundNav_PlanMode::Cold;

    /** Stable link ids THIS query may not traverse. A denied link is skipped where the search admits
     *  crossings, so the answer routes around it or does not exist - it is never merely dearer, and a
     *  corridor that came back can never be holding one. The veto is the query's, not the field's:
     *  what a link joins, and every reachability label that follows from it, is unchanged. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSet<int32> _DeniedLinkIds;

    /** The same denial by CLASS: a link whose authored _UserTypeTag matches this container is denied,
     *  so a body that cannot use ladders needs no id for any of them. The RECORD'S tag is matched
     *  against the container, so naming a parent denies every link tagged under it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _DeniedLinkUserTypeTags;

    /** Stable link id to the multiplier THIS query prices its traverse at, REPLACING the authored one.
     *  Never below 1.0: every edge must cost at least the distance it covers or the search's Euclidean
     *  heuristic stops being admissible, so a multiplier below that bound is refused where the
     *  request is made rather than clamped somewhere the caller cannot see. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TMap<int32, float> _LinkCostMultipliers;

    /** Which class of walker is planning. Empty - the default - plans over the volume's untagged
     *  field; a tag plans over the field baked for that profile, and a world holding none for it
     *  leaves the episode parked exactly as an unbuilt start does. Stamped once when the episode
     *  opens: a search is a statement about ONE surface, and a route half planned on two of them
     *  would be a corridor no agent can walk. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _ProfileTag;

public:
    CK_PROPERTY_GET(_From);
    CK_PROPERTY_GET(_Goal);
    CK_PROPERTY(_RequestRevision);
    CK_PROPERTY(_IsShadow);
    CK_PROPERTY(_PlanMode);
    CK_PROPERTY(_DeniedLinkIds);
    CK_PROPERTY(_DeniedLinkUserTypeTags);
    CK_PROPERTY(_LinkCostMultipliers);
    CK_PROPERTY(_ProfileTag);

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
 * Which end of an authored link a published waypoint stands on.
 *
 * The reflected twin of ck::groundnav::ECk_GroundNav_LinkWaypointRole. The plan's own role rides an
 * unreflected value in Search/, which no reflected header may include from here downwards, and the two
 * are converted where the plan is flattened onto the result.
 */
UENUM(BlueprintType)
enum class ECk_GroundNavPath_LinkWaypointRole : uint8
{
    // No link put this waypoint on the route.
    None,

    // Where the body steps ONTO the link.
    Entry,

    // Where it steps off the far end.
    Exit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNavPath_LinkWaypointRole);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One published waypoint that an authored link put on the route.
 *
 * A PARALLEL array keyed by index into _Waypoints rather than a richer element type for _Waypoints
 * itself: a consumer that reads only the locations reads exactly the array it read before links
 * existed, and a route that crosses none carries no second array at all.
 *
 * The id is the STABLE authored one and never the field-local link index, because an installed path
 * outlives the field it was planned against and _ResolvedLinks is re-derived wholesale on every
 * publish, so an index would quietly name a different link after the next add or remove.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNavPath_LinkWaypoint
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNavPath_LinkWaypoint);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _WaypointIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _LinkId = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNavPath_LinkWaypointRole _Role = ECk_GroundNavPath_LinkWaypointRole::None;

    /** Carried on both ends of one traversal, so an exit answers what its entry did rather than making
     *  a consumer look back up the array for it. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNav_LinkDirection _EntryDirection = ECk_GroundNav_LinkDirection::Bidirectional;

    /** The plan's own integrated distance to this waypoint. Carried rather than left to be recomputed:
     *  the polyline length is a number the post-process already answered, and a query that integrated
     *  _Waypoints a second time would be a second definition of it. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _DistanceFromStartUu = 0.0f;

public:
    CK_PROPERTY_GET(_WaypointIndex);
    CK_PROPERTY_GET(_LinkId);
    CK_PROPERTY_GET(_Role);
    CK_PROPERTY_GET(_EntryDirection);
    CK_PROPERTY_GET(_DistanceFromStartUu);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNavPath_LinkWaypoint,
        _WaypointIndex, _LinkId, _Role, _EntryDirection, _DistanceFromStartUu);
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

    /** What a repair did with the corridor it was handed. None on every cold plan, and on every plan
     *  that asked to repair with nothing cached to repair. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNav_RepairVerdict _RepairVerdict = ECk_GroundNav_RepairVerdict::None;

    /** Where this route steps onto and off the authored links it crosses, keyed by index into
     *  _Waypoints. Empty on every route that crosses none. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_GroundNavPath_LinkWaypoint> _LinkWaypoints;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_Waypoints);
    CK_PROPERTY(_RequestRevision);
    CK_PROPERTY(_IsShadow);
    CK_PROPERTY(_LengthUu);
    CK_PROPERTY(_ExpansionCount);
    CK_PROPERTY(_SearchDurationMs);
    CK_PROPERTY(_PlannedAgainstEpoch);
    CK_PROPERTY(_RepairVerdict);
    CK_PROPERTY(_LinkWaypoints);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * What this agent's planner is doing, as values and nothing else.
 *
 * Every field is a COPY taken while the agent's own path fragments were readable - no field pointer,
 * no handle, no world - so a viewer reads it a frame later, or off a record kept after the world that
 * produced it is gone, without asking whether anything behind it still exists.
 *
 * It carries what GROUNDNAV owns and stops there. How far along the route a body has walked is the
 * CROWD's cursor and lives on the crowd agent, so it is not here; and the corridor is named by the
 * plan's own stable link ids rather than by area tags, because a tag is a name only the volume's
 * records resolve and resolving one here would be a second answer to a question the volume answers.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FFragment_GroundNavPath_Diagnostics
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_GroundNavPath_Diagnostics);

    // The one pass that stamps this. Everything else reads it.
    friend class ck::FProcessor_GroundNavPath_Diagnostics;

private:
    /** Whether the pass has ever written this value. False on a default one and on the one an invalid
     *  handle answers with, true from the first visit onwards. Every other column is read under it:
     *  an enum has no spare "nobody has said" value to spend, so this bool is what tells a stamped
     *  default from a default nothing has stamped. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _HasBeenStamped = false;

    /** Which provider answers the world this agent stands in. A GroundNav planner in a Recast world
     *  is a planner nothing is driving, which is what this column is here to make visible. The
     *  initialiser is the enum's own first value and not a claim - _HasBeenStamped is the gate, and
     *  it is false until a pass has read a world. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_NavSurface_Provider _Provider = ECk_NavSurface_Provider::Recast;

    // The profile the cached corridor was planned for. Empty is the volume's untagged default.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _ProfileTag;

    /** What the last FINISHED episode answered, CARRIED across the search that follows it. Read only
     *  off a slot that is fresh and then left standing, so a viewer asking mid-search is told the
     *  verdict that stands rather than the InProgress every unanswered slot reads as. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNav_PathStatus _PathStatus = ECk_GroundNav_PathStatus::InProgress;

    // How many waypoints the slot is publishing. Zero on every status that publishes no route.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _PublishedWaypointCount = 0;

    /** The AUTHORED ids of the links the cached corridor crosses, in walk order and without repeats.
     *  Empty for a route that crosses none. Ids rather than names: an id is volume-scoped, monotone
     *  and never reused, and a name is the volume's own read of its records. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<int32> _CorridorLinkIds;

    /** The field epoch that corridor was found on, as its bare counter: FCk_GroundNav_Epoch is a
     *  plain struct and is not reflected. Zero where nothing is cached. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int64 _CorridorEpoch = 0;

    // Whether the agent stands flagged for a repath by ground that moved under its corridor.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _RepathRequired = false;

    /** The world time at the tick a plan was seen to publish, and zero before the first one. Dated by
     *  the WORLD rather than by the platform clock, so it reads against everything else a world-dated
     *  view shows and means the same thing in a second world. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _LastPlanWorldTime;

    /** The published sequence the date above was taken at. The pass re-dates when the slot's sequence
     *  has moved past this and never otherwise, so a standing plan read for the next hundred ticks
     *  keeps the date it was planned at rather than the date it was last looked at. */
    int32 _LastPlanSequence = 0;

public:
    CK_PROPERTY_GET(_HasBeenStamped);
    CK_PROPERTY_GET(_Provider);
    CK_PROPERTY_GET(_ProfileTag);
    CK_PROPERTY_GET(_PathStatus);
    CK_PROPERTY_GET(_PublishedWaypointCount);
    CK_PROPERTY_GET(_CorridorLinkIds);
    CK_PROPERTY_GET(_CorridorEpoch);
    CK_PROPERTY_GET(_RepathRequired);
    CK_PROPERTY_GET(_LastPlanWorldTime);
    CK_PROPERTY_GET(_LastPlanSequence);
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
