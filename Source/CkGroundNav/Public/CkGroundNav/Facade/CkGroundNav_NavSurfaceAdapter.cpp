#include "CkGroundNav/Facade/CkGroundNav_NavSurfaceAdapter.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Boundary.h"
#include "CkGroundNav/Query/CkGroundNav_Query_BuildStatus.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"
#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::nav_surface_adapter_private
{
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

    // ----------------------------------------------------------------------------------------------------------------

    auto Do_ProjectPoint(
        UWorld*                                InWorld,
        const FCk_NavSurface_ProjectionQuery&  InQuery) -> FCk_NavSurface_ProjectionResult
    {
        auto Result = FCk_NavSurface_ProjectionResult{};

        const auto Field = world_fields::TryGet_Field(InWorld, InQuery.Get_Location());

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

        const auto Field = world_fields::TryGet_Field(InWorld, InQuery.Get_Start());

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

        const auto Field = world_fields::TryGet_Field(InWorld, InQuery.Get_Start());

        if (NOT Field.IsValid())
        {
            Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
            return Result;
        }

        auto Query = FCk_GroundNav_RaycastQuery{};
        Query._Start = InQuery.Get_Start();
        Query._End = InQuery.Get_End();
        Query._StartVerticalToleranceUu = Get_VerticalToleranceUu();

        const auto RaycastResult = Get_SurfaceRaycast(*Field, Query);

        Result.Set_Status(RaycastResult._Status);
        Result.Set_HitLocation(RaycastResult._HitLocation);

        return Result;
    }

    auto Do_BoundarySegments(
        UWorld*                             InWorld,
        const FCk_NavSurface_BoundaryQuery& InQuery) -> FCk_NavSurface_BoundaryResult
    {
        auto Result = FCk_NavSurface_BoundaryResult{};

        const auto Field = world_fields::TryGet_Field(InWorld, InQuery.Get_Center());

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

        const auto Field = world_fields::TryGet_Field(InWorld, InQuery.Get_Start());

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

    auto Do_SurfaceRevision(
        UWorld* InWorld) -> int64
    {
        auto Revision = int64{0};

        for (const auto& Field : world_fields::Get_Fields(InWorld))
        { Revision = FMath::Max(Revision, Field->_Epoch._Value); }

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
    Table._SurfaceBounds = &nav_surface_adapter_private::Do_SurfaceBounds;
    Table._ProviderHealth = &nav_surface_adapter_private::Do_ProviderHealth;
    Table._IsBuildInProgress = &nav_surface_adapter_private::Do_IsBuildInProgress;
    Table._SurfaceRevision = &nav_surface_adapter_private::Do_SurfaceRevision;
    Table._RequestSurfaceRebuild = &nav_surface_adapter_private::Do_RequestSurfaceRebuild;

    ck::nav_surface::Register_Provider(ECk_NavSurface_Provider::GroundNav, MoveTemp(Table));
}

// --------------------------------------------------------------------------------------------------------------------
