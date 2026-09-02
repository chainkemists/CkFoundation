#include "CkGroundNav_GeometryBackend_Jolt.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

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

        // The soup's indices are relative to its own vertex array, so they are rebased onto whatever the
        // batch already holds — this call APPENDS, so a caller may accumulate several regions.
        const auto FirstVertex = OutBatch._Vertices.Num();

        OutBatch._Vertices.Append(Soup._Vertices);

        OutBatch._Indices.Reserve(OutBatch._Indices.Num() + Soup._Indices.Num());

        for (const auto& Index : Soup._Indices)
        { OutBatch._Indices.Emplace(FirstVertex + Index); }

        return TriangleCount;
    }

    auto
        FCk_GroundNav_GeometryBackend_Jolt::
        Get_WorldRevision() const
        -> uint64
    {
        return _Session.Get_StaticWorldRevision();
    }
}

// --------------------------------------------------------------------------------------------------------------------
