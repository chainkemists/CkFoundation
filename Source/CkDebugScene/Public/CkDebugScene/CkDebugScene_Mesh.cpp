#include "CkDebugScene_Mesh.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Algo/Sort.h>
#include <Engine/StaticMesh.h>
#include <MeshDescription.h>
#include <StaticMeshAttributes.h>
#include <StaticMeshOperations.h>

namespace ck_debug_scene_mesh
{
constexpr int32 LeafTriangleCount = 8;

auto
IsFinite(const FVector& InVector) -> bool
{
    return FMath::IsFinite(InVector.X) && FMath::IsFinite(InVector.Y) && FMath::IsFinite(InVector.Z);
}

auto
Get_TriangleBounds(const FCk_DebugScene_Triangle& InTriangle) -> FBox
{
    auto Bounds = FBox{ForceInit};
    Bounds += InTriangle._A;
    Bounds += InTriangle._B;
    Bounds += InTriangle._C;
    return Bounds;
}

auto
RayHitsBox(const FBox& InBounds, const FVector& InOrigin, const FVector& InDirection, double InMaxDistance) -> bool
{
    double Near = 0.0;
    double Far = InMaxDistance;

    for (const auto Axis : {0, 1, 2})
    {
        const auto Origin = static_cast<double>(InOrigin[Axis]);
        const auto Direction = static_cast<double>(InDirection[Axis]);
        const auto Minimum = static_cast<double>(InBounds.Min[Axis]);
        const auto Maximum = static_cast<double>(InBounds.Max[Axis]);

        if (FMath::IsNearlyZero(Direction))
        {
            if (Origin < Minimum || Origin > Maximum)
            {
                return false;
            }
            continue;
        }

        auto T0 = (Minimum - Origin) / Direction;
        auto T1 = (Maximum - Origin) / Direction;
        if (T0 > T1)
        {
            Swap(T0, T1);
        }

        Near = FMath::Max(Near, T0);
        Far = FMath::Min(Far, T1);
        if (Near > Far)
        {
            return false;
        }
    }

    return Far >= 0.0;
}
} // namespace ck_debug_scene_mesh

FCk_DebugScene_Mesh::
    FCk_DebugScene_Mesh(
        TArray<FCk_DebugScene_Triangle> InTriangles)
    : _Triangles(MoveTemp(InTriangles))
{
    for (const auto& Triangle : _Triangles)
    {
        _LocalBounds += Triangle._A;
        _LocalBounds += Triangle._B;
        _LocalBounds += Triangle._C;
    }
}

auto
    FCk_DebugScene_Mesh::
    Create_FromTriangles(TArray<FCk_DebugScene_Triangle> InTriangles)
    -> TSharedPtr<FCk_DebugScene_Mesh>
{
    const auto HasTriangles = NOT InTriangles.IsEmpty();
    CK_ENSURE_IF_NOT(HasTriangles, TEXT("CkDebugScene rejected a mesh with no triangles"))
    {
        return {};
    }

    auto InputIsValid = true;
    for (const auto& Triangle : InTriangles)
    {
        const auto HasFiniteCoordinates = ck_debug_scene_mesh::IsFinite(Triangle._A) &&
                                          ck_debug_scene_mesh::IsFinite(Triangle._B) &&
                                          ck_debug_scene_mesh::IsFinite(Triangle._C);
        const auto HasArea =
            FVector::CrossProduct(Triangle._B - Triangle._A, Triangle._C - Triangle._A).SizeSquared() > SMALL_NUMBER;
        InputIsValid &= HasFiniteCoordinates && HasArea;
    }

    CK_ENSURE_IF_NOT(InputIsValid, TEXT("CkDebugScene rejected invalid mesh triangle input"))
    {
        return {};
    }

    TSharedPtr<FCk_DebugScene_Mesh> Result = MakeShareable(new FCk_DebugScene_Mesh{MoveTemp(InTriangles)});
    auto Description = FMeshDescription{};
    auto Attributes = FStaticMeshAttributes{Description};
    Attributes.Register();
    auto VertexPositions = Attributes.GetVertexPositions();
    const auto PolygonGroup = Description.CreatePolygonGroup();
    const auto MaterialSlotName = FName{TEXT("CkDebugScene")};
    Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroup] = MaterialSlotName;

    // Reserve up front. Without this the mesh description reallocates its way through a
    // city-sized navmesh one vertex at a time.
    const auto TriangleCount = Result->_Triangles.Num();
    Description.ReserveNewVertices(TriangleCount * 3);
    Description.ReserveNewVertexInstances(TriangleCount * 3);
    Description.ReserveNewTriangles(TriangleCount);
    Description.ReserveNewPolygons(TriangleCount);

    auto VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
    auto VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
    auto VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();

    for (const auto& Triangle : Result->_Triangles)
    {
        // Flat face normal, taken from the cross product this function already needs for its area
        // check. Debug geometry is flat-shaded triangle soup with no shared vertices, so the
        // smoothing-group aware FStaticMeshOperations pass that used to run here had nothing to
        // smooth — it just cost ~53ms per rebuild on a real navmesh, measured. Tangents only matter
        // to normal-mapped materials, which debug draw does not use, so an arbitrary consistent
        // basis is enough.
        // Cross order is (C-A) x (B-A), matching the orientation FStaticMeshOperations used to
        // produce here. The opposite order yields unit normals pointing the wrong way, which
        // Ck.Jolt.DebugDraw.SingleTriangleBuild catches as "the UE winding preserves Jolt's
        // exterior normal" while still passing its normalized check.
        const auto Edge1 = Triangle._B - Triangle._A;
        const auto Edge2 = Triangle._C - Triangle._A;
        const auto Normal = FVector3f{FVector::CrossProduct(Edge2, Edge1).GetSafeNormal()};
        const auto Tangent = FVector3f{Edge1.GetSafeNormal()};

        auto VertexInstances = TArray<FVertexInstanceID>{};
        VertexInstances.Reserve(3);

        for (const auto& Position : {Triangle._A, Triangle._B, Triangle._C})
        {
            const auto Vertex = Description.CreateVertex();
            VertexPositions[Vertex] = FVector3f{Position};
            const auto Instance = Description.CreateVertexInstance(Vertex);
            VertexInstanceNormals[Instance] = Normal;
            VertexInstanceTangents[Instance] = Tangent;
            VertexInstanceBinormalSigns[Instance] = 1.0f;
            VertexInstances.Emplace(Instance);
        }

        Description.CreatePolygon(PolygonGroup, VertexInstances);
    }

    auto* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), NAME_None, RF_Transient);
    const auto MeshWasCreated = ck::IsValid(Mesh);
    CK_ENSURE_IF_NOT(MeshWasCreated, TEXT("CkDebugScene failed to create its transient static mesh"))
    {
        return {};
    }

    Mesh->SetStaticMaterials({FStaticMaterial{nullptr, MaterialSlotName}});

    auto Params = UStaticMesh::FBuildMeshDescriptionsParams{};
    Params.bBuildSimpleCollision = false;
    Params.bFastBuild = true;
    Params.bCommitMeshDescription = false;
    Params.bMarkPackageDirty = false;

    const auto MeshBuilt = Mesh->BuildFromMeshDescriptions({&Description}, Params);
    CK_ENSURE_IF_NOT(MeshBuilt, TEXT("CkDebugScene failed to build a transient static mesh from triangles"))
    {
        return {};
    }

    Result->_StaticMesh.Reset(Mesh);
    return Result;
}

auto
    FCk_DebugScene_Mesh::
    Get_StaticMesh() const
    -> UStaticMesh*
{
    return _StaticMesh.Get();
}

auto
    FCk_DebugScene_Mesh::
    Get_LocalBounds() const
    -> const FBox&
{
    return _LocalBounds;
}

auto
    FCk_DebugScene_Mesh::
    Ensure_Bvh() const
    -> void
{
    if (_BvhBuilt)
    {
        return;
    }

    _BvhTriangleIndices.Reset(_Triangles.Num());
    for (auto Index = 0; Index < _Triangles.Num(); ++Index)
    {
        _BvhTriangleIndices.Emplace(Index);
    }

    _BvhNodes.Reset();
    if (NOT _BvhTriangleIndices.IsEmpty())
    {
        Build_Bvh(_BvhTriangleIndices, 0, _BvhTriangleIndices.Num());
    }

    _BvhBuilt = true;
}

auto
    FCk_DebugScene_Mesh::
    Build_Bvh(TArray<int32>& InOutTriangleIndices, int32 InBegin, int32 InEnd) const
    -> int32
{
    auto Node = FBvhNode{};
    for (auto Index = InBegin; Index < InEnd; ++Index)
    {
        Node._Bounds += ck_debug_scene_mesh::Get_TriangleBounds(_Triangles[InOutTriangleIndices[Index]]);
    }

    const auto NodeIndex = _BvhNodes.Add(MoveTemp(Node));
    const auto Count = InEnd - InBegin;
    if (Count <= ck_debug_scene_mesh::LeafTriangleCount)
    {
        _BvhNodes[NodeIndex]._FirstTriangle = InBegin;
        _BvhNodes[NodeIndex]._TriangleCount = Count;
        return NodeIndex;
    }

    const auto Extent = _BvhNodes[NodeIndex]._Bounds.GetExtent();
    const auto Axis = Extent.X >= Extent.Y && Extent.X >= Extent.Z ? 0 : (Extent.Y >= Extent.Z ? 1 : 2);
    const auto Middle = InBegin + Count / 2;
    auto Triangles = MakeArrayView(InOutTriangleIndices.GetData() + InBegin, Count);
    Algo::Sort(Triangles,
               [&](int32 InLeft, int32 InRight)
               {
                   return ck_debug_scene_mesh::Get_TriangleBounds(_Triangles[InLeft]).GetCenter()[Axis] <
                          ck_debug_scene_mesh::Get_TriangleBounds(_Triangles[InRight]).GetCenter()[Axis];
               });

    _BvhNodes[NodeIndex]._Left = Build_Bvh(InOutTriangleIndices, InBegin, Middle);
    _BvhNodes[NodeIndex]._Right = Build_Bvh(InOutTriangleIndices, Middle, InEnd);
    return NodeIndex;
}

auto
    FCk_DebugScene_Mesh::
    TryIntersect_Triangle(const FCk_DebugScene_Triangle& InTriangle, const FVector& InOrigin,
                                           const FVector& InDirection, double InMaxDistance, double& OutDistance) const
    -> bool
{
    const auto EdgeOne = InTriangle._B - InTriangle._A;
    const auto EdgeTwo = InTriangle._C - InTriangle._A;
    const auto P = FVector::CrossProduct(InDirection, EdgeTwo);
    const auto Determinant = FVector::DotProduct(EdgeOne, P);
    if (FMath::IsNearlyZero(Determinant))
    {
        return false;
    }

    const auto InverseDeterminant = 1.0 / static_cast<double>(Determinant);
    const auto T = InOrigin - InTriangle._A;
    const auto U = static_cast<double>(FVector::DotProduct(T, P)) * InverseDeterminant;
    if (U < 0.0 || U > 1.0)
    {
        return false;
    }

    const auto Q = FVector::CrossProduct(T, EdgeOne);
    const auto V = static_cast<double>(FVector::DotProduct(InDirection, Q)) * InverseDeterminant;
    if (V < 0.0 || U + V > 1.0)
    {
        return false;
    }

    const auto Distance = static_cast<double>(FVector::DotProduct(EdgeTwo, Q)) * InverseDeterminant;
    if (Distance < 0.0 || Distance >= InMaxDistance)
    {
        return false;
    }

    OutDistance = Distance;
    return true;
}

auto
    FCk_DebugScene_Mesh::
    TryIntersect_Ray(const FVector& InLocalOrigin, const FVector& InLocalDirection,
                                      double InMaxDistance, double& OutDistance) const -> bool
{
    Ensure_Bvh();
    if (_BvhNodes.IsEmpty())
    {
        return false;
    }

    auto Nearest = InMaxDistance;
    int32 NodeStack[64];
    auto StackSize = 0;
    NodeStack[StackSize++] = 0;
    auto DidHit = false;

    while (StackSize > 0)
    {
        const auto NodeIndex = NodeStack[--StackSize];
        const auto& Node = _BvhNodes[NodeIndex];
        if (NOT ck_debug_scene_mesh::RayHitsBox(Node._Bounds, InLocalOrigin, InLocalDirection, Nearest))
        {
            continue;
        }

        if (Node._TriangleCount > 0)
        {
            for (auto Index = Node._FirstTriangle; Index < Node._FirstTriangle + Node._TriangleCount; ++Index)
            {
                DidHit |= TryIntersect_Triangle(_Triangles[_BvhTriangleIndices[Index]], InLocalOrigin, InLocalDirection,
                                                Nearest, Nearest);
            }
            continue;
        }

        NodeStack[StackSize++] = Node._Left;
        NodeStack[StackSize++] = Node._Right;
    }

    if (NOT DidHit)
    {
        return false;
    }

    OutDistance = Nearest;
    return true;
}
