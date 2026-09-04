#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include "CkGroundNavVolume_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_GroundNavVolume"))
class CKGROUNDNAV_API UCk_Utils_GroundNavVolume_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GroundNavVolume_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_GroundNavVolume);

public:
    /** Add the grounded-navigation feature to InOwner as a child entity carrying the bake params.
     *  FProcessor_GroundNavVolume_Setup consumes its NeedsSetup tag on the next tick and, unless the
     *  params opted out, arms the first build from there. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Add Volume Feature")
    static FCk_Handle_GroundNavVolume
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_GroundNavVolume_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Has Volume Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    /** Bakes the volume, resuming across as many ticks as the probe budget needs. A request arriving
     *  while a build is already underway is an idempotent no-op unless it forces a restart.
     *
     *  The completion delegate fires when the BUILD ends, not when the request is accepted — that is
     *  ticks later, and it is the only outcome a caller can act on. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Build",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_Build(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Build& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Declares a world-space box whose ground is no longer trustworthy and schedules a LOCAL repair of
     *  exactly that ground - never a whole-volume rebake. Boxes arriving before the repair opens are
     *  UNIONED into one pass, so many bodies moving in one frame cost one repair.
     *
     *  For a MOVED body the box is the union of where it was and where it is: the new half closes the
     *  ground it arrived on, the old half reopens the ground it left.
     *
     *  Refused before it is enqueued when the box is not a box - degenerate on any axis, or carrying a
     *  non-finite corner - because a repair over nothing publishes an epoch nothing changed.
     *
     *  The completion delegate fires when the REPAIR ends, not when the box is accepted; a build that
     *  STARTS after the box was accepted takes it over and completes it at its publish, having rebaked
     *  the same ground. A box raised while a build is already running waits for that publish and is
     *  repaired against the field it produces. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Repair",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_Repair(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Repair& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Paints an authored area tag onto the ground a shape covers, keyed on the markup ENTITY: a
     *  second request naming the same entity updates that record in place instead of adding another,
     *  so moving, retagging or disabling a placed volume is one call rather than a release and an add.
     *
     *  Completes Failed when the entity is invalid, when nothing published what the area tag MEANS, or
     *  when the shape and transform bound nothing. A record whose footprint misses every tile or lands
     *  where nothing has baked is NOT rejected — what a markup reaches is the bake's answer. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Area Markup",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_AreaMarkup(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_AreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Drops the record a markup entity owns, and the back-pointer that entity carries. Releasing a
     *  markup the volume does not hold completes Succeeded: the caller's intent already holds. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Release Area Markup",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_ReleaseAreaMarkup(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_ReleaseAreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Authors a navigation link between two world points, keyed on the link ENTITY: a second request
     *  naming the same entity updates that record in place instead of adding another, so moving,
     *  re-pricing or disabling a placed link is one call rather than a release and an add.
     *
     *  Completes Failed when the entity is invalid, when the two endpoints are not two distinct finite
     *  points, when either lies outside the volume's bounds, when either cost multiplier is below 1.0,
     *  when the clearance admits no agent, or when a carried area tag has no registered policy. An
     *  UNSET area tag is admitted - a link's traversal stands on its own.
     *
     *  A link whose endpoints reach ground nobody has baked yet is NOT rejected: what a link resolves
     *  to is the composition's answer, and the next publish re-resolves it. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Link",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_Link(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_Link& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /** Drops the record a link entity owns, and the back-pointer that entity carries. Releasing a link
     *  the volume does not hold completes Succeeded: the caller's intent already holds. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Request Release Link",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_GroundNavVolume
    Request_ReleaseLink(
        UPARAM(ref) FCk_Handle_GroundNavVolume& InVolume,
        const FCk_Request_GroundNavVolume_ReleaseLink& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Is Built")
    static bool
    Get_IsBuilt(
        const FCk_Handle_GroundNavVolume& InVolume);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Is Building")
    static bool
    Get_IsBuilding(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** Bumps on every completed build. A consumer holding a field compares against this to learn it is
     *  behind — staleness is derived here rather than stored anywhere. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Build Epoch")
    static int64
    Get_BuildEpoch(
        const FCk_Handle_GroundNavVolume& InVolume);

public:
    /**
     * How many tiles the published field is divided into, or 0 while nothing is published.
     *
     * A NUMBER, not the structure. Get_Field below refuses to hand the field itself to Blueprint or
     * AngelScript because a per-call copy would cost more than every query made through it; a count
     * carries none of that cost, and it makes no self-consistency promise either — it is a snapshot
     * of one integer taken at the moment of the call, and a rebuild between two of these calls is
     * exactly what Get_BuildEpoch is there to expose.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Tile Count")
    static int32
    Get_TileCount(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** How many of the published field's tiles actually baked, or 0 while nothing is published. A
     *  snapshot number for the same reason Get_TileCount is one. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Built Tile Count")
    static int32
    Get_BuiltTileCount(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** How many crossings join the published field's tiles across their seams, or 0 while nothing is
     *  published. A snapshot number for the same reason Get_TileCount is one. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Seam Portal Count")
    static int32
    Get_SeamPortalCount(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** How many cells of the published field have a walkable surface, summed over its tiles, or 0 while
     *  nothing is published. A snapshot number for the same reason Get_TileCount is one. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Walkable Cell Count")
    static int32
    Get_WalkableCellCount(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** How many of the published field's links have an end that found no ground, or 0 while nothing is
     *  published. A dropped link is a STATUS and never a warning: this is the number that says so. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Unresolved Link Count")
    static int32
    Get_UnresolvedLinkCount(
        const FCk_Handle_GroundNavVolume& InVolume);

    /**
     * Every navigation link the volume holds, in admission order, or an empty array when it holds none.
     *
     * A COPY, unlike the view Get_LinkEntries hands the composition stages, and reflected rather than
     * C++-only: the same link has to be authored and read back from C++, from Blueprint and from
     * AngelScript, and neither of the latter two has a shape for a view. The list is short by
     * construction - one entry per authored link - so the copy costs what the read is worth.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Link Records")
    static TArray<FCk_GroundNav_LinkRecord>
    Get_LinkRecords(
        const FCk_Handle_GroundNavVolume& InVolume);

public:
    /**
     * Whether the ground under a point is built, seen through the VOLUME rather than through a field.
     *
     * A published field answers Built or Unbuilt and never Building — it is immutable, so its tiles
     * are baked or they are not. Building is this layer's word, said while a build is in flight, and
     * an invalid volume answers OutsideField: a caller that cannot name a volume is over no ground.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Region Status At")
    static ECk_GroundNav_RegionStatus
    Get_RegionStatusAt(
        const FCk_Handle_GroundNavVolume& InVolume,
        const FVector& InLocation);

    /** The same answer folded over every tile a box touches: Built when all are, Unbuilt when none
     *  are, PartiallyBuilt otherwise — with either of the latter two promoted to Building while a
     *  build is in flight. Only XY decides which tiles are touched. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Region Status Within")
    static ECk_GroundNav_RegionStatus
    Get_RegionStatusWithin(
        const FCk_Handle_GroundNavVolume& InVolume,
        const FBox& InBounds);

    /**
     * The world box the published field's BUILT tiles cover, or an empty box while nothing is
     * published.
     *
     * Empty rather than the authored volume bounds on purpose: a consumer fitting a view to this must
     * be told there is nothing to fit rather than handed bounds that do not hold ground yet.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Surface Bounds")
    static FBox
    Get_SurfaceBounds(
        const FCk_Handle_GroundNavVolume& InVolume);

    /** Building while a build is in flight, Ready once a field is published, Error when nothing is
     *  published and the last build failed, NoData when nothing has been tried. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Provider Health")
    static ECk_NavSurface_ProviderHealth
    Get_ProviderHealth(
        const FCk_Handle_GroundNavVolume& InVolume);

public:
    /**
     * The published field, or an invalid pointer if nothing has been built yet.
     *
     * C++ only, and deliberately so: the caller takes a shared reference to an immutable structure and
     * is guaranteed a self-consistent field for as long as it holds it. There is no Blueprint or
     * AngelScript shape for that guarantee, and one that copied the field per call would cost more than
     * every query made through it.
     */
    static auto
    Get_Field(
        const FCk_Handle_GroundNavVolume& InVolume) -> ck::groundnav::FCk_GroundNav_FieldPtr;

    /**
     * Every area markup the volume holds, in admission order, or an empty view when it holds none.
     *
     * C++ only, and a VIEW rather than a copy: the consumers are the bake stages, which are C++ by
     * construction, and a per-call copy of every record would cost more than the reduction it feeds.
     * The view is valid until the next markup request drains.
     */
    static auto
    Get_MarkupRecords(
        const FCk_Handle_GroundNavVolume& InVolume) -> TConstArrayView<ck::FCk_GroundNav_MarkupEntry>;

    /** The record carrying this id, or unset when the volume holds none. Ids are never reused, so an
     *  unset answer means released and never means renumbered. */
    static auto
    TryGet_MarkupRecord(
        const FCk_Handle_GroundNavVolume& InVolume,
        int32 InRecordId) -> TOptional<FCk_GroundNav_MarkupRecord>;

    /**
     * The dirty ground accumulated on the volume that no repair has opened for yet, or an invalid box
     * when none is pending.
     *
     * C++ only, for the same reason Get_Field is: the consumers are the repair stages. An invalid box
     * is the answer to "nothing is pending", never an empty region at the origin.
     */
    static auto
    Get_PendingDirtyBounds(
        const FCk_Handle_GroundNavVolume& InVolume) -> FBox;

    /**
     * Every navigation link the volume holds, in admission order, or an empty view when it holds none.
     *
     * C++ only, and a VIEW rather than a copy, for the same reason Get_MarkupRecords is one: the
     * consumers are the composition stages, which are C++ by construction. The view is valid until the
     * next link request drains. Get_LinkRecords above is the reflected read-back.
     */
    static auto
    Get_LinkEntries(
        const FCk_Handle_GroundNavVolume& InVolume) -> TConstArrayView<ck::FCk_GroundNav_LinkEntry>;

    /** The link record carrying this id, or unset when the volume holds none. Ids are never reused, so
     *  an unset answer means released and never means renumbered. */
    static auto
    TryGet_LinkRecord(
        const FCk_Handle_GroundNavVolume& InVolume,
        int32 InRecordId) -> TOptional<FCk_GroundNav_LinkRecord>;

    /**
     * Whether an authored link is LIVE on the published ground.
     *
     * DERIVED AT THE READ, and nothing anywhere stores it - the same shape Get_IsMarkupLive has, and
     * narrower in exactly one way that has no markup analogue: the link must have RESOLVED. A markup
     * that reaches nothing is admitted and simply decides nothing, where a link that did not resolve
     * is a link that is not there.
     *
     * Both of its endpoint tiles must be Built and must carry an epoch STRICTLY PAST the one the
     * record was submitted against. Strictly past, not at or past, because _RequestedAtEpoch is
     * stamped at admission with the epoch the field was ALREADY published at: an equal epoch is the
     * very publish the record was submitted against, which by construction knew nothing about it.
     *
     * Stated over a field and a record, which is all it needs - and what lets it be verified without a
     * world.
     */
    static auto
    Get_IsLinkLiveOnField(
        const ck::groundnav::FCk_GroundNav_Field& InField,
        const FCk_GroundNav_LinkRecord&           InRecord) -> bool;

    /**
     * The same rule reached through the link ENTITY, which is the identity every request keys on.
     *
     * False for an entity carrying no back-pointer - the request has not drained onto a volume yet -
     * for a record the named volume no longer holds, and for a volume with nothing published.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNavVolume",
              DisplayName="[Ck][GroundNavVolume] Get Is Link Live")
    static bool
    Get_IsLinkLive(
        const FCk_Handle& InLinkEntity);

    /** Whether a repair state currently holds a source field and is being sliced. A volume with dirty
     *  ground pending but no repair opened yet reads false - that is what the pending bounds say. */
    static auto
    Get_IsRepairInProgress(
        const FCk_Handle_GroundNavVolume& InVolume) -> bool;

    /**
     * Nothing is in flight on this volume and nothing is pending: a field is published, no build and
     * no repair is running or armed, no cost re-derive and no link re-derive is owed, and none of the
     * four request queues holds anything. The published field is therefore the one every query will
     * answer from until something new is asked of the volume.
     *
     * This is the named condition a test settles on after a paint, a release, or a rebuild kick,
     * rather than a hop count that has to be re-guessed whenever a stage's internal staging changes.
     *
     * C++ only, for the same reason Get_PendingDirtyBounds is: the consumers are the provider adapter
     * and the fixtures that wait on it, both C++ by construction.
     */
    static auto
    Get_IsSettled(
        const FCk_Handle_GroundNavVolume& InVolume) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
