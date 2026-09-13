#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkUsf/Outline/CkUsf_Outline_ProjectSettings.h"
#include "CkUsfRenderer/Outline/CkUsf_Outline_Renderer.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Math/Float16Color.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCkUsf_OutlineSubsystem::Initialize(FSubsystemCollectionBase& InCollection) -> void
{
    Super::Initialize(InCollection);
    _LutData.SetNumZeroed(kLutWidth * kLutHeight);
    TrySet_ThicknessSettings(UCk_Utils_Usf_Outline_Settings_UE::Get_ThicknessSettings());
}

auto UCkUsf_OutlineSubsystem::Deinitialize() -> void
{
    if (_Renderer.IsValid()) { _Renderer->Deactivate(); }
    _Renderer.Reset();
    Super::Deinitialize();
}

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
    Set_ResolvedOutline(
        UPrimitiveComponent* InComponent,
        const FCk_Handle& InRenderOwner,
        const ck::FFragment_Usf_OutlineResolved& InResolved)
    -> void
{
    const auto IsComponentValid = ck::IsValid(InComponent);
    CK_ENSURE_IF_NOT(IsComponentValid, TEXT("Set_ResolvedOutline: component is INVALID")) {}
    if (NOT IsComponentValid) { return; }

    const auto IsOwnerValid = ck::IsValid(InRenderOwner);
    CK_ENSURE_IF_NOT(IsOwnerValid,
        TEXT("Set_ResolvedOutline: render owner is INVALID for component [{}]"), InComponent) {}
    if (NOT IsOwnerValid) { return; }

    const auto IsPresetValid = ck::IsValid(InResolved.Get_Preset().Get());
    CK_ENSURE_IF_NOT(IsPresetValid,
        TEXT("Set_ResolvedOutline: resolved preset is INVALID for owner [{}]"), InRenderOwner) {}
    if (NOT IsPresetValid) { return; }

    const auto IsSourceValid = ck::IsValid(InResolved.Get_Source());
    CK_ENSURE_IF_NOT(IsSourceValid,
        TEXT("Set_ResolvedOutline: resolved source is INVALID for owner [{}]"), InRenderOwner) {}
    if (NOT IsSourceValid) { return; }

    const auto IsLayerIndexValid = InResolved.Get_LayerIndex() >= 0;
    CK_ENSURE_IF_NOT(IsLayerIndexValid,
        TEXT("Set_ResolvedOutline: resolved layer index [{}] is INVALID for owner [{}]"),
        InResolved.Get_LayerIndex(), InRenderOwner) {}
    if (NOT IsLayerIndexValid) { return; }

    const auto IsOwnershipDistanceValid = InResolved.Get_OwnershipDistance() >= 0;
    CK_ENSURE_IF_NOT(IsOwnershipDistanceValid,
        TEXT("Set_ResolvedOutline: resolved ownership distance [{}] is INVALID for owner [{}]"),
        InResolved.Get_OwnershipDistance(), InRenderOwner) {}
    if (NOT IsOwnershipDistanceValid) { return; }

    auto RuntimeConfig = FCk_Usf_OutlineRuntimeConfig{};
    if (NOT UCk_Utils_Usf_Outline_Settings_UE::TryGet_RuntimeConfig(RuntimeConfig)) { return; }

    const auto* Definition = RuntimeConfig.TryGet(InResolved.Get_OutlineTag());
    const auto IsOutlineConfigured = Definition != nullptr;
    CK_ENSURE_IF_NOT(IsOutlineConfigured,
        TEXT("Set_ResolvedOutline: outline tag [{}] is invalid or unconfigured for owner [{}]"),
        InResolved.Get_OutlineTag(), InRenderOwner) {}
    if (NOT IsOutlineConfigured) { return; }

    const auto DoesLayerMatch = Definition->LayerTag.MatchesTagExact(InResolved.Get_LayerTag());
    CK_ENSURE_IF_NOT(DoesLayerMatch,
        TEXT("Set_ResolvedOutline: layer [{}] does not match configured layer [{}] for outline [{}]"),
        InResolved.Get_LayerTag(), Definition->LayerTag, InResolved.Get_OutlineTag()) {}
    if (NOT DoesLayerMatch) { return; }

    const auto DoesLayerIndexMatch = Definition->LayerIndex == InResolved.Get_LayerIndex();
    CK_ENSURE_IF_NOT(DoesLayerIndexMatch,
        TEXT("Set_ResolvedOutline: layer index [{}] does not match configured index [{}] for outline [{}]"),
        InResolved.Get_LayerIndex(), Definition->LayerIndex, InResolved.Get_OutlineTag()) {}
    if (NOT DoesLayerIndexMatch) { return; }

    const auto DoesPresetMatch = Definition->Preset == InResolved.Get_Preset().Get();
    CK_ENSURE_IF_NOT(DoesPresetMatch,
        TEXT("Set_ResolvedOutline: preset does not match configured preset for outline [{}]"),
        InResolved.Get_OutlineTag()) {}
    if (NOT DoesPresetMatch) { return; }

    auto& Owners = _ResolvedOwners.FindOrAdd(InComponent);
    auto* Existing = Owners.FindByPredicate([&InRenderOwner](const FResolvedOwner& InOwner)
    { return InOwner.RenderOwner == InRenderOwner; });
    if (Existing != nullptr)
    { Existing->Resolved = InResolved; }
    else
    { Owners.Add(FResolvedOwner{InRenderOwner, InResolved}); }

    DoReconcile_ResolvedOutline(InComponent);
}

auto
    UCkUsf_OutlineSubsystem::
    Clear_ResolvedOutline(
        UPrimitiveComponent* InComponent,
        const FCk_Handle& InRenderOwner)
    -> void
{
    const auto IsComponentValid = ck::IsValid(InComponent);
    CK_ENSURE_IF_NOT(IsComponentValid, TEXT("Clear_ResolvedOutline: component is INVALID")) {}
    if (NOT IsComponentValid) { return; }

    const auto IsOwnerValid = ck::IsValid(InRenderOwner);
    CK_ENSURE_IF_NOT(IsOwnerValid,
        TEXT("Clear_ResolvedOutline: render owner is INVALID for component [{}]"), InComponent) {}
    if (NOT IsOwnerValid) { return; }

    auto* Owners = _ResolvedOwners.Find(InComponent);
    if (Owners == nullptr) { return; }

    Owners->RemoveAll([&InRenderOwner](const FResolvedOwner& InOwner)
    { return InOwner.RenderOwner == InRenderOwner; });
    if (Owners->IsEmpty())
    {
        _ResolvedOwners.Remove(InComponent);
        DoRemove_PhysicalOutline(InComponent);
        return;
    }

    DoReconcile_ResolvedOutline(InComponent);
}

auto
    UCkUsf_OutlineSubsystem::
    Get_CurrentOutlinePreset(
        UPrimitiveComponent* InComponent) const
    -> UCkUsf_OutlinePreset*
{
    const auto* Applied = _AppliedComponents.Find(InComponent);
    return Applied != nullptr ? Applied->Preset.Get() : nullptr;
}

auto
    UCkUsf_OutlineSubsystem::
    Get_OutlineOwnerCount(
        UPrimitiveComponent* InComponent) const
    -> int32
{
    const auto* Owners = _ResolvedOwners.Find(InComponent);
    return Owners != nullptr ? Owners->Num() : 0;
}

auto
    UCkUsf_OutlineSubsystem::
    DoFind_WinningResolvedOwner(
        const TArray<FResolvedOwner>& InOwners) const
    -> const FResolvedOwner*
{
    const FResolvedOwner* Winner = nullptr;
    for (const auto& Candidate : InOwners)
    {
        if (Winner == nullptr)
        {
            Winner = &Candidate;
            continue;
        }

        const auto& Current = Winner->Resolved;
        const auto& Proposed = Candidate.Resolved;
        auto ProposedWins = Proposed.Get_LayerIndex() < Current.Get_LayerIndex();
        if (Proposed.Get_LayerIndex() == Current.Get_LayerIndex())
        {
            ProposedWins = Proposed.Get_OwnershipDistance() < Current.Get_OwnershipDistance();
            if (Proposed.Get_OwnershipDistance() == Current.Get_OwnershipDistance())
            {
                ProposedWins = Proposed.Get_Source() < Current.Get_Source();
                if (Proposed.Get_Source() == Current.Get_Source())
                {
                    const auto ProposedTag = Proposed.Get_OutlineTag().ToString();
                    const auto CurrentTag = Current.Get_OutlineTag().ToString();
                    ProposedWins = ProposedTag < CurrentTag ||
                                   (ProposedTag == CurrentTag && Candidate.RenderOwner < Winner->RenderOwner);
                }
            }
        }
        if (ProposedWins) { Winner = &Candidate; }
    }
    return Winner;
}

auto
    UCkUsf_OutlineSubsystem::
    DoReconcile_ResolvedOutline(
        UPrimitiveComponent* InComponent)
    -> void
{
    auto* Owners = _ResolvedOwners.Find(InComponent);
    if (Owners == nullptr || Owners->IsEmpty())
    {
        DoRemove_PhysicalOutline(InComponent);
        return;
    }

    const auto* Winner = DoFind_WinningResolvedOwner(*Owners);
    check(Winner != nullptr);
    auto* WinnerPreset = Winner->Resolved.Get_Preset().Get();
    const auto* Applied = _AppliedComponents.Find(InComponent);
    const auto IsAlreadyApplied = Applied != nullptr && Applied->Preset == WinnerPreset &&
                                  InComponent->bRenderCustomDepth &&
                                  InComponent->CustomDepthStencilValue == Applied->StencilValue;
    if (IsAlreadyApplied) { return; }

    DoApply_PhysicalOutline(InComponent, WinnerPreset);
}

auto
    UCkUsf_OutlineSubsystem::
    DoApply_PhysicalOutline(
        UPrimitiveComponent* InComponent,
        UCkUsf_OutlinePreset* InPreset)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComponent) &&
                     ck::IsValid(InPreset),
        TEXT("DoApply_PhysicalOutline: null component or preset"))
    { return; }

    if (DoEnsure_ViewEffect() == false)
    {
        return;
    }

    if (_AppliedComponents.Contains(InComponent))
    { DoRemove_PhysicalOutline(InComponent); }

    const auto Stencil = Get_OrAllocate_StencilFor(InPreset);
    if (Stencil == 0)
    { return; } // range exhausted — already warned

    const auto PreviousRenderCustomDepth = InComponent->bRenderCustomDepth;
    const auto PreviousStencilValue = InComponent->CustomDepthStencilValue;
    InComponent->SetRenderCustomDepth(true);
    InComponent->SetCustomDepthStencilValue(static_cast<int32>(Stencil));
    _AppliedComponents.Add(InComponent, FAppliedOutline{
        InPreset,
        static_cast<int32>(Stencil),
        PreviousRenderCustomDepth != 0,
        PreviousStencilValue});
}

auto
    UCkUsf_OutlineSubsystem::
    DoRemove_PhysicalOutline(
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
    const auto StillOwnsComponentState = InComponent->bRenderCustomDepth &&
                                        InComponent->CustomDepthStencilValue == Applied->StencilValue;
    if (StillOwnsComponentState)
    {
        InComponent->SetCustomDepthStencilValue(Applied->PreviousStencilValue);
        InComponent->SetRenderCustomDepth(Applied->PreviousRenderCustomDepth);
    }

    // An expired preset must not release: FWeakObjectPtr treats ALL invalid weak ptrs as equal, so a
    // nullptr Find against the weak-keyed _ActivePresets can match an unrelated expired entry.
    if (Applied->Preset.IsValid())
    { Release_StencilFor(Applied->Preset.Get()); }
    _AppliedComponents.Remove(InComponent);
}

auto
    UCkUsf_OutlineSubsystem::
    TrySet_ThicknessSettings(
        const FCk_Usf_OutlineThicknessSettings& InSettings)
    -> bool
{
    if (NOT UCk_Utils_Usf_Outline_Settings_UE::TryValidate_ThicknessSettings(InSettings))
    { return false; }
    _ThicknessSettings = InSettings;
    _ThicknessSettingsAreValid = true;
    DoUpload_Lut();
    return true;
}

auto
    UCkUsf_OutlineSubsystem::
    Get_OrAllocate_StencilFor(
        UCkUsf_OutlinePreset* InPreset)
    -> uint8
{
    if (ck::Is_NOT_Valid(InPreset))
    { return 0; }

    // External renderers allocate directly. Validate the view/configuration before publishing any slot.
    if (NOT DoEnsure_ViewEffect()) { return 0; }

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
    if (_Renderer.IsValid()) { return true; }
    auto* World = GetWorld();
    const auto IsWorldValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(IsWorldValid, TEXT("Outline renderer requires a valid world")) {}
    if (NOT IsWorldValid) { return false; }
    CK_ENSURE_IF_NOT(_ThicknessSettingsAreValid, TEXT("Outline renderer requires valid thickness settings")) {}
    if (NOT _ThicknessSettingsAreValid) { return false; }
    _Renderer = MakeShared<ck::usf::FOutlineRenderer, ESPMode::ThreadSafe>(World);
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
    if (NOT _Renderer.IsValid() || _LutData.IsEmpty()) { return; }
    auto State = ck::usf::FOutlineRenderState{};
    State.StencilMin = _StencilMin;
    State.WorldSpace = _ThicknessSettings.Get_Space() == ECk_Usf_OutlineThicknessSpace::WorldSpace;
    State.SquareCorners = _ThicknessSettings.Get_SquareCorners();
    State.Thickness = State.WorldSpace ? _ThicknessSettings.Get_WorldSpaceThickness() :
        _ThicknessSettings.Get_ScreenSpaceThickness();
    for (const auto& Active : _ActivePresets)
    { State.ActiveMask |= 1u << (Active.Value.Value - _StencilMin); }
    for (auto Index = 0; Index < kLutWidth; ++Index)
    {
        const auto Outline = _LutData[kLutRow_Outline * kLutWidth + Index].GetFloats();
        const auto Fill = _LutData[kLutRow_Fill * kLutWidth + Index].GetFloats();
        State.Outline[Index] = FVector4f(Outline.R, Outline.G, Outline.B, Outline.A);
        State.Fill[Index] = FVector4f(Fill.R, Fill.G, Fill.B, Fill.A);
    }
    _Renderer->Set_State(State);
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
    for (const auto& Owners : _ResolvedOwners)
    {
        if (Owners.Key.IsValid() == false)
        { Dead.AddUnique(Owners.Key); }
    }

    for (const auto& DeadComponent : Dead)
    {
        // Same expired-preset guard as DoRemove_PhysicalOutline. No value guard here: the component
        // is already gone, so there is nothing to disable and nothing to protect.
        if (auto* Applied = _AppliedComponents.Find(DeadComponent);
            Applied != nullptr && Applied->Preset.IsValid())
        { Release_StencilFor(Applied->Preset.Get()); }
        _AppliedComponents.Remove(DeadComponent);
        _ResolvedOwners.Remove(DeadComponent);
    }
}
