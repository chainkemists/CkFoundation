#include "CkJolt_DebugRenderer.h"

#if JPH_DEBUG_RENDERER

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"

#include <Components/InstancedStaticMeshComponent.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <HAL/IConsoleManager.h>
#include <Materials/Material.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <MeshDescription.h>
#include <StaticMeshAttributes.h>
#include <UObject/Package.h>
#include <UObject/StrongObjectPtr.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_DebugDraw_Reconcile"), STAT_CkJolt_DebugDrawReconcile, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debug_renderer
{
    namespace cvar
    {
        static float DebugDrawOpacity = 0.5f;
        static FAutoConsoleVariableRef CVar_DebugDrawOpacity(TEXT("ck.Jolt.DebugDraw.Opacity"),
            DebugDrawOpacity,
            TEXT("Opacity of the batched Jolt debug-draw meshes (0..1). Applies live."));
    }

    // CPU-side triangle batch handed back to Jolt as a Ref<RefTargetVirtual>. Jolt's shapes cache these
    // (one per unique geometry), so the transient render mesh is built at most once per batch.
    class FBatch : public JPH::RefTargetVirtual
    {
    public:
        auto AddRef() -> void override { ++_RefCount; }
        auto Release() -> void override { if (--_RefCount == 0) { delete this; } }

        auto Get_RefCount() const -> uint32 { return _RefCount.load(); }

        auto
        GetOrBuild_Mesh() -> UStaticMesh*;

    public:
        TArray<FVector3f> _Positions;
        TArray<uint32> _Indices;

    private:
        TStrongObjectPtr<UStaticMesh> _Mesh;
        bool _BuildAttempted = false;
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

            // Emit BOTH windings: ck::jolt::Conv is a component-wise passthrough between Jolt's
            // right-handed convention and UE's left-handed one, so a single winding renders
            // inside-out. With both, backface culling keeps exactly one face per surface visible.
            Description.CreatePolygon(Group, TArray<FVertexInstanceID>{InstanceA, InstanceB, InstanceC});
            Description.CreatePolygon(Group, TArray<FVertexInstanceID>{InstanceA, InstanceC, InstanceB});
            ++EmittedTriangles;
        }

        if (EmittedTriangles == 0)
        { return nullptr; }

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

    // ----------------------------------------------------------------------------------------------------------------

    struct FBucketKey
    {
        FBatch* _Batch = nullptr;
        uint32 _ColorU32 = 0;

        auto operator==(const FBucketKey& InOther) const -> bool
        {
            return _Batch == InOther._Batch && _ColorU32 == InOther._ColorU32;
        }

        friend auto GetTypeHash(const FBucketKey& InKey) -> uint32
        {
            return HashCombine(GetTypeHash(InKey._Batch), GetTypeHash(InKey._ColorU32));
        }
    };

    struct FBucket
    {
        // Keeps the FBatch (and its transient mesh) alive for as long as the bucket exists.
        JPH::DebugRenderer::Batch _BatchRef;
        FLinearColor _BaseColor = FLinearColor::White;
        TWeakObjectPtr<UInstancedStaticMeshComponent> _Ism;
        TWeakObjectPtr<UMaterialInstanceDynamic> _Mid;
        TArray<FTransform> _Desired;
        TArray<FTransform> _Applied;
        bool _Touched = false;
        bool _IsmCreateFailed = false;
    };

    auto
        Get_TintedColor(
            const FLinearColor& InBaseColor)
        -> FLinearColor
    {
        auto Color = InBaseColor;
        Color.A = FMath::Clamp(cvar::DebugDrawOpacity, 0.0f, 1.0f);
        return Color;
    }

    auto
        Get_AreTransformsEqual(
            const TArray<FTransform>& InA,
            const TArray<FTransform>& InB)
        -> bool
    {
        if (InA.Num() != InB.Num())
        { return false; }

        for (auto Index = 0; Index < InA.Num(); ++Index)
        {
            if (NOT InA[Index].Equals(InB[Index]))
            { return false; }
        }

        return true;
    }

    auto
        Create_BucketIsm(
            FBucket& InOutBucket,
            UWorld& InWorld,
            UStaticMesh& InMesh)
        -> bool
    {
        auto* Ism = UCk_Utils_Object_UE::Request_CreateNewObject<UInstancedStaticMeshComponent>(
            &InWorld, UInstancedStaticMeshComponent::StaticClass(), nullptr,
            FCk_ObjectPooling_PoolParams{}.Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease), nullptr);

        CK_ENSURE_IF_NOT(ck::IsValid(Ism),
            TEXT("Failed to create an InstancedStaticMeshComponent for the Jolt debug renderer"))
        { return false; }

        // Debug geometry must never influence gameplay systems.
        Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ism->SetCanEverAffectNavigation(false);
        Ism->SetCastShadow(false);
        Ism->SetMobility(EComponentMobility::Movable);

        // Registered at identity: instance transforms are authored in world space, so component
        // space == world space and every Add/BatchUpdate below passes local-space transforms.
        Ism->SetWorldLocation(FVector::ZeroVector);
        // No BeginPlay (protected on UStaticMeshComponent, unlike UProceduralMeshComponent): a
        // render-only ownerless component needs its render state — registration provides that.
        Ism->RegisterComponentWithWorld(&InWorld);

        Ism->SetVisibility(true);
        Ism->SetHiddenInGame(false);
        Ism->SetStaticMesh(&InMesh);

        InOutBucket._Ism = Ism;

        static auto TranslucentMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));

        CK_ENSURE_IF_NOT(ck::IsValid(TranslucentMaterial),
            TEXT("Failed to load M_SimpleUnlitTranslucent for the Jolt debug renderer — bodies will draw untinted with the default material"))
        { return true; }

        auto DynamicMaterial = UMaterialInstanceDynamic::Create(TranslucentMaterial, Ism);
        if (ck::IsValid(DynamicMaterial))
        {
            DynamicMaterial->SetVectorParameterValue(FName{TEXT("Color")}, Get_TintedColor(InOutBucket._BaseColor));
            Ism->SetMaterial(0, DynamicMaterial);
            InOutBucket._Mid = DynamicMaterial;
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

struct CkJoltDebugger::FImpl
{
    TMap<ck_jolt_debug_renderer::FBucketKey, ck_jolt_debug_renderer::FBucket> _Buckets;
    float _AppliedOpacity = -1.0f;
    bool _AnyLive = false;
};

// --------------------------------------------------------------------------------------------------------------------

CkJoltDebugger::CkJoltDebugger()
    : _Impl(MakePimpl<FImpl>())
{
    // Creates the shared unit-geometry batches (box/sphere/capsule/...) through our
    // CreateTriangleBatch overrides — required by the base-class contract.
    Initialize();
}

CkJoltDebugger::~CkJoltDebugger()
{
    for (auto& Kvp : _Impl->_Buckets)
    {
        auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        // unpin before DestroyComponent (destroy garbage-marks the object, failing release's validity check)
        UCk_Utils_Object_UE::TryReleaseToPool(Ism);
        Ism->DestroyComponent();
    }
}

auto
    CkJoltDebugger::
    DrawLine(
        JPH::RVec3Arg inFrom,
        JPH::RVec3Arg inTo,
        JPH::ColorArg inColor)
    -> void
{
    if (ck::Is_NOT_Valid(_World))
    { return; }

    UCk_Utils_DebugDraw_UE::DrawDebugLine(_World.Get(), ck::jolt::Conv(inFrom), ck::jolt::Conv(inTo),
        ck::jolt::Conv(inColor));
}

auto
    CkJoltDebugger::
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
    CkJoltDebugger::
    DrawText3D(
        JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor,
        float inHeight)
    -> void
{
    if (ck::Is_NOT_Valid(_World))
    { return; }

    UCk_Utils_DebugDraw_UE::DrawDebugString(_World.Get(), ck::jolt::Conv(inPosition),
        FString{static_cast<int32>(inString.length()), inString.data()}, ck::jolt::Conv(inColor));
}

auto
    CkJoltDebugger::
    CreateTriangleBatch(
        const Triangle* inTriangles,
        int inTriangleCount)
    -> Batch
{
    auto* NewBatch = new ck_jolt_debug_renderer::FBatch{};

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
    CkJoltDebugger::
    CreateTriangleBatch(
        const Vertex* inVertices,
        int inVertexCount,
        const JPH::uint32* inIndices,
        int inIndexCount)
    -> Batch
{
    auto* NewBatch = new ck_jolt_debug_renderer::FBatch{};

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
    CkJoltDebugger::
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
    if (inGeometry.GetPtr() == nullptr || inGeometry->mLODs.empty())
    { return; }

    // Highest-detail LOD unconditionally — instancing makes the triangle count a GPU non-issue,
    // and there is no reliable camera position to select against.
    const auto& TriangleBatch = inGeometry->mLODs.front().mTriangleBatch;
    if (TriangleBatch.GetPtr() == nullptr)
    { return; }

    auto* BatchImpl = static_cast<ck_jolt_debug_renderer::FBatch*>(TriangleBatch.GetPtr());
    if (BatchImpl->_Indices.IsEmpty())
    { return; }

    const auto Key = ck_jolt_debug_renderer::FBucketKey{BatchImpl, inModelColor.mU32};
    auto& Bucket = _Impl->_Buckets.FindOrAdd(Key);

    if (Bucket._BatchRef.GetPtr() == nullptr)
    {
        Bucket._BatchRef = TriangleBatch;
        Bucket._BaseColor = ck::jolt::Conv(inModelColor);
    }

    Bucket._Desired.Emplace(FTransform{ck::jolt::Conv(inModelMatrix)});
    Bucket._Touched = true;
}

auto
    CkJoltDebugger::
    BeginFrame()
    -> void
{
    for (auto& Kvp : _Impl->_Buckets)
    {
        Kvp.Value._Desired.Reset();
        Kvp.Value._Touched = false;
    }
}

auto
    CkJoltDebugger::
    EndFrame()
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkJolt_DebugDrawReconcile);

    auto* World = _World.Get();
    if (ck::Is_NOT_Valid(World))
    { return; }

    const auto Opacity = FMath::Clamp(ck_jolt_debug_renderer::cvar::DebugDrawOpacity, 0.0f, 1.0f);
    const auto OpacityChanged = NOT FMath::IsNearlyEqual(Opacity, _Impl->_AppliedOpacity);

    auto AnyLive = false;
    auto StaleKeys = TArray<ck_jolt_debug_renderer::FBucketKey>{};

    for (auto& Kvp : _Impl->_Buckets)
    {
        auto& Bucket = Kvp.Value;

        if (NOT Bucket._Touched)
        {
            // Refcount 1 == only the bucket still holds this batch: every Jolt geometry that used it is
            // gone (shapes re-cooked across gym restarts, static-world re-bakes). Without pruning, the
            // transient mesh + instanced component leak once per re-cook for the rest of the session.
            if (Kvp.Key._Batch->Get_RefCount() == 1)
            {
                auto* StaleIsm = Bucket._Ism.Get();
                if (ck::IsValid(StaleIsm))
                {
                    // unpin before DestroyComponent (destroy garbage-marks the object, failing release's validity check)
                    UCk_Utils_Object_UE::TryReleaseToPool(StaleIsm);
                    StaleIsm->DestroyComponent();
                }

                StaleKeys.Add(Kvp.Key);
                continue;
            }

            if (Bucket._Applied.Num() > 0)
            {
                auto* StaleIsm = Bucket._Ism.Get();
                if (ck::IsValid(StaleIsm))
                { StaleIsm->ClearInstances(); }

                Bucket._Applied.Reset();
            }

            continue;
        }

        auto* Ism = Bucket._Ism.Get();

        if (ck::Is_NOT_Valid(Ism))
        {
            if (Bucket._IsmCreateFailed)
            { continue; }

            auto* Mesh = Kvp.Key._Batch->GetOrBuild_Mesh();
            if (Mesh == nullptr)
            {
                // Empty batch or a build failure (the latter already ensured, loudly, once).
                Bucket._IsmCreateFailed = true;
                continue;
            }

            if (NOT ck_jolt_debug_renderer::Create_BucketIsm(Bucket, *World, *Mesh))
            {
                Bucket._IsmCreateFailed = true;
                continue;
            }

            Ism = Bucket._Ism.Get();
            Bucket._Applied.Reset();
        }

        auto* Mid = Bucket._Mid.Get();
        if (OpacityChanged && ck::IsValid(Mid))
        {
            Mid->SetVectorParameterValue(FName{TEXT("Color")},
                ck_jolt_debug_renderer::Get_TintedColor(Bucket._BaseColor));
        }

        if (NOT ck_jolt_debug_renderer::Get_AreTransformsEqual(Bucket._Desired, Bucket._Applied))
        {
            if (Bucket._Desired.Num() == Bucket._Applied.Num())
            {
                constexpr auto WorldSpace = false;
                constexpr auto MarkRenderStateDirty = true;
                constexpr auto Teleport = true;
                Ism->BatchUpdateInstancesTransforms(0, Bucket._Desired, WorldSpace, MarkRenderStateDirty, Teleport);
            }
            else
            {
                Ism->ClearInstances();

                constexpr auto ReturnIndices = false;
                Ism->AddInstances(Bucket._Desired, ReturnIndices);
            }

            Bucket._Applied = Bucket._Desired;
        }

        AnyLive |= Bucket._Applied.Num() > 0;
    }

    for (const auto& StaleKey : StaleKeys)
    { _Impl->_Buckets.Remove(StaleKey); }

    _Impl->_AppliedOpacity = Opacity;
    _Impl->_AnyLive = AnyLive;
}

auto
    CkJoltDebugger::
    HideAll()
    -> void
{
    if (NOT _Impl->_AnyLive)
    { return; }

    for (auto& Kvp : _Impl->_Buckets)
    {
        auto& Bucket = Kvp.Value;

        auto* Ism = Bucket._Ism.Get();
        if (ck::IsValid(Ism))
        { Ism->ClearInstances(); }

        Bucket._Applied.Reset();
        Bucket._Desired.Reset();
        Bucket._Touched = false;
    }

    _Impl->_AnyLive = false;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
