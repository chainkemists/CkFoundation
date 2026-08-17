#include "CkJolt_DebugRenderer.h"

#if JPH_DEBUG_RENDERER

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"

#include <Algo/Sort.h>
#include <Components/InstancedStaticMeshComponent.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <MeshDescription.h>
#include <Misc/CoreDelegates.h>
#include <StaticMeshAttributes.h>
#include <StaticMeshOperations.h>
#include <UObject/Package.h>
#include <UObject/StrongObjectPtr.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_DebugDraw_Reconcile"), STAT_CkJolt_DebugDrawReconcile, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debug_renderer
{
    auto
        TryIntersect_RayBox(
            const FVector& InOrigin,
            const FVector& InDirection,
            const FBox3f& InBox,
            double InMaxDistance)
        -> TOptional<double>
    {
        auto EntryDistance = 0.0;
        auto ExitDistance = InMaxDistance;

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto AxisDirection = InDirection[Axis];
            const auto AxisOrigin = InOrigin[Axis];

            if (FMath::IsNearlyZero(AxisDirection))
            {
                if (AxisOrigin < static_cast<double>(InBox.Min[Axis]) ||
                    AxisOrigin > static_cast<double>(InBox.Max[Axis]))
                { return {}; }

                continue;
            }

            const auto InverseDirection = 1.0 / AxisDirection;
            auto NearDistance = (static_cast<double>(InBox.Min[Axis]) - AxisOrigin) * InverseDirection;
            auto FarDistance = (static_cast<double>(InBox.Max[Axis]) - AxisOrigin) * InverseDirection;

            if (NearDistance > FarDistance)
            { Swap(NearDistance, FarDistance); }

            EntryDistance = FMath::Max(EntryDistance, NearDistance);
            ExitDistance = FMath::Min(ExitDistance, FarDistance);

            if (EntryDistance > ExitDistance)
            { return {}; }
        }

        return EntryDistance < InMaxDistance ? TOptional<double>{EntryDistance} : TOptional<double>{};
    }

    auto
        TryIntersect_RayTriangle(
            const FVector& InOrigin,
            const FVector& InDirection,
            const FVector& InA,
            const FVector& InB,
            const FVector& InC,
            double InMaxDistance)
        -> TOptional<double>
    {
        const auto EdgeAB = InB - InA;
        const auto EdgeAC = InC - InA;
        const auto DirectionCrossAC = FVector::CrossProduct(InDirection, EdgeAC);
        const auto Determinant = FVector::DotProduct(EdgeAB, DirectionCrossAC);

        constexpr auto ParallelTolerance = 1.0e-10;
        if (FMath::Abs(Determinant) <= ParallelTolerance)
        { return {}; }

        const auto InverseDeterminant = 1.0 / Determinant;
        const auto OriginFromA = InOrigin - InA;
        const auto BarycentricB = FVector::DotProduct(OriginFromA, DirectionCrossAC) * InverseDeterminant;

        if (BarycentricB < 0.0 || BarycentricB > 1.0)
        { return {}; }

        const auto OriginCrossAB = FVector::CrossProduct(OriginFromA, EdgeAB);
        const auto BarycentricC = FVector::DotProduct(InDirection, OriginCrossAB) * InverseDeterminant;

        if (BarycentricC < 0.0 || BarycentricB + BarycentricC > 1.0)
        { return {}; }

        const auto Distance = FVector::DotProduct(EdgeAC, OriginCrossAB) * InverseDeterminant;
        return Distance >= 0.0 && Distance < InMaxDistance
            ? TOptional<double>{Distance}
            : TOptional<double>{};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    class FBatch : public JPH::RefTargetVirtual
    {
    public:
        auto AddRef() -> void override { ++_RefCount; }
        auto Release() -> void override { if (--_RefCount == 0) { delete this; } }

        auto Get_RefCount() const -> uint32 { return _RefCount.load(); }

        auto GetOrBuild_Mesh() -> UStaticMesh*;
        auto TryIntersect_Ray(const FVector& InOrigin, const FVector& InDirection, double InMaxDistance)
            -> TOptional<double>;

    public:
        TArray<FVector3f> _Positions;
        TArray<uint32> _Indices;

    private:
        struct FPickTriangle
        {
            uint32 _A = 0;
            uint32 _B = 0;
            uint32 _C = 0;
            FVector3f _Centroid = FVector3f::ZeroVector;
        };

        struct FPickBvhNode
        {
            FBox3f _Bounds = FBox3f{ForceInit};
            int32 _FirstTriangle = 0;
            int32 _TriangleCount = 0;
            int32 _LeftChild = INDEX_NONE;
            int32 _RightChild = INDEX_NONE;

            auto Is_Leaf() const -> bool { return _TriangleCount > 0; }
        };

        auto Ensure_PickBvh() -> void;
        auto Build_PickBvhNode(int32 InFirstTriangle, int32 InTriangleCount) -> int32;

        TStrongObjectPtr<UStaticMesh> _Mesh;
        bool _BuildAttempted = false;
        bool _PickBvhBuilt = false;
        TArray<FPickTriangle> _PickTriangles;
        TArray<FPickBvhNode> _PickBvhNodes;
        std::atomic<uint32> _RefCount = 0;
    };

    auto
        FBatch::
        GetOrBuild_Mesh()
        -> UStaticMesh*
    {
        if (_BuildAttempted)
        { return _Mesh.Get(); }

        _BuildAttempted = true;

        if (_Positions.IsEmpty() || _Indices.IsEmpty())
        { return nullptr; }

        auto Description = FMeshDescription{};
        auto Attributes = FStaticMeshAttributes{Description};
        Attributes.Register();

        const auto Group = Description.CreatePolygonGroup();
        Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("JoltDebug");

        auto VertexPositions = Attributes.GetVertexPositions();

        auto Instances = TArray<FVertexInstanceID>{};
        Instances.Reserve(_Positions.Num());

        for (const auto& Position : _Positions)
        {
            const auto Vertex = Description.CreateVertex();
            VertexPositions[Vertex] = Position;
            Instances.Add(Description.CreateVertexInstance(Vertex));
        }

        const auto VertexCount = static_cast<uint32>(_Positions.Num());
        auto EmittedTriangles = 0;

        for (auto Index = 0; Index + 2 < _Indices.Num(); Index += 3)
        {
            const auto A = _Indices[Index];
            const auto B = _Indices[Index + 1];
            const auto C = _Indices[Index + 2];

            if (A == B || B == C || A == C)
            { continue; }

            if (A >= VertexCount || B >= VertexCount || C >= VertexCount)
            { continue; }

            const auto InstanceA = Instances[static_cast<int32>(A)];
            const auto InstanceB = Instances[static_cast<int32>(B)];
            const auto InstanceC = Instances[static_cast<int32>(C)];

            // Jolt's outward winding is the opposite of UE's mesh-description winding under the XYZ passthrough.
            Description.CreatePolygon(Group, TArray<FVertexInstanceID>{InstanceA, InstanceC, InstanceB});
            ++EmittedTriangles;
        }

        if (EmittedTriangles == 0)
        { return nullptr; }

        // BuildFromMeshDescriptions' runtime fast path copies the vertex-instance tangent basis verbatim; it
        // does not run the editor mesh builder that normally repairs missing normals. The old unlit debug
        // material hid that omission, but any DefaultLit material shades the zero normals black. Generate the
        // complete tangent basis once while the cached batch mesh is built so editor and packaged viewports
        // receive the same valid lighting data.
        FStaticMeshOperations::ComputeTriangleTangentsAndNormals(Description);
        FStaticMeshOperations::ComputeTangentsAndNormals(
            Description, EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents);

        auto* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), NAME_None, RF_Transient);
        Mesh->SetStaticMaterials({FStaticMaterial(nullptr, TEXT("JoltDebug"))});

        auto Params = UStaticMesh::FBuildMeshDescriptionsParams{};
        Params.bCommitMeshDescription = false;
        Params.bBuildSimpleCollision  = false;
        Params.bAllowCpuAccess       = false;
        Params.bMarkPackageDirty     = false;
        Params.bFastBuild            = true;

        CK_ENSURE_IF_NOT(Mesh->BuildFromMeshDescriptions({&Description}, Params),
            TEXT("BuildFromMeshDescriptions FAILED for a Jolt debug batch ([{}] verts, [{}] indices) — this batch will never draw"),
            _Positions.Num(), _Indices.Num())
        { return nullptr; }

        _Mesh = TStrongObjectPtr{Mesh};
        return Mesh;
    }

    auto
        FBatch::
        Ensure_PickBvh()
        -> void
    {
        if (_PickBvhBuilt)
        { return; }

        // Picking is driven by the Slate viewport on the game thread, after capture has finished publishing the
        // immutable batch. Build once on first use rather than making every captured shape pay for a BVH it may
        // never need.
        _PickBvhBuilt = true;

        const auto VertexCount = static_cast<uint32>(_Positions.Num());
        _PickTriangles.Reserve(_Indices.Num() / 3);

        for (auto Index = 0; Index + 2 < _Indices.Num(); Index += 3)
        {
            const auto A = _Indices[Index];
            const auto B = _Indices[Index + 1];
            const auto C = _Indices[Index + 2];

            if (A == B || B == C || A == C || A >= VertexCount || B >= VertexCount || C >= VertexCount)
            { continue; }

            auto Triangle = FPickTriangle{};
            Triangle._A = A;
            Triangle._B = B;
            Triangle._C = C;
            Triangle._Centroid = (_Positions[static_cast<int32>(A)] +
                                  _Positions[static_cast<int32>(B)] +
                                  _Positions[static_cast<int32>(C)]) / 3.0f;
            _PickTriangles.Emplace(MoveTemp(Triangle));
        }

        if (_PickTriangles.IsEmpty())
        { return; }

        _PickBvhNodes.Reserve(_PickTriangles.Num() * 2);
        Build_PickBvhNode(0, _PickTriangles.Num());
    }

    auto
        FBatch::
        Build_PickBvhNode(
            int32 InFirstTriangle,
            int32 InTriangleCount)
        -> int32
    {
        const auto NodeIndex = _PickBvhNodes.AddDefaulted();
        auto Bounds = FBox3f{ForceInit};
        auto CentroidBounds = FBox3f{ForceInit};

        for (auto Index = InFirstTriangle; Index < InFirstTriangle + InTriangleCount; ++Index)
        {
            const auto& Triangle = _PickTriangles[Index];
            Bounds += _Positions[static_cast<int32>(Triangle._A)];
            Bounds += _Positions[static_cast<int32>(Triangle._B)];
            Bounds += _Positions[static_cast<int32>(Triangle._C)];
            CentroidBounds += Triangle._Centroid;
        }

        constexpr auto MaxTrianglesPerLeaf = 12;
        if (InTriangleCount <= MaxTrianglesPerLeaf)
        {
            auto& Node = _PickBvhNodes[NodeIndex];
            Node._Bounds = Bounds;
            Node._FirstTriangle = InFirstTriangle;
            Node._TriangleCount = InTriangleCount;
            return NodeIndex;
        }

        const auto CentroidExtent = CentroidBounds.GetExtent();
        auto SplitAxis = 0;
        if (CentroidExtent.Y > CentroidExtent.X)
        { SplitAxis = 1; }
        if (CentroidExtent.Z > CentroidExtent[SplitAxis])
        { SplitAxis = 2; }

        auto Triangles = MakeArrayView(_PickTriangles.GetData() + InFirstTriangle, InTriangleCount);
        Algo::Sort(Triangles, [SplitAxis](const FPickTriangle& InLeft, const FPickTriangle& InRight)
        {
            return InLeft._Centroid[SplitAxis] < InRight._Centroid[SplitAxis];
        });

        const auto LeftCount = InTriangleCount / 2;
        const auto LeftChild = Build_PickBvhNode(InFirstTriangle, LeftCount);
        const auto RightChild = Build_PickBvhNode(InFirstTriangle + LeftCount, InTriangleCount - LeftCount);

        auto& Node = _PickBvhNodes[NodeIndex];
        Node._Bounds = Bounds;
        Node._LeftChild = LeftChild;
        Node._RightChild = RightChild;
        return NodeIndex;
    }

    auto
        FBatch::
        TryIntersect_Ray(
            const FVector& InOrigin,
            const FVector& InDirection,
            double InMaxDistance)
        -> TOptional<double>
    {
        Ensure_PickBvh();

        if (_PickBvhNodes.IsEmpty() || InDirection.IsNearlyZero() || InMaxDistance <= 0.0)
        { return {}; }

        auto NearestDistance = InMaxDistance;
        auto DidHit = false;

        // The tree is median-split, so its depth is bounded by the bit width of the int32 triangle count. This
        // fixed stack avoids allocating on every mouse move.
        int32 NodeStack[64];
        auto StackSize = 0;
        NodeStack[StackSize++] = 0;

        while (StackSize > 0)
        {
            const auto NodeIndex = NodeStack[--StackSize];
            const auto& Node = _PickBvhNodes[NodeIndex];

            if (NOT ck_jolt_debug_renderer::TryIntersect_RayBox(
                InOrigin, InDirection, Node._Bounds, NearestDistance).IsSet())
            { continue; }

            if (Node.Is_Leaf())
            {
                for (auto TriangleIndex = Node._FirstTriangle;
                     TriangleIndex < Node._FirstTriangle + Node._TriangleCount;
                     ++TriangleIndex)
                {
                    const auto& Triangle = _PickTriangles[TriangleIndex];
                    const auto HitDistance = ck_jolt_debug_renderer::TryIntersect_RayTriangle(
                        InOrigin,
                        InDirection,
                        FVector{_Positions[static_cast<int32>(Triangle._A)]},
                        FVector{_Positions[static_cast<int32>(Triangle._B)]},
                        FVector{_Positions[static_cast<int32>(Triangle._C)]},
                        NearestDistance);

                    if (HitDistance.IsSet())
                    {
                        NearestDistance = *HitDistance;
                        DidHit = true;
                    }
                }

                continue;
            }

            const auto LeftDistance = ck_jolt_debug_renderer::TryIntersect_RayBox(
                InOrigin, InDirection, _PickBvhNodes[Node._LeftChild]._Bounds, NearestDistance);
            const auto RightDistance = ck_jolt_debug_renderer::TryIntersect_RayBox(
                InOrigin, InDirection, _PickBvhNodes[Node._RightChild]._Bounds, NearestDistance);

            // LIFO: push the farther child first so the nearer child can lower the pruning distance sooner.
            if (LeftDistance.IsSet() && RightDistance.IsSet())
            {
                const auto NearChild = *LeftDistance <= *RightDistance ? Node._LeftChild : Node._RightChild;
                const auto FarChild = NearChild == Node._LeftChild ? Node._RightChild : Node._LeftChild;
                NodeStack[StackSize++] = FarChild;
                NodeStack[StackSize++] = NearChild;
            }
            else if (LeftDistance.IsSet())
            { NodeStack[StackSize++] = Node._LeftChild; }
            else if (RightDistance.IsSet())
            { NodeStack[StackSize++] = Node._RightChild; }
        }

        return DidHit ? TOptional<double>{NearestDistance} : TOptional<double>{};
    }

    auto
        TryIntersect_BatchRay(
            FBatch* InBatch,
            const FVector& InOrigin,
            const FVector& InDirection,
            double InMaxDistance)
        -> TOptional<double>
    {
        return InBatch != nullptr
            ? InBatch->TryIntersect_Ray(InOrigin, InDirection, InMaxDistance)
            : TOptional<double>{};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debug_renderer
{
    static TUniquePtr<FCk_Jolt_DebugRenderer> GRenderer;
    static bool GExitHandlerRegistered = false;

    // Per-batch live-bucket census across EVERY target; see Note_BucketHolder* in the impl header.
    static TMap<JPH::RefTargetVirtual*, int32> GBucketHolderCounts;

    struct FPendingDraw
    {
        ck::jolt::debug_draw::FBucketKey _Key;
        FTransform _Transform;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    auto
        Note_BucketHolderAdded(
            JPH::RefTargetVirtual* InBatch)
        -> void
    {
        if (InBatch == nullptr)
        { return; }

        ++ck_jolt_debug_renderer::GBucketHolderCounts.FindOrAdd(InBatch);
    }

    auto
        Note_BucketHolderRemoved(
            JPH::RefTargetVirtual* InBatch)
        -> void
    {
        if (InBatch == nullptr)
        { return; }

        auto* Count = ck_jolt_debug_renderer::GBucketHolderCounts.Find(InBatch);
        if (Count == nullptr)
        { return; }

        *Count = FMath::Max(0, *Count - 1);

        if (*Count == 0)
        { ck_jolt_debug_renderer::GBucketHolderCounts.Remove(InBatch); }
    }

    auto
        Get_BucketHolderCount(
            JPH::RefTargetVirtual* InBatch)
        -> int32
    {
        if (InBatch == nullptr)
        { return 0; }

        const auto* Count = ck_jolt_debug_renderer::GBucketHolderCounts.Find(InBatch);
        return Count != nullptr ? *Count : 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Jolt_DebugRenderer::FImpl
{
    FCk_Jolt_DebugDrawTarget* _ActiveTarget = nullptr;

    TArray<ck_jolt_debug_renderer::FPendingDraw> _PendingBodyDraws;
    uint64 _CaptureBodyKey = 0;
    uint8 _ActiveColorClassIndex = 0;
    bool _ActiveBodyIsSensor = false;
    bool _CaptureBodyOpen = false;
};

// --------------------------------------------------------------------------------------------------------------------

FCk_Jolt_DebugRenderer::FCk_Jolt_DebugRenderer()
    : _Impl(MakePimpl<FImpl>())
{
    // Base-class contract: builds the shared unit-geometry batches through our CreateTriangleBatch overrides.
    Initialize();
}

FCk_Jolt_DebugRenderer::~FCk_Jolt_DebugRenderer() = default;

auto
    FCk_Jolt_DebugRenderer::
    Get_OrCreate()
    -> FCk_Jolt_DebugRenderer&
{
    if (NOT ck_jolt_debug_renderer::GRenderer.IsValid())
    {
        ck_jolt_debug_renderer::GRenderer = MakeUnique<FCk_Jolt_DebugRenderer>();

        if (NOT ck_jolt_debug_renderer::GExitHandlerRegistered)
        {
            ck_jolt_debug_renderer::GExitHandlerRegistered = true;

            FCoreDelegates::OnEnginePreExit.AddLambda([]() -> void
            {
                ck_jolt_debug_renderer::GRenderer.Reset();
            });
        }
    }

    return *ck_jolt_debug_renderer::GRenderer;
}

auto
    FCk_Jolt_DebugRenderer::
    DrawLine(
        JPH::RVec3Arg inFrom,
        JPH::RVec3Arg inTo,
        JPH::ColorArg inColor)
    -> void
{
    const auto From = ck::jolt::Conv(inFrom);
    const auto To = ck::jolt::Conv(inTo);
    const auto Color = ck::jolt::Conv(inColor);

    // The recorder is tested FIRST, before the bound target, and that order is load-bearing. A bound target is a
    // game-thread capture, but Jolt's solve is multi-threaded and — in async mode — belongs to a DIFFERENT world
    // that may be stepping right now; appending to the target's unguarded line array from a solve worker is a
    // data race. The recording atomic is the only discriminator available inside DrawLine, so while any world is
    // recording, every line goes to the guarded buffer. When nothing records this is one acquire load and a
    // return. Accepted consequence: a capture that overlaps another world's async solve loses its own lines to
    // that record for the frame — a missing line beats a torn TArray.
    if (ck::jolt::debug_draw::TryRecord_ContactLine(From, To, Color))
    { return; }

    auto* Target = _Impl->_ActiveTarget;
    if (Target == nullptr)
    { return; }

    Target->_Impl->_JphLines.Emplace(ck::jolt::debug_draw::Make_DebugDrawLine(From, To, Color));
}

auto
    FCk_Jolt_DebugRenderer::
    DrawTriangle(
        JPH::RVec3Arg inV1,
        JPH::RVec3Arg inV2,
        JPH::RVec3Arg inV3,
        JPH::ColorArg inColor,
        ECastShadow inCastShadow)
    -> void
{
    // Rare path — solid geometry goes through CreateTriangleBatch/DrawGeometry.
    DrawLine(inV1, inV2, inColor);
    DrawLine(inV2, inV3, inColor);
    DrawLine(inV3, inV1, inColor);
}

auto
    FCk_Jolt_DebugRenderer::
    DrawText3D(
        JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor,
        float inHeight)
    -> void
{
    auto* Target = _Impl->_ActiveTarget;
    if (Target == nullptr)
    { return; }

    // inHeight is dropped on purpose: the facility stores labels rather than rendering them, and each consumer
    // (a viewport OnPaint projection, DrawDebugString) sizes text in its own space.
    Target->_Impl->_Labels.Emplace(FCk_Jolt_DebugDrawLabel{
        ck::jolt::Conv(inPosition),
        FString{static_cast<int32>(inString.length()), inString.data()},
        ck::jolt::Conv(inColor)});
}

auto
    FCk_Jolt_DebugRenderer::
    CreateTriangleBatch(
        const Triangle* inTriangles,
        int inTriangleCount)
    -> Batch
{
    auto* NewBatch = new ck::jolt::debug_draw::FBatch{};

    if (inTriangles != nullptr && inTriangleCount > 0)
    {
        NewBatch->_Positions.Reserve(inTriangleCount * 3);
        NewBatch->_Indices.Reserve(inTriangleCount * 3);

        for (auto Index = 0; Index < inTriangleCount; ++Index)
        {
            for (const auto& TriangleVertex : inTriangles[Index].mV)
            {
                NewBatch->_Indices.Add(NewBatch->_Positions.Num());
                NewBatch->_Positions.Emplace(TriangleVertex.mPosition.x, TriangleVertex.mPosition.y,
                    TriangleVertex.mPosition.z);
            }
        }
    }

    return NewBatch;
}

auto
    FCk_Jolt_DebugRenderer::
    CreateTriangleBatch(
        const Vertex* inVertices,
        int inVertexCount,
        const JPH::uint32* inIndices,
        int inIndexCount)
    -> Batch
{
    auto* NewBatch = new ck::jolt::debug_draw::FBatch{};

    if (inVertices != nullptr && inVertexCount > 0 && inIndices != nullptr && inIndexCount > 0)
    {
        NewBatch->_Positions.Reserve(inVertexCount);

        for (auto Index = 0; Index < inVertexCount; ++Index)
        {
            NewBatch->_Positions.Emplace(inVertices[Index].mPosition.x, inVertices[Index].mPosition.y,
                inVertices[Index].mPosition.z);
        }

        NewBatch->_Indices.Append(inIndices, inIndexCount);
    }

    return NewBatch;
}

auto
    FCk_Jolt_DebugRenderer::
    DrawGeometry(
        JPH::RMat44Arg inModelMatrix,
        const JPH::AABox& inWorldSpaceBounds,
        float inLODScaleSq,
        JPH::ColorArg inModelColor,
        const GeometryRef& inGeometry,
        ECullMode inCullMode,
        ECastShadow inCastShadow,
        EDrawMode inDrawMode)
    -> void
{
    auto* Target = _Impl->_ActiveTarget;
    if (Target == nullptr)
    { return; }

    // Instanced geometry only exists inside a body scope — that is what names the slot it reconciles against.
    // Constraint and contact drawing run outside one and are line-shaped; nothing there emits geometry.
    if (NOT _Impl->_CaptureBodyOpen)
    { return; }

    if (inGeometry.GetPtr() == nullptr || inGeometry->mLODs.empty())
    { return; }

    // Highest-detail LOD unconditionally — instancing makes the triangle count a GPU non-issue.
    const auto& TriangleBatch = inGeometry->mLODs.front().mTriangleBatch;
    if (TriangleBatch.GetPtr() == nullptr)
    { return; }

    auto* BatchImpl = static_cast<ck::jolt::debug_draw::FBatch*>(TriangleBatch.GetPtr());
    if (BatchImpl->_Indices.IsEmpty())
    { return; }

    const auto Key = ck::jolt::debug_draw::FBucketKey{
        BatchImpl, inModelColor.mU32, _Impl->_ActiveColorClassIndex, _Impl->_ActiveBodyIsSensor};
    auto& Bucket = Target->_Impl->_Buckets.FindOrAdd(Key);

    if (Bucket._BatchKeepAlive.GetPtr() == nullptr)
    {
        Bucket._BatchKeepAlive = TriangleBatch;
        Bucket._BaseColor = ck::jolt::Conv(inModelColor);
        ck::jolt::debug_draw::Note_BucketHolderAdded(BatchImpl);
    }

    const auto Transform = FTransform{ck::jolt::Conv(inModelMatrix)};

    _Impl->_PendingBodyDraws.Emplace(ck_jolt_debug_renderer::FPendingDraw{Key, Transform});
}

auto
    FCk_Jolt_DebugRenderer::
    TryEnsure_BucketIsm(
        FCk_Jolt_DebugDrawTarget& InTarget,
        const ck::jolt::debug_draw::FBucketKey& InKey,
        ck::jolt::debug_draw::FBucket& InOutBucket)
    -> bool
{
    if (ck::IsValid(InOutBucket._Ism.Get()))
    { return true; }

    if (InOutBucket._IsmCreateFailed)
    { return false; }

    auto* World = InTarget._Impl->_World.Get();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    auto* Mesh = InKey._Batch->GetOrBuild_Mesh();
    if (Mesh == nullptr)
    {
        // Empty batch or a build failure (the latter already ensured, loudly, once).
        InOutBucket._IsmCreateFailed = true;
        return false;
    }

    // Plain NewObject, not the pooling wrapper: the pool's DestroyOnRelease policy only ever provided pinning,
    // which the bucket's TStrongObjectPtr now provides — and preview/transient worlds host no pooling subsystem.
    auto* Ism = NewObject<UInstancedStaticMeshComponent>(World,
        MakeUniqueObjectName(World, UInstancedStaticMeshComponent::StaticClass(), TEXT("CkJoltDebugDrawIsm")));

    CK_ENSURE_IF_NOT(ck::IsValid(Ism),
        TEXT("Failed to create an InstancedStaticMeshComponent for the Jolt debug renderer"))
    {
        InOutBucket._IsmCreateFailed = true;
        return false;
    }

    Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Ism->SetCanEverAffectNavigation(false);
    Ism->SetCastShadow(false);
    Ism->SetMobility(EComponentMobility::Movable);

    // Registered at identity so component space == world space; Add/BatchUpdate pass WorldSpace=false.
    Ism->SetWorldLocation(FVector::ZeroVector);
    // No BeginPlay (protected here, unlike UProceduralMeshComponent) — registration alone gives the render state.
    Ism->RegisterComponentWithWorld(World);

    // The overlays are translucent geometry drawn over the body they trace. The contact glow sits inside hover
    // and selection by scale; priorities mirror that nesting so translucent sorting cannot make the shells jitter.
    if (InKey._ColorClassIndex == ck::jolt::debug_draw::SensorContactClassIndex)
    { Ism->SetTranslucentSortPriority(1); }
    else if (InKey._ColorClassIndex == ck::jolt::debug_draw::HighlightClassIndex ||
             InKey._ColorClassIndex == ck::jolt::debug_draw::HoverClassIndex)
    { Ism->SetTranslucentSortPriority(2); }

    // A bucket born into a hidden class must not flash into view before the next toggle.
    Ism->SetVisibility(InTarget.Get_IsClassVisible(InKey._ColorClassIndex));
    Ism->SetHiddenInGame(false);
    Ism->SetStaticMesh(Mesh);

    InOutBucket._Ism.Reset(Ism);
    InOutBucket._SlotCount = 0;

    ck::jolt::debug_draw::Apply_BucketMaterial(InOutBucket, InTarget._Impl->_Palette, InKey._ColorClassIndex,
        InKey._IsSensor, InTarget._Impl->_RenderMode);

    return true;
}

auto
    FCk_Jolt_DebugRenderer::
    BeginCapture(
        FCk_Jolt_DebugDrawTarget& InTarget)
    -> void
{
    _Impl->_ActiveTarget = &InTarget;
    _Impl->_CaptureBodyOpen = false;
    _Impl->_ActiveBodyIsSensor = false;
    _Impl->_PendingBodyDraws.Reset();

    InTarget._Impl->_LastCaptureStats = ck::jolt::debug_draw::FDebugDrawStats{};

    // JPH line and label output is per-frame, so the component is flushed here and refilled by this capture. The
    // retained External sub-channels are deliberately NOT cleared — EndCapture re-emits them as they stand.
    ck::jolt::debug_draw::Reset_LineChannels(*InTarget._Impl);
}

auto
    FCk_Jolt_DebugRenderer::
    BeginBody(
        uint64 InBodyKey,
        uint8 InColorClassIndex,
        bool InIsSensor)
    -> void
{
    _Impl->_CaptureBodyKey = InBodyKey;
    _Impl->_ActiveColorClassIndex = InColorClassIndex;
    _Impl->_ActiveBodyIsSensor = InIsSensor;
    _Impl->_CaptureBodyOpen = true;
    _Impl->_PendingBodyDraws.Reset();
}

auto
    FCk_Jolt_DebugRenderer::
    EndBody()
    -> void
{
    _Impl->_CaptureBodyOpen = false;

    auto* Target = _Impl->_ActiveTarget;
    if (Target == nullptr)
    {
        _Impl->_PendingBodyDraws.Reset();
        return;
    }

    const auto BodyKey = _Impl->_CaptureBodyKey;
    const auto& Pending = _Impl->_PendingBodyDraws;

    auto* ExistingSlots = Target->_Impl->_BodySlots.Find(BodyKey);

    // Every slot must still be addressable before ANY in-place update runs: a bucket whose ISM was destroyed
    // or whose instance id was invalidated has to fall through to the rebuild path, which re-creates the ISM.
    const auto SlotsStillMatch = [&]() -> bool
    {
        if (ExistingSlots == nullptr || ExistingSlots->Num() != Pending.Num())
        { return false; }

        for (auto Index = 0; Index < Pending.Num(); ++Index)
        {
            const auto& Slot = (*ExistingSlots)[Index];

            if (NOT (Slot._Bucket == Pending[Index]._Key))
            { return false; }

            auto* Bucket = Target->_Impl->_Buckets.Find(Slot._Bucket);
            if (Bucket == nullptr)
            { return false; }

            auto* Ism = Bucket->_Ism.Get();
            if (ck::Is_NOT_Valid(Ism) || NOT Ism->IsValidId(Slot._InstanceId))
            { return false; }
        }

        return true;
    }();

    if (SlotsStillMatch)
    {
        for (auto Index = 0; Index < Pending.Num(); ++Index)
        {
            const auto& Slot = (*ExistingSlots)[Index];
            auto* Ism = Target->_Impl->_Buckets.Find(Slot._Bucket)->_Ism.Get();

            constexpr auto WorldSpace = false;
            Ism->UpdateInstanceTransformById(Slot._InstanceId, Pending[Index]._Transform, WorldSpace);
            ++Target->_Impl->_LastCaptureStats._InstancesUpdated;
        }

        _Impl->_PendingBodyDraws.Reset();
        return;
    }

    ck::jolt::debug_draw::Release_SlotsForKey(*Target->_Impl, BodyKey,
        ck::jolt::debug_draw::EStatCounting::Counted);

    auto NewSlots = TArray<ck::jolt::debug_draw::FBodySlot>{};
    NewSlots.Reserve(Pending.Num());

    for (const auto& Draw : Pending)
    {
        auto* Bucket = Target->_Impl->_Buckets.Find(Draw._Key);
        if (Bucket == nullptr)
        { continue; }

        if (NOT TryEnsure_BucketIsm(*Target, Draw._Key, *Bucket))
        { continue; }

        auto* Ism = Bucket->_Ism.Get();

        constexpr auto WorldSpace = false;
        const auto InstanceId = Ism->AddInstanceById(Draw._Transform, WorldSpace);

        ++Bucket->_SlotCount;
        ++Target->_Impl->_LastCaptureStats._InstancesAdded;

        NewSlots.Emplace(ck::jolt::debug_draw::FBodySlot{Draw._Key, InstanceId});
    }

    if (NewSlots.IsEmpty())
    {
        _Impl->_PendingBodyDraws.Reset();
        return;
    }

    Target->_Impl->_BodySlots.Add(BodyKey, MoveTemp(NewSlots));
    _Impl->_PendingBodyDraws.Reset();
}

auto
    FCk_Jolt_DebugRenderer::
    Release_BodySlots(
        uint64 InBodyKey,
        ck::jolt::debug_draw::EStatCounting InStatCounting)
    -> void
{
    if (_Impl->_ActiveTarget == nullptr)
    { return; }

    auto& TargetImpl = *_Impl->_ActiveTarget->_Impl;

    ck::jolt::debug_draw::Release_SlotsForKey(TargetImpl, InBodyKey, InStatCounting);
    ck::jolt::debug_draw::Release_SlotsForKey(TargetImpl, ck::jolt::debug_draw::Make_HighlightKey(InBodyKey),
        InStatCounting);
    ck::jolt::debug_draw::Release_SlotsForKey(TargetImpl, ck::jolt::debug_draw::Make_HoverKey(InBodyKey),
        InStatCounting);
    ck::jolt::debug_draw::Release_SlotsForKey(TargetImpl,
        ck::jolt::debug_draw::Make_SensorContactKey(InBodyKey), InStatCounting);
}

auto
    FCk_Jolt_DebugRenderer::
    EndCapture()
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkJolt_DebugDrawReconcile);

    auto* Target = _Impl->_ActiveTarget;
    _Impl->_ActiveTarget = nullptr;
    _Impl->_CaptureBodyOpen = false;
    _Impl->_ActiveBodyIsSensor = false;
    _Impl->_PendingBodyDraws.Reset();

    if (Target == nullptr)
    { return; }

    const auto Opacity = FMath::Clamp(Target->_Impl->_Palette.Get_Opacity(), 0.0f, 1.0f);
    const auto OpacityChanged = NOT FMath::IsNearlyEqual(Opacity, Target->_Impl->_AppliedOpacity);

    auto AnyLive = false;
    auto StaleKeys = TArray<ck::jolt::debug_draw::FBucketKey>{};

    for (auto& Kvp : Target->_Impl->_Buckets)
    {
        auto& Bucket = Kvp.Value;

        if (Bucket._SlotCount == 0)
        {
            const auto OnlyBucketsStillHoldThisBatch =
                Kvp.Key._Batch->Get_RefCount() == static_cast<uint32>(ck::jolt::debug_draw::Get_BucketHolderCount(Kvp.Key._Batch));

            if (OnlyBucketsStillHoldThisBatch)
            {
                ck::jolt::debug_draw::Destroy_BucketIsm(Bucket);
                StaleKeys.Add(Kvp.Key);
                continue;
            }
        }

        if (OpacityChanged)
        {
            ck::jolt::debug_draw::Apply_BucketMaterial(Bucket, Target->_Impl->_Palette, Kvp.Key._ColorClassIndex,
                Kvp.Key._IsSensor, Target->_Impl->_RenderMode);
        }

        AnyLive |= Bucket._SlotCount > 0;
    }

    for (const auto& StaleKey : StaleKeys)
    { Target->_Impl->_Buckets.Remove(StaleKey); }

    // One DrawLines per channel rather than one per line: ULineBatchComponent::DrawLine marks the render state
    // dirty on every call, which at per-body-extra line counts is the whole cost.
    ck::jolt::debug_draw::Flush_LineChannels(*Target->_Impl);

    Target->_Impl->_AppliedOpacity = Opacity;
    Target->_Impl->_AnyLive = AnyLive;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
