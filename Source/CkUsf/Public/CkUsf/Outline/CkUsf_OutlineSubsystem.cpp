#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Math/Float16Color.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_OutlineSubsystem::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    // A dedicated server renders nothing, so nothing should make it build the params LUT and the view
    // machinery per world. Process-level rather than per-world on purpose: at this point the world has no
    // NetDriver, so its net mode would only re-derive the same process answer. Known gap, matching the
    // three Stylize subsystems' precedent: a PIE dedicated-server world lives in the editor process and
    // still gets one.
    if (IsRunningDedicatedServer())
    { return false; }

    return Super::ShouldCreateSubsystem(InOuter);
}

auto
    UCkUsf_OutlineSubsystem::
    Get_OutlineSubsystem(
        const UObject* InWorldContextObject)
    -> UCkUsf_OutlineSubsystem*
{
    if (ck::Is_NOT_Valid(GEngine))
    { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCkUsf_OutlineSubsystem>();
}

auto
    UCkUsf_OutlineSubsystem::
    Apply_Outline_To_Actor(
        AActor* InActor,
        UCkUsf_OutlinePreset* InPreset)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Apply_Outline_To_Actor: null actor"))
    { return; }

    TArray<UPrimitiveComponent*> Primitives;
    InActor->GetComponents(Primitives);
    for (auto* Primitive : Primitives)
    { Apply_Outline_To_Component(Primitive, InPreset); }
}

auto
    UCkUsf_OutlineSubsystem::
    Apply_Outline_To_Component(
        UPrimitiveComponent* InComponent,
        UCkUsf_OutlinePreset* InPreset)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComponent) &&
                     ck::IsValid(InPreset),
        TEXT("Apply_Outline_To_Component: null component or preset"))
    { return; }

    if (DoEnsure_ViewEffect() == false)
    {
        ck::usf::Warning(TEXT("Outline view effect unavailable (SolidOutline master missing? run Generate Look Materials)"));
        return;
    }

    if (_AppliedComponents.Contains(InComponent))
    { Remove_Outline_From_Component(InComponent); }

    const auto Stencil = Get_OrAllocate_StencilFor(InPreset);
    if (Stencil == 0)
    { return; } // range exhausted — already warned

    InComponent->SetRenderCustomDepth(true);
    InComponent->SetCustomDepthStencilValue(static_cast<int32>(Stencil));
    _AppliedComponents.Add(InComponent, FAppliedOutline{InPreset, static_cast<int32>(Stencil)});
}

auto
    UCkUsf_OutlineSubsystem::
    Remove_Outline_From_Actor(
        AActor* InActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Remove_Outline_From_Actor: null actor"))
    { return; }

    TArray<UPrimitiveComponent*> Primitives;
    InActor->GetComponents(Primitives);
    for (auto* Primitive : Primitives)
    { Remove_Outline_From_Component(Primitive); }
}

auto
    UCkUsf_OutlineSubsystem::
    Remove_Outline_From_Component(
        UPrimitiveComponent* InComponent)
    -> void
{
    if (ck::Is_NOT_Valid(InComponent))
    { return; }

    auto* Applied = _AppliedComponents.Find(InComponent);
    if (Applied == nullptr)
    { return; }

    // Only undo what THIS feature still owns. A lower-precedence claim (cel pattern, effect mask) may
    // have taken the byte over since — its sync processor writes every frame, and by design it does NOT
    // clear this map. Disabling custom depth unconditionally would then blank that claim permanently:
    // its own applied-state still says "written", so its sync early-outs forever and nothing re-asserts.
    // The two sibling features guard their undos identically.
    if (InComponent->CustomDepthStencilValue == Applied->StencilValue)
    { InComponent->SetRenderCustomDepth(false); }

    // An expired preset must not release: FWeakObjectPtr treats ALL invalid weak ptrs as equal, so a
    // nullptr Find against the weak-keyed _ActivePresets can match an unrelated expired entry.
    if (Applied->Preset.IsValid())
    { Release_StencilFor(Applied->Preset.Get()); }
    _AppliedComponents.Remove(InComponent);
}

auto
    UCkUsf_OutlineSubsystem::
    Set_GlobalOutlineThickness(
        float InThickness)
    -> void
{
    _GlobalThickness = InThickness;
    if (_OutlineMID != nullptr)
    { _OutlineMID->SetScalarParameterValue(TEXT("GlobalThickness"), InThickness); }
}

auto
    UCkUsf_OutlineSubsystem::
    Get_OrAllocate_StencilFor(
        UCkUsf_OutlinePreset* InPreset)
    -> uint8
{
    if (ck::Is_NOT_Valid(InPreset))
    { return 0; }

    // External renderers (shadow ISM, ISKM SKMCs, batched clusters) allocate directly without ever calling
    // Apply_Outline_To_Component. Failure here is non-fatal (headless/tests): DoEnsure_ViewEffect re-writes
    // every active row on its first success, so early allocations are not left with zeroed LUT rows.
    DoEnsure_ViewEffect();

    DoReap_DeadComponents();

    if (auto* Found = _ActivePresets.Find(InPreset))
    {
        ++Found->RefCount;
        return Found->Value;
    }

    TSet<uint8> UsedStencilValues;
    for (const auto& Active : _ActivePresets)
    { UsedStencilValues.Add(Active.Value.Value); }

    for (auto Value = static_cast<int32>(_StencilMin); Value <= static_cast<int32>(_StencilMax); ++Value)
    {
        if (UsedStencilValues.Contains(static_cast<uint8>(Value)) == false)
        {
            const auto Chosen = static_cast<uint8>(Value);
            _ActivePresets.Add(InPreset, FStencilSlot{ Chosen, 1 });
            DoWrite_PresetRow(Chosen - _StencilMin, InPreset);
            DoUpload_Lut();
            return Chosen;
        }
    }

    ck::usf::Warning(TEXT("Outline preset stencil range [{}..{}] exhausted ({} active presets max); outline not assigned"),
        static_cast<int32>(_StencilMin), static_cast<int32>(_StencilMax),
        static_cast<int32>(_StencilMax) - static_cast<int32>(_StencilMin) + 1);
    return 0;
}

auto
    UCkUsf_OutlineSubsystem::
    Release_StencilFor(
        UCkUsf_OutlinePreset* InPreset)
    -> void
{
    auto* Found = _ActivePresets.Find(InPreset);
    if (Found == nullptr)
    { return; }

    --Found->RefCount;
    if (Found->RefCount > 0)
    { return; }

    const auto Slot = static_cast<int32>(Found->Value) - static_cast<int32>(_StencilMin);
    _ActivePresets.Remove(InPreset);

    if (_LutData.IsValidIndex(Slot))
    {
        _LutData[kLutRow_Outline * kLutWidth + Slot] = FFloat16Color(FLinearColor::Black);
        _LutData[kLutRow_Fill * kLutWidth + Slot] = FFloat16Color(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
        DoUpload_Lut();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_OutlineSubsystem::
    DoEnsure_ViewEffect()
    -> bool
{
    if (_OutlineMID != nullptr)
    { return true; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    // ---- Params LUT (16 x 2 RGBA16f; sampled at exact texel centers, never streamed) ----
    _ParamsTex = UTexture2D::CreateTransient(kLutWidth, kLutHeight, PF_FloatRGBA);
    if (_ParamsTex == nullptr)
    { return false; }

    _ParamsTex->SRGB = false;
    _ParamsTex->Filter = TF_Nearest;
    _ParamsTex->AddressX = TA_Clamp;
    _ParamsTex->AddressY = TA_Clamp;
    _ParamsTex->NeverStream = true;
    _ParamsTex->UpdateResource();

    _LutData.SetNumZeroed(kLutWidth * kLutHeight);
    DoUpload_Lut();

    // ---- SolidOutline master -> MID ----
    const auto MasterPath = ck::usf::Get_GeneratedMasterObjectPath(FName(TEXT("SolidOutline")));
    auto* Master = LoadObject<UMaterialInterface>(nullptr, *MasterPath);
    if (ck::Is_NOT_Valid(Master))
    {
        ck::usf::Warning(TEXT("SolidOutline master not found at [{}] — run Generate Look Materials"), MasterPath);
        return false;
    }

    _OutlineMID = UMaterialInstanceDynamic::Create(Master, this);
    _OutlineMID->SetTextureParameterValue(TEXT("OutlineParams"), _ParamsTex);
    _OutlineMID->SetScalarParameterValue(TEXT("GlobalThickness"), _GlobalThickness);
    _OutlineMID->SetScalarParameterValue(TEXT("StencilMin"), static_cast<float>(_StencilMin));
    _OutlineMID->SetScalarParameterValue(TEXT("StencilMax"), static_cast<float>(_StencilMax));

    // ---- Unbound post-process component on a transient actor -> whole-view blendable ----
    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.Name = TEXT("CkUsf_OutlineView");
    _ViewActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
    if (_ViewActor == nullptr)
    {
        // The MID is what this function early-outs on, so leaving it set turns a failed spawn into a
        // permanent silent success: every later call returns true with no post-process component attached
        // and outlines never render in this world. Same shape as UCkUsf_ScreenDitherSubsystem's.
        _OutlineMID = nullptr;
        return false;
    }

    _ViewPP = NewObject<UPostProcessComponent>(_ViewActor, TEXT("OutlinePostProcess"));
    _ViewActor->SetRootComponent(_ViewPP);
    _ViewPP->bUnbound = true;
    _ViewPP->RegisterComponent();
    _ViewPP->Settings.AddBlendable(_OutlineMID, 1.0f);

    // Presets may have allocated stencils before the view effect existed (renderers call
    // Get_OrAllocate_StencilFor directly) — their LUT rows were unwritable then; write them now.
    for (const auto& Active : _ActivePresets)
    {
        if (auto* Preset = Active.Key.Get())
        { DoWrite_PresetRow(static_cast<int32>(Active.Value.Value) - static_cast<int32>(_StencilMin), Preset); }
    }
    DoUpload_Lut();

    return true;
}

auto
    UCkUsf_OutlineSubsystem::
    DoWrite_PresetRow(
        int32 InSlot,
        const UCkUsf_OutlinePreset* InPreset)
    -> void
{
    if (_LutData.IsValidIndex(kLutRow_Outline * kLutWidth + InSlot) == false ||
        _LutData.IsValidIndex(kLutRow_Fill * kLutWidth + InSlot) == false ||
        ck::Is_NOT_Valid(InPreset))
    { return; }

    const auto Brightness = InPreset->_OutlineBrightness;
    const auto OutlineColor = FLinearColor(
        InPreset->_OutlineColor.R * Brightness,
        InPreset->_OutlineColor.G * Brightness,
        InPreset->_OutlineColor.B * Brightness,
        static_cast<float>(static_cast<uint8>(InPreset->_OutlineType))); // alpha carries the outline type (0/1/2)

    auto FillColor = InPreset->_FillColor;
    FillColor.A = InPreset->_FillEnabled ? InPreset->_FillOpacity : 0.0f;

    _LutData[kLutRow_Outline * kLutWidth + InSlot] = FFloat16Color(OutlineColor);
    _LutData[kLutRow_Fill * kLutWidth + InSlot] = FFloat16Color(FillColor);
}

auto
    UCkUsf_OutlineSubsystem::
    DoUpload_Lut()
    -> void
{
    if (_ParamsTex == nullptr || _LutData.Num() == 0)
    { return; }

    auto* PlatformData = _ParamsTex->GetPlatformData();
    if (PlatformData == nullptr || PlatformData->Mips.Num() == 0)
    { return; }

    auto& Mip = PlatformData->Mips[0];
    auto* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Dest, _LutData.GetData(), _LutData.Num() * sizeof(FFloat16Color));
    Mip.BulkData.Unlock();
    _ParamsTex->UpdateResource();
}

auto
    UCkUsf_OutlineSubsystem::
    DoReap_DeadComponents()
    -> void
{
    TArray<TWeakObjectPtr<UPrimitiveComponent>> Dead;
    for (const auto& Applied : _AppliedComponents)
    {
        if (Applied.Key.IsValid() == false)
        { Dead.Add(Applied.Key); }
    }

    for (const auto& DeadComponent : Dead)
    {
        // Same expired-preset guard as Remove_Outline_From_Component. No value guard here: the component
        // is already gone, so there is nothing to disable and nothing to protect.
        if (auto* Applied = _AppliedComponents.Find(DeadComponent);
            Applied != nullptr && Applied->Preset.IsValid())
        { Release_StencilFor(Applied->Preset.Get()); }
        _AppliedComponents.Remove(DeadComponent);
    }
}
