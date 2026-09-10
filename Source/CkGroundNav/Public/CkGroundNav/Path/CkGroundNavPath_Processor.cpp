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
#include "CkGroundNav/Field/CkGroundNav_FieldMarkupCost.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Search/CkGroundNav_FilterCompile.h"
#include "CkGroundNav/Search/CkGroundNav_PathPostProcess.h"

#include "HAL/PlatformTime.h"

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

    static TAutoConsoleVariable<int32> CVarSliceServiceWindow(
        TEXT("ck.GroundNav.Debug.SliceServiceWindow"),
        0,
        TEXT("1 logs one world-scoped GroundNav slice-service accumulator at start and at most once per second. "
             "Diagnostic only; default 0."),
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

    /** One synchronous paired replay per world, only after the Crowd watchdog has timed the live
     *  episode out. It stays separate from the timeout-state switch because it spends extra work. */
    static TAutoConsoleVariable<int32> CVarStrictCrowdCostTimeoutReplay(
        TEXT("ck.GroundNav.Debug.StrictCrowdCostTimeoutReplay"),
        0,
        TEXT("1 retains one active GroundNav query for a paired Crowd timeout replay per world. "
             "Requires ck.Crowd.Debug.PendingTimeoutState=1. Diagnostic only; default 0."),
        ECVF_Default);

    constexpr auto StrictCrowdCostTimeoutReplayIterationCap = 50'000;
    constexpr auto StrictCrowdCostTimeoutReplaySliceIterations = 1'024;

    auto Get_ShouldCaptureStrictCrowdCostTimeoutReplay() -> bool
    {
        return CVarStrictCrowdCostTimeoutReplay.GetValueOnGameThread() != 0;
    }

    auto Get_ShouldLogSliceServiceWindow() -> bool
    {
        return CVarSliceServiceWindow.GetValueOnGameThread() != 0;
    }

    auto Get_StrictCrowdCostTimeoutReplayWorlds() -> TSet<TWeakObjectPtr<UWorld>>&
    {
        static auto Worlds = TSet<TWeakObjectPtr<UWorld>>{};
        return Worlds;
    }

    // What the corridor box is grown by beyond the body's own radius lives in the fragment header,
    // because the invalidator grows an in-flight search's request bounds by the same number:
    // ck::kCorridorInflationMarginUu.

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
        Cost._ShortcutSpanCap = InParams.Get_ShortcutSpanCap();

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
            const FCk_GroundNav_FieldPtr&             InField,
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
        Query._DynamicObstacles = InRequest.Get_DynamicObstacles();

        // The request's filter, compiled once per (field snapshot, tag, overlay): its excluded areas
        // become plates this search may not enter, and its per-area multipliers become the price of
        // the plates carrying them. Assigned rather than merged for the reason Get_CostParams states
        // - it leaves both tables empty, so the filter's answer IS the query's answer.
        const auto& FilterTables = Get_CompiledFilterTables(
            InField, InRequest.Get_QueryFilter(), InRequest.Get_QueryFilterOverlay());

        Query._Cost._PlateCostMultipliers = FilterTables._Multipliers;
        Query._Cost._DeniedPlates = FilterTables._Denied;

        return Query;
    }

    /** The agent location the post-process drops the first waypoint against is the request's own From:
     *  the path entity carries no transform of its own, and From is where the caller said the body was
     *  when it asked.
     *
     *  The post-process prices and shortcuts under the SAME filter the search routed under. Without
     *  that the shortcut's chord would be judged against unfiltered ground and could cut straight
     *  across the very plates the corridor went round. */
    auto
        Get_PostParams(
            const ck::FFragment_GroundNavPath_Params& InParams,
            const FCk_GroundNav_FieldPtr&             InField,
            const FCk_Request_GroundNavPath_FindPath& InRequest,
            const FVector&                            InAgentLocation)
        -> FCk_GroundNav_PathPostParams
    {
        auto PostParams = FCk_GroundNav_PathPostParams{};

        PostParams._Agent._RadiusUu = InParams.Get_AgentRadiusUu();
        PostParams._VerticalToleranceUu = InParams.Get_VerticalToleranceUu();
        PostParams._AgentLocation = InAgentLocation;
        PostParams._Cost = Get_CostParams(InParams);

        const auto& FilterTables = Get_CompiledFilterTables(
            InField, InRequest.Get_QueryFilter(), InRequest.Get_QueryFilterOverlay());

        PostParams._Cost._PlateCostMultipliers = FilterTables._Multipliers;
        PostParams._Cost._DeniedPlates = FilterTables._Denied;

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

        if (InResult._RouteKind == ECk_GroundNav_PathRouteKind::StrictCell)
        {
            for (const auto& Edge : InResult._CellRoute)
            {
                if (Edge._Kind == ECk_GroundNav_CellRouteEdgeKind::Link &&
                    Edge._LinkStableId != INDEX_NONE)
                { LinkIds.AddUnique(Edge._LinkStableId); }
            }
            return LinkIds;
        }

        for (const auto& Crossing : InResult._Crossings)
        {
            if (NOT InField._ResolvedLinks.IsValidIndex(Crossing._LinkIndex))
            { continue; }

            LinkIds.AddUnique(InField._ResolvedLinks[Crossing._LinkIndex]._Id);
        }

        return LinkIds;
    }

    /** A strict cell route has no portal corridor, so derive the invalidator's plate set from every
     *  endpoint it actually traversed. Both ends matter for a seam or link edge. */
    auto
        Get_CorridorFlatPlates(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_PathResult& InResult)
        -> TArray<int32>
    {
        if (InResult._RouteKind != ECk_GroundNav_PathRouteKind::StrictCell)
        { return InResult._PlateCorridor; }

        auto Plates = TArray<int32>{};
        const auto AddSurfacePlate = [&InField, &Plates](const FCk_GroundNav_SurfaceRef& InSurface) -> void
        {
            const auto FlatPlate = Get_FlatPlateIndex(
                InField, InSurface._TileIndex, InSurface._PlateIndex);
            if (FlatPlate != INDEX_NONE)
            { Plates.AddUnique(FlatPlate); }
        };
        for (const auto& Edge : InResult._CellRoute)
        {
            AddSurfacePlate(Edge._FromSurface);
            AddSurfacePlate(Edge._ToSurface);
        }
        return Plates;
    }

    /**
     * Names every fixed-lattice tile the completed search route actually traverses.
     *
     * The result's surface references belong to the pinned field, so resolve flat plates against that
     * same field. The current publication is consulted only after these stable tile indices have been
     * recovered; comparing graph-local plate ids directly across publications would be invalid.
     */
    auto
        TryGet_RouteTileIndices(
            const FCk_GroundNav_Field&      InPinnedField,
            const FCk_GroundNav_PathResult& InResult,
            TSet<int32>&                    OutTileIndices)
        -> bool
    {
        OutTileIndices.Reset();

        const auto TryAddSurface = [&InPinnedField, &OutTileIndices](
            const FCk_GroundNav_SurfaceRef& InSurface) -> bool
        {
            if (NOT InPinnedField._Tiles.IsValidIndex(InSurface._TileIndex))
            { return false; }

            OutTileIndices.Add(InSurface._TileIndex);
            return true;
        };

        if (NOT TryAddSurface(InResult._StartSurface) || NOT TryAddSurface(InResult._GoalSurface))
        { return false; }

        if (InResult._RouteKind == ECk_GroundNav_PathRouteKind::StrictCell)
        {
            for (const auto& Edge : InResult._CellRoute)
            {
                if (NOT TryAddSurface(Edge._FromSurface) || NOT TryAddSurface(Edge._ToSurface))
                { return false; }
            }

            return true;
        }

        for (const auto FlatPlate : InResult._PlateCorridor)
        {
            auto TileIndex = int32{INDEX_NONE};
            auto PlateIndex = int32{INDEX_NONE};
            if (NOT Get_TileAndPlate(InPinnedField, FlatPlate, TileIndex, PlateIndex))
            { return false; }

            OutTileIndices.Add(TileIndex);
        }

        return true;
    }

    /** A newer geometry publish may remove route ground while a sliced search still reads its pin. */
    auto
        Get_RouteRemainsBuilt(
            const FCk_GroundNav_Field&      InPinnedField,
            const FCk_GroundNav_PathResult& InResult,
            const FCk_GroundNav_Field&      InCurrentField)
        -> bool
    {
        auto RouteTileIndices = TSet<int32>{};
        if (NOT TryGet_RouteTileIndices(InPinnedField, InResult, RouteTileIndices))
        { return false; }

        for (const auto TileIndex : RouteTileIndices)
        {
            if (NOT InCurrentField._Tiles.IsValidIndex(TileIndex) ||
                InCurrentField._Tiles[TileIndex]._Coord != InPinnedField._Tiles[TileIndex]._Coord ||
                NOT InCurrentField._Tiles[TileIndex].Get_IsBuilt())
            { return false; }
        }

        return true;
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

        for (const auto FlatPlate : Get_CorridorFlatPlates(InField, InResult))
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
        InCurrent._ActiveQueryForTimeoutReplay.Reset();
        InCurrent._PendingRequest = FCk_Request_GroundNavPath_FindPath{};
        InCurrent._HasBegun = false;
        InCurrent._PendingSince = FCk_Time{};
        InCurrent._SearchTimeSpent = FCk_Time{};
        InCurrent._HasSearchDuration = false;
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
            .Set_HasSearchDuration(InCurrent._HasSearchDuration)
            .Set_PlannedAgainstEpoch(InPlannedAgainstEpoch)
            .Set_RepairVerdict(InCurrent._Search.Get_RepairVerdict());

        InResult._HasFreshResult = true;
        ++InResult._PublishSequence;

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

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InPathEntity);
        const auto Current = groundnav::world_fields::TryGet_FieldSnapshot(
            World, Request.Get_From(), Request.Get_ProfileTag());
        if (NOT Current.IsSet() ||
            (Current->_PublishNote._LastGeometryEpoch.Get_IsNewerThan(SearchResult._PlannedAgainstEpoch) &&
                NOT Get_RouteRemainsBuilt(*InCurrent._Field, SearchResult, *Current->_Field)))
        {
            DoPublish_Failure(InPathEntity, InCurrent, InResult, ECk_GroundNav_PathStatus::Unbuilt,
                SearchResult._ExpansionCount, SearchResult._PlannedAgainstEpoch._Value);
            return;
        }

        const auto SearchDurationMs =
            static_cast<float>(InCurrent._SearchTimeSpent.Get_Milliseconds());

        const auto Plan = groundnav::Get_PathPlan(
            SearchResult,
            *InCurrent._Field,
            Get_PostParams(InParams, InCurrent._Field, Request, Request.Get_From()));

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
            .Set_HasSearchDuration(InCurrent._HasSearchDuration)
            .Set_PlannedAgainstEpoch(Plan._PlannedAgainstEpoch._Value)
            .Set_RepairVerdict(InCurrent._Search.Get_RepairVerdict());

        InResult._HasFreshResult = true;
        ++InResult._PublishSequence;

        DoLog_Published(InPathEntity, InResult._Result);

        const auto CorridorInflationUu = InParams.Get_AgentRadiusUu() + kCorridorInflationMarginUu;

        const auto CorridorBounds =
            Get_CorridorBounds(*InCurrent._Field, SearchResult, CorridorInflationUu);

        InCurrent._LastCorridorKeys = Get_CorridorKeys(SearchResult);
        InCurrent._HasCachedRoute = true;
        InCurrent._LastRouteKind = SearchResult._RouteKind;
        InCurrent._LastCorridorLinkIds = Get_CorridorLinkIds(*InCurrent._Field, SearchResult);
        InCurrent._LastCorridorFlatPlates = Get_CorridorFlatPlates(*InCurrent._Field, SearchResult);
        InCurrent._LastCorridorEpoch = SearchResult._PlannedAgainstEpoch;
        InCurrent._ProfileTag = Request.Get_ProfileTag();
        InCurrent._LastCorridorQueryFilter = Request.Get_QueryFilter();
        InCurrent._LastCorridorQueryFilterOverlay = Request.Get_QueryFilterOverlay();
        InCurrent._LastCorridorBounds = CorridorBounds;
        InCurrent._CorridorInflationUu = CorridorBounds.IsValid != 0 ? CorridorInflationUu : 0.0f;
        InCurrent._LastSourceFlatPlate = InCurrent._LastCorridorFlatPlates.IsEmpty()
            ? INDEX_NONE
            : InCurrent._LastCorridorFlatPlates[0];

        // A rebuild that landed while this search was in flight is ground the search never read: the
        // field snapshot was pinned at Request_Begin. The route publishes anyway - a half-answered
        // episode is worth nothing to the consumer - and is flagged here so the next tick re-plans it
        // once against the field as it now is. HERE and not in the invalidator, because when that
        // rebuild arrived this agent held no corridor for it to measure.
        //
        // AT MOST ONCE PER AGENT LIFETIME, and the corridor above is what holds that - not a flag.
        // DoTry_ArmInFlightSearch is reached only while this slot holds no corridor, and
        // _LastCorridorBounds, set a few lines above, never goes invalid again once a route has
        // published: the corridor half of the invalidator owns every rebuild from here on.
        //
        // SUCCESS only. A terminal failure has no route to re-plan and the crowd retries on its own,
        // so DoPublish_Failure leaves the flag standing for the next request's drain to clear.
        if (InResult.Get_RebuiltWhileInFlight())
        {
            InResult._RebuiltWhileInFlight = false;

            InPathEntity.AddOrGet<FTag_GroundNavPath_RepathRequired>();

            groundnav::Verbose(
                TEXT("GroundNav Path [{}] flagged for repath: a surface rebuild landed while this ")
                TEXT("search was in flight, so the route it just published was planned over ground ")
                TEXT("that has since moved"),
                InPathEntity);
        }

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

        const auto Query = Get_Query(InParams, Field, InCurrent._PendingRequest);

        if (Get_ShouldCaptureStrictCrowdCostTimeoutReplay())
        { InCurrent._ActiveQueryForTimeoutReplay = Query; }
        else
        { InCurrent._ActiveQueryForTimeoutReplay.Reset(); }

        const auto RepairWasAsked =
            InCurrent._PendingRequest.Get_PlanMode() == ECk_GroundNav_PlanMode::Repair;

        // A repair of NOTHING is a cold plan, not a fallback: a warm start seeds the search from the
        // prefix of a corridor that still resolves, and the prefix of no corridor is the source node
        // alone - which is exactly what Request_Begin opens with. Said out loud rather than quietly
        // substituted, because the result's verdict reads None either way and this line is the only
        // thing that separates "asked for cold" from "asked for repair and had nothing to repair".
        const auto HasStrictCellRoute = RepairWasAsked && InCurrent._HasCachedRoute &&
            InCurrent._LastRouteKind == groundnav::ECk_GroundNav_PathRouteKind::StrictCell;
        const auto CanRepair = RepairWasAsked && NOT InCurrent._LastCorridorKeys.IsEmpty();

        if (RepairWasAsked && NOT CanRepair && NOT HasStrictCellRoute)
        {
            groundnav::Verbose(
                TEXT("GroundNav Path [{}] asked to repair rev [{}] with no corridor cached - planning cold"),
                InPathEntity, InCurrent._PendingRequest.Get_RequestRevision());
        }

        const auto BeginBeganAt = FPlatformTime::Seconds();

        const auto Status = CanRepair || HasStrictCellRoute
            ? Search.Request_BeginRepair(
                Field, Query, InCurrent._LastCorridorKeys, InCurrent._LastCorridorEpoch, HasStrictCellRoute)
            : Search.Request_Begin(Field, Query);

        // Ground the field itself has not baked. The episode stays parked and re-probes next tick,
        // because the volume covering it may still publish.
        if (Status == ECk_GroundNav_PathStatus::Unbuilt)
        { return; }

        InCurrent._SearchTimeSpent = InCurrent._SearchTimeSpent + FCk_Time{FPlatformTime::Seconds() - BeginBeganAt};
        InCurrent._HasSearchDuration = true;

        InCurrent._Field = Field;
        InCurrent._Search = MoveTemp(Search);
        InCurrent._HasBegun = true;
    }

    auto
        FFragment_GroundNavPath_Current::
        Try_RunStrictCrowdCostTimeoutReplay(
            UWorld*             InWorld,
            const FGameplayTag& InCrowdCostAreaTag,
            int32               InServedExpansionCount) const
        -> void
    {
        using namespace ck_groundnav_path_processor;

        const auto HasReplayInput = _Field.IsValid() && _ActiveQueryForTimeoutReplay.IsSet();
        const auto IsStrict = HasReplayInput && NOT _ActiveQueryForTimeoutReplay->_DynamicObstacles.Get_IsEmpty();

        if (NOT Get_ShouldCaptureStrictCrowdCostTimeoutReplay() || NOT IsStrict ||
            InServedExpansionCount <= 0 || NOT IsValid(InWorld))
        { return; }

        auto& ReplayedWorlds = Get_StrictCrowdCostTimeoutReplayWorlds();
        for (auto WorldIt = ReplayedWorlds.CreateIterator(); WorldIt; ++WorldIt)
        {
            if (NOT WorldIt->IsValid())
            { WorldIt.RemoveCurrent(); }
        }

        const auto WorldKey = TWeakObjectPtr<UWorld>{InWorld};
        if (ReplayedWorlds.Contains(WorldKey))
        { return; }

        auto CostIdentityMarkups = _Field->_Params._MarkupRecords;
        auto CrowdCostRecordCount = 0;

        for (auto& Markup : CostIdentityMarkups)
        {
            const auto IsCrowdCostRecord = Markup.Get_AreaTag() == InCrowdCostAreaTag &&
                Markup.Get_Kind() == ECk_GroundNav_MarkupKind::Cost;

            if (NOT IsCrowdCostRecord)
            { continue; }

            // Keep its area identity, shape and enable state. Only suppress this policy's field cost.
            Markup.Set_CostMultiplier(1.0f);
            ++CrowdCostRecordCount;
        }

        const auto CostIdentityField = groundnav::Get_FieldWithMarkupCost(
            *_Field, CostIdentityMarkups, _Field->_Epoch);
        const auto HasCostIdentityField =
            CostIdentityField.Key.IsValid() && CostIdentityField.Value.Get_IsCompleted();

        CK_ENSURE_IF_NOT(HasCostIdentityField,
            TEXT("GroundNav strict Crowd timeout replay could not derive its cost-only comparison field"))
        {
        }

        if (NOT HasCostIdentityField)
        { return; }

        // Claim only after every immutable replay input exists. A failed derive leaves no half-capture.
        ReplayedWorlds.Add(WorldKey);

        struct FReplayRow
        {
            ECk_GroundNav_PathStatus _Status = ECk_GroundNav_PathStatus::InProgress;
            bool _IsTerminal = false;
            int32 _ExpansionCount = 0;
            float _SearchCost = 0.0f;
        };

        const auto RunReplay = [this](const groundnav::FCk_GroundNav_FieldPtr& InReplayField) -> FReplayRow
        {
            auto Search = groundnav::FCk_GroundNav_PathSearch{};
            Search.Request_Begin(InReplayField, _ActiveQueryForTimeoutReplay.GetValue());

            auto IterationAllowance = StrictCrowdCostTimeoutReplayIterationCap;
            auto Slice = groundnav::FCk_GroundNav_PathSliceParams{};
            // Zero is the path-search contract for no wall-clock ceiling. The only replay ceiling is
            // the explicit iteration cap below, so a capped row cannot hide a second time budget.
            Slice._Budget = FCk_Time{};

            while (NOT Search.Get_IsTerminal() && IterationAllowance > 0)
            {
                Slice._MaxIterations = FMath::Min(StrictCrowdCostTimeoutReplaySliceIterations, IterationAllowance);
                Search.ContinueSearch(Slice);
                IterationAllowance -= Slice._MaxIterations;
            }

            auto Row = FReplayRow{};
            Row._Status = Search.Get_Status();
            Row._IsTerminal = Search.Get_IsTerminal();

            // A cap is an explicit incomplete diagnostic result. Get_Result is terminal-only here.
            if (Row._IsTerminal)
            {
                const auto& Result = Search.Get_Result();
                Row._ExpansionCount = Result._ExpansionCount;
                Row._SearchCost = Result._SearchCost;
            }

            return Row;
        };

        const auto Exact = RunReplay(_Field);
        const auto CostIdentity = RunReplay(CostIdentityField.Key);
        const auto& Query = _ActiveQueryForTimeoutReplay.GetValue();
        auto MaxFilterMultiplier = 1.0f;

        for (const auto& Pair : Query._Cost._PlateCostMultipliers)
        { MaxFilterMultiplier = FMath::Max(MaxFilterMultiplier, Pair.Value); }

        groundnav::Display(
            TEXT("[GROUNDNAV-STRICT-CROWD-COST-REPLAY] exact terminal [{}] capped [{}] status [{}] "
                 "expansions [{}] cost [{}] start [{}] goal [{}] radius [{}] verticalTolerance [{}] greedyW [{}] "
                 "maxExpansions [{}] maxCorridor [{}] partial [{}] discs [{}] obbs [{}] fieldEpoch [{}] "
                 "markupRecords [{}] crowdCostRecords [{}] filterMultipliers [{}] filterMultiplierMax [{}] deniedPlates [{}] replayIterationCap [{}]"),
            Exact._IsTerminal, NOT Exact._IsTerminal, Exact._Status,
            Exact._IsTerminal ? Exact._ExpansionCount : INDEX_NONE,
            Exact._IsTerminal ? Exact._SearchCost : -1.0f,
            Query._Start, Query._Goal, Query._Agent._RadiusUu, Query._VerticalToleranceUu, Query._GreedyWeightW,
            Query._MaxExpansions, Query._MaxCorridorLength, Query._AllowPartialPath,
            Query._DynamicObstacles.Get_Discs().Num(), Query._DynamicObstacles.Get_Obbs().Num(), _Field->_Epoch._Value,
            _Field->_Params._MarkupRecords.Num(), CrowdCostRecordCount,
            Query._Cost._PlateCostMultipliers.Num(), MaxFilterMultiplier, Query._Cost._DeniedPlates.Num(),
            StrictCrowdCostTimeoutReplayIterationCap);

        groundnav::Display(
            TEXT("[GROUNDNAV-STRICT-CROWD-COST-REPLAY] crowd-cost-identity terminal [{}] capped [{}] status [{}] "
                 "expansions [{}] cost [{}] start [{}] goal [{}] radius [{}] verticalTolerance [{}] greedyW [{}] "
                 "maxExpansions [{}] maxCorridor [{}] partial [{}] discs [{}] obbs [{}] fieldEpoch [{}] "
                 "markupRecords [{}] crowdCostRecords [{}] filterMultipliers [{}] filterMultiplierMax [{}] deniedPlates [{}] replayIterationCap [{}]"),
            CostIdentity._IsTerminal, NOT CostIdentity._IsTerminal, CostIdentity._Status,
            CostIdentity._IsTerminal ? CostIdentity._ExpansionCount : INDEX_NONE,
            CostIdentity._IsTerminal ? CostIdentity._SearchCost : -1.0f,
            Query._Start, Query._Goal, Query._Agent._RadiusUu, Query._VerticalToleranceUu, Query._GreedyWeightW,
            Query._MaxExpansions, Query._MaxCorridorLength, Query._AllowPartialPath,
            Query._DynamicObstacles.Get_Discs().Num(), Query._DynamicObstacles.Get_Obbs().Num(), CostIdentityField.Key->_Epoch._Value,
            CostIdentityField.Key->_Params._MarkupRecords.Num(), CrowdCostRecordCount,
            Query._Cost._PlateCostMultipliers.Num(), MaxFilterMultiplier, Query._Cost._DeniedPlates.Num(),
            StrictCrowdCostTimeoutReplayIterationCap);
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

        // The news belonged to the episode being replaced. A plan about to be made against the field
        // as it is published NOW owes nothing to a rebuild that moved ground under the last one.
        InResult._RebuiltWhileInFlight = false;

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

        // Abandoned with nothing to publish, so the rebuild this episode was carrying has no route
        // left to re-plan.
        InResult._RebuiltWhileInFlight = false;
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
        const auto ShouldLogServiceWindow = Get_ShouldLogSliceServiceWindow();
        const auto NowSeconds = ShouldLogServiceWindow ? FPlatformTime::Seconds() : 0.0;
        if (NOT ShouldLogServiceWindow)
        {
            if (_SliceServiceWindow._IsActive)
            { _SliceServiceWindow = FSliceServiceWindow{}; }
        }
        else if (NOT _SliceServiceWindow._IsActive)
        {
            _SliceServiceWindow._IsActive = true;
            _SliceServiceWindow._StartedAtSeconds = NowSeconds;
            _SliceServiceWindow._LastLoggedAtSeconds = NowSeconds - 1.0;
            _SliceServiceWindow._StartedAtFrame = GFrameCounter;
        }
        if (ShouldLogServiceWindow) { ++_SliceServiceWindow._TickCount; }

        _SearchesRemainingThisTick = FMath::Max(1, CVarMaxSearchesPerFrame.GetValueOnGameThread());

        _SliceRemainingThisTick = FCk_Time{
            FMath::Max(MinSliceMs, static_cast<double>(CVarSliceBudgetMs.GetValueOnGameThread()))
            / 1000.0};

        // A saved path is only a turn marker. It owns no lifetime: terminal publishes, abandons and
        // destruction may all remove it from this view between ticks. Starting from the head in that
        // case is the only deterministic answer; holding a stale cursor would skip the whole pass.
        if (NOT ck::IsValid(_NextPathToServe)
            || NOT _NextPathToServe.Has<FTag_GroundNavPath_SearchInFlight>()
            || _NextPathToServe.Has<FTag_DestroyEntity_Initiate>())
        { _NextPathToServe = {}; }

        _WaitingForNextPathThisTick = ck::IsValid(_NextPathToServe);
        _FoundNextPathThisTick = false;

        TProcessor::DoTick(InDeltaT);

        // The target was live when the tick began but disappeared before the base traversal reached
        // it. Do not carry a dead/superseded turn marker into the next frame.
        if (_WaitingForNextPathThisTick && NOT _FoundNextPathThisTick)
        { _NextPathToServe = {}; }

        if (ShouldLogServiceWindow)
        {
            _SliceServiceWindow._TickTimeSpent = _SliceServiceWindow._TickTimeSpent +
                FCk_Time{FPlatformTime::Seconds() - NowSeconds};
            if (_SliceRemainingThisTick.Get_Seconds() <= 0.0) { ++_SliceServiceWindow._TimeBudgetExhaustedTicks; }
            if (_SearchesRemainingThisTick <= 0) { ++_SliceServiceWindow._SearchCapExhaustedTicks; }
            if (NowSeconds - _SliceServiceWindow._LastLoggedAtSeconds >= 1.0)
            {
                groundnav::Display(
                    TEXT("[GROUNDNAV-SLICE-SERVICE] world [{}] elapsedWall [{}] frameDelta [{}] ticks [{}] served [{}] "
                         "tickMs [{}] searchMs [{}] timeCapTicks [{}] searchCapTicks [{}]"),
                    _SliceServiceWindow._WorldName,
                    NowSeconds - _SliceServiceWindow._StartedAtSeconds,
                    GFrameCounter - _SliceServiceWindow._StartedAtFrame,
                    _SliceServiceWindow._TickCount, _SliceServiceWindow._ServedSearchCount,
                    _SliceServiceWindow._TickTimeSpent.Get_Milliseconds(),
                    _SliceServiceWindow._SearchTimeSpent.Get_Milliseconds(),
                    _SliceServiceWindow._TimeBudgetExhaustedTicks, _SliceServiceWindow._SearchCapExhaustedTicks);
                _SliceServiceWindow._LastLoggedAtSeconds = NowSeconds;
            }
        }
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

        // The view itself admits only live, in-flight paths. Entries skipped before the saved turn
        // marker consume neither ceiling; this is rotation, not a hidden reduction of the budget.
        if (_WaitingForNextPathThisTick)
        {
            if (InPathEntity != _NextPathToServe)
            { return; }

            _WaitingForNextPathThisTick = false;
            _FoundNextPathThisTick = true;
            _NextPathToServe = {};
        }

        // Both ceilings are read on every ELIGIBLE entity after the turn marker. Once one is spent,
        // remember the first following eligible path for the next tick, then leave the rest untouched.
        // This makes the cursor follow the base view's real order rather than assuming entity numbers
        // or retaining a raw registry pointer.
        if (_SearchesRemainingThisTick <= 0 || _SliceRemainingThisTick.Get_Seconds() <= 0.0)
        {
            if (NOT ck::IsValid(_NextPathToServe))
            { _NextPathToServe = InPathEntity; }
            return;
        }

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

        if (_SliceServiceWindow._IsActive)
        {
            ++_SliceServiceWindow._ServedSearchCount;
            _SliceServiceWindow._SearchTimeSpent = _SliceServiceWindow._SearchTimeSpent + FCk_Time{SpentSeconds};
            if (_SliceServiceWindow._WorldName == TEXT("<unserved>"))
            {
                if (const auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InPathEntity))
                { _SliceServiceWindow._WorldName = World->GetName(); }
            }
        }

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
