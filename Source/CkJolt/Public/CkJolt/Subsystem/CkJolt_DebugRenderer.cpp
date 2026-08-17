#include "CkJolt_DebugRenderer.h"

#if JPH_DEBUG_RENDERER

#include "CkDebugScene/CkDebugScene_Mesh.h"

#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"

#include <Misc/CoreDelegates.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_DebugDraw_Reconcile"), STAT_CkJolt_DebugDrawReconcile, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    class FBatch : public JPH::RefTargetVirtual
    {
    public:
        auto
        AddRef() -> void override
        {
            ++_RefCount;
        }
        auto
        Release() -> void override
        {
            if (--_RefCount == 0)
            {
                delete this;
            }
        }

        auto
        Get_RefCount() const -> uint32
        {
            return _RefCount.load();
        }

        auto
        GetOrBuild_DebugSceneMesh() -> TSharedPtr<FCk_DebugScene_Mesh>;

    public:
        TArray<FVector3f> _Positions;
        TArray<uint32> _Indices;

    private:
        TSharedPtr<FCk_DebugScene_Mesh> _DebugSceneMesh;
        std::atomic<uint32> _RefCount = 0;
    };

    auto
        FBatch::
        GetOrBuild_DebugSceneMesh()
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        if (_DebugSceneMesh.IsValid())
        {
            return _DebugSceneMesh;
        }

        const auto VertexCount = static_cast<uint32>(_Positions.Num());
        auto Triangles = TArray<FCk_DebugScene_Triangle>{};
        Triangles.Reserve(_Indices.Num() / 3);

        for (auto Index = 0; Index + 2 < _Indices.Num(); Index += 3)
        {
            const auto A = _Indices[Index];
            const auto B = _Indices[Index + 1];
            const auto C = _Indices[Index + 2];

            if (A == B || B == C || A == C || A >= VertexCount || B >= VertexCount || C >= VertexCount)
            {
                continue;
            }

            Triangles.Add({
                FVector{_Positions[static_cast<int32>(A)]},
                FVector{_Positions[static_cast<int32>(C)]},
                FVector{_Positions[static_cast<int32>(B)]}});
        }

        _DebugSceneMesh = FCk_DebugScene_Mesh::Create_FromTriangles(MoveTemp(Triangles));
        return _DebugSceneMesh;
    }

    auto
    Get_DebugSceneMesh(FBatch* InBatch) -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        return InBatch != nullptr ? InBatch->GetOrBuild_DebugSceneMesh() : TSharedPtr<FCk_DebugScene_Mesh>{};
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
        ck::jolt::debug_draw::Note_BucketHolderAdded(BatchImpl);
    }

    const auto Transform = FTransform{ck::jolt::Conv(inModelMatrix)};

    _Impl->_PendingBodyDraws.Emplace(ck_jolt_debug_renderer::FPendingDraw{Key, Transform});
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

    if (Pending.IsEmpty())
    {
        ck::jolt::debug_draw::Release_SlotsForKey(*Target->_Impl, BodyKey,
            ck::jolt::debug_draw::EStatCounting::Counted);
        _Impl->_PendingBodyDraws.Reset();
        return;
    }

    auto Instances = TArray<FCk_DebugScene_Instance, TInlineAllocator<4>>{};
    Instances.Reserve(Pending.Num());
    for (const auto& Draw : Pending)
    {
        const auto Mesh = ck::jolt::debug_draw::Get_DebugSceneMesh(Draw._Key._Batch);
        if (NOT Mesh.IsValid())
        {
            _Impl->_PendingBodyDraws.Reset();
            return;
        }
        auto& Bucket = Target->_Impl->_Buckets.FindChecked(Draw._Key);
        const auto Opacity = Target->_Impl->_Palette.Get_Opacity();
        if (NOT Bucket._HasAppearance || Bucket._AppearanceRenderMode != Target->_Impl->_RenderMode ||
            NOT FMath::IsNearlyEqual(Bucket._AppearanceOpacity, Opacity))
        {
            Bucket._Appearance = ck::jolt::debug_draw::Make_DebugSceneAppearance(
                Draw._Key, Target->_Impl->_Palette, Target->_Impl->_RenderMode);
            Bucket._AppearanceRenderMode = Target->_Impl->_RenderMode;
            Bucket._AppearanceOpacity = Opacity;
            Bucket._HasAppearance = true;
        }
        Instances.Emplace(FCk_DebugScene_Instance{}
            .Set_Mesh(Mesh)
            .Set_Transform(Draw._Transform)
            .Set_Appearance(Bucket._Appearance)
            .Set_PickIdentity(BodyKey));
    }

    if (NOT Target->_Impl->_SceneTarget.IsValid() ||
        NOT Target->_Impl->_SceneTarget->TryReconcile_One(BodyKey, Instances))
    {
        _Impl->_PendingBodyDraws.Reset();
        return;
    }
    Target->_Impl->_SceneTarget->Set_ItemPickable(BodyKey,
        NOT Target->_Impl->_InternalBodyKeys.Contains(BodyKey) &&
        (BodyKey & (ck::jolt::debug_draw::HighlightKeyBit | ck::jolt::debug_draw::HoverKeyBit |
            ck::jolt::debug_draw::SensorContactKeyBit)) == 0);

    auto* ExistingSlots = Target->_Impl->_BodySlots.Find(BodyKey);

    // A bucket may have been pruned since the previous capture. In that case the lightweight census must rebuild
    // instead of treating a stale bucket key as an in-place update.
    const auto SlotsStillMatch = [&]() -> bool
    {
        if (ExistingSlots == nullptr || ExistingSlots->Num() != Pending.Num())
        { return false; }

        for (auto Index = 0; Index < Pending.Num(); ++Index)
        {
            const auto& Slot = (*ExistingSlots)[Index];

            if (NOT (Slot._Bucket == Pending[Index]._Key))
            { return false; }

            if (Target->_Impl->_Buckets.Find(Slot._Bucket) == nullptr)
            { return false; }
        }

        return true;
    }();

    if (SlotsStillMatch)
    {
        for (auto Index = 0; Index < Pending.Num(); ++Index)
        {
            ++Target->_Impl->_LastCaptureStats._InstancesUpdated;
        }

        _Impl->_PendingBodyDraws.Reset();
        return;
    }

    constexpr auto RemoveSceneItem = false;
    ck::jolt::debug_draw::Release_SlotsForKey(
        *Target->_Impl, BodyKey, ck::jolt::debug_draw::EStatCounting::Counted, RemoveSceneItem);

    auto NewSlots = TArray<ck::jolt::debug_draw::FBodySlot>{};
    NewSlots.Reserve(Pending.Num());

    for (const auto& Draw : Pending)
    {
        auto* Bucket = Target->_Impl->_Buckets.Find(Draw._Key);
        if (Bucket == nullptr)
        { continue; }

        ++Bucket->_SlotCount;
        ++Target->_Impl->_LastCaptureStats._InstancesAdded;

        NewSlots.Emplace(ck::jolt::debug_draw::FBodySlot{Draw._Key});
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
                ck::jolt::debug_draw::Release_Bucket(Bucket);
                StaleKeys.Add(Kvp.Key);
                continue;
            }
        }

        AnyLive |= Bucket._SlotCount > 0;
    }

    for (const auto& StaleKey : StaleKeys)
    { Target->_Impl->_Buckets.Remove(StaleKey); }

    // Publish once after the capture instead of dirtying retained render state per JPH callback.
    ck::jolt::debug_draw::Flush_LineChannels(*Target->_Impl);

    Target->_Impl->_AppliedOpacity = FMath::Clamp(Target->_Impl->_Palette.Get_Opacity(), 0.0f, 1.0f);
    Target->_Impl->_AnyLive = AnyLive;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
