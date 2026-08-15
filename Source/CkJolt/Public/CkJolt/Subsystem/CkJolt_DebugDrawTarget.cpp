#include "CkJolt_DebugDrawTarget.h"

#include <Jolt/Jolt.h>

#if JPH_DEBUG_RENDERER

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget_Impl.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include <Components/InstancedStaticMeshComponent.h>
#include <Engine/World.h>
#include <Materials/Material.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debug_draw_target
{
    const auto ColorParameterName = FName{TEXT("Color")};

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
