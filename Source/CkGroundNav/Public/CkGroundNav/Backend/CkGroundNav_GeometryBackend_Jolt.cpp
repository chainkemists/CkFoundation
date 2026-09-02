#include "CkGroundNav_GeometryBackend_Jolt.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_geometrybackend_jolt
{
    // A soup's indices are relative to its own vertex array, so they are rebased onto whatever the batch
    // already holds. Shared by the region fetch and the per-body fetch: both APPEND, so a caller may
    // accumulate several regions or several bodies into one batch, and two copies of the rebase would
    // drift the moment either changed.
    static auto DoAppend_SoupToBatch(
        const ck::jolt::FCk_Jolt_TriangleSoup& InSoup,
        FCk_GroundNav_GeometryBatch& OutBatch) -> void
    {
        const auto FirstVertex = OutBatch._Vertices.Num();

        OutBatch._Vertices.Append(InSoup._Vertices);

        OutBatch._Indices.Reserve(OutBatch._Indices.Num() + InSoup._Indices.Num());

        for (const auto& Index : InSoup._Indices)
        { OutBatch._Indices.Emplace(FirstVertex + Index); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    FCk_GroundNav_GeometryBackend_Jolt::
        FCk_GroundNav_GeometryBackend_Jolt(
            const UObject* InWorldContextObject)
    {
        const auto WorldContextIsValid = ck::IsValid(InWorldContextObject);

        CK_ENSURE_IF_NOT(WorldContextIsValid,
            TEXT("Cannot resolve a GroundNav geometry backend without a World Context Object"))
        { return; }

        _Session = ck::jolt::FCk_Jolt_QuerySession{InWorldContextObject};
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_IsValid() const
        -> bool
    {
        return _Session.Get_IsValid();
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_HasGeometryInBounds(
            const FBox& InBounds) const
        -> bool
    {
        auto BodyIds = TArray<uint64>{};
        _Session.Get_BodiesInAABox(InBounds, BodyIds);

        return NOT BodyIds.IsEmpty();
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_StaticBodiesInBounds(
            const FBox& InBounds,
            TArray<FCk_GroundNav_BodyRef>& OutBodies) const
        -> int32
    {
        OutBodies.Reset();

        auto BodyIds = TArray<uint64>{};
        _Session.Get_BodiesInAABox(InBounds, BodyIds);

        OutBodies.Reserve(BodyIds.Num());

        for (const auto& BodyId : BodyIds)
        { OutBodies.Emplace(FCk_GroundNav_BodyRef{BodyId}); }

        return OutBodies.Num();
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_TrianglesInBounds(
            const FBox& InBounds,
            FCk_GroundNav_GeometryBatch& OutBatch) const
        -> int32
    {
        auto Soup = ck::jolt::FCk_Jolt_TriangleSoup{};

        const auto TriangleCount = _Session.Get_StaticTrianglesInAABox(InBounds, Soup);

        if (TriangleCount == 0)
        { return 0; }

        ck_groundnav_geometrybackend_jolt::DoAppend_SoupToBatch(Soup, OutBatch);

        return TriangleCount;
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_WorldRevision() const
        -> uint64
    {
        return _Session.Get_StaticWorldRevision();
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_BodyKind(
            const FCk_GroundNav_BodyRef& InBody) const
        -> ECk_GroundNav_BodyKind
    {
        // A body the session can no longer read reports Solid deliberately: it then yields no triangles, and
        // a check with no geometry to inspect must stay silent. Reporting Surface instead would exempt a
        // body from the closure contract on the strength of the session having lost it.
        return _Session.Get_StaticBodyKind(InBody._Value) == ck::jolt::ECk_Jolt_StaticBodyKind::Surface
            ? ECk_GroundNav_BodyKind::Surface
            : ECk_GroundNav_BodyKind::Solid;
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_BodyBounds(
            const FCk_GroundNav_BodyRef& InBody) const
        -> FBox
    {
        return _Session.Get_StaticBodyBounds(InBody._Value);
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_BodyTriangles(
            const FCk_GroundNav_BodyRef& InBody,
            FCk_GroundNav_GeometryBatch& OutBatch) const
        -> int32
    {
        auto Soup = ck::jolt::FCk_Jolt_TriangleSoup{};

        const auto TriangleCount = _Session.Get_StaticBodyTriangles(InBody._Value, Soup);

        if (TriangleCount == 0)
        { return 0; }

        ck_groundnav_geometrybackend_jolt::DoAppend_SoupToBatch(Soup, OutBatch);

        return TriangleCount;
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_BodyDescription(
            const FCk_GroundNav_BodyRef& InBody) const
        -> FString
    {
        return _Session.Get_StaticBodyDescription(InBody._Value);
    }
}

// --------------------------------------------------------------------------------------------------------------------
