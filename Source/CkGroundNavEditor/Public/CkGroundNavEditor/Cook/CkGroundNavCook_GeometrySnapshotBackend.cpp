#include "CkGroundNavCook_GeometrySnapshotBackend.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::cook
{
    FCk_GroundNav_CookGeometrySnapshotBackend::
        FCk_GroundNav_CookGeometrySnapshotBackend(
            const ck::jolt::cook::FCk_Jolt_CookGeometrySnapshot& InSnapshot,
            FCk_GroundNav_DataLayerSelector InSelector)
        : _Snapshot(&InSnapshot)
        , _Selector(MoveTemp(InSelector))
    { }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_IsValid() const
        -> bool
    {
        return _Snapshot != nullptr && _Snapshot->Get_IsComplete() && _Selector.Get_IsCanonical();
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_IsSelected(
            const ck::jolt::cook::FCk_Jolt_CookGeometryBody& InBody) const
        -> bool
    {
        return _Selector.Get_IsAll() || _Selector.Get_MatchesAny(InBody._DataLayerNames);
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_Body(
            const FCk_GroundNav_BodyRef& InBody) const
        -> const ck::jolt::cook::FCk_Jolt_CookGeometryBody*
    {
        if (NOT Get_IsValid() || InBody._Value == 0 ||
            InBody._Value > static_cast<uint64>(_Snapshot->Get_Bodies().Num()))
        { return nullptr; }

        const auto& Body = _Snapshot->Get_Bodies()[static_cast<int32>(InBody._Value - 1)];
        return Get_IsSelected(Body) ? &Body : nullptr;
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Append(
            const ck::jolt::FCk_Jolt_TriangleSoup& InSoup,
            FCk_GroundNav_GeometryBatch& OutBatch)
        -> int32
    {
        const auto FirstVertex = OutBatch._Vertices.Num();
        OutBatch._Vertices.Append(InSoup._Vertices);

        OutBatch._Indices.Reserve(OutBatch._Indices.Num() + InSoup._Indices.Num());
        for (const auto Index : InSoup._Indices)
        { OutBatch._Indices.Emplace(FirstVertex + Index); }

        return InSoup.Get_TriangleCount();
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_HasGeometryInBounds(
            const FBox& InBounds) const
        -> bool
    {
        auto Bodies = TArray<FCk_GroundNav_BodyRef>{};
        return Get_StaticBodiesInBounds(InBounds, Bodies) > 0;
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_StaticBodiesInBounds(
            const FBox& InBounds,
            TArray<FCk_GroundNav_BodyRef>& OutBodies) const
        -> int32
    {
        OutBodies.Reset();
        if (NOT Get_IsValid())
        { return 0; }

        for (auto Index = 0; Index < _Snapshot->Get_Bodies().Num(); ++Index)
        {
            const auto& Body = _Snapshot->Get_Bodies()[Index];
            if (Get_IsSelected(Body) && Body._Bounds.Intersect(InBounds))
            { OutBodies.Emplace(FCk_GroundNav_BodyRef{static_cast<uint64>(Index + 1)}); }
        }

        return OutBodies.Num();
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_TrianglesInBounds(
            const FBox& InBounds,
            FCk_GroundNav_GeometryBatch& OutBatch) const
        -> int32
    {
        auto Bodies = TArray<FCk_GroundNav_BodyRef>{};
        Get_StaticBodiesInBounds(InBounds, Bodies);

        auto TriangleCount = 0;
        for (const auto Body : Bodies)
        {
            const auto* Value = Get_Body(Body);
            if (Value == nullptr)
            { continue; }

            for (auto Triangle = 0; Triangle < Value->_Triangles.Get_TriangleCount(); ++Triangle)
            {
                const auto Base = Triangle * 3;
                const auto A = Value->_Triangles._Vertices[Value->_Triangles._Indices[Base]];
                const auto B = Value->_Triangles._Vertices[Value->_Triangles._Indices[Base + 1]];
                const auto C = Value->_Triangles._Vertices[Value->_Triangles._Indices[Base + 2]];

                auto TriangleBounds = FBox{ForceInit};
                TriangleBounds += A;
                TriangleBounds += B;
                TriangleBounds += C;
                if (NOT TriangleBounds.Intersect(InBounds))
                { continue; }

                OutBatch.Add_Triangle(A, B, C);
                ++TriangleCount;
            }
        }

        return TriangleCount;
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_BodyKind(
            const FCk_GroundNav_BodyRef& InBody) const
        -> ECk_GroundNav_BodyKind
    {
        const auto* Body = Get_Body(InBody);
        return Body != nullptr && Body->_Kind == ck::jolt::ECk_Jolt_StaticBodyKind::Surface
            ? ECk_GroundNav_BodyKind::Surface
            : ECk_GroundNav_BodyKind::Solid;
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_BodyBounds(
            const FCk_GroundNav_BodyRef& InBody) const
        -> FBox
    {
        const auto* Body = Get_Body(InBody);
        return Body != nullptr ? Body->_Bounds : FBox{ForceInit};
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_BodyTriangles(
            const FCk_GroundNav_BodyRef& InBody,
            FCk_GroundNav_GeometryBatch& OutBatch) const
        -> int32
    {
        const auto* Body = Get_Body(InBody);
        return Body != nullptr ? Append(Body->_Triangles, OutBatch) : 0;
    }

    auto
        FCk_GroundNav_CookGeometrySnapshotBackend::
        Get_BodyDescription(
            const FCk_GroundNav_BodyRef& InBody) const
        -> FString
    {
        const auto* Body = Get_Body(InBody);
        return Body != nullptr
            ? Body->_Description
            : FString::Printf(TEXT("Snapshot body %llu (not held)"), InBody._Value);
    }
}

// --------------------------------------------------------------------------------------------------------------------
