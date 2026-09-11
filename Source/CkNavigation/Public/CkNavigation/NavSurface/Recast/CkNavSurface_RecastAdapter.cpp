#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"
#include "CkNavigation/NavSurface/CkNavFilterDefinition_Registry.h"
#include "CkNavigation/NavSurface/CkNavSurface_Fragment.h"
#include "CkNavigation/Revision/CkNavigationRevision_Subsystem.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>
#include <NavAreas/NavArea.h>
#include <NavigationData.h>
#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_recast_adapter
{
    struct FTables
    {
        TMap<FGameplayTag, TSubclassOf<UNavArea>> AreaClassByTag;
    };

    auto Get_Tables() -> FTables&
    {
        static auto Tables = FTables{};
        return Tables;
    }

    auto Get_PendingRegistrations() -> TArray<TFunction<void()>>&
    {
        static auto Pending = TArray<TFunction<void()>>{};
        return Pending;
    }

    auto DoFlushPendingRegistrations() -> void
    {
        auto& Pending = Get_PendingRegistrations();
        if (Pending.IsEmpty())
        { return; }

        auto Running = MoveTemp(Pending);
        Pending.Reset();

        for (const auto& Registration : Running)
        { Registration(); }
    }

    auto Get_SeededTables() -> FTables&
    {
        DoFlushPendingRegistrations();
        return Get_Tables();
    }

    auto Get_ProjectionExtent(const FVector& InRequested) -> FVector
    {
        return InRequested.IsNearlyZero()
            ? UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec()
            : InRequested;
    }

    auto Get_HalfExtents(const FCk_AnyShape& InShape) -> FVector
    {
        switch (InShape.Get_ShapeType())
        {
            case ECk_Shape_Type::Box:
            {
                return InShape.Get_Box().Get_HalfExtents();
            }
            case ECk_Shape_Type::Capsule:
            {
                const auto Radius = static_cast<double>(InShape.Get_Capsule().Get_Radius());
                return FVector{Radius, Radius, static_cast<double>(InShape.Get_Capsule().Get_HalfHeight())};
            }
            case ECk_Shape_Type::Cylinder:
            {
                const auto Radius = static_cast<double>(InShape.Get_Cylinder().Get_Radius());
                return FVector{Radius, Radius, static_cast<double>(InShape.Get_Cylinder().Get_HalfHeight())};
            }
            case ECk_Shape_Type::Sphere:
            {
                return FVector{static_cast<double>(InShape.Get_Sphere().Get_Radius())};
            }
            default:
            {
                return FVector::ZeroVector;
            }
        }
    }

    // Down/Up narrow the symmetric search box to the half that lies on the requested side, so a
    // projection cannot answer with a surface the caller excluded by asking for a direction.
    auto Get_ProjectionBox(
        const FVector& InLocation,
        const FVector& InHalfExtents,
        ECk_NavSurface_ProjectionMode InMode,
        FVector& OutQueryLocation) -> FVector
    {
        switch (InMode)
        {
            case ECk_NavSurface_ProjectionMode::Down:
            {
                const auto VerticalHalf = InHalfExtents.Z * 0.5;
                OutQueryLocation = InLocation - FVector{0.0, 0.0, VerticalHalf};
                return FVector{InHalfExtents.X, InHalfExtents.Y, VerticalHalf};
            }
            case ECk_NavSurface_ProjectionMode::Up:
            {
                const auto VerticalHalf = InHalfExtents.Z * 0.5;
                OutQueryLocation = InLocation + FVector{0.0, 0.0, VerticalHalf};
                return FVector{InHalfExtents.X, InHalfExtents.Y, VerticalHalf};
            }
            default:
            {
                OutQueryLocation = InLocation;
                return InHalfExtents;
            }
        }
    }

    auto DoApplyExcludedArea(
        ARecastNavMesh& InNavData,
        const FGameplayTag& InAreaTag,
        TSet<UClass*>& InOutSeenAreaClasses,
        FSharedNavQueryFilter& InOutFilter) -> bool
    {
        const auto AreaClass = ck::nav_surface_recast::Get_AreaClass(InAreaTag);

        const auto AreaClassIsValid = ck::IsValid(AreaClass.Get());
        CK_ENSURE_IF_NOT(AreaClassIsValid,
            TEXT("Nav query filter area tag [{}] resolves to no registered nav area class"), InAreaTag)
        {}

        if (NOT AreaClassIsValid)
        { return false; }

        if (InOutSeenAreaClasses.Contains(AreaClass.Get()))
        { return true; }
        InOutSeenAreaClasses.Add(AreaClass.Get());

        const auto AreaId = InNavData.GetAreaID(AreaClass);
        const auto AreaIsRegistered = AreaId != INDEX_NONE;
        CK_ENSURE_IF_NOT(AreaIsRegistered,
            TEXT("Nav query filter area [{}] is not registered on NavData [{}]"),
            GetNameSafe(AreaClass.Get()), InNavData.GetName())
        {}

        if (NOT AreaIsRegistered)
        { return false; }

        InOutFilter->SetExcludedArea(static_cast<uint8>(AreaId));
        return true;
    }

    auto DoApplyDefinition(
        ARecastNavMesh& InNavData,
        const FCk_NavFilter_Definition& InDefinition,
        TSet<UClass*>& InOutSeenAreaClasses,
        FSharedNavQueryFilter& InOutFilter) -> bool
    {
        for (const auto& ExcludedTag : InDefinition.Get_ExcludedAreaTags())
        {
            if (NOT DoApplyExcludedArea(InNavData, ExcludedTag, InOutSeenAreaClasses, InOutFilter))
            { return false; }
        }

        // A required set is expressed to Recast as the exclusion of every OTHER registered area:
        // the engine filter has no allow-list primitive.
        if (NOT InDefinition.Get_RequiredAreaTags().IsEmpty())
        {
            // The exclusion translation below only visits registered tags. Check every authored
            // required tag first so a valid but unknown tag cannot weaken to an all-excluded filter.
            for (const auto& RequiredTag : InDefinition.Get_RequiredAreaTags())
            {
                const auto RequiredAreaClass = ck::nav_surface_recast::Get_AreaClass(RequiredTag);
                const auto RequiredAreaClassIsValid = ck::IsValid(RequiredAreaClass.Get());
                CK_ENSURE_IF_NOT(RequiredAreaClassIsValid,
                    TEXT("Nav query filter required area tag [{}] resolves to no registered nav area class"),
                    RequiredTag)
                {}

                if (NOT RequiredAreaClassIsValid)
                { return false; }

                const auto RequiredAreaId = InNavData.GetAreaID(RequiredAreaClass);
                const auto RequiredAreaIsRegistered = RequiredAreaId != INDEX_NONE;
                CK_ENSURE_IF_NOT(RequiredAreaIsRegistered,
                    TEXT("Nav query filter required area [{}] is not registered on NavData [{}]"),
                    GetNameSafe(RequiredAreaClass.Get()), InNavData.GetName())
                {}

                if (NOT RequiredAreaIsRegistered)
                { return false; }
            }

            for (const auto& RegisteredTag : ck::nav_surface_recast::Get_RegisteredAreaTags())
            {
                if (InDefinition.Get_RequiredAreaTags().HasTagExact(RegisteredTag))
                { continue; }

                if (NOT DoApplyExcludedArea(InNavData, RegisteredTag, InOutSeenAreaClasses, InOutFilter))
                { return false; }
            }
        }

        for (const auto& CostEntry : InDefinition.Get_AreaCostMultipliers())
        {
            const auto AreaClass = ck::nav_surface_recast::Get_AreaClass(CostEntry.Key);

            const auto AreaClassIsValid = ck::IsValid(AreaClass.Get());
            CK_ENSURE_IF_NOT(AreaClassIsValid,
                TEXT("Nav query filter cost tag [{}] resolves to no registered nav area class"), CostEntry.Key)
            {}

            if (NOT AreaClassIsValid)
            { return false; }

            const auto AreaId = InNavData.GetAreaID(AreaClass);
            const auto AreaIsRegistered = AreaId != INDEX_NONE;
            CK_ENSURE_IF_NOT(AreaIsRegistered,
                TEXT("Nav query filter cost area [{}] is not registered on NavData [{}]"),
                GetNameSafe(AreaClass.Get()), InNavData.GetName())
            {}

            if (NOT AreaIsRegistered)
            { return false; }

            const auto* AreaDefaults = AreaClass.GetDefaultObject();
            const auto AreaDefaultsAreValid = ck::IsValid(AreaDefaults);
            CK_ENSURE_IF_NOT(AreaDefaultsAreValid,
                TEXT("Nav query filter cost area [{}] has no valid class default object"),
                GetNameSafe(AreaClass.Get()))
            {}

            if (NOT AreaDefaultsAreValid)
            { return false; }

            const auto AuthoredCost = AreaDefaults->DefaultCost;
            InOutFilter->SetAreaCost(static_cast<uint8>(AreaId), AuthoredCost * CostEntry.Value);
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface_recast
{
    FRegistrar::
        FRegistrar(
            TFunction<void()> InRegistration)
    {
        ck_nav_surface_recast_adapter::Get_PendingRegistrations().Add(MoveTemp(InRegistration));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Register_AreaTag(
            const FGameplayTag& InAreaTag,
            TSubclassOf<UNavArea> InAreaClass)
        -> void
    {
        const auto RegistrationIsValid = InAreaTag.IsValid() && ck::IsValid(InAreaClass.Get());
        CK_ENSURE_IF_NOT(RegistrationIsValid,
            TEXT("Rejected nav area registration: tag [{}] class [{}]"),
            InAreaTag, GetNameSafe(InAreaClass.Get()))
        { return; }

        ck_nav_surface_recast_adapter::Get_Tables().AreaClassByTag.Add(InAreaTag, InAreaClass);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_AreaClass(
            const FGameplayTag& InAreaTag)
        -> TSubclassOf<UNavArea>
    {
        if (NOT InAreaTag.IsValid())
        { return {}; }

        const auto* Found = ck_nav_surface_recast_adapter::Get_SeededTables().AreaClassByTag.Find(InAreaTag);
        return Found != nullptr ? *Found : TSubclassOf<UNavArea>{};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_RegisteredAreaTags()
        -> TArray<FGameplayTag>
    {
        auto AreaTags = TArray<FGameplayTag>{};
        ck_nav_surface_recast_adapter::Get_SeededTables().AreaClassByTag.GetKeys(AreaTags);
        return AreaTags;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CompiledQueryFilter(
            ARecastNavMesh& InNavData,
            const FGameplayTag& InFilterTag,
            const FCk_Nav_QueryFilterOverlay& InOverlay)
        -> FSharedConstNavQueryFilter
    {
        auto BaseFilter = InNavData.GetDefaultQueryFilter();

        const auto BaseFilterIsValid = BaseFilter.IsValid();
        CK_ENSURE_IF_NOT(BaseFilterIsValid,
            TEXT("Nav query filter resolution failed: NavData [{}] has no default filter"),
            InNavData.GetName())
        {}

        if (NOT BaseFilterIsValid)
        { return {}; }

        const auto Definition = ck::nav_surface::TryGet_FilterDefinition(InFilterTag);

        const auto NamedFilterResolved = NOT InFilterTag.IsValid() || Definition.IsSet();
        CK_ENSURE_IF_NOT(NamedFilterResolved,
            TEXT("Recast rejected named query filter [{}] because no valid definition resolved"), InFilterTag)
        {}

        if (NOT NamedFilterResolved)
        { return {}; }

        if (NOT Definition.IsSet() && InOverlay.Get_ExcludedAreaTags().IsEmpty())
        { return BaseFilter; }

        auto CompiledFilter = BaseFilter->GetCopy();
        const auto CompiledFilterIsValid = CompiledFilter.IsValid();
        CK_ENSURE_IF_NOT(CompiledFilterIsValid,
            TEXT("Nav query filter resolution failed: could not copy the default filter of NavData [{}]"),
            InNavData.GetName())
        {}

        if (NOT CompiledFilterIsValid)
        { return {}; }

        auto SeenAreaClasses = TSet<UClass*>{};

        if (Definition.IsSet()
            && NOT ck_nav_surface_recast_adapter::DoApplyDefinition(
                InNavData, *Definition, SeenAreaClasses, CompiledFilter))
        { return {}; }

        for (const auto& ExcludedTag : InOverlay.Get_ExcludedAreaTags())
        {
            if (NOT ck_nav_surface_recast_adapter::DoApplyExcludedArea(
                InNavData, ExcludedTag, SeenAreaClasses, CompiledFilter))
            { return {}; }
        }

        return CompiledFilter;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryGet_NavSystem(
            UWorld* InWorld)
        -> UNavigationSystemV1*
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return nullptr; }

        return UNavigationSystemV1::GetCurrent(InWorld);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryGet_NavData(
            UWorld* InWorld)
        -> ARecastNavMesh*
    {
        auto* NavSys = TryGet_NavSystem(InWorld);
        if (NavSys == nullptr)
        { return nullptr; }

        return Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
    }

    // ----------------------------------------------------------------------------------------------------------------

    // _ProfileTag IS IGNORED HERE, and by every other query in this file. Recast bakes one navmesh per
    // agent-radius entry and has no surface keyed on a profile tag, so there is nothing for a tag to
    // select between: a tagged query is answered from the same navmesh an untagged one is. Said once
    // rather than at each site because it is one gap and not five. A caller that needs a profile's own
    // walkable set has to be on the GroundNav provider, which does key its fields on the tag.
    auto
        Try_ProjectPoint(
            UWorld* InWorld,
            const FCk_NavSurface_ProjectionQuery& InQuery)
        -> FCk_NavSurface_ProjectionResult
    {
        auto Result = FCk_NavSurface_ProjectionResult{};

        auto* NavSys = TryGet_NavSystem(InWorld);
        auto* NavData = TryGet_NavData(InWorld);
        if (NavSys == nullptr || NavData == nullptr)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto QueryFilter = Get_CompiledQueryFilter(
            *NavData, InQuery.Get_QueryFilter(), InQuery.Get_QueryFilterOverlay());
        if (NOT QueryFilter.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            return Result;
        }

        auto QueryLocation = FVector::ZeroVector;
        const auto Extent = ck_nav_surface_recast_adapter::Get_ProjectionBox(
            InQuery.Get_Location(),
            ck_nav_surface_recast_adapter::Get_ProjectionExtent(InQuery.Get_SearchHalfExtents()),
            InQuery.Get_Mode(),
            QueryLocation);

        auto Projected = FNavLocation{};
        if (NOT NavSys->ProjectPointToNavigation(QueryLocation, Projected, Extent, NavData, QueryFilter))
        {
            Result.Set_Status(Get_IsBuildInProgress(InWorld)
                ? ECk_NavSurface_QueryStatus::Unbuilt
                : ECk_NavSurface_QueryStatus::NoSurface);
            return Result;
        }

        Result.Set_Status(ECk_NavSurface_QueryStatus::Success);
        Result.Set_Location(Projected.Location);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Try_MoveAlongSurface(
            UWorld* InWorld,
            const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery)
        -> FCk_NavSurface_MoveAlongSurfaceResult
    {
        auto Result = FCk_NavSurface_MoveAlongSurfaceResult{};

        auto* NavSys = TryGet_NavSystem(InWorld);
        auto* NavData = TryGet_NavData(InWorld);
        if (NavSys == nullptr || NavData == nullptr)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto QueryFilter = Get_CompiledQueryFilter(
            *NavData, InQuery.Get_QueryFilter(), InQuery.Get_QueryFilterOverlay());
        if (NOT QueryFilter.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            return Result;
        }

        const auto Extent = UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec();
        auto StartOnMesh = FNavLocation{};
        if (NOT NavSys->ProjectPointToNavigation(InQuery.Get_Start(), StartOnMesh, Extent, NavData, QueryFilter))
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoSurface);
            return Result;
        }

        auto Reached = FNavLocation{};
        if (NOT NavData->FindMoveAlongSurface(StartOnMesh, InQuery.Get_End(), Reached, QueryFilter))
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            Result.Set_ReachedLocation(StartOnMesh.Location);
            return Result;
        }

        Result.Set_Status(ECk_NavSurface_QueryStatus::Success);
        Result.Set_ReachedLocation(Reached.Location);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Try_SurfaceRaycast(
            UWorld* InWorld,
            const FCk_NavSurface_RaycastQuery& InQuery)
        -> FCk_NavSurface_RaycastResult
    {
        auto Result = FCk_NavSurface_RaycastResult{};

        auto* NavData = TryGet_NavData(InWorld);
        if (NavData == nullptr)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto QueryFilter = Get_CompiledQueryFilter(
            *NavData, InQuery.Get_QueryFilter(), InQuery.Get_QueryFilterOverlay());
        if (NOT QueryFilter.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            return Result;
        }

        auto HitLocation = FVector::ZeroVector;
        const auto IsBlocked = NavData->Raycast(
            InQuery.Get_Start(), InQuery.Get_End(), HitLocation, QueryFilter);

        Result.Set_Status(IsBlocked
            ? ECk_NavSurface_QueryStatus::Blocked
            : ECk_NavSurface_QueryStatus::Success);
        Result.Set_HitLocation(HitLocation);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_BoundarySegments(
            UWorld* InWorld,
            const FCk_NavSurface_BoundaryQuery& InQuery)
        -> FCk_NavSurface_BoundaryResult
    {
        auto Result = FCk_NavSurface_BoundaryResult{};

        auto* NavData = TryGet_NavData(InWorld);
        if (NavData == nullptr)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto Extent = ck_nav_surface_recast_adapter::Get_ProjectionExtent(InQuery.Get_SearchHalfExtents());
        const auto CenterPoly = NavData->FindNearestPoly(InQuery.Get_Center(), Extent);
        if (CenterPoly == INVALID_NAVNODEREF)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoSurface);
            return Result;
        }

        auto Edges = TArray<FNavigationWallEdge>{};
        if (NOT NavData->FindEdges(CenterPoly, InQuery.Get_Center(), InQuery.Get_Radius(), nullptr, Edges))
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoSurface);
            return Result;
        }

        auto Segments = TArray<FCk_NavSurface_BoundarySegment>{};
        Segments.Reserve(Edges.Num());
        for (const auto& Edge : Edges)
        {
            // A wall segment is emitted in its source polygon's winding, and Recast winds the
            // interior onto the (-dY, dX) side once Unreal2Recast has negated both horizontal
            // axes — so that perpendicular points AWAY from walkable space.
            const auto Along = Edge.End - Edge.Start;
            const auto Outward = FVector{-Along.Y, Along.X, 0.0}.GetSafeNormal();

            auto Segment = FCk_NavSurface_BoundarySegment{};
            Segment.Set_Start(Edge.Start);
            Segment.Set_End(Edge.End);
            Segment.Set_InwardNormal(-Outward);
            Segments.Add(Segment);
        }

        Result.Set_Status(ECk_NavSurface_QueryStatus::Success);
        Result.Set_Segments(MoveTemp(Segments));
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsReachable(
            UWorld* InWorld,
            const FCk_NavSurface_ReachabilityQuery& InQuery)
        -> FCk_NavSurface_ReachabilityResult
    {
        auto Result = FCk_NavSurface_ReachabilityResult{};

        auto* NavSys = TryGet_NavSystem(InWorld);
        auto* NavData = TryGet_NavData(InWorld);
        if (NavSys == nullptr || NavData == nullptr)
        {
            Result.Set_Reachability(ECk_NavSurface_Reachability::Unknown_ProviderNotReady);
            return Result;
        }

        const auto QueryFilter = Get_CompiledQueryFilter(*NavData, InQuery.Get_QueryFilter(), {});
        if (NOT QueryFilter.IsValid())
        {
            Result.Set_Reachability(ECk_NavSurface_Reachability::Unknown_ProviderNotReady);
            return Result;
        }

        const auto Extent = UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec();
        auto StartProj = FNavLocation{};
        auto EndProj = FNavLocation{};
        if (NOT NavSys->ProjectPointToNavigation(InQuery.Get_Start(), StartProj, Extent, NavData, QueryFilter)
            || NOT NavSys->ProjectPointToNavigation(InQuery.Get_End(), EndProj, Extent, NavData, QueryFilter))
        {
            Result.Set_Reachability(ECk_NavSurface_Reachability::Unreachable);
            return Result;
        }

        auto Query = FPathFindingQuery{
            /* Owner */        nullptr,
            /* NavData */      *NavData,
            /* Start */        StartProj.Location,
            /* End */          EndProj.Location,
            /* SourceFilter */ QueryFilter};
        Query.SetAllowPartialPaths(false);

        const auto IsReachable = ARecastNavMesh::TestPath(Query.NavAgentProperties, Query, nullptr);
        Result.Set_Reachability(IsReachable
            ? ECk_NavSurface_Reachability::Reachable
            : ECk_NavSurface_Reachability::Unreachable);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    // _MaxExpansions and _MaxCorridorLength ARE IGNORED HERE, as the query struct says: Detour's
    // synchronous find takes neither ceiling. Nothing is silently substituted for them - a bounded
    // query is answered unbounded, which is why the neutral query carries no wall-clock budget that
    // one provider could honour and the other could not.
    //
    // _AgentRadiusUu IS ALSO IGNORED HERE, by design: Detour always uses the navmesh's own baked
    // agent, at any value - AgentRadiusForFirstSkip below is a distinct zero for the skip-first pass
    // and is not where a caller's radius would go if this provider read it.
    auto
        Try_FindPathSync(
            UWorld* InWorld,
            const FCk_NavSurface_PathQuery& InQuery)
        -> FCk_NavSurface_PathResult
    {
        auto Result = FCk_NavSurface_PathResult{};

        auto* NavSys = TryGet_NavSystem(InWorld);
        auto* NavData = TryGet_NavData(InWorld);
        if (NavSys == nullptr || NavData == nullptr)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto Extent = ck_nav_surface_recast_adapter::Get_ProjectionExtent(InQuery.Get_SearchHalfExtents());

        const auto AllowPartial = InQuery.Get_AllowPartial() == ECk_EnableDisable::Enable;

        // Zero, deliberately: the skip-first pass drops a waypoint the ASKING BODY already stands on,
        // and a query carries no body. A consumer that wants it drops the waypoint itself, against
        // where its body actually is.
        constexpr auto AgentRadiusForFirstSkip = 0.0f;

        // This provider's own corner treatment IS the navmesh's baked agent radius - it is what every
        // direct Detour caller passed by hand, and so what ProviderDefault has to keep meaning here.
        const auto CornerOffsetDistanceUu = [&]() -> float
        {
            switch (InQuery.Get_CornerOffset())
            {
                case ECk_NavSurface_CornerOffset::None:
                { return 0.0f; }
                case ECk_NavSurface_CornerOffset::Explicit:
                { return InQuery.Get_CornerOffsetDistanceUu(); }
                case ECk_NavSurface_CornerOffset::ProviderDefault:
                default:
                { return NavData->GetConfig().AgentRadius; }
            }
        }();

        auto NavResult = FCk_Nav_PathResult{};

        FCk_Nav_Algorithm::FindPathSync(
            *NavSys,
            *NavData,
            InQuery.Get_Start(),
            InQuery.Get_End(),
            AllowPartial,
            static_cast<float>(Extent.X),
            static_cast<float>(Extent.Z),
            AgentRadiusForFirstSkip,
            NavResult,
            InQuery.Get_QueryFilter(),
            CornerOffsetDistanceUu,
            InQuery.Get_QueryFilterOverlay());

        const auto& Diagnostics = NavResult.Get_Diagnostics();

        Result.Set_StartProjected(Diagnostics.Get_LastProjectedStart());
        Result.Set_EndProjected(Diagnostics.Get_LastProjectedEnd());

        switch (NavResult.Get_Status())
        {
            case ECk_Nav_PathStatus::Ready:
            {
                Result.Set_Status(ECk_NavSurface_QueryStatus::Success);
                break;
            }
            case ECk_Nav_PathStatus::Partial:
            {
                // A partial answer nobody asked for is not a shorter route, it is no route. Refused
                // rather than handed over, which is the same rule GroundNav's side keeps.
                Result.Set_Status(AllowPartial
                    ? ECk_NavSurface_QueryStatus::Success
                    : ECk_NavSurface_QueryStatus::Blocked);
                Result.Set_IsPartial(AllowPartial);
                break;
            }
            default:
            {
                switch (Diagnostics.Get_LastFailReason())
                {
                    case ECk_Nav_PathFailReason::NoNavSystem:
                    case ECk_Nav_PathFailReason::NoNavData:
                    case ECk_Nav_PathFailReason::NoDefaultFilter:
                    {
                        Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
                        break;
                    }
                    case ECk_Nav_PathFailReason::StartProjectFailed:
                    case ECk_Nav_PathFailReason::EndProjectFailed:
                    {
                        // The same split Try_ProjectPoint above makes, for the same reason: ground
                        // nobody has baked yet is worth waiting for and ground with nowhere to stand
                        // on it is not, and a consumer defers on one and gives up on the other.
                        Result.Set_Status(Get_IsBuildInProgress(InWorld)
                            ? ECk_NavSurface_QueryStatus::Unbuilt
                            : ECk_NavSurface_QueryStatus::NoSurface);
                        break;
                    }
                    default:
                    {
                        Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
                        break;
                    }
                }

                return Result;
            }
        }

        auto Waypoints = NavResult.Get_Waypoints();

        auto LengthUu = 0.0;
        for (auto Index = 1; Index < Waypoints.Num(); ++Index)
        { LengthUu += FVector::Dist(Waypoints[Index - 1], Waypoints[Index]); }

        Result.Set_LengthUu(static_cast<float>(LengthUu));
        Result.Set_Waypoints(MoveTemp(Waypoints));

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Try_FindDistanceToWall(
            UWorld* InWorld,
            const FCk_NavSurface_WallDistanceQuery& InQuery)
        -> FCk_NavSurface_WallDistanceResult
    {
        auto Result = FCk_NavSurface_WallDistanceResult{};

        auto* NavData = TryGet_NavData(InWorld);
        if (NavData == nullptr)
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        const auto QueryFilter = Get_CompiledQueryFilter(
            *NavData, InQuery.Get_QueryFilter(), InQuery.Get_QueryFilterOverlay());
        if (NOT QueryFilter.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::Blocked);
            return Result;
        }

        // Detour reports "no wall in range" by leaving the out point untouched, so the sentinel has to
        // be a value it could never write. A bool the consumer can read is what replaces it here.
        const auto SentinelValue = TNumericLimits<FVector::FReal>::Max();
        auto ClosestWall = FVector{SentinelValue, SentinelValue, SentinelValue};

        const auto DistanceUu = NavData->FindDistanceToWall(
            InQuery.Get_Location(), QueryFilter, InQuery.Get_MaxRadiusUu(), &ClosestWall);

        const auto FoundWall = ClosestWall.X != SentinelValue;

        Result.Set_Status(ECk_NavSurface_QueryStatus::Success);
        Result.Set_FoundWall(FoundWall);
        Result.Set_DistanceUu(FoundWall ? static_cast<float>(DistanceUu) : 0.0f);
        Result.Set_ClosestWallPoint(FoundWall ? ClosestWall : FVector::ZeroVector);

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_SurfaceBounds(
            UWorld* InWorld)
        -> FBox
    {
        auto* NavData = TryGet_NavData(InWorld);
        if (NavData == nullptr)
        { return FBox{ForceInit}; }

        return NavData->GetNavMeshBounds();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ProviderHealth(
            UWorld* InWorld)
        -> ECk_NavSurface_ProviderHealth
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return ECk_NavSurface_ProviderHealth::Error; }

        auto* NavData = TryGet_NavData(InWorld);
        if (NavData == nullptr)
        { return ECk_NavSurface_ProviderHealth::NoData; }

        // An actor with no Detour mesh under it is data nobody can query, so it answers NoData rather
        // than Ready. This is the guard every direct-Recast caller used to carry itself - the
        // `NavData == nullptr || NOT NavData->HasValidNavmesh()` pair that stood at
        // CkPathNetwork_Processor.cpp's Resolve_OffPathLeg, Try_ResolvePathOntoNavmesh and
        // Try_ResolvePathOntoNavmeshWithRibbonConstraints - restated once, here, where the facade can
        // answer it for every provider's consumers at the same time.
        if (NOT NavData->HasValidNavmesh())
        { return ECk_NavSurface_ProviderHealth::NoData; }

        return Get_IsBuildInProgress(InWorld)
            ? ECk_NavSurface_ProviderHealth::Building
            : ECk_NavSurface_ProviderHealth::Ready;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsBuildInProgress(
            UWorld* InWorld)
        -> bool
    {
        auto* NavSys = TryGet_NavSystem(InWorld);
        if (NavSys == nullptr)
        { return false; }

        return NavSys->IsNavigationBuildInProgress();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsSurfaceSettled(
            UWorld* InWorld)
        -> bool
    {
        if (Get_ProviderHealth(InWorld) != ECk_NavSurface_ProviderHealth::Ready)
        { return false; }

        auto* NavSys = TryGet_NavSystem(InWorld);
        if (NavSys == nullptr)
        { return false; }

        // Deliberately NOT IsNavigationDirty(): that reports a navmesh with zero tiles as permanently
        // needing a rebuild (ARecastNavMesh::NeedsRebuild, bHasNoTileData), so a world with nothing
        // walkable in it would never settle and every caller waiting on this would wait forever. The
        // dirty-areas queue drains, which is what "nothing pending" has to mean for a waiter.
        return NOT NavSys->HasDirtyAreasQueued();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_SurfaceRevision(
            UWorld* InWorld)
        -> int64
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return 0; }

        auto* RevisionSubsystem = InWorld->GetSubsystem<UCk_NavigationRevisionSubsystem_UE>();
        if (ck::Is_NOT_Valid(RevisionSubsystem))
        { return 0; }

        // The observer binds lazily: a world whose NavigationSystem appeared after subsystem
        // initialization would otherwise never advance its revision at all.
        RevisionSubsystem->TryEnsureBound();

        return static_cast<int64>(RevisionSubsystem->Get_Revision());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_SurfaceRebuild(
            UWorld* InWorld)
        -> bool
    {
        auto* NavSys = TryGet_NavSystem(InWorld);
        if (NavSys == nullptr)
        { return false; }

        NavSys->Build();
        ck::nav::Verbose(TEXT("Request_SurfaceRebuild kicked off Build() on world [{}]"), GetNameSafe(InWorld));
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsAreaLiveAt(
            UWorld* InWorld,
            const FGameplayTag& InAreaTag,
            const FVector& InLocation,
            const FVector& InSearchHalfExtents)
        -> bool
    {
        auto* NavData = TryGet_NavData(InWorld);
        if (NavData == nullptr)
        { return false; }

        const auto AreaClass = Get_AreaClass(InAreaTag);
        if (ck::Is_NOT_Valid(AreaClass.Get()))
        { return false; }

        const auto AreaId = NavData->GetAreaID(AreaClass);
        if (AreaId == INDEX_NONE)
        { return false; }

        const auto Extent = ck_nav_surface_recast_adapter::Get_ProjectionExtent(InSearchHalfExtents);
        const auto PolyRef = NavData->FindNearestPoly(InLocation, Extent);
        if (PolyRef == INVALID_NAVNODEREF)
        { return false; }

        return static_cast<int32>(NavData->GetPolyAreaID(PolyRef)) == AreaId;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Apply_AreaMarkup(
            UWorld*                                  InWorld,
            FCk_Handle&                              InMarkupEntity,
            const FCk_Request_NavSurface_AreaMarkup& InRequest)
        -> bool
    {
        auto& Current = InMarkupEntity.Get<FFragment_NavSurfaceMarkup_Current>();

        if (Current.Get_Markup().IsValid())
        { UCk_Utils_NavAreaMarkup_UE::Request_Destroy(Current.Get_Markup().Get()); }

        Current._Markup = nullptr;

        if (InRequest.Get_Enable() == ECk_EnableDisable::Disable)
        {
            Current._AreaTag = {};
            Current._Location = FVector::ZeroVector;
            Current._HalfExtents = FVector::ZeroVector;
            return true;
        }

        const auto AreaClass = Get_AreaClass(InRequest.Get_AreaTag());
        const auto AreaClassIsValid = ck::IsValid(AreaClass.Get());
        CK_ENSURE_IF_NOT(AreaClassIsValid,
            TEXT("NavSurface markup on [{}] asked for area tag [{}], which no provider area is registered for"),
            InMarkupEntity, InRequest.Get_AreaTag())
        { return false; }

        const auto HalfExtents = ck_nav_surface_recast_adapter::Get_HalfExtents(InRequest.Get_Shape());
        const auto ShapeIsPaintable = NOT HalfExtents.IsNearlyZero();
        CK_ENSURE_IF_NOT(ShapeIsPaintable,
            TEXT("NavSurface markup on [{}] was given a shape [{}] with no extent"),
            InMarkupEntity, InRequest.Get_Shape().Get_ShapeType())
        { return false; }

        auto* Markup = UCk_Utils_NavAreaMarkup_UE::Request_Create(
            InMarkupEntity,
            InRequest.Get_WorldTransform(),
            HalfExtents,
            AreaClass);

        const auto MarkupIsValid = ck::IsValid(Markup);
        CK_ENSURE_IF_NOT(MarkupIsValid,
            TEXT("NavSurface markup on [{}] failed to register its nav-area painter"), InMarkupEntity)
        { return false; }

        Current._Markup = Markup;
        Current._AreaTag = InRequest.Get_AreaTag();
        Current._Location = InRequest.Get_WorldTransform().GetLocation();
        Current._HalfExtents = HalfExtents;

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsMarkupLive(
            UWorld*           InWorld,
            const FCk_Handle& InMarkupEntity)
        -> bool
    {
        const auto& Current = InMarkupEntity.Get<FFragment_NavSurfaceMarkup_Current>();

        if (NOT Current.Get_Markup().IsValid())
        { return false; }

        return Get_IsAreaLiveAt(
            InWorld, Current.Get_AreaTag(), Current.Get_Location(), Current.Get_HalfExtents());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Release_AreaMarkup(
            UWorld*     InWorld,
            FCk_Handle& InMarkupEntity)
        -> void
    {
        auto& Current = InMarkupEntity.Get<FFragment_NavSurfaceMarkup_Current>();

        if (Current.Get_Markup().IsValid())
        { UCk_Utils_NavAreaMarkup_UE::Request_Destroy(Current.Get_Markup().Get()); }

        Current._Markup = nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------
