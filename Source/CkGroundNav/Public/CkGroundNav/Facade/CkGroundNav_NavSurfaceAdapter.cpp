#include "CkGroundNav/Facade/CkGroundNav_NavSurfaceAdapter.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"
#include "CkGroundNav/Debug/CkGroundNav_DebugGates.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldMarkupCost.h"
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

        // A volume is what HOLDS a record, so a paint that reaches none has nowhere to be recorded and
        // nothing to become live on. That is a caller error rather than a deferred paint: this provider
        // answers for the ground its volumes cover, and there is no volume here to cover this one.
        CK_ENSURE_IF_NOT(AnyVolumeTookIt,
            TEXT("GroundNav cannot apply the area markup on [{}] - its bounds [{}] meet no ground-nav "
                 "volume in world [{}], and a volume is the only thing that holds a record"),
            InMarkupEntity, MarkupBounds, GetNameSafe(InWorld))
        { return false; }

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
    { return false; }

    auto ReachedAnyTile = false;

    for (const auto& Tile : InField._Tiles)
    {
        if (NOT Get_TileWorldBounds(InField._Params, Tile).Intersect(RecordBounds))
        { continue; }

        ReachedAnyTile = true;

        const auto TileCarriesTheRecord = Tile.Get_IsBuilt() &&
                                          Tile._Epoch._Value > InRecord.Get_RequestedAtEpoch();

        if (NOT TileCarriesTheRecord)
        { return false; }
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
    { return false; }

    // Debug-only and off unless a run asked for it. Forcing this true makes a fixture that settles
    // on liveness wait for nothing. It sits after the guards above so it can never report a markup
    // that was never recorded on a volume as live.
    if (debug::Get_IsMarkupLiveGateBypassed())
    { return true; }

    const auto& MarkupRef = InMarkupEntity.Get<ck::FFragment_GroundNav_MarkupRef>();

    auto VolumeEntity = MarkupRef.Get_VolumeEntity();

    auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

    if (ck::Is_NOT_Valid(Volume))
    { return false; }

    const auto Record = UCk_Utils_GroundNavVolume_UE::TryGet_MarkupRecord(
        Volume, MarkupRef.Get_RecordId());

    if (NOT Record.IsSet())
    { return false; }

    // A disabled markup has no paint to be live, which is also the answer the Recast provider gives
    // once it has torn its painter down; the two providers must agree on what the flag means.
    if (Record->Get_Enable() == ECk_EnableDisable::Disable)
    { return false; }

    const auto Field = UCk_Utils_GroundNavVolume_UE::Get_Field(Volume);

    if (NOT Field.IsValid())
    { return false; }

    return Get_IsMarkupLive(*Field, *Record);
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
    Table._ApplyAreaMarkup = &nav_surface_adapter_private::Do_ApplyAreaMarkup;
    Table._IsMarkupLive = &nav_surface_adapter_private::Do_IsMarkupLive;
    Table._ReleaseAreaMarkup = &nav_surface_adapter_private::Do_ReleaseAreaMarkup;

    ck::nav_surface::Register_Provider(ECk_NavSurface_Provider::GroundNav, MoveTemp(Table));
}

// --------------------------------------------------------------------------------------------------------------------
