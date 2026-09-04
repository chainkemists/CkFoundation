#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"
#include "CkGroundNav/Path/CkGroundNavPath_Fragment_Data.h"

#include "CkGroundNavPath_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * One authored link a published route walks, as the pair of waypoints that walk it.
 *
 * The exit index is INDEX_NONE where the route ENDS on the link - a partial plan that stopped at the
 * far node has an entry and nothing after it - so a span is always the answer and a caller never has
 * to tell "no such link" apart from "the route does not finish it".
 *
 * Both distances are the plan's own integrated ones, carried on the metadata; nothing here integrates
 * the polyline a second time. An open span carries no exit distance, for the same reason it carries
 * no exit index.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNavPath_LinkSpan
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNavPath_LinkSpan);

private:
    // The STABLE authored id, and INDEX_NONE for the answer that names no link.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _LinkId = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _EntryWaypointIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _ExitWaypointIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _EntryDistanceUu = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _ExitDistanceUu = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_GroundNav_LinkDirection _EntryDirection = ECk_GroundNav_LinkDirection::Bidirectional;

public:
    CK_PROPERTY_GET(_LinkId);
    CK_PROPERTY_GET(_EntryWaypointIndex);
    CK_PROPERTY_GET(_EntryDistanceUu);
    CK_PROPERTY_GET(_EntryDirection);

    // The only two a span learns after it is made: the entry names the link, and the exit closes it.
    CK_PROPERTY(_ExitWaypointIndex);
    CK_PROPERTY(_ExitDistanceUu);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNavPath_LinkSpan,
        _LinkId, _EntryWaypointIndex, _EntryDistanceUu, _EntryDirection);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The links a published route walks, in walk order, each with the waypoints and the distances that
     * bound it.
     *
     * A pure function over the result's own metadata: no field, no world, no registry, so a test asks
     * it of a value it wrote by hand and gets the same answer a consumer does.
     *
     * An entry with no exit after it answers with an OPEN span rather than being dropped, which is what
     * a partial route that stopped on a link produces.
     */
    CKGROUNDNAV_API auto
    Get_LinksOnPath(
        const FCk_GroundNavPath_Result& InResult) -> TArray<FCk_GroundNavPath_LinkSpan>;

    /**
     * The first link the route steps onto strictly BEYOND a distance already walked, and a span naming
     * no link when there is none left.
     *
     * Strictly beyond, so a body standing exactly on an entry is asked about the link AFTER the one it
     * is already on rather than being handed the same one again.
     */
    CKGROUNDNAV_API auto
    TryGet_NextLinkBeyond(
        const FCk_GroundNavPath_Result& InResult,
        float                           InDistanceUu) -> FCk_GroundNavPath_LinkSpan;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_GroundNavPath"))
class CKGROUNDNAV_API UCk_Utils_GroundNavPath_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GroundNavPath_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_GroundNavPath);

public:
    /** Give an entity the ability to plan ground paths. The fragments are stamped directly on InHandle
     *  rather than on a child entity: a path belongs to the agent that walks it, and every consumer
     *  that reads waypoints already holds the agent's handle.
     *
     *  Which field is planned over is NOT bound here. GroundNav is a world surface, not an agent-scoped
     *  volume, so the field is resolved per request from the world and the start point. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Add Feature")
    static FCk_Handle_GroundNavPath
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_GroundNavPath_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /** Plan a route over whichever published field covers the start. The search is SLICED across
     *  frames, so the completion delegate fires when the search ends, not when the request is
     *  accepted - Succeeded only when waypoints are readable.
     *
     *  A start over ground nobody has baked is neither an answer nor a failure: the episode parks and
     *  re-probes every tick, and only after ck.GroundNav.MaxDeferralSeconds does it complete as Failed
     *  with the Unbuilt status. Every other terminal status completes as Failed immediately and fires
     *  OnPathFailed carrying which one it was.
     *
     *  A second FindPath supersedes the first: the one it replaces completes as Failed_Cancelled. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Request Find Path",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavPath
    Request_FindPath(
        UPARAM(ref) FCk_Handle_GroundNavPath& InPath,
        const FCk_Request_GroundNavPath_FindPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** The release half of Request_FindPath: drops the search in flight and returns the slot to
     *  nothing-planned, so nothing is left computing an answer for an episode the caller has ended.
     *
     *  ENQUEUED rather than applied here, unlike CkVoxelNav's: an abandon that jumped the queue would
     *  release an episode the drain has not started yet, and the order of the two is the whole meaning
     *  of an episode. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Request Abandon Path",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavPath
    Request_AbandonPath(
        UPARAM(ref) FCk_Handle_GroundNavPath& InPath,
        const FCk_Request_GroundNavPath_AbandonPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // Everything the last FINISHED episode answered. Meaningful only while Get Has Fresh Result is true.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Get Result")
    static FCk_GroundNavPath_Result
    Get_Result(
        const FCk_Handle_GroundNavPath& InPath);

    /** InProgress until an episode has finished, and the terminal status afterwards. A slot with no
     *  fresh result reads as InProgress rather than as its last verdict, because a stale Ready is what
     *  a caller would act on hardest and there is no None in this enum to say "nothing planned". */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Get Status")
    static ECk_GroundNav_PathStatus
    Get_Status(
        const FCk_Handle_GroundNavPath& InPath);

    // Whether the stored result belongs to a finished episode rather than one still being searched.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Get Has Fresh Result")
    static bool
    Get_HasFreshResult(
        const FCk_Handle_GroundNavPath& InPath);

    /** Everything the diagnostics pass last copied off this agent's planner: which provider answers
     *  its world, which profile its corridor was planned for, the last verdict, how many waypoints
     *  the slot publishes, the links that corridor crosses with the epoch it was found on, whether
     *  the agent stands flagged for a repath, and the world time the plan was dated at.
     *
     *  All values. An agent the pass has not visited yet reads back defaults, which is also what an
     *  invalid handle answers - the two are indistinguishable on purpose, because neither is a
     *  planner state anybody should act on. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Get Diagnostics")
    static FFragment_GroundNavPath_Diagnostics
    Get_Diagnostics(
        const FCk_Handle_GroundNavPath& InPath);

    /** The world box the agent's last published corridor covers, ALREADY inflated by the body's radius
     *  plus the corridor margin. Invalid where nothing is cached.
     *
     *  C++ ONLY, and deliberately not a UFUNCTION: a corridor box is a native reader's question (a debug report, a test) asked once per publish and never by a graph. The fragment's WRITES
     *  stay friend-scoped to the processors - this is a read and nothing else. */
    static auto
    Get_LastCorridorBounds(
        const FCk_Handle_GroundNavPath& InPath) -> FBox;

    // The authored links the last published route walks, in walk order. Empty where it walks none.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Get Links On Path")
    static TArray<FCk_GroundNavPath_LinkSpan>
    Get_LinksOnPath(
        const FCk_Handle_GroundNavPath& InPath);

    /** The next link the last published route steps onto past a distance already walked. A span whose
     *  link id is INDEX_NONE is the answer for a route with none left. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Try Get Next Link Beyond")
    static FCk_GroundNavPath_LinkSpan
    TryGet_NextLinkBeyond(
        const FCk_Handle_GroundNavPath& InPath,
        float InDistanceUu);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Bind To OnPathReady")
    static FCk_Handle_GroundNavPath
    BindTo_OnPathReady(
        UPARAM(ref) FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Unbind From OnPathReady")
    static FCk_Handle_GroundNavPath
    UnbindFrom_OnPathReady(
        UPARAM(ref) FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathReady& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Bind To OnPathFailed")
    static FCk_Handle_GroundNavPath
    BindTo_OnPathFailed(
        UPARAM(ref) FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavPath",
              DisplayName="[Ck][GroundNavPath] Unbind From OnPathFailed")
    static FCk_Handle_GroundNavPath
    UnbindFrom_OnPathFailed(
        UPARAM(ref) FCk_Handle_GroundNavPath& InPath,
        const FCk_Delegate_GroundNavPath_OnPathFailed& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
