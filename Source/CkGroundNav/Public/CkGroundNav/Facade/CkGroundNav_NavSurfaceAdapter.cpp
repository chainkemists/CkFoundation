#include "CkGroundNav/Facade/CkGroundNav_NavSurfaceAdapter.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"
#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Debug/CkGroundNav_DebugGates.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldMarkupCost.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Query/CkGroundNav_Query_DynamicObstacles.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Boundary.h"
#include "CkGroundNav/Query/CkGroundNav_Query_BuildStatus.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.h"
#include "CkGroundNav/Search/CkGroundNav_FilterCompile.h"
#include "CkGroundNav/Search/CkGroundNav_PathPostProcess.h"
#include "CkGroundNav/Search/CkGroundNav_PathSearch.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"
#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include <Engine/World.h>
#include <HAL/IConsoleManager.h>
#include <UObject/Class.h>
#include <UObject/PropertyPortFlags.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::nav_surface_adapter_private
{
    static TAutoConsoleVariable<int32> CVar_MarkupLiveDiagnostics(
        TEXT("ck.GroundNav.Debug.MarkupLiveDiagnostics"), 0,
        TEXT("1 writes visible GroundNav markup-liveness gate diagnostics. It does not change the "
             "provider answer or retain markup state."));

    auto Get_ShouldLogMarkupLiveDiagnostics() -> bool
    {
        return CVar_MarkupLiveDiagnostics.GetValueOnAnyThread() != 0;
    }

    // The neutral queries opt into the project extents by carrying a zero vector. GroundNav's own
    // queries have no such sentinel, so the fold happens here — once, in one place, so the projection
    // and the boundary window cannot drift apart about what "unset" means.
    auto Get_HorizontalExtentUu(
        const FVector& InSearchHalfExtents) -> float
    {
        return InSearchHalfExtents.IsNearlyZero()
            ? UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent()
            : static_cast<float>(InSearchHalfExtents.X);
    }

    auto Get_VerticalExtentUu(
        const FVector& InSearchHalfExtents) -> float
    {
        return InSearchHalfExtents.IsNearlyZero()
            ? static_cast<float>(UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec().Z)
            : static_cast<float>(InSearchHalfExtents.Z);
    }

    // The tolerance a walk, a raycast and a reachability query resolve their ENDS with. The neutral
    // shapes carry no extent of their own, so the project's vertical reach is the only answer.
    auto Get_VerticalToleranceUu() -> float
    {
        return static_cast<float>(UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec().Z);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto Get_MappedReachability(
        const FCk_GroundNav_ReachabilityResult& InResult) -> ECk_NavSurface_Reachability
    {
        switch (InResult._Status)
        {
            case ECk_NavSurface_QueryStatus::NoSurface:
            {
                // Nowhere to stand at one end. Nothing can walk to a place that is not ground, and the
                // field is BUILT there, so this is a verdict rather than an absence of one.
                return ECk_NavSurface_Reachability::Unreachable;
            }
            case ECk_NavSurface_QueryStatus::Unbuilt:
            case ECk_NavSurface_QueryStatus::Blocked:
            {
                return ECk_NavSurface_Reachability::Unknown_ProviderNotReady;
            }
            case ECk_NavSurface_QueryStatus::Success:
            {
                switch (InResult._Reachability)
                {
                    case ECk_GroundNav_Reachability::PossiblyReachable:
                    {
                        return ECk_NavSurface_Reachability::Reachable;
                    }
                    case ECk_GroundNav_Reachability::Unreachable:
                    {
                        return ECk_NavSurface_Reachability::Unreachable;
                    }
                    default:
                    {
                        return ECk_NavSurface_Reachability::Unknown_ProviderNotReady;
                    }
                }
            }
            default:
            {
                return ECk_NavSurface_Reachability::Unknown_ProviderNotReady;
            }
        }
    }

    /**
     * The map from a search's own vocabulary onto the neutral one.
     *
     * BudgetExceeded folds into Blocked rather than into a status of its own: a search that ran out of
     * budget answered with no corridor, which is the same thing a consumer must do about it as a
     * search that found none. Unbuilt stays apart from NoSurface here as everywhere - a consumer waits
     * on the first and gives up on the second.
     */
    auto Get_MappedPathStatus(
        ECk_GroundNav_PathStatus InStatus,
        bool                     InAllowPartial) -> ECk_NavSurface_QueryStatus
    {
        switch (InStatus)
        {
            case ECk_GroundNav_PathStatus::Ready:
            {
                return ECk_NavSurface_QueryStatus::Success;
            }
            case ECk_GroundNav_PathStatus::Partial:
            {
                // A partial answer nobody asked for is not a shorter route, it is no route - so it is
                // refused rather than handed over as though it reached the goal.
                return InAllowPartial
                    ? ECk_NavSurface_QueryStatus::Success
                    : ECk_NavSurface_QueryStatus::Blocked;
            }
            case ECk_GroundNav_PathStatus::Unbuilt:
            {
                return ECk_NavSurface_QueryStatus::Unbuilt;
            }
            case ECk_GroundNav_PathStatus::NoStartSurface:
            case ECk_GroundNav_PathStatus::NoGoalSurface:
            {
                return ECk_NavSurface_QueryStatus::NoSurface;
            }
            case ECk_GroundNav_PathStatus::Unreachable:
            case ECk_GroundNav_PathStatus::BudgetExceeded:
            case ECk_GroundNav_PathStatus::Blocked:
            default:
            {
                return ECk_NavSurface_QueryStatus::Blocked;
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto Do_ProjectPoint(
        UWorld*                                InWorld,
        const FCk_NavSurface_ProjectionQuery&  InQuery) -> FCk_NavSurface_ProjectionResult
    {
        auto Result = FCk_NavSurface_ProjectionResult{};

        // The profile tag rides every one of these reads. An empty one is the volume's untagged
        // default, and a tag the volume authored no variant for is answered by no field at all - the
        // status below - rather than by the default's ground, which the named profile cannot walk.
        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Location(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto VerticalExtentUu = Get_VerticalExtentUu(InQuery.Get_SearchHalfExtents());

        auto Query = FCk_GroundNav_ProjectionQuery{};
        Query._Location = InQuery.Get_Location();
        Query._HorizontalExtentUu = Get_HorizontalExtentUu(InQuery.Get_SearchHalfExtents());
        Query._UpExtentUu = VerticalExtentUu;
        Query._DownExtentUu = VerticalExtentUu;
        Query._Mode = InQuery.Get_Mode();

        // _QueryFilter and _QueryFilterOverlay are unread, and unlike the raycast beneath this there
        // is nothing here that COULD read them: FCk_GroundNav_ProjectionQuery carries no cost table
        // and no cost cap, so a projection has no channel a filter could act through at all. The
        // raycast has one (_PlateCostMultipliers / _MaxCost) and is only missing the tag-to-plate
        // translation; this one is missing the input.

        const auto GroundResult = Get_ProjectPoint(*Field, Query);

        Result.Set_Status(GroundResult._Status);
        Result.Set_Location(GroundResult._Location);
        Result.Set_SurfaceNormal(GroundResult._SurfaceNormal);

        // Area tags stay EMPTY: a GroundNav projection result carries none.

        return Result;
    }

    auto Do_MoveAlongSurface(
        UWorld*                                        InWorld,
        const FCk_NavSurface_MoveAlongSurfaceQuery&    InQuery) -> FCk_NavSurface_MoveAlongSurfaceResult
    {
        auto Result = FCk_NavSurface_MoveAlongSurfaceResult{};

        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Start(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        auto Query = FCk_GroundNav_SurfaceWalkQuery{};
        Query._Start = InQuery.Get_Start();
        Query._Target = InQuery.Get_End();
        Query._StartVerticalToleranceUu = Get_VerticalToleranceUu();

        auto Diagnostics = FCk_GroundNav_SurfaceWalkDiagnostics{};

        const auto WalkResult = Get_MoveAlongSurface(*Field, Query, Diagnostics);

        Result.Set_Status(WalkResult._Status);
        Result.Set_ReachedLocation(WalkResult._Location);

        return Result;
    }

    auto Do_SurfaceRaycast(
        UWorld*                             InWorld,
        const FCk_NavSurface_RaycastQuery&  InQuery) -> FCk_NavSurface_RaycastResult
    {
        auto Result = FCk_NavSurface_RaycastResult{};

        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Start(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        auto Query = FCk_GroundNav_RaycastQuery{};
        Query._Start = InQuery.Get_Start();
        Query._End = InQuery.Get_End();
        Query._StartVerticalToleranceUu = Get_VerticalToleranceUu();
        Query._MaxCost = InQuery.Get_MaxCost();

        // The cap is a COST cap, and the cost it caps has to be the one the field was authored with -
        // otherwise every plate weighs 1.0, the cap degenerates into a distance cap, and a query
        // capped to refuse expensive ground admits it. Unconditional rather than gated on the cap
        // being set: with no cap the accumulation is never compared against anything, so this changes
        // no verdict a query without a cap could observe.
        Query._UseBakedPlateCost = true;

        // The filter, compiled once per (field snapshot, tag, overlay): its excluded areas become
        // ground this ray may not walk onto, and its per-area multipliers become the price of the
        // plates carrying them.
        const auto& FilterTables = Get_CompiledFilterTables(
            Field, InQuery.Get_QueryFilter(), InQuery.Get_QueryFilterOverlay());

        Query._PlateCostMultipliers = FilterTables._Multipliers;
        Query._DeniedPlates = FilterTables._Denied;

        const auto RaycastResult = Get_SurfaceRaycast(*Field, Query);

        Result.Set_Status(RaycastResult._Status);
        Result.Set_HitLocation(RaycastResult._HitLocation);

        return Result;
    }

    /**
     * One route, answered inside the call. No entity, no fragment, no processor: a one-shot search is
     * the sliced one with no limits, over an immutable field snapshot the registry already holds.
     *
     * _QueryFilter and _QueryFilterOverlay are compiled into the plate tables the search reads -
     * excluded areas become plates it may not enter, per-area multipliers become what their plates
     * cost. Do_SurfaceRaycast above compiles the same filter into the same two tables.
     */
    auto Get_IsFinite(const FVector& InLocation) -> bool
    {
        return FMath::IsFinite(InLocation.X) && FMath::IsFinite(InLocation.Y) && FMath::IsFinite(InLocation.Z);
    }

    auto Do_FindPathSync_Impl(
        UWorld*                                             InWorld,
        const FCk_NavSurface_PathQuery&                     InQuery,
        const FCk_GroundNav_DynamicObstacleSnapshot&        InDynamicObstacles) -> FCk_NavSurface_PathResult
    {
        auto Result = FCk_NavSurface_PathResult{};

        // The registry key is geometry. Reject malformed geometry before the lookup, because a NaN
        // endpoint must never enter field-coordinate arithmetic just to produce a diagnostic.
        const auto EndpointsAreFinite = Get_IsFinite(InQuery.Get_Start()) && Get_IsFinite(InQuery.Get_End());

        CK_ENSURE_IF_NOT(EndpointsAreFinite,
            TEXT("A GroundNav facade path query received non-finite endpoints: start [{}], end [{}]"),
            InQuery.Get_Start(), InQuery.Get_End())
        {}

        if (NOT EndpointsAreFinite)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            return Result;
        }

        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Start(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto AllowPartial = InQuery.Get_AllowPartial() == ECk_EnableDisable::Enable;

        auto Query = FCk_GroundNav_PathQuery{};
        Query._Start = InQuery.Get_Start();
        Query._Goal = InQuery.Get_End();
        Query._VerticalToleranceUu = Get_VerticalExtentUu(InQuery.Get_SearchHalfExtents());
        Query._MaxExpansions = InQuery.Get_MaxExpansions();
        Query._MaxCorridorLength = InQuery.Get_MaxCorridorLength();
        Query._AllowPartialPath = InQuery.Get_AllowPartial();
        Query._DynamicObstacles = InDynamicObstacles;

        // Left at its zero default rather than assigned unconditionally: zero is what
        // FCk_NavSurface_PathQuery::_AgentRadiusUu documents as the provider's own agent, and today
        // that default IS the search this function already ran before AgentRadiusUu existed - a point
        // agent, with clearance filtering skipped (CkGroundNav_QueryCore.cpp's RadiusUu <= 0 rule).
        if (InQuery.Get_AgentRadiusUu() > 0.0f)
        { Query._Agent._RadiusUu = InQuery.Get_AgentRadiusUu(); }

        const auto& FilterTables = Get_CompiledFilterTables(
            Field, InQuery.Get_QueryFilter(), InQuery.Get_QueryFilterOverlay());

        Query._Cost._PlateCostMultipliers = FilterTables._Multipliers;
        Query._Cost._DeniedPlates = FilterTables._Denied;

        auto Search = FCk_GroundNav_PathSearch{};

        const auto SearchStatus = Search.Request_Begin(Field, Query);

        // Request_Begin answers the query outright or stands a search up for the slices that will.
        // A DEFAULT slice has both of its ceilings off (CkGroundNav_PathSearch.h:58-61), so the one
        // call below runs a stood-up search all the way to a terminal status - which is what makes
        // the one-shot form the sliced form with no limits rather than a second driver.
        if (SearchStatus == ECk_GroundNav_PathStatus::InProgress)
        { Search.ContinueSearch(FCk_GroundNav_PathSliceParams{}); }

        // The invariant the ensure guards is therefore ONE UNLIMITED SLICE IS TERMINAL: an InProgress
        // still standing here means the search stopped for a reason no ceiling named.
        const auto TerminalStatus = Search.Get_Status();
        const auto SearchIsTerminal = TerminalStatus != ECk_GroundNav_PathStatus::InProgress;

        CK_ENSURE_IF_NOT(SearchIsTerminal,
            TEXT("A one-shot GroundNav path search from [{}] to [{}] answered InProgress. A begin plus "
                 "one slice with no limits must answer terminally."),
            InQuery.Get_Start(), InQuery.Get_End())
        {}

        if (NOT SearchIsTerminal)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            return Result;
        }

        const auto& SearchResult = Search.Get_Result();

        Result.Set_StartProjected(SearchResult._StartPoint);
        Result.Set_EndProjected(SearchResult._GoalPoint);
        Result.Set_Status(Get_MappedPathStatus(TerminalStatus, AllowPartial));
        Result.Set_IsPartial(AllowPartial && TerminalStatus == ECk_GroundNav_PathStatus::Partial);

        if (Result.Get_Status() != ECk_NavSurface_QueryStatus::Success)
        {
            // A non-Success terminal answer otherwise leaves no trace: the caller sees a mapped
            // status and nothing else, so diagnosing *why* a strict filter or a denied plate turned
            // a reachable-looking query into Blocked/Unreachable means re-running at Verbose after
            // the fact. One line here carries the search's own verdict and what it spent alongside
            // the mapped status a consumer actually reads.
            const auto StartSurfaceIsValid = SearchResult._StartSurface.Get_IsValid();
            const auto GoalSurfaceIsValid = SearchResult._GoalSurface.Get_IsValid();

            if (StartSurfaceIsValid && GoalSurfaceIsValid)
            {
                const auto StartFlatPlate = Get_FlatPlateIndex(
                    *Field, SearchResult._StartSurface._TileIndex, SearchResult._StartSurface._PlateIndex);
                const auto GoalFlatPlate = Get_FlatPlateIndex(
                    *Field, SearchResult._GoalSurface._TileIndex, SearchResult._GoalSurface._PlateIndex);

                ck::groundnav::Verbose(
                    TEXT("A GroundNav path query from [{}] to [{}] answered [{}] (search verdict [{}], "
                         "[{}] expansions, [{}] denied plates, start/goal flat plate [{}]/[{}])"),
                    InQuery.Get_Start(), InQuery.Get_End(), Result.Get_Status(), TerminalStatus,
                    SearchResult._ExpansionCount, FilterTables._Denied.Num(), StartFlatPlate, GoalFlatPlate);
            }
            else
            {
                ck::groundnav::Verbose(
                    TEXT("A GroundNav path query from [{}] to [{}] answered [{}] (search verdict [{}], "
                         "[{}] expansions, [{}] denied plates)"),
                    InQuery.Get_Start(), InQuery.Get_End(), Result.Get_Status(), TerminalStatus,
                    SearchResult._ExpansionCount, FilterTables._Denied.Num());
            }

            return Result;
        }

        // The agent location the post-process drops its first waypoint against is the query's own
        // start: nothing here has a body, and the start is where the caller said the route begins.
        auto PostParams = FCk_GroundNav_PathPostParams{};
        PostParams._VerticalToleranceUu = Query._VerticalToleranceUu;
        PostParams._AgentLocation = InQuery.Get_Start();
        PostParams._Agent._RadiusUu = Query._Agent._RadiusUu;

        // The post-process prices and shortcuts under the SAME filter the search routed under. Without
        // this the shortcut's chord would be judged against unfiltered ground and could cut straight
        // across the very plates the corridor went round.
        PostParams._Cost._PlateCostMultipliers = Query._Cost._PlateCostMultipliers;
        PostParams._Cost._DeniedPlates = Query._Cost._DeniedPlates;

        // _CornerOffsetK is a MULTIPLE of the radius the funnel already ran with, and
        // _CornerOffsetDistanceUu is a uu distance - so this provider's own corner treatment is the
        // struct's default multiple, and an explicit distance only reaches a K through a radius to be
        // a multiple of.
        const auto EffectiveRadiusUu = PostParams._Agent._RadiusUu;

        switch (InQuery.Get_CornerOffset())
        {
            case ECk_NavSurface_CornerOffset::None:
            {
                PostParams._Cost._CornerOffsetK = 0.0f;
                break;
            }
            case ECk_NavSurface_CornerOffset::Explicit:
            {
                if (EffectiveRadiusUu > 0.0f)
                { PostParams._Cost._CornerOffsetK = InQuery.Get_CornerOffsetDistanceUu() / EffectiveRadiusUu; }
                else
                {
                    PostParams._Cost._CornerOffsetK = 0.0f;

                    ck::groundnav::Verbose(
                        TEXT("A GroundNav path query from [{}] to [{}] asked for an explicit corner offset of "
                             "[{}]uu with no agent radius - a point agent has no radius to scale the offset by, "
                             "so its corners are left raw"),
                        InQuery.Get_Start(), InQuery.Get_End(), InQuery.Get_CornerOffsetDistanceUu());
                }
                break;
            }
            case ECk_NavSurface_CornerOffset::ProviderDefault:
            default:
            {
                // K STAYS AT THE STRUCT DEFAULT - that default multiple of the query's radius IS this
                // provider's own treatment, and so nothing at all when the query named no radius,
                // because Get_PathPlan applies K * radius and that radius is zero.
                break;
            }
        }

        const auto Plan = Get_PathPlan(SearchResult, *Field, PostParams);

        auto Waypoints = TArray<FVector>{};
        Waypoints.Reserve(Plan._Waypoints.Num());

        for (const auto& Waypoint : Plan._Waypoints)
        { Waypoints.Emplace(Waypoint._Location); }

        Result.Set_LengthUu(static_cast<float>(Plan._LengthUu));
        Result.Set_Waypoints(MoveTemp(Waypoints));

        return Result;
    }

    auto Do_FindPathSync(
        UWorld*                          InWorld,
        const FCk_NavSurface_PathQuery&  InQuery) -> FCk_NavSurface_PathResult
    {
        return Do_FindPathSync_Impl(InWorld, InQuery, FCk_GroundNav_DynamicObstacleSnapshot{});
    }

    auto Do_FindDistanceToWall(
        UWorld*                                 InWorld,
        const FCk_NavSurface_WallDistanceQuery& InQuery) -> FCk_NavSurface_WallDistanceResult
    {
        auto Result = FCk_NavSurface_WallDistanceResult{};

        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Location(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        auto Query = FCk_GroundNav_ClosestBoundaryQuery{};
        Query._Location = InQuery.Get_Location();
        Query._MaxRadiusUu = InQuery.Get_MaxRadiusUu();
        Query._VerticalWindowUu = Get_VerticalToleranceUu();

        const auto BoundaryResult = Get_ClosestBoundary(*Field, Query);

        const auto FoundWall = BoundaryResult.Get_IsSuccess();

        // NoSurface folds into a Success that found nothing rather than staying a failure: the ring
        // search answers it both when the point stands on no ground AND when it exhausted the radius
        // without meeting a wall, and the two are indistinguishable at the return. Recast conflates
        // exactly the same pair - a point that projects to no polygon leaves its out point untouched,
        // which reads as "no wall in range" there too - so this is the two providers agreeing rather
        // than this one losing something the other keeps. _FoundWall is what a consumer reads.
        Result.Set_Status(BoundaryResult._Status == ECk_NavSurface_QueryStatus::NoSurface
            ? ECk_NavSurface_QueryStatus::Success
            : BoundaryResult._Status);

        Result.Set_FoundWall(FoundWall);
        Result.Set_DistanceUu(FoundWall ? BoundaryResult._DistanceUu : 0.0f);
        Result.Set_ClosestWallPoint(FoundWall ? BoundaryResult._ClosestPoint : FVector::ZeroVector);

        return Result;
    }

    auto Do_BoundarySegments(
        UWorld*                             InWorld,
        const FCk_NavSurface_BoundaryQuery& InQuery) -> FCk_NavSurface_BoundaryResult
    {
        auto Result = FCk_NavSurface_BoundaryResult{};

        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Center(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        auto Query = FCk_GroundNav_BoundaryQuery{};
        Query._Location = InQuery.Get_Center();
        Query._RadiusUu = InQuery.Get_Radius();
        Query._VerticalWindowUu = Get_VerticalExtentUu(InQuery.Get_SearchHalfExtents());
        Query._MaxSegments = 0;

        auto Segments = TArray<FCk_GroundNav_BoundarySegment>{};

        const auto Status = Get_BoundarySegments(*Field, Query, Segments);

        auto NeutralSegments = TArray<FCk_NavSurface_BoundarySegment>{};
        NeutralSegments.Reserve(Segments.Num());

        for (const auto& Segment : Segments)
        {
            auto NeutralSegment = FCk_NavSurface_BoundarySegment{};
            NeutralSegment.Set_Start(Segment._Start);
            NeutralSegment.Set_End(Segment._End);
            NeutralSegment.Set_InwardNormal(
                FVector{Segment._InwardNormalXY.X, Segment._InwardNormalXY.Y, 0.0});

            NeutralSegments.Emplace(NeutralSegment);
        }

        Result.Set_Status(Status);
        Result.Set_Segments(NeutralSegments);

        return Result;
    }

    auto Do_IsReachable(
        UWorld*                                 InWorld,
        const FCk_NavSurface_ReachabilityQuery& InQuery) -> FCk_NavSurface_ReachabilityResult
    {
        auto Result = FCk_NavSurface_ReachabilityResult{};

        const auto Field = world_fields::TryGet_Field(
            InWorld, InQuery.Get_Start(), InQuery.Get_ProfileTag());

        if (NOT Field.IsValid())
        {
            Result.Set_Reachability(ECk_NavSurface_Reachability::Unknown_ProviderNotReady);
            return Result;
        }

        auto Query = FCk_GroundNav_ReachabilityQuery{};
        Query._Start = InQuery.Get_Start();
        Query._End = InQuery.Get_End();
        Query._VerticalToleranceUu = Get_VerticalToleranceUu();

        Result.Set_Reachability(Get_MappedReachability(Get_IsReachable(*Field, Query)));

        return Result;
    }

    auto Do_SurfaceBounds(
        UWorld* InWorld) -> FBox
    {
        auto Bounds = FBox{ForceInit};

        for (const auto& Field : world_fields::Get_Fields(InWorld))
        {
            const auto FieldBounds = Get_SurfaceBounds(*Field);

            if (NOT FieldBounds.IsValid)
            { continue; }

            Bounds += FieldBounds;
        }

        return Bounds;
    }

    auto Do_ProviderHealth(
        UWorld* InWorld) -> ECk_NavSurface_ProviderHealth
    {
        // Ready or NoData, never Building. A published field is immutable and never half-built, so
        // whether a build is in flight is the VOLUME's word — reported by Do_IsBuildInProgress, which
        // is game-thread only for exactly that reason.
        return world_fields::Get_FieldCount(InWorld) > 0
            ? ECk_NavSurface_ProviderHealth::Ready
            : ECk_NavSurface_ProviderHealth::NoData;
    }

    auto Do_IsBuildInProgress(
        UWorld* InWorld) -> bool
    {
        // GAME THREAD: resolving a volume handle reads the ECS registry, which the field snapshot
        // deliberately does not.
        auto VolumeEntities = world_fields::Get_VolumeEntities(InWorld);

        for (auto& VolumeEntity : VolumeEntities)
        {
            if (ck::Is_NOT_Valid(VolumeEntity))
            { continue; }

            auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

            if (ck::Is_NOT_Valid(Volume))
            { continue; }

            if (UCk_Utils_GroundNavVolume_UE::Get_IsBuilding(Volume))
            { return true; }
        }

        return false;
    }

    auto Do_IsSurfaceSettled(
        UWorld* InWorld) -> bool
    {
        // GAME THREAD, for the same reason Do_IsBuildInProgress is: settledness is the VOLUME's word,
        // and resolving a volume handle reads the ECS registry.
        auto AnyVolumeAnswered = false;

        auto VolumeEntities = world_fields::Get_VolumeEntities(InWorld);

        for (auto& VolumeEntity : VolumeEntities)
        {
            if (ck::Is_NOT_Valid(VolumeEntity))
            { continue; }

            auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

            if (ck::Is_NOT_Valid(Volume))
            { continue; }

            if (NOT UCk_Utils_GroundNavVolume_UE::Get_IsSettled(Volume))
            { return false; }

            AnyVolumeAnswered = true;
        }

        // A world with no volume has nothing that could settle. True there would tell a fixture the
        // surface it is waiting on is ready when there is no surface at all.
        return AnyVolumeAnswered;
    }

    /**
     * MONOTONE for the life of a world, and it is the REGISTRY that makes it so.
     *
     * The sum runs over the fields world_fields currently holds PLUS the epoch sums it kept when a
     * volume unpublished at end-play (CkGroundNav_WorldFieldRegistry.cpp). Ground that went away with
     * its volume keeps counting even though nothing answers a query from it any more.
     *
     * That retained sum is what keeps this number from FALLING, which is the property consumers read it
     * for: a watcher treats any move as "the surface changed", so a drop would announce a rebuild that
     * never happened and, worse, could land back on a value it had already caught up to.
     */
    auto Do_SurfaceRevision(
        UWorld* InWorld) -> int64
    {
        // Ground that was unpublished with its volume keeps counting, so the number never falls.
        auto Revision = world_fields::Get_RetiredRevision(InWorld);

        // Profile-variant fields count too. A publish that moved only a variant moved ground somebody
        // walks on, and a revision that could not see it would tell a watcher the surface stood still.
        Revision += world_fields::Get_VariantRevision(InWorld);

        // The SUM of every field's per-tile epoch sum, not a maximum of anything. Tiles rebuild
        // independently and so do volumes, so two worlds whose newest tile shares an epoch can still
        // differ in every other tile — and a consumer watching this number for "the surface moved"
        // would sit through exactly that change without noticing it.
        for (const auto& Field : world_fields::Get_Fields(InWorld))
        { Revision += Field->Get_AggregatedTileEpochSum(); }

        return Revision;
    }

    auto Do_RequestSurfaceRebuild(
        UWorld* InWorld) -> bool
    {
        auto AnyRequestWasIssued = false;

        const auto Delegate = FCk_Delegate_Request_OnCompleted{};

        auto VolumeEntities = world_fields::Get_VolumeEntities(InWorld);

        for (auto& VolumeEntity : VolumeEntities)
        {
            if (ck::Is_NOT_Valid(VolumeEntity))
            { continue; }

            auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

            if (ck::Is_NOT_Valid(Volume))
            { continue; }

            UCk_Utils_GroundNavVolume_UE::Request_Build(
                Volume, FCk_Request_GroundNavVolume_Build{}, Delegate);

            AnyRequestWasIssued = true;
        }

        return AnyRequestWasIssued;
    }

    // ----------------------------------------------------------------------------------------------------------------

    // The world bounds a paint would cover, taken through the one reduction the bake itself uses. The
    // kind is irrelevant to bounds and is only here because a record cannot be built without one.
    auto Get_RequestWorldBounds(
        const FCk_Request_NavSurface_AreaMarkup& InRequest) -> FBox
    {
        const auto Probe = FCk_GroundNav_MarkupRecord{
            INDEX_NONE,
            InRequest.Get_Shape(),
            InRequest.Get_WorldTransform(),
            ECk_GroundNav_MarkupKind::Cost};

        return Get_MarkupWorldBounds(Probe);
    }

    auto Get_VolumesInWorld(
        UWorld* InWorld) -> TArray<FCk_Handle_GroundNavVolume>
    {
        auto Volumes = TArray<FCk_Handle_GroundNavVolume>{};

        auto VolumeEntities = world_fields::Get_VolumeEntities(InWorld);

        Volumes.Reserve(VolumeEntities.Num());

        for (auto& VolumeEntity : VolumeEntities)
        {
            if (ck::Is_NOT_Valid(VolumeEntity))
            { continue; }

            auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

            if (ck::Is_NOT_Valid(Volume))
            { continue; }

            Volumes.Emplace(Volume);
        }

        return Volumes;
    }

    auto Do_ApplyAreaMarkup(
        UWorld*                                  InWorld,
        FCk_Handle&                              InMarkupEntity,
        const FCk_Request_NavSurface_AreaMarkup& InRequest) -> bool
    {
        const auto MarkupBounds = Get_RequestWorldBounds(InRequest);

        const auto ShapeBoundsSomething = MarkupBounds.IsValid != 0;

        CK_ENSURE_IF_NOT(ShapeBoundsSomething,
            TEXT("GroundNav cannot apply the area markup on [{}] - its shape [{}] and transform bound "
                 "nothing. A degenerate volume and a volume that covers no ground are different answers, "
                 "and only the second is admissible."),
            InMarkupEntity, InRequest.Get_Shape().Get_ShapeType())
        { return false; }

        auto AnyVolumeTookIt = false;

        for (auto& Volume : Get_VolumesInWorld(InWorld))
        {
            const auto VolumeBounds =
                Volume.Get<ck::FFragment_GroundNavVolume_Params>().Get_VolumeBounds();

            if (NOT VolumeBounds.Intersect(MarkupBounds))
            { continue; }

            // The SAME markup entity on every volume it reaches, because that entity is the identity a
            // record is keyed on: a paint straddling two volumes is one markup held twice, not two
            // markups, and releasing it later has to be able to find both from the one handle.
            UCk_Utils_GroundNavVolume_UE::Request_AreaMarkup(Volume,
                FCk_Request_GroundNavVolume_AreaMarkup{
                    InMarkupEntity,
                    InRequest.Get_Shape(),
                    InRequest.Get_WorldTransform(),
                    InRequest.Get_AreaTag()}
                .Set_Enable(InRequest.Get_Enable()),
                {});

            AnyVolumeTookIt = true;
        }

        // A volume is what HOLDS a record, so a paint that reaches none has nowhere to be recorded
        // and nothing to become live on. That is this provider saying "no surface here" about ground
        // it does not cover, and NOT a caller error: the crowd paints under every standing body, so a
        // world whose volumes cover part of a level would fire once per body per repaint. The markup
        // entity stays valid and Get_IsMarkupLive answers false for it, which is the whole of what a
        // consumer reads - a paint that landed nowhere and a paint that has not landed yet are the
        // same answer to the only question anybody asks.
        if (NOT AnyVolumeTookIt)
        {
            ck::groundnav::Verbose(
                TEXT("GroundNav did not take the area markup on [{}] - its bounds [{}] meet no "
                     "ground-nav volume in world [{}], so there is no ground here for it to be live on"),
                InMarkupEntity, MarkupBounds, GetNameSafe(InWorld));

            return false;
        }

        return true;
    }

    auto Do_IsMarkupLive(
        UWorld*           InWorld,
        const FCk_Handle& InMarkupEntity) -> bool
    {
        // The world is deliberately unread: a markup entity names the volume holding its record, and
        // that volume names the field the record was admitted onto. Resolving a field from the world
        // instead would answer about ground the paint was never recorded on.
        return nav_surface_adapter::Get_IsMarkupLive(InMarkupEntity);
    }

    auto Do_ReleaseAreaMarkup(
        UWorld*     InWorld,
        FCk_Handle& InMarkupEntity) -> void
    {
        // Every volume that HOLDS an entry for the entity, not just the one its back-pointer names: a
        // paint that straddled two volumes left a record on each, and the entity carries only the last
        // one to admit it. Releasing on a volume that holds none is an idempotent no-op anyway, so the
        // filter is about not queueing work rather than about correctness.
        for (auto& Volume : Get_VolumesInWorld(InWorld))
        {
            const auto VolumeHoldsThisMarkup = ck::algo::AnyOf(
                UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(Volume),
                [&](const ck::FCk_GroundNav_MarkupEntry& InEntry) -> bool
                {
                    return InEntry.Get_MarkupEntity() == InMarkupEntity;
                });

            if (NOT VolumeHoldsThisMarkup)
            { continue; }

            UCk_Utils_GroundNavVolume_UE::Request_ReleaseAreaMarkup(Volume,
                FCk_Request_GroundNavVolume_ReleaseAreaMarkup{InMarkupEntity}, {});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::nav_surface_adapter::
    Get_IsMarkupLive(
        const FCk_GroundNav_Field&        InField,
        const FCk_GroundNav_MarkupRecord& InRecord)
    -> bool
{
    const auto RecordBounds = Get_MarkupWorldBounds(InRecord);

    if (NOT RecordBounds.IsValid)
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: record bounds are invalid (requested epoch [{}])"),
                InRecord.Get_RequestedAtEpoch());
        }
        return false;
    }

    // Live means the field PRICED the record, and the epoch alone proves only that a build happened
    // after it was asked for: a tile's epoch bumps on mere reach, and a build already in flight when
    // the paint drained publishes a higher epoch off the record snapshot it took BEFORE it arrived.
    // The published params are what the plates were stamped from, so asking them is asking the plates.
    const auto FieldPricedTheRecord = ck::algo::AnyOf(InField._Params._MarkupRecords,
        [&](const FCk_GroundNav_MarkupRecord& InPriced) -> bool
        {
            return FCk_GroundNav_MarkupRecord::StaticStruct()->CompareScriptStruct(
                &InPriced, &InRecord, PPF_None);
        });

    if (NOT FieldPricedTheRecord)
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: published field epoch [{}] has no matching record "
                         "requested at epoch [{}]"),
                InField._Epoch._Value, InRecord.Get_RequestedAtEpoch());
        }
        return false;
    }

    auto ReachedAnyTile = false;

    for (const auto& Tile : InField._Tiles)
    {
        if (NOT Get_TileWorldBounds(InField._Params, Tile).Intersect(RecordBounds))
        { continue; }

        ReachedAnyTile = true;

        const auto TileCarriesTheRecord = Tile.Get_IsBuilt() &&
                                          Tile._Epoch._Value > InRecord.Get_RequestedAtEpoch();

        if (NOT TileCarriesTheRecord)
        {
            if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
            {
                ck::groundnav::Log(TEXT("GroundNav MarkupLive false: intersecting tile [{}, {}] is built [{}] at epoch [{}], "
                             "but markup was requested at epoch [{}]"),
                    Tile._Coord._X, Tile._Coord._Y, Tile.Get_IsBuilt(), Tile._Epoch._Value,
                    InRecord.Get_RequestedAtEpoch());
            }
            return false;
        }
    }

    if (NOT ReachedAnyTile && nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
    {
        ck::groundnav::Log(TEXT("GroundNav MarkupLive false: matching record requested at epoch [{}] intersects no field tile"),
            InRecord.Get_RequestedAtEpoch());
    }
    return ReachedAnyTile;
}

auto
    ck::groundnav::nav_surface_adapter::
    Get_IsMarkupLive(
        const FCk_Handle& InMarkupEntity)
    -> bool
{
    if (ck::Is_NOT_Valid(InMarkupEntity) || NOT InMarkupEntity.Has<ck::FFragment_GroundNav_MarkupRef>())
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: markup [{}] is invalid or has no GroundNav markup reference"),
                InMarkupEntity);
        }
        return false;
    }

    // Debug-only and off unless a run asked for it. Forcing this true makes a fixture that settles
    // on liveness wait for nothing. It sits after the guards above so it can never report a markup
    // that was never recorded on a volume as live.
    if (debug::Get_IsMarkupLiveGateBypassed())
    { return true; }

    const auto& MarkupRef = InMarkupEntity.Get<ck::FFragment_GroundNav_MarkupRef>();

    auto VolumeEntity = MarkupRef.Get_VolumeEntity();

    auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

    if (ck::Is_NOT_Valid(Volume))
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: markup [{}] record [{}] names invalid volume [{}]"),
                InMarkupEntity, MarkupRef.Get_RecordId(), VolumeEntity);
        }
        return false;
    }

    const auto Record = UCk_Utils_GroundNavVolume_UE::TryGet_MarkupRecord(
        Volume, MarkupRef.Get_RecordId());

    if (NOT Record.IsSet())
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: markup [{}] record [{}] is absent from volume [{}]"),
                InMarkupEntity, MarkupRef.Get_RecordId(), Volume);
        }
        return false;
    }

    // A disabled markup has no paint to be live, which is also the answer the Recast provider gives
    // once it has torn its painter down; the two providers must agree on what the flag means.
    if (Record->Get_Enable() == ECk_EnableDisable::Disable)
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: markup [{}] record [{}] is disabled"),
                InMarkupEntity, MarkupRef.Get_RecordId());
        }
        return false;
    }

    const auto Field = UCk_Utils_GroundNavVolume_UE::Get_Field(Volume);

    if (NOT Field.IsValid())
    {
        if (nav_surface_adapter_private::Get_ShouldLogMarkupLiveDiagnostics())
        {
            ck::groundnav::Log(TEXT("GroundNav MarkupLive false: markup [{}] record [{}] volume [{}] has no published field"),
                InMarkupEntity, MarkupRef.Get_RecordId(), Volume);
        }
        return false;
    }

    return Get_IsMarkupLive(*Field, *Record);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::nav_surface_adapter::
    Try_FindPathSyncWithDynamicObstacles(
        UWorld*                                      InWorld,
        const FCk_NavSurface_PathQuery&              InQuery,
        const FCk_GroundNav_DynamicObstacleSnapshot& InDynamicObstacles)
    -> FCk_NavSurface_PathResult
{
    return nav_surface_adapter_private::Do_FindPathSync_Impl(InWorld, InQuery, InDynamicObstacles);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::nav_surface_adapter::
    Register()
    -> void
{
    auto Table = FCk_NavSurface_ProviderTable{};

    Table._ProjectPoint = &nav_surface_adapter_private::Do_ProjectPoint;
    Table._MoveAlongSurface = &nav_surface_adapter_private::Do_MoveAlongSurface;
    Table._SurfaceRaycast = &nav_surface_adapter_private::Do_SurfaceRaycast;
    Table._BoundarySegments = &nav_surface_adapter_private::Do_BoundarySegments;
    Table._IsReachable = &nav_surface_adapter_private::Do_IsReachable;
    Table._FindPathSync = &nav_surface_adapter_private::Do_FindPathSync;
    Table._FindDistanceToWall = &nav_surface_adapter_private::Do_FindDistanceToWall;
    Table._SurfaceBounds = &nav_surface_adapter_private::Do_SurfaceBounds;
    Table._ProviderHealth = &nav_surface_adapter_private::Do_ProviderHealth;
    Table._IsBuildInProgress = &nav_surface_adapter_private::Do_IsBuildInProgress;
    Table._IsSurfaceSettled = &nav_surface_adapter_private::Do_IsSurfaceSettled;
    Table._SurfaceRevision = &nav_surface_adapter_private::Do_SurfaceRevision;
    Table._RequestSurfaceRebuild = &nav_surface_adapter_private::Do_RequestSurfaceRebuild;
    Table._ApplyAreaMarkup = &nav_surface_adapter_private::Do_ApplyAreaMarkup;
    Table._IsMarkupLive = &nav_surface_adapter_private::Do_IsMarkupLive;
    Table._ReleaseAreaMarkup = &nav_surface_adapter_private::Do_ReleaseAreaMarkup;

    ck::nav_surface::Register_Provider(ECk_NavSurface_Provider::GroundNav, MoveTemp(Table));
}

// --------------------------------------------------------------------------------------------------------------------
