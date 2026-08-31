#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Revision/CkNavigationRevision_Subsystem.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
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
        TMap<FGameplayTag, FCk_NavFilter_Definition> FilterDefinitionByTag;
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
        { return false; }

        if (InOutSeenAreaClasses.Contains(AreaClass.Get()))
        { return true; }
        InOutSeenAreaClasses.Add(AreaClass.Get());

        const auto AreaId = InNavData.GetAreaID(AreaClass);
        const auto AreaIsRegistered = AreaId != INDEX_NONE;
        CK_ENSURE_IF_NOT(AreaIsRegistered,
            TEXT("Nav query filter area [{}] is not registered on NavData [{}]"),
            GetNameSafe(AreaClass.Get()), InNavData.GetName())
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
            { return false; }

            const auto AreaId = InNavData.GetAreaID(AreaClass);
            const auto AreaIsRegistered = AreaId != INDEX_NONE;
            CK_ENSURE_IF_NOT(AreaIsRegistered,
                TEXT("Nav query filter cost area [{}] is not registered on NavData [{}]"),
                GetNameSafe(AreaClass.Get()), InNavData.GetName())
            { return false; }

            const auto* AreaDefaults = AreaClass.GetDefaultObject();
            const auto AuthoredCost = ck::IsValid(AreaDefaults) ? AreaDefaults->DefaultCost : 1.0f;
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
        Register_FilterDefinition(
            const FGameplayTag& InFilterTag,
            FCk_NavFilter_Definition InDefinition)
        -> void
    {
        const auto RegistrationIsValid = InFilterTag.IsValid();
        CK_ENSURE_IF_NOT(RegistrationIsValid,
            TEXT("Rejected nav filter definition registration: invalid tag"))
        { return; }

        ck_nav_surface_recast_adapter::Get_Tables().FilterDefinitionByTag.Add(
            InFilterTag, MoveTemp(InDefinition));
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
        TryGet_FilterDefinition(
            const FGameplayTag& InFilterTag)
        -> TOptional<FCk_NavFilter_Definition>
    {
        if (NOT InFilterTag.IsValid())
        { return {}; }

        const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
        if (ck::IsValid(Settings))
        {
            if (const auto* Configured = Settings->Get_QueryFilters().Find(InFilterTag))
            {
                const auto* Definition = Configured->LoadSynchronous();

                const auto DefinitionIsValid = ck::IsValid(Definition);
                CK_ENSURE_IF_NOT(DefinitionIsValid,
                    TEXT("Nav QueryFilter tag [{}] maps to a filter definition that failed to load — using default filter"),
                    InFilterTag)
                { return {}; }

                return Definition->Get_Definition();
            }
        }

        const auto& Tables = ck_nav_surface_recast_adapter::Get_SeededTables();
        if (const auto* Native = Tables.FilterDefinitionByTag.Find(InFilterTag))
        { return *Native; }

        CK_TRIGGER_ENSURE(
            TEXT("Nav QueryFilter tag [{}] has no mapping in Ck Navigation project settings — using default filter"),
            InFilterTag);
        return {};
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
        { return {}; }

        const auto Definition = TryGet_FilterDefinition(InFilterTag);
        if (NOT Definition.IsSet() && InOverlay.Get_ExcludedAreaTags().IsEmpty())
        { return BaseFilter; }

        auto CompiledFilter = BaseFilter->GetCopy();
        const auto CompiledFilterIsValid = CompiledFilter.IsValid();
        CK_ENSURE_IF_NOT(CompiledFilterIsValid,
            TEXT("Nav query filter resolution failed: could not copy the default filter of NavData [{}]"),
            InNavData.GetName())
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

        const auto QueryFilter = Get_CompiledQueryFilter(*NavData, InQuery.Get_QueryFilter(), {});
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

        if (TryGet_NavData(InWorld) == nullptr)
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
}

// --------------------------------------------------------------------------------------------------------------------
