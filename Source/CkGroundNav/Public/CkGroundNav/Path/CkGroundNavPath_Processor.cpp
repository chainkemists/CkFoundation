#include "CkGroundNavPath_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Search/CkGroundNav_PathPostProcess.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_Slice);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_path_processor
{
    using namespace ck::groundnav;

    // ----------------------------------------------------------------------------------------------------------------

    /** How many agents one tick may touch. Clamped to at least one: a cap of zero would park every
     *  episode forever, and a search nobody ever slices is a stall, not a saving. */
    static TAutoConsoleVariable<int32> CVarMaxSearchesPerFrame(
        TEXT("ck.GroundNav.MaxSearchesPerFrame"),
        8,
        TEXT("Agents whose ground-path search may be advanced in one tick. The rest wait a tick.\n")
        TEXT("Clamped to at least 1. Default 8."),
        ECVF_Default);

    /** Wall-clock the whole tick may spend searching, shared across those agents. There is deliberately
     *  NO unbounded setting: a per-frame slice budget that never runs out is the failure this processor
     *  exists to prevent. */
    static TAutoConsoleVariable<float> CVarSliceBudgetMs(
        TEXT("ck.GroundNav.SliceBudgetMs"),
        1.0f,
        TEXT("Milliseconds one tick may spend across ALL ground-path searches. Each agent is handed\n")
        TEXT("what the agents before it left. Clamped to at least 0.01ms. Default 1.0."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVarMaxIterationsPerSlice(
        TEXT("ck.GroundNav.MaxIterationsPerSlice"),
        0,
        TEXT("Expansions one agent may make in one tick. Zero means as many as the time budget allows.\n")
        TEXT("Slicing changes nothing but where the work stops, so this never changes a verdict."),
        ECVF_Default);

    /** Mirrors ck.Nav.MaxDeferralSeconds. An episode parked on ground nobody has baked is worth waiting
     *  for, but not forever - past this it is failed with Unbuilt so the caller leaves Pending. */
    static TAutoConsoleVariable<float> CVarMaxDeferralSeconds(
        TEXT("ck.GroundNav.MaxDeferralSeconds"),
        5.0f,
        TEXT("Hard timeout for an episode parked on unbuilt ground. After this many seconds without a\n")
        TEXT("field to plan over, the request is force-failed with Unbuilt so the caller transitions\n")
        TEXT("out of Pending. Default 5s."),
        ECVF_Default);

    /**
     * What the corridor box is grown by beyond the body's own radius: ONE CELL of the field's default
     * lattice (FCk_GroundNav_BakeConfig::_CellSizeUu, 25uu).
     *
     * The box covers plate RECTANGLES, and a plate rectangle is where the ground is, not where the
     * body may be while walking it: a string-pulled route hugs a plate edge, and a body of radius r
     * standing on that edge occupies r past it. The radius answers that. The cell on top answers the
     * lattice itself - a rebuild that changed only the cells either side of a door moves ground the
     * corridor was priced through while leaving every plate rectangle it named intact, and a box cut
     * exactly to those rectangles would read such a rebuild as untouching the route.
     *
     * One cell rather than the field's own cell size because a compile-time constant cannot ask a
     * field that does not exist yet, and because the margin exists to absorb the lattice's grain
     * rather than to measure it: a field baked finer than the default is covered by more than a cell
     * of margin, which errs toward invalidating.
     */
    constexpr auto kCorridorInflationMarginUu = 25.0f;

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CostParams(
            const ck::FFragment_GroundNavPath_Params& InParams)
        -> FCk_GroundNav_PathCostParams
    {
        auto Cost = FCk_GroundNav_PathCostParams{};

        Cost._SlopePenaltyK = InParams.Get_SlopePenaltyK();
        Cost._ClearanceBiasK = InParams.Get_ClearanceBiasK();
        Cost._CornerOffsetK = InParams.Get_CornerOffsetK();

        // The per-plate multiplier table is deliberately left empty: it is markup the field carries,
        // and an empty table prices every plate at one, which is the unmarked field.
        return Cost;
    }

    /**
     * The same cost model with THIS request's link veto laid over it.
     *
     * Kept apart from the params-only form rather than folded into it because the two callers differ:
     * a search prices link traverses and admits link crossings, and the post-process prices neither -
     * it walks a corridor the search already chose. A veto handed to the post-process would be a value
     * nothing reads, which is the sort of thing that later reads as a bug.
     */
    auto
        Get_CostParams(
            const ck::FFragment_GroundNavPath_Params& InParams,
            const FCk_Request_GroundNavPath_FindPath& InRequest)
        -> FCk_GroundNav_PathCostParams
    {
        auto Cost = Get_CostParams(InParams);

        Cost._DeniedLinkIds = InRequest.Get_DeniedLinkIds();
        Cost._DeniedLinkUserTypeTags = InRequest.Get_DeniedLinkUserTypeTags();
        Cost._LinkCostMultipliers = InRequest.Get_LinkCostMultipliers();

        return Cost;
    }

    auto
        Get_Query(
            const ck::FFragment_GroundNavPath_Params& InParams,
            const FCk_Request_GroundNavPath_FindPath& InRequest)
        -> FCk_GroundNav_PathQuery
    {
        auto Query = FCk_GroundNav_PathQuery{};

        Query._Start = InRequest.Get_From();
        Query._Goal = InRequest.Get_Goal();
        Query._VerticalToleranceUu = InParams.Get_VerticalToleranceUu();
        Query._Agent._RadiusUu = InParams.Get_AgentRadiusUu();
        Query._Cost = Get_CostParams(InParams, InRequest);
        Query._GreedyWeightW = InParams.Get_GreedyWeightW();
        Query._MaxExpansions = InParams.Get_MaxExpansions();
        Query._MaxCorridorLength = InParams.Get_MaxCorridorLength();
        Query._AllowPartialPath = InParams.Get_AllowPartialPath();

        return Query;
    }

    /** The agent location the post-process drops the first waypoint against is the request's own From:
     *  the path entity carries no transform of its own, and From is where the caller said the body was
     *  when it asked. */
    auto
        Get_PostParams(
            const ck::FFragment_GroundNavPath_Params& InParams,
            const FVector&                            InAgentLocation)
        -> FCk_GroundNav_PathPostParams
    {
        auto PostParams = FCk_GroundNav_PathPostParams{};

        PostParams._Agent._RadiusUu = InParams.Get_AgentRadiusUu();
        PostParams._VerticalToleranceUu = InParams.Get_VerticalToleranceUu();
        PostParams._AgentLocation = InAgentLocation;
        PostParams._Cost = Get_CostParams(InParams);

        return PostParams;
    }

    /** The plan's unreflected role as the published one. Two enums rather than one because the plan is
     *  a Search/ value and carries no reflection, and this is the single point they meet at. */
    auto
        Get_PublishedRole(
            ECk_GroundNav_LinkWaypointRole InRole)
        -> ECk_GroundNavPath_LinkWaypointRole
    {
        switch (InRole)
        {
            case ECk_GroundNav_LinkWaypointRole::Entry:
            { return ECk_GroundNavPath_LinkWaypointRole::Entry; }
            case ECk_GroundNav_LinkWaypointRole::Exit:
            { return ECk_GroundNavPath_LinkWaypointRole::Exit; }
            case ECk_GroundNav_LinkWaypointRole::None:
            default:
            { return ECk_GroundNavPath_LinkWaypointRole::None; }
        }
    }

    /** The corridor keyed by the ONE durable identity a crossing has. Node ids are per-search pool ids
     *  and mean nothing to a second search, so a corridor is stored as keys or it is not stored. */
    auto
        Get_CorridorKeys(
            const FCk_GroundNav_PathResult& InResult)
        -> TArray<FCk_GroundNav_CrossingKey>
    {
        auto Keys = TArray<FCk_GroundNav_CrossingKey>{};
        Keys.Reserve(InResult._Crossings.Num());

        for (const auto& Crossing : InResult._Crossings)
        { Keys.Emplace(Make_CrossingKey(Crossing)); }

        return Keys;
    }

    /**
     * The same corridor said in link identities: the authored id behind every link crossing on it.
     *
     * Resolved HERE, against the field the plan was made on, because that is the only place the index
     * a crossing carries still means anything - _ResolvedLinks is rebuilt wholesale per publish and a
     * removal shifts every entry after it. Walk order and no repeats, so a route that crosses one link
     * twice names it once.
     */
    auto
        Get_CorridorLinkIds(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_PathResult& InResult)
        -> TArray<int32>
    {
        auto LinkIds = TArray<int32>{};

        for (const auto& Crossing : InResult._Crossings)
        {
            if (NOT InField._ResolvedLinks.IsValidIndex(Crossing._LinkIndex))
            { continue; }

            LinkIds.AddUnique(InField._ResolvedLinks[Crossing._LinkIndex]._Id);
        }

        return LinkIds;
    }

    /**
     * The world box one flat plate covers: its cell rectangle over its own tile's lattice, spanning the
     * field's vertical slab.
     *
     * A plate is a rectangle in XY and carries no absolute Z of its own - the heights are per cell -
     * so the slab is what Get_TileWorldBounds already uses for the same reason, and it is the shape an
     * invalidator compares a republished TILE against. Bounds are inclusive in cells, so the far corner
     * is one cell past the last one.
     */
    auto
        Get_FlatPlateWorldBounds(
            const FCk_GroundNav_Field& InField,
            int32                      InFlatPlate)
        -> FBox
    {
        int32 TileIndex = INDEX_NONE;
        int32 PlateIndex = INDEX_NONE;

        if (NOT Get_TileAndPlate(InField, InFlatPlate, TileIndex, PlateIndex))
        { return FBox{ForceInit}; }

        if (NOT InField._Tiles.IsValidIndex(TileIndex))
        { return FBox{ForceInit}; }

        const auto& Tile = InField._Tiles[TileIndex];

        if (NOT Tile._Plates._Plates.IsValidIndex(PlateIndex))
        { return FBox{ForceInit}; }

        const auto& Plate = Tile._Plates._Plates[PlateIndex];

        const auto CellSize = static_cast<double>(Tile._CellSizeUu);

        return FBox{
            FVector{Tile._Origin.X + (Plate._MinX * CellSize),
                    Tile._Origin.Y + (Plate._MinY * CellSize),
                    static_cast<double>(InField._Params._MinZUu)},
            FVector{Tile._Origin.X + ((Plate._MaxX + 1) * CellSize),
                    Tile._Origin.Y + ((Plate._MaxY + 1) * CellSize),
                    static_cast<double>(InField._Params._MaxZUu)}};
    }

    /** The corridor's plates unioned in world space, inflated ONCE by what the caller stores beside it. */
    auto
        Get_CorridorBounds(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_PathResult& InResult,
            float                           InInflationUu)
        -> FBox
    {
        auto Bounds = FBox{ForceInit};

        for (const auto FlatPlate : InResult._PlateCorridor)
        {
            const auto PlateBounds = Get_FlatPlateWorldBounds(InField, FlatPlate);

            if (PlateBounds.IsValid == 0)
            { continue; }

            Bounds += PlateBounds;
        }

        if (Bounds.IsValid == 0)
        { return Bounds; }

        return Bounds.ExpandBy(static_cast<double>(InInflationUu));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drops the episode without touching the corridor keys: those outlive the episode that found them,
    // because they are what a later repair re-canonicalises against.
    auto
        FGroundNavPath_Episode::
        DoClear(
            FFragment_GroundNavPath_Current& InCurrent)
        -> void
    {
        InCurrent._Search = groundnav::FCk_GroundNav_PathSearch{};
        InCurrent._Field.Reset();
        InCurrent._PendingRequest = FCk_Request_GroundNavPath_FindPath{};
        InCurrent._HasBegun = false;
        InCurrent._PendingSince = FCk_Time{};
        InCurrent._SearchTimeSpent = FCk_Time{};
    }

    // The one line that proves this provider is alive: every published verdict, terminal or timed out.
    auto
        FGroundNavPath_Episode::
        DoLog_Published(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FCk_GroundNavPath_Result&       InPublished)
        -> void
    {
        groundnav::Display(
            TEXT("GroundNav Path [{}] published [{}] rev [{}] epoch [{}] waypoints [{}] expansions [{}] ")
            TEXT("repair [{}] shadow [{}] search [{}]ms"),
            InPathEntity, InPublished.Get_Status(), InPublished.Get_RequestRevision(),
            InPublished.Get_PlannedAgainstEpoch(), InPublished.Get_Waypoints().Num(),
            InPublished.Get_ExpansionCount(), InPublished.Get_RepairVerdict(),
            InPublished.Get_IsShadow(), InPublished.Get_SearchDurationMs());
    }

    auto
        FGroundNavPath_Episode::
        DoPublish_Failure(
            FCk_Handle_GroundNavPath         InPathEntity,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result&  InResult,
            ECk_GroundNav_PathStatus         InStatus,
            int32                            InExpansionCount,
            int64                            InPlannedAgainstEpoch)
        -> void
    {
        const auto Request = InCurrent._PendingRequest;

        const auto SearchDurationMs =
            static_cast<float>(InCurrent._SearchTimeSpent.Get_Milliseconds());

        // Waypoints are cleared here and NOT at the install boundary: this module answers what its own
        // search found, and a failed search found nothing. What a consumer does with the route it was
        // already walking is the consumer's decision, made where the plan is installed.
        InResult._Result
            .Set_Status(InStatus)
            .Set_Waypoints({})
            .Set_RequestRevision(Request.Get_RequestRevision())
            .Set_IsShadow(Request.Get_IsShadow())
            .Set_LengthUu(0.0)
            .Set_ExpansionCount(InExpansionCount)
            .Set_SearchDurationMs(SearchDurationMs)
            .Set_PlannedAgainstEpoch(InPlannedAgainstEpoch)
            .Set_RepairVerdict(InCurrent._Search.Get_RepairVerdict());

        InResult._HasFreshResult = true;

        DoLog_Published(InPathEntity, InResult._Result);

        DoClear(InCurrent);
        InPathEntity.Try_Remove<FTag_GroundNavPath_SearchInFlight>();

        Request.TryFireCompletion(InPathEntity, ECk_Request_OperationResult::Failed);

        UUtils_Signal_OnGroundNavPathFailed::Broadcast(
            InPathEntity, MakePayload(InPathEntity, InStatus));
    }

    auto
        FGroundNavPath_Episode::
        DoPublish_Success(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current&      InCurrent,
            FFragment_GroundNavPath_Result&       InResult)
        -> void
    {
        using namespace ck_groundnav_path_processor;

        const auto& SearchResult = InCurrent._Search.Get_Result();
        const auto Request = InCurrent._PendingRequest;

        const auto SearchDurationMs =
            static_cast<float>(InCurrent._SearchTimeSpent.Get_Milliseconds());

        const auto Plan = groundnav::Get_PathPlan(
            SearchResult,
            *InCurrent._Field,
            Get_PostParams(InParams, Request.Get_From()));

        auto Locations = TArray<FVector>{};
        Locations.Reserve(Plan._Waypoints.Num());

        // Collapsed out of the waypoints rather than carried as a richer element type, so what an
        // existing consumer reads is the same flat array of locations it has always read.
        auto LinkWaypoints = TArray<FCk_GroundNavPath_LinkWaypoint>{};

        for (auto Index = 0; Index < Plan._Waypoints.Num(); ++Index)
        {
            const auto& Waypoint = Plan._Waypoints[Index];

            Locations.Emplace(Waypoint._Location);

            if (Waypoint._LinkRole == groundnav::ECk_GroundNav_LinkWaypointRole::None)
            { continue; }

            LinkWaypoints.Emplace(FCk_GroundNavPath_LinkWaypoint{
                Index,
                Waypoint._LinkId,
                Get_PublishedRole(Waypoint._LinkRole),
                Waypoint._LinkEntryDirection,
                static_cast<float>(Waypoint._DistanceFromStart)});
        }

        InResult._Result
            .Set_Status(Plan._Status)
            .Set_Waypoints(Locations)
            .Set_LinkWaypoints(LinkWaypoints)
            .Set_RequestRevision(Request.Get_RequestRevision())
            .Set_IsShadow(Request.Get_IsShadow())
            .Set_LengthUu(Plan._LengthUu)
            .Set_ExpansionCount(SearchResult._ExpansionCount)
            .Set_SearchDurationMs(SearchDurationMs)
            .Set_PlannedAgainstEpoch(Plan._PlannedAgainstEpoch._Value)
            .Set_RepairVerdict(InCurrent._Search.Get_RepairVerdict());

        InResult._HasFreshResult = true;

        DoLog_Published(InPathEntity, InResult._Result);

        const auto CorridorInflationUu = InParams.Get_AgentRadiusUu() + kCorridorInflationMarginUu;

        const auto CorridorBounds =
            Get_CorridorBounds(*InCurrent._Field, SearchResult, CorridorInflationUu);

        InCurrent._LastCorridorKeys = Get_CorridorKeys(SearchResult);
        InCurrent._LastCorridorLinkIds = Get_CorridorLinkIds(*InCurrent._Field, SearchResult);
        InCurrent._LastCorridorEpoch = SearchResult._PlannedAgainstEpoch;
        InCurrent._ProfileTag = Request.Get_ProfileTag();
        InCurrent._LastCorridorBounds = CorridorBounds;
        InCurrent._CorridorInflationUu = CorridorBounds.IsValid != 0 ? CorridorInflationUu : 0.0f;
        InCurrent._LastSourceFlatPlate = SearchResult._PlateCorridor.IsEmpty()
            ? INDEX_NONE
            : SearchResult._PlateCorridor[0];

        DoClear(InCurrent);
        InPathEntity.Try_Remove<FTag_GroundNavPath_SearchInFlight>();

        Request.TryFireCompletion(InPathEntity, ECk_Request_OperationResult::Succeeded);

        UUtils_Signal_OnGroundNavPathReady::Broadcast(
            InPathEntity, MakePayload(InPathEntity));
    }

    auto
        FGroundNavPath_Episode::
        DoPublish_Terminal(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current&      InCurrent,
            FFragment_GroundNavPath_Result&       InResult)
        -> void
    {
        const auto& SearchResult = InCurrent._Search.Get_Result();
        const auto Status = SearchResult._Status;

        if (Status == ECk_GroundNav_PathStatus::Ready || Status == ECk_GroundNav_PathStatus::Partial)
        {
            DoPublish_Success(InPathEntity, InParams, InCurrent, InResult);
            return;
        }

        groundnav::Verbose(TEXT("GroundNav Path [{}] found no route from [{}] to [{}]: [{}]"),
            InPathEntity, SearchResult._StartPoint, SearchResult._GoalPoint, Status);

        DoPublish_Failure(
            InPathEntity, InCurrent, InResult,
            Status, SearchResult._ExpansionCount, SearchResult._PlannedAgainstEpoch._Value);
    }

    /**
     * Stands a search up over whichever field covers the start, or leaves the episode parked.
     *
     * A world with no field near the start is treated exactly as Unbuilt rather than given a status of
     * its own: both mean ground nothing is known about, both are worth waiting for, and a consumer that
     * told them apart would have nothing different to do about either.
     */
    auto
        FGroundNavPath_Episode::
        DoTry_Begin(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current&      InCurrent)
        -> void
    {
        using namespace ck_groundnav_path_processor;

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InPathEntity);

        // The request's profile tag rides the lookup, so an episode opened for a tagged walker plans
        // over that walker's field. A tag the world holds no field for is parked, not answered from
        // the untagged one - the same wait an unbuilt start gets, and for the same reason.
        const auto Field = groundnav::world_fields::TryGet_Field(
            World, InCurrent._PendingRequest.Get_From(), InCurrent._PendingRequest.Get_ProfileTag());

        if (NOT Field.IsValid())
        { return; }

        auto Search = groundnav::FCk_GroundNav_PathSearch{};

        const auto Query = Get_Query(InParams, InCurrent._PendingRequest);

        const auto RepairWasAsked =
            InCurrent._PendingRequest.Get_PlanMode() == ECk_GroundNav_PlanMode::Repair;

        // A repair of NOTHING is a cold plan, not a fallback: a warm start seeds the search from the
        // prefix of a corridor that still resolves, and the prefix of no corridor is the source node
        // alone - which is exactly what Request_Begin opens with. Said out loud rather than quietly
        // substituted, because the result's verdict reads None either way and this line is the only
        // thing that separates "asked for cold" from "asked for repair and had nothing to repair".
        const auto CanRepair = RepairWasAsked && NOT InCurrent._LastCorridorKeys.IsEmpty();

        if (RepairWasAsked && NOT CanRepair)
        {
            groundnav::Verbose(
                TEXT("GroundNav Path [{}] asked to repair rev [{}] with no corridor cached - planning cold"),
                InPathEntity, InCurrent._PendingRequest.Get_RequestRevision());
        }

        const auto Status = CanRepair
            ? Search.Request_BeginRepair(
                Field, Query, InCurrent._LastCorridorKeys, InCurrent._LastCorridorEpoch)
            : Search.Request_Begin(Field, Query);

        // Ground the field itself has not baked. The episode stays parked and re-probes next tick,
        // because the volume covering it may still publish.
        if (Status == ECk_GroundNav_PathStatus::Unbuilt)
        { return; }

        InCurrent._Field = Field;
        InCurrent._Search = MoveTemp(Search);
        InCurrent._HasBegun = true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            FFragment_GroundNavPath_Requests& InRequests) const
        -> void
    {
        // Copied and reset BEFORE the drain, not reset after it: publishing a terminal verdict
        // broadcasts from inside this call, and a FindPath enqueued by a signal handler must not be
        // wiped by a reset that happens afterwards.
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        ck::algo::ForEachRequest(RequestsCopy, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InPathEntity, InParams, InCurrent, InResult, InRequest);
            }), policy::DontResetContainer{});

        // LOAD-BEARING, not bookkeeping. CkCrowd's OnGroundNavPathResolved reads a surviving queue as
        // proof that whatever the slot holds predates the request in flight, so a fragment left on the
        // entity makes every published result look stale forever and the agent never leaves Pending.
        if (InRequests._Requests.IsEmpty())
        {
            InPathEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_GroundNavPath_HandleRequests::
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            const FCk_Request_GroundNavPath_FindPath& InRequest)
        -> void
    {
        // No completion guard here, unlike CkVoxelNav's synchronous search: the outcome is frames away,
        // so the delegate rides _PendingRequest and is fired by whoever publishes the terminal result.
        // A replaced episode strands whoever was waiting on it, so that one completes as cancelled.
        InCurrent._PendingRequest.TryFireCompletion(
            InPathEntity, ECk_Request_OperationResult::Failed_Cancelled);

        FGroundNavPath_Episode::DoClear(InCurrent);

        InResult._HasFreshResult = false;

        InCurrent._PendingRequest = InRequest;
        InCurrent._PendingSince = FCk_Time{FPlatformTime::Seconds()};

        InPathEntity.AddOrGet<FTag_GroundNavPath_SearchInFlight>();

        FGroundNavPath_Episode::DoTry_Begin(InPathEntity, InParams, InCurrent);

        if (InCurrent._HasBegun && InCurrent._Search.Get_IsTerminal())
        { FGroundNavPath_Episode::DoPublish_Terminal(InPathEntity, InParams, InCurrent, InResult); }
    }

    auto
        FProcessor_GroundNavPath_HandleRequests::
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            const FCk_Request_GroundNavPath_AbandonPath& InRequest)
        -> void
    {
        auto RequestResult = ECk_Request_OperationResult::Succeeded;
        const auto Guard = MakeCompletionGuard(InRequest, InPathEntity, RequestResult);

        InCurrent._PendingRequest.TryFireCompletion(
            InPathEntity, ECk_Request_OperationResult::Failed_Cancelled);

        FGroundNavPath_Episode::DoClear(InCurrent);

        InPathEntity.Try_Remove<FTag_GroundNavPath_SearchInFlight>();

        // The slot is returned to nothing-planned, which is what a cleared _HasFreshResult means. There
        // is no None in ECk_GroundNav_PathStatus and inventing one here would put a status on the wire
        // that no search can produce; the post-abandon revision is carried so a consumer can still date
        // the slot it is reading.
        InResult._Result
            .Set_Waypoints({})
            .Set_RequestRevision(InRequest.Get_RequestRevision());

        InResult._HasFreshResult = false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_Slice::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        using namespace ck_groundnav_path_processor;

        constexpr auto MinSliceMs = 0.01;

        _SearchesRemainingThisTick = FMath::Max(1, CVarMaxSearchesPerFrame.GetValueOnGameThread());

        _SliceRemainingThisTick = FCk_Time{
            FMath::Max(MinSliceMs, static_cast<double>(CVarSliceBudgetMs.GetValueOnGameThread()))
            / 1000.0};

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_GroundNavPath_Slice::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult) const
        -> void
    {
        using namespace ck_groundnav_path_processor;

        // Both ceilings are read on EVERY entity and an exhausted one stops the pass. A budget that is
        // seeded and never consulted bounds nothing, which is the state CkNavigation's own per-frame
        // query cap is in.
        if (_SearchesRemainingThisTick <= 0 || _SliceRemainingThisTick.Get_Seconds() <= 0.0)
        { return; }

        --_SearchesRemainingThisTick;

        if (NOT InCurrent._HasBegun)
        {
            FGroundNavPath_Episode::DoTry_Begin(InPathEntity, InParams, InCurrent);

            if (NOT InCurrent._HasBegun)
            {
                const auto DeferredForSeconds =
                    FPlatformTime::Seconds() - InCurrent._PendingSince.Get_Seconds();

                const auto MaxDeferralSeconds =
                    static_cast<double>(CVarMaxDeferralSeconds.GetValueOnGameThread());

                if (DeferredForSeconds >= MaxDeferralSeconds)
                {
                    groundnav::Display(
                        TEXT("GroundNav Path [{}] deferral timed out after [{}]s with no field to plan ")
                        TEXT("over at [{}] - failing rev [{}] as Unbuilt"),
                        InPathEntity, DeferredForSeconds, InCurrent._PendingRequest.Get_From(),
                        InCurrent._PendingRequest.Get_RequestRevision());

                    constexpr auto NoExpansions = 0;
                    constexpr auto NoEpoch = int64{0};

                    FGroundNavPath_Episode::DoPublish_Failure(InPathEntity, InCurrent, InResult,
                        ECk_GroundNav_PathStatus::Unbuilt, NoExpansions, NoEpoch);
                }

                return;
            }

            if (InCurrent._Search.Get_IsTerminal())
            {
                FGroundNavPath_Episode::DoPublish_Terminal(InPathEntity, InParams, InCurrent, InResult);
                return;
            }
        }

        auto Slice = groundnav::FCk_GroundNav_PathSliceParams{};

        Slice._MaxIterations = FMath::Max(0, CVarMaxIterationsPerSlice.GetValueOnGameThread());
        Slice._Budget = _SliceRemainingThisTick;

        const auto SliceBeganAt = FPlatformTime::Seconds();

        InCurrent._Search.ContinueSearch(Slice);

        const auto SpentSeconds = FPlatformTime::Seconds() - SliceBeganAt;

        InCurrent._SearchTimeSpent = InCurrent._SearchTimeSpent + FCk_Time{SpentSeconds};

        _SliceRemainingThisTick = FCk_Time{
            FMath::Max(0.0, _SliceRemainingThisTick.Get_Seconds() - SpentSeconds)};

        if (InCurrent._Search.Get_IsTerminal())
        { FGroundNavPath_Episode::DoPublish_Terminal(InPathEntity, InParams, InCurrent, InResult); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            FFragment_GroundNavPath_Current& InCurrent,
            const FFragment_GroundNavPath_Requests& InRequests)
        -> void
    {
        // Two separate populations, and missing either one strands a caller. The QUEUE holds requests
        // the drain never reached; _PendingRequest holds the delegate that has been riding the
        // multi-frame search and is the one a caller is most likely actually waiting on.
        request::FireCancelledForPending(InPathEntity, InRequests.Get_Requests());

        InCurrent._PendingRequest.TryFireCompletion(
            InPathEntity, ECk_Request_OperationResult::Failed_Cancelled);

        // Drops the field snapshot with the entity rather than leaving it to fragment teardown.
        InCurrent._Field.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
