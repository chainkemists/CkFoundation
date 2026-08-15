#include "CkJolt_DebugDrawTarget.h"

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget_Impl.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat_Defaults.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Components/InstancedStaticMeshComponent.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <Materials/Material.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debug_draw_target
{
    const auto ColorParameterName = FName{TEXT("Color")};

    static_assert(static_cast<int32>(ECk_Jolt_DebugDraw_ColorClass::Count) <= 8,
        "The hidden-class mask is a uint8 bitfield. Adding a ninth colour class shifts the bit out of range and "
        "silently makes that class permanently visible — widen _HiddenClassMask before adding one.");

    auto
        Get_ClassBit(
            ECk_Jolt_DebugDraw_ColorClass InColorClass)
        -> uint8
    {
        return static_cast<uint8>(1u << static_cast<uint8>(InColorClass));
    }

    // Rooted, not a bare static UMaterial*: a function-local raw pointer survives the GC that collects the
    // material it points at, and the next dereference is on freed memory.
    // Loaded directly rather than through GEngine->WireframeMaterial, which is null whenever the platform
    // RequiresCookedData. Both engine debug materials are special-engine materials, so the ISM usage checks
    // do not reject them.
    auto
        Get_SolidBaseMaterial()
        -> UMaterial*
    {
        static auto Material = TStrongObjectPtr<UMaterial>{LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"))};

        return Material.Get();
    }

    auto
        Get_WireframeBaseMaterial()
        -> UMaterial*
    {
        static auto Material = TStrongObjectPtr<UMaterial>{LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/WireframeMaterial.WireframeMaterial"))};

        return Material.Get();
    }

    // Where one instance sits and how big its geometry is, in the instance's OWN space. Keeping the box local
    // and the transform beside it is what lets the pick test be an oriented-box test instead of a world AABB
    // one, at no extra cost.
    struct FInstancePlacement
    {
        FTransform _Transform;
        FBox _LocalBounds = FBox{ForceInit};
    };

    auto
        TryGet_InstancePlacement(
            const TMap<ck::jolt::debug_draw::FBucketKey, ck::jolt::debug_draw::FBucket>& InBuckets,
            const ck::jolt::debug_draw::FBodySlot& InSlot)
        -> TOptional<FInstancePlacement>
    {
        const auto* Bucket = InBuckets.Find(InSlot._Bucket);
        if (Bucket == nullptr)
        { return {}; }

        auto* Ism = Bucket->_Ism.Get();
        if (ck::Is_NOT_Valid(Ism) || NOT Ism->IsValidId(InSlot._InstanceId))
        { return {}; }

        const UStaticMesh* Mesh = Ism->GetStaticMesh();
        if (ck::Is_NOT_Valid(Mesh))
        { return {}; }

        const auto InstanceIndex = Ism->GetInstanceIndexForId(InSlot._InstanceId);
        if (InstanceIndex == INDEX_NONE)
        { return {}; }

        auto Placement = FInstancePlacement{};
        Placement._LocalBounds = Mesh->GetBounds().GetBox();

        constexpr auto WorldSpace = true;
        if (NOT Ism->GetInstanceTransform(InstanceIndex, Placement._Transform, WorldSpace))
        { return {}; }

        return Placement;
    }

    // Slab test. The returned distance is PARAMETRIC along InDirection, which is all a nearest-hit comparison
    // needs and spares every caller a normalize. A ray whose origin is already inside the box hits at 0.
    auto
        TryIntersect_RayBox(
            const FVector& InOrigin,
            const FVector& InDirection,
            const FBox& InBox)
        -> TOptional<double>
    {
        auto EntryDistance = 0.0;
        auto ExitDistance = TNumericLimits<double>::Max();

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto AxisDirection = InDirection[Axis];
            const auto AxisOrigin = InOrigin[Axis];

            if (FMath::IsNearlyZero(AxisDirection))
            {
                if (AxisOrigin < InBox.Min[Axis] || AxisOrigin > InBox.Max[Axis])
                { return {}; }

                continue;
            }

            const auto InverseDirection = 1.0 / AxisDirection;

            auto NearDistance = (InBox.Min[Axis] - AxisOrigin) * InverseDirection;
            auto FarDistance = (InBox.Max[Axis] - AxisOrigin) * InverseDirection;

            if (NearDistance > FarDistance)
            { Swap(NearDistance, FarDistance); }

            EntryDistance = FMath::Max(EntryDistance, NearDistance);
            ExitDistance = FMath::Min(ExitDistance, FarDistance);

            if (EntryDistance > ExitDistance)
            { return {}; }
        }

        return EntryDistance;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::debug_draw
{
    auto
        Get_TintedColor(
            const FLinearColor& InBaseColor,
            float InOpacity)
        -> FLinearColor
    {
        auto Color = InBaseColor;
        Color.A = FMath::Clamp(InOpacity, 0.0f, 1.0f);
        return Color;
    }

    auto
        Make_BodyKey(
            uint32 InIndexAndSequenceNumber)
        -> uint64
    {
        return static_cast<uint64>(InIndexAndSequenceNumber);
    }

    auto
        Make_CharacterBodyKey(
            const FCk_Handle& InCharacterEntity)
        -> uint64
    {
        if (ck::Is_NOT_Valid(InCharacterEntity))
        { return 0; }

        return Make_CharacterBodyKey_FromEntityId(
            static_cast<uint64>(InCharacterEntity.Get_Entity().Get_ID()));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_DebugDrawPalette::
    Get_Color(
        ECk_Jolt_DebugDraw_ColorClass InColorClass) const
    -> FLinearColor
{
    switch (InColorClass)
    {
        case ECk_Jolt_DebugDraw_ColorClass::Static:
        { return _StaticColor; }
        case ECk_Jolt_DebugDraw_ColorClass::Kinematic:
        { return _KinematicColor; }
        case ECk_Jolt_DebugDraw_ColorClass::Dynamic_Awake:
        { return _DynamicAwakeColor; }
        case ECk_Jolt_DebugDraw_ColorClass::Dynamic_Sleeping:
        {
            const auto Dim = FMath::Clamp(_SleepingDimFactor, 0.0f, 1.0f);
            return FLinearColor{
                _DynamicSleepingColor.R * Dim,
                _DynamicSleepingColor.G * Dim,
                _DynamicSleepingColor.B * Dim,
                _DynamicSleepingColor.A};
        }
        case ECk_Jolt_DebugDraw_ColorClass::Sensor:
        { return _SensorColor; }
        case ECk_Jolt_DebugDraw_ColorClass::BakedStatic:
        { return _BakedStaticColor; }
        case ECk_Jolt_DebugDraw_ColorClass::Character:
        { return _CharacterColor; }
        case ECk_Jolt_DebugDraw_ColorClass::Highlight:
        { return _HighlightColor; }
        case ECk_Jolt_DebugDraw_ColorClass::Count:
        { break; }
    }

    return _StaticColor;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Jolt_DebugDrawTarget::
    FCk_Jolt_DebugDrawTarget(
        UWorld* InWorld)
    : _Impl(MakePimpl<FImpl>())
{
    _Impl->_World = InWorld;

    // OnWorldCleanup is the one delegate that fires for BOTH PIE end and map unload (UWorld::CleanupWorld is
    // the shared funnel), and it runs before components are torn down, so releasing here is ordered.
    _Impl->_WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
        [this](UWorld* InCleanedWorld, bool, bool) -> void
        {
            if (InCleanedWorld != _Impl->_World.Get())
            { return; }

            HideAll();

            for (auto& Kvp : _Impl->_Buckets)
            { ck::jolt::debug_draw::Destroy_BucketIsm(Kvp.Value); }

            _Impl->_Buckets.Reset();
        });
}

FCk_Jolt_DebugDrawTarget::
    ~FCk_Jolt_DebugDrawTarget()
{
    FWorldDelegates::OnWorldCleanup.Remove(_Impl->_WorldCleanupHandle);

    for (auto& Kvp : _Impl->_Buckets)
    { ck::jolt::debug_draw::Destroy_BucketIsm(Kvp.Value); }
}

namespace ck::jolt::debug_draw
{
    auto
        Destroy_BucketIsm(
            FBucket& InOutBucket)
        -> void
    {
        if (InOutBucket._BatchKeepAlive.GetPtr() != nullptr)
        { Note_BucketHolderRemoved(InOutBucket._BatchKeepAlive.GetPtr()); }

        auto* Ism = InOutBucket._Ism.Get();
        if (ck::IsValid(Ism))
        { Ism->DestroyComponent(); }

        InOutBucket._Ism.Reset();
        InOutBucket._SolidMid = nullptr;
        InOutBucket._WireframeMid = nullptr;
        InOutBucket._SlotCount = 0;
        InOutBucket._BatchKeepAlive = nullptr;
    }

    auto
        Release_SlotsForKey(
            FCk_Jolt_DebugDrawTarget::FImpl& InOutTargetImpl,
            uint64 InSlotKey)
        -> void
    {
        auto* Slots = InOutTargetImpl._BodySlots.Find(InSlotKey);
        if (Slots == nullptr)
        { return; }

        for (const auto& Slot : *Slots)
        {
            auto* Bucket = InOutTargetImpl._Buckets.Find(Slot._Bucket);
            if (Bucket == nullptr)
            { continue; }

            auto* Ism = Bucket->_Ism.Get();
            if (ck::Is_NOT_Valid(Ism) || NOT Ism->IsValidId(Slot._InstanceId))
            { continue; }

            Ism->RemoveInstanceById(Slot._InstanceId);
            ++InOutTargetImpl._LastCaptureStats._InstancesRemoved;
            Bucket->_SlotCount = FMath::Max(0, Bucket->_SlotCount - 1);
        }

        InOutTargetImpl._BodySlots.Remove(InSlotKey);
    }

    auto
        Apply_BucketMaterial(
            FBucket& InOutBucket,
            const FCk_Jolt_DebugDrawPalette& InPalette,
            ECk_Jolt_DebugDraw_RenderMode& InOutRenderMode)
        -> void
    {
        auto* Ism = InOutBucket._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { return; }

        const auto TintedColor = Get_TintedColor(InOutBucket._BaseColor, InPalette.Get_Opacity());

        const auto& Get_OrCreate_Mid = [&](TWeakObjectPtr<UMaterialInstanceDynamic>& InOutMid, UMaterial* InBase)
            -> UMaterialInstanceDynamic*
        {
            if (auto* Existing = InOutMid.Get(); ck::IsValid(Existing))
            { return Existing; }

            if (ck::Is_NOT_Valid(InBase))
            { return nullptr; }

            auto* Created = UMaterialInstanceDynamic::Create(InBase, Ism);
            InOutMid = Created;
            return Created;
        };

        // Deliberately NON-terminating recovery: an unavailable wireframe material degrades the whole target to
        // Solid and execution CONTINUES to assign the solid MID, because leaving the bucket unmaterialed would
        // be a worse failure than the mode silently not applying.
        if (InOutRenderMode == ECk_Jolt_DebugDraw_RenderMode::Wireframe)
        {
            auto* WireframeBase = ck_jolt_debug_draw_target::Get_WireframeBaseMaterial();

            CK_ENSURE_IF_NOT(ck::IsValid(WireframeBase),
                TEXT("Failed to load WireframeMaterial for the Jolt debug renderer — staying in Solid render mode"))
            {
                InOutRenderMode = ECk_Jolt_DebugDraw_RenderMode::Solid;
            }
        }

        const auto UseWireframe = InOutRenderMode == ECk_Jolt_DebugDraw_RenderMode::Wireframe;

        auto* Mid = UseWireframe
            ? Get_OrCreate_Mid(InOutBucket._WireframeMid, ck_jolt_debug_draw_target::Get_WireframeBaseMaterial())
            : Get_OrCreate_Mid(InOutBucket._SolidMid, ck_jolt_debug_draw_target::Get_SolidBaseMaterial());

        CK_ENSURE_IF_NOT(ck::IsValid(Mid),
            TEXT("Failed to create the Jolt debug-draw material instance — bodies will draw untinted with the default material"))
        { return; }

        Mid->SetVectorParameterValue(ck_jolt_debug_draw_target::ColorParameterName, TintedColor);
        Ism->SetMaterial(0, Mid);
    }
}

auto
    FCk_Jolt_DebugDrawTarget::
    HideAll()
    -> void
{
    if (NOT _Impl->_AnyLive && _Impl->_BodySlots.IsEmpty() && NOT _Impl->_FullPassEverRan)
    { return; }

    for (auto& Kvp : _Impl->_Buckets)
    {
        auto& Bucket = Kvp.Value;

        auto* Ism = Bucket._Ism.Get();
        if (ck::IsValid(Ism))
        { Ism->ClearInstances(); }

        Bucket._Applied.Reset();
        Bucket._Desired.Reset();
        Bucket._SlotCount = 0;
        Bucket._Touched = false;
    }

    _Impl->_BodySlots.Reset();
    _Impl->_StaticBodyKeys.Reset();
    _Impl->_PrevActiveBodyKeys.Reset();
    _Impl->_SleepingBodyKeys.Reset();
    _Impl->_CharacterKeys.Reset();
    _Impl->_FullPassEverRan = false;
    _Impl->_AnyLive = false;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_IsDesired(
        bool InIsDesired)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_IsDesired == InIsDesired)
    { return *this; }

    _Impl->_IsDesired = InIsDesired;

    if (NOT InIsDesired)
    { HideAll(); }

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_RenderMode(
        ECk_Jolt_DebugDraw_RenderMode InRenderMode)
    -> void
{
    if (_Impl->_RenderMode == InRenderMode)
    { return; }

    _Impl->_RenderMode = InRenderMode;

    for (auto& Kvp : _Impl->_Buckets)
    { ck::jolt::debug_draw::Apply_BucketMaterial(Kvp.Value, _Impl->_Palette, _Impl->_RenderMode); }
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_ClassVisibility(
        ECk_Jolt_DebugDraw_ColorClass InColorClass,
        bool InIsVisible)
    -> FCk_Jolt_DebugDrawTarget&
{
    const auto ClassBit = ck_jolt_debug_draw_target::Get_ClassBit(InColorClass);

    const auto NewMask = InIsVisible
        ? static_cast<uint8>(_Impl->_HiddenClassMask & ~ClassBit)
        : static_cast<uint8>(_Impl->_HiddenClassMask | ClassBit);

    if (_Impl->_HiddenClassMask == NewMask)
    { return *this; }

    _Impl->_HiddenClassMask = NewMask;

    for (auto& Kvp : _Impl->_Buckets)
    {
        if (Kvp.Key._ColorClass != InColorClass)
        { continue; }

        auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        Ism->SetVisibility(InIsVisible);
    }

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_IsClassVisible(
        ECk_Jolt_DebugDraw_ColorClass InColorClass) const
    -> bool
{
    return (_Impl->_HiddenClassMask & ck_jolt_debug_draw_target::Get_ClassBit(InColorClass)) == 0;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_ContentBounds() const
    -> FBox
{
    auto Bounds = FBox{ForceInit};

    for (const auto& Kvp : _Impl->_Buckets)
    {
        if (NOT Get_IsClassVisible(Kvp.Key._ColorClass))
        { continue; }

        const auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism) || Ism->GetInstanceCount() == 0)
        { continue; }

        // CalcBounds, not the cached Bounds: the cache is refreshed by the deferred render-state update, so a
        // component whose instances were added this frame still reports its registration-time (empty) box.
        Bounds += Ism->CalcBounds(Ism->GetComponentTransform()).GetBox();
    }

    return Bounds;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_HighlightedBody(
        TOptional<uint64> InBodyKey)
    -> FCk_Jolt_DebugDrawTarget&
{
    if (_Impl->_HighlightedBodyKey == InBodyKey)
    { return *this; }

    if (_Impl->_HighlightedBodyKey.IsSet())
    {
        ck::jolt::debug_draw::Release_SlotsForKey(*_Impl,
            ck::jolt::debug_draw::Make_HighlightKey(*_Impl->_HighlightedBodyKey));
    }

    _Impl->_HighlightedBodyKey = InBodyKey;

    // The old selection's sample must not survive as the new one's: the next capture is what fills it.
    _Impl->_HighlightedBodyLinearVelocity.Reset();

    // The overlay is produced by the capture's own draw path, so a body only the revision-keyed full pass ever
    // draws — a static, or one asleep since before this target opened — would not gain its overlay until the
    // scene happened to change. Re-arming the full pass makes the very next capture produce it.
    _Impl->_FullPassEverRan = false;

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HighlightedBody() const
    -> TOptional<uint64>
{
    return _Impl->_HighlightedBodyKey;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HighlightedBodyBounds() const
    -> TOptional<FBox>
{
    if (NOT _Impl->_HighlightedBodyKey.IsSet())
    { return {}; }

    const auto* Slots = _Impl->_BodySlots.Find(*_Impl->_HighlightedBodyKey);
    if (Slots == nullptr)
    { return {}; }

    auto Bounds = FBox{ForceInit};

    for (const auto& Slot : *Slots)
    {
        const auto Placement = ck_jolt_debug_draw_target::TryGet_InstancePlacement(_Impl->_Buckets, Slot);

        if (NOT Placement.IsSet())
        { continue; }

        Bounds += Placement->_LocalBounds.TransformBy(Placement->_Transform);
    }

    if (Bounds.IsValid == 0)
    { return {}; }

    return Bounds;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_HighlightedBodyLinearVelocity() const
    -> TOptional<FVector>
{
    return _Impl->_HighlightedBodyLinearVelocity;
}

auto
    FCk_Jolt_DebugDrawTarget::
    TryPick_Body(
        const FVector& InOrigin,
        const FVector& InDirection) const
    -> TOptional<uint64>
{
    const auto DirectionIsUsable = NOT InDirection.IsNearlyZero();

    CK_ENSURE_IF_NOT(DirectionIsUsable,
        TEXT("TryPick_Body was given a degenerate ray direction [{}] — no body can be picked from it"),
        InDirection)
    { return {}; }

    auto NearestKey = TOptional<uint64>{};
    auto NearestDistance = TNumericLimits<double>::Max();

    for (const auto& Kvp : _Impl->_BodySlots)
    {
        for (const auto& Slot : Kvp.Value)
        {
            if (Slot._Bucket._ColorClass == ECk_Jolt_DebugDraw_ColorClass::Highlight)
            { continue; }

            if (NOT Get_IsClassVisible(Slot._Bucket._ColorClass))
            { continue; }

            const auto Placement = ck_jolt_debug_draw_target::TryGet_InstancePlacement(_Impl->_Buckets, Slot);

            if (NOT Placement.IsSet())
            { continue; }

            // The ray goes into instance space rather than the box coming out of it: an affine transform
            // preserves the parametric distance, so the test stays an oriented-box one and the hits from
            // differently-rotated instances remain directly comparable.
            const auto LocalOrigin = Placement->_Transform.InverseTransformPosition(InOrigin);
            const auto LocalDirection = Placement->_Transform.InverseTransformVector(InDirection);

            const auto HitDistance = ck_jolt_debug_draw_target::TryIntersect_RayBox(
                LocalOrigin, LocalDirection, Placement->_LocalBounds);

            if (NOT HitDistance.IsSet() || *HitDistance >= NearestDistance)
            { continue; }

            NearestDistance = *HitDistance;
            NearestKey = Kvp.Key;
        }
    }

    return NearestKey;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_Opacity(
        float InOpacity)
    -> void
{
    _Impl->_Palette.Set_Opacity(InOpacity);
}

auto
    FCk_Jolt_DebugDrawTarget::
    Set_Palette(
        const FCk_Jolt_DebugDrawPalette& InPalette)
    -> FCk_Jolt_DebugDrawTarget&
{
    _Impl->_Palette = InPalette;

    _Impl->_FullPassEverRan = false;
    _Impl->_AppliedOpacity = -1.0f;

    return *this;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_World() const
    -> TWeakObjectPtr<UWorld>
{
    return _Impl->_World;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_IsDesired() const
    -> bool
{
    return _Impl->_IsDesired;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_RenderMode() const
    -> ECk_Jolt_DebugDraw_RenderMode
{
    return _Impl->_RenderMode;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_Palette() const
    -> const FCk_Jolt_DebugDrawPalette&
{
    return _Impl->_Palette;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_LastCaptureStats() const
    -> const ck::jolt::debug_draw::FDebugDrawStats&
{
    return _Impl->_LastCaptureStats;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumInstances() const
    -> int32
{
    auto Total = 0;

    for (const auto& Kvp : _Impl->_Buckets)
    {
        const auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        Total += Ism->GetInstanceCount();
    }

    return Total;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_NumBuckets() const
    -> int32
{
    return _Impl->_Buckets.Num();
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_Isms() const
    -> TArray<UInstancedStaticMeshComponent*>
{
    auto Isms = TArray<UInstancedStaticMeshComponent*>{};
    Isms.Reserve(_Impl->_Buckets.Num());

    for (const auto& Kvp : _Impl->_Buckets)
    {
        auto* Ism = Kvp.Value._Ism.Get();
        if (ck::Is_NOT_Valid(Ism))
        { continue; }

        Isms.Emplace(Ism);
    }

    return Isms;
}

auto
    FCk_Jolt_DebugDrawTarget::
    Get_BucketColorClasses() const
    -> TArray<ECk_Jolt_DebugDraw_ColorClass>
{
    auto ColorClasses = TArray<ECk_Jolt_DebugDraw_ColorClass>{};
    ColorClasses.Reserve(_Impl->_Buckets.Num());

    for (const auto& Kvp : _Impl->_Buckets)
    { ColorClasses.Emplace(Kvp.Key._ColorClass); }

    return ColorClasses;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
