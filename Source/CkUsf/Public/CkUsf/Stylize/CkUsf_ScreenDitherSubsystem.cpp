#include "CkUsf/Stylize/CkUsf_ScreenDitherSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Stylize/CkUsf_ScreenDitherPreset.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

const FName UCkUsf_ScreenDitherSubsystem::kLookName = TEXT("ScreenDither");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_ScreenDitherSubsystem::
    Get_ScreenDitherSubsystem(
        const UObject* InWorldContextObject)
    -> UCkUsf_ScreenDitherSubsystem*
{
    if (ck::Is_NOT_Valid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCkUsf_ScreenDitherSubsystem>();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Request_SetEnabled(
        ECk_EnableDisable InEnabled)
    -> void
{
    _Settings.Set_Enabled(InEnabled);
    DoSync_ViewEffect();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Get_IsEnabled() const
    -> ECk_EnableDisable
{
    return _Settings.Get_Enabled();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Apply_Preset(
        UCkUsf_ScreenDitherPreset* InPreset)
    -> void
{
    const auto PresetIsValid = ck::IsValid(InPreset, ck::IsValid_Policy_NullptrOnly{});

    CK_ENSURE_IF_NOT(PresetIsValid,
        TEXT("Apply_Preset: null ScreenDither preset; settings left untouched"))
    {}

    if (NOT PresetIsValid)
    { return; }

    Request_SetSettings(InPreset->Get_AsParams());
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Request_SetSettings(
        const FCk_Usf_ScreenDither_Params& InSettings)
    -> void
{
    // An empty palette in CustomPalette mode is not a subtle mistake: every pixel snaps to the black
    // the unused slots are filled with, so the whole view goes black with nothing naming the cause.
    const auto PaletteIsUsable =
        InSettings.Get_PaletteMode() != ECk_Usf_PaletteMode::CustomPalette ||
        InSettings.Get_Palette().IsEmpty() == false;

    CK_ENSURE_IF_NOT(PaletteIsUsable,
        TEXT("Request_SetSettings: ScreenDither PaletteMode is CustomPalette with an EMPTY palette; "
             "settings left untouched"))
    {}

    if (NOT PaletteIsUsable)
    { return; }

    _Settings = InSettings;
    DoSync_ViewEffect();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Get_Settings() const
    -> FCk_Usf_ScreenDither_Params
{
    return _Settings;
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Request_ResetToDefaults()
    -> void
{
    Request_SetSettings(FCk_Usf_ScreenDither_Params{});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_ScreenDitherSubsystem::
    DoEnsure_ViewEffect()
    -> bool
{
    if (_DitherMID != nullptr)
    { return true; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto MasterPath = ck::usf::Get_GeneratedMasterObjectPath(kLookName);
    auto* Master = LoadObject<UMaterialInterface>(nullptr, *MasterPath);
    if (ck::Is_NOT_Valid(Master, ck::IsValid_Policy_NullptrOnly{}))
    {
        if (NOT _WarnedMissingMaster)
        {
            _WarnedMissingMaster = true;
            ck::usf::Warning(TEXT("ScreenDither master not found at [{}] — run Generate Look Materials. "
                                  "Settings are still tracked; nothing renders until the master exists."),
                MasterPath);
        }
        return false;
    }

    _DitherMID = UMaterialInstanceDynamic::Create(Master, this);

    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.Name = TEXT("CkUsf_ScreenDitherView");
    _ViewActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
    if (_ViewActor == nullptr)
    {
        _DitherMID = nullptr;
        return false;
    }

    _ViewPP = NewObject<UPostProcessComponent>(_ViewActor, TEXT("ScreenDitherPostProcess"));
    _ViewActor->SetRootComponent(_ViewPP);
    _ViewPP->bUnbound = true;
    _ViewPP->RegisterComponent();
    _ViewPP->Settings.AddBlendable(_DitherMID, 1.0f);

    return true;
}

auto
    UCkUsf_ScreenDitherSubsystem::
    DoSync_ViewEffect()
    -> void
{
    if (DoEnsure_ViewEffect() == false)
    { return; }

    // The component is owned by a world-spawned actor, so it can be torn down under us while the MID
    // (outered to this subsystem) survives and keeps DoEnsure_ViewEffect returning true.
    if (ck::IsValid(_ViewPP))
    { _ViewPP->bEnabled = _Settings.Get_Enabled() == ECk_EnableDisable::Enable; }

    DoWrite_ChangedParams();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    DoWrite_ChangedParams()
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? _Settings : _WrittenSettings.GetValue();

    const auto Set_Scalar = [&](const TCHAR* InName, float InValue, float InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _DitherMID->SetScalarParameterValue(InName, InValue); }
    };

    const auto Set_Vector = [&](const TCHAR* InName, const FLinearColor& InValue, const FLinearColor& InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _DitherMID->SetVectorParameterValue(InName, InValue); }
    };

    const auto Get_EnumIndex = [](auto InEnumValue) -> float
    {
        return static_cast<float>(static_cast<uint8>(InEnumValue));
    };

    const auto Get_Flag = [](ECk_EnableDisable InEnableDisable) -> float
    {
        return InEnableDisable == ECk_EnableDisable::Enable ? 1.0f : 0.0f;
    };

    Set_Scalar(TEXT("DitherPattern"),
        Get_EnumIndex(_Settings.Get_Pattern()), Get_EnumIndex(Previous.Get_Pattern()));
    Set_Scalar(TEXT("PixelScale"), _Settings.Get_PixelScale(), Previous.Get_PixelScale());
    Set_Scalar(TEXT("DitherStrength"), _Settings.Get_DitherStrength(), Previous.Get_DitherStrength());
    Set_Scalar(TEXT("Animate"),
        Get_Flag(_Settings.Get_Animate()), Get_Flag(Previous.Get_Animate()));
    Set_Scalar(TEXT("AnimationPeriod"), _Settings.Get_AnimationPeriod(), Previous.Get_AnimationPeriod());
    Set_Scalar(TEXT("BoxFilterDownsample"),
        Get_Flag(_Settings.Get_BoxFilterDownsample()), Get_Flag(Previous.Get_BoxFilterDownsample()));
    Set_Scalar(TEXT("StabilizeGrid"),
        Get_Flag(_Settings.Get_StabilizeGrid()), Get_Flag(Previous.Get_StabilizeGrid()));
    Set_Scalar(TEXT("PaletteMode"),
        Get_EnumIndex(_Settings.Get_PaletteMode()), Get_EnumIndex(Previous.Get_PaletteMode()));
    Set_Scalar(TEXT("ColorSteps"),
        static_cast<float>(_Settings.Get_ColorSteps()), static_cast<float>(Previous.Get_ColorSteps()));
    Set_Scalar(TEXT("ColorSpace"),
        Get_EnumIndex(_Settings.Get_ColorSpace()), Get_EnumIndex(Previous.Get_ColorSpace()));
    Set_Scalar(TEXT("PreGamma"), _Settings.Get_PreGamma(), Previous.Get_PreGamma());
    Set_Scalar(TEXT("Monochrome"),
        Get_Flag(_Settings.Get_Monochrome()), Get_Flag(Previous.Get_Monochrome()));
    Set_Scalar(TEXT("Saturation"), _Settings.Get_Saturation(), Previous.Get_Saturation());
    Set_Scalar(TEXT("Contrast"), _Settings.Get_Contrast(), Previous.Get_Contrast());
    Set_Scalar(TEXT("Weight"), _Settings.Get_Weight(), Previous.Get_Weight());
    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(_Settings.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Vector(TEXT("MonochromeShadowTint"),
        _Settings.Get_MonochromeShadowTint(), Previous.Get_MonochromeShadowTint());
    Set_Vector(TEXT("MonochromeHighlightTint"),
        _Settings.Get_MonochromeHighlightTint(), Previous.Get_MonochromeHighlightTint());

    // Entries past the shader's fixed 8-wide palette are dropped, and unfilled slots are written black so a
    // shrunk palette cannot leave a stale colour behind for the nearest-entry search to find.
    const auto Get_PaletteEntry = [](const FCk_Usf_ScreenDither_Params& InParams, int32 InIndex) -> FLinearColor
    {
        return InParams.Get_Palette().IsValidIndex(InIndex) ? InParams.Get_Palette()[InIndex] : FLinearColor::Black;
    };

    const auto Get_PaletteCount = [](const FCk_Usf_ScreenDither_Params& InParams) -> float
    {
        return static_cast<float>(FMath::Clamp(
            InParams.Get_Palette().Num(), 1, FCk_Usf_ScreenDither_Params::MaxPaletteEntries));
    };

    Set_Scalar(TEXT("PaletteCount"), Get_PaletteCount(_Settings), Get_PaletteCount(Previous));

    for (auto Index = 0; Index < FCk_Usf_ScreenDither_Params::MaxPaletteEntries; ++Index)
    {
        Set_Vector(*FString::Printf(TEXT("PaletteColor%d"), Index),
            Get_PaletteEntry(_Settings, Index), Get_PaletteEntry(Previous, Index));
    }

    _WrittenSettings = _Settings;
}

// --------------------------------------------------------------------------------------------------------------------
