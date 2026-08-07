#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"
#include "CkUsf/Stylize/CkUsf_CelShadePreset.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

const FName UCkUsf_CelShadeSubsystem::kLookName = TEXT("CelShade");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CelShadeSubsystem::
    Get_CelShadeSubsystem(
        const UObject* InWorldContextObject)
    -> UCkUsf_CelShadeSubsystem*
{
    if (ck::Is_NOT_Valid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCkUsf_CelShadeSubsystem>();
}

auto
    UCkUsf_CelShadeSubsystem::
    Request_SetEnabled(
        ECk_EnableDisable InEnabled)
    -> void
{
    _Settings.Set_Enabled(InEnabled);
    DoSync_ViewEffect();
}

auto
    UCkUsf_CelShadeSubsystem::
    Get_IsEnabled() const
    -> ECk_EnableDisable
{
    return _Settings.Get_Enabled();
}

auto
    UCkUsf_CelShadeSubsystem::
    Apply_Preset(
        UCkUsf_CelShadePreset* InPreset)
    -> void
{
    const auto PresetIsValid = ck::IsValid(InPreset, ck::IsValid_Policy_NullptrOnly{});

    CK_ENSURE_IF_NOT(PresetIsValid,
        TEXT("Apply_Preset: null CelShade preset; settings left untouched"))
    {}

    if (NOT PresetIsValid)
    { return; }

    Request_SetSettings(InPreset->Get_AsParams());
}

auto
    UCkUsf_CelShadeSubsystem::
    Request_SetSettings(
        const FCk_Usf_CelShade_Params& InSettings)
    -> void
{
    const auto StencilRangeIsAddressable = DoGet_StencilRangeIsAddressable(InSettings);

    CK_ENSURE_IF_NOT(StencilRangeIsAddressable,
        TEXT("Request_SetSettings: CelShade stencil range [{}, {}] reaches the engine's NO-STENCIL value "
             "0 or below; every untagged pixel in the view would read as a tagged one. Settings left "
             "untouched"),
        InSettings.Get_StencilRangeMin(), InSettings.Get_StencilRangeMax())
    {}

    if (NOT StencilRangeIsAddressable)
    { return; }

    const auto StencilRangeAvoidsOutline = DoGet_StencilRangeAvoidsOutline(InSettings);

    CK_ENSURE_IF_NOT(StencilRangeAvoidsOutline,
        TEXT("Request_SetSettings: CelShade stencil range [{}, {}] COLLIDES with the outline subsystem's "
             "allocated range; settings left untouched"),
        InSettings.Get_StencilRangeMin(), InSettings.Get_StencilRangeMax())
    {}

    if (NOT StencilRangeAvoidsOutline)
    { return; }

    _Settings = InSettings;
    DoSync_ViewEffect();
}

auto
    UCkUsf_CelShadeSubsystem::
    Get_Settings() const
    -> FCk_Usf_CelShade_Params
{
    return _Settings;
}

auto
    UCkUsf_CelShadeSubsystem::
    Request_ResetToDefaults()
    -> void
{
    Request_SetSettings(FCk_Usf_CelShade_Params{});
}

auto
    UCkUsf_CelShadeSubsystem::
    Get_StencilRangeIsFree(
        const FCk_Usf_CelShade_Params& InSettings) const
    -> bool
{
    return DoGet_StencilRangeIsAddressable(InSettings) && DoGet_StencilRangeAvoidsOutline(InSettings);
}

auto
    UCkUsf_CelShadeSubsystem::
    DoGet_StencilRangeIsAddressable(
        const FCk_Usf_CelShade_Params& InSettings)
    -> bool
{
    if (InSettings.Get_EnableStencilPatterns() != ECk_EnableDisable::Enable)
    { return true; }

    // 0 is what the renderer leaves in Custom Stencil for every mesh that wrote nothing. A span reaching
    // it hands the whole untagged view a slot: base 1 makes suppression the default everywhere, base 0
    // forces pattern 0 on every pixel. Both read as "the look is broken", not as a misconfiguration.
    return InSettings.Get_StencilRangeMin() >= 1;
}

auto
    UCkUsf_CelShadeSubsystem::
    DoGet_StencilRangeAvoidsOutline(
        const FCk_Usf_CelShade_Params& InSettings) const
    -> bool
{
    if (InSettings.Get_EnableStencilPatterns() != ECk_EnableDisable::Enable)
    { return true; }

    // No outline subsystem in this world means nothing else is claiming stencil values through CkUsf.
    // Renderer modules that write stencil directly are out of reach of any check we could make here.
    const auto* Outline = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(GetWorld());
    if (ck::Is_NOT_Valid(Outline, ck::IsValid_Policy_NullptrOnly{}))
    { return true; }

    const auto OutlineMin = static_cast<int32>(Outline->Get_StencilMin());
    const auto OutlineMax = static_cast<int32>(Outline->Get_StencilMax());

    return InSettings.Get_StencilRangeMax() < OutlineMin || InSettings.Get_StencilRangeMin() > OutlineMax;
}

auto
    UCkUsf_CelShadeSubsystem::
    Get_StencilValueFor(
        ECk_Usf_CelPattern InPattern) const
    -> int32
{
    if (_Settings.Get_EnableStencilPatterns() != ECk_EnableDisable::Enable)
    { return 0; }

    return _Settings.Get_StencilBase() + static_cast<int32>(InPattern);
}

auto
    UCkUsf_CelShadeSubsystem::
    Get_StencilSuppressValue() const
    -> int32
{
    if (_Settings.Get_EnableStencilPatterns() != ECk_EnableDisable::Enable)
    { return 0; }

    return _Settings.Get_StencilRangeMin();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CelShadeSubsystem::
    DoEnsure_ViewEffect()
    -> bool
{
    if (_CelMID != nullptr)
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
            ck::usf::Warning(TEXT("CelShade master not found at [{}] — run Generate Look Materials. "
                                  "Settings are still tracked; nothing renders until the master exists."),
                MasterPath);
        }
        return false;
    }

    _CelMID = UMaterialInstanceDynamic::Create(Master, this);

    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.Name = TEXT("CkUsf_CelShadeView");
    _ViewActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
    if (_ViewActor == nullptr)
    {
        _CelMID = nullptr;
        return false;
    }

    _ViewPP = NewObject<UPostProcessComponent>(_ViewActor, TEXT("CelShadePostProcess"));
    _ViewActor->SetRootComponent(_ViewPP);
    _ViewPP->bUnbound = true;
    _ViewPP->RegisterComponent();
    _ViewPP->Settings.AddBlendable(_CelMID, 1.0f);

    return true;
}

auto
    UCkUsf_CelShadeSubsystem::
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
    UCkUsf_CelShadeSubsystem::
    DoWrite_ChangedParams()
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? _Settings : _WrittenSettings.GetValue();

    const auto Set_Scalar = [&](const TCHAR* InName, float InValue, float InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _CelMID->SetScalarParameterValue(InName, InValue); }
    };

    const auto Set_Vector = [&](const TCHAR* InName, const FLinearColor& InValue, const FLinearColor& InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _CelMID->SetVectorParameterValue(InName, InValue); }
    };

    const auto Get_EnumIndex = [](auto InEnumValue) -> float
    {
        return static_cast<float>(static_cast<uint8>(InEnumValue));
    };

    const auto Get_Flag = [](ECk_EnableDisable InEnableDisable) -> float
    {
        return InEnableDisable == ECk_EnableDisable::Enable ? 1.0f : 0.0f;
    };

    const auto Get_Count = [](int32 InValue) -> float
    {
        return static_cast<float>(InValue);
    };

    Set_Scalar(TEXT("Bands"), Get_Count(_Settings.Get_Bands()), Get_Count(Previous.Get_Bands()));
    Set_Scalar(TEXT("Midpoint"), _Settings.Get_Midpoint(), Previous.Get_Midpoint());
    Set_Scalar(TEXT("BandOffset"), _Settings.Get_BandOffset(), Previous.Get_BandOffset());
    Set_Scalar(TEXT("Distribution"), _Settings.Get_Distribution(), Previous.Get_Distribution());
    Set_Scalar(TEXT("BandSoftness"), _Settings.Get_BandSoftness(), Previous.Get_BandSoftness());
    Set_Scalar(TEXT("ShadowLift"), _Settings.Get_ShadowLift(), Previous.Get_ShadowLift());
    Set_Scalar(TEXT("Strength"), _Settings.Get_Strength(), Previous.Get_Strength());
    Set_Scalar(TEXT("QuantizeFinalColor"),
        Get_Flag(_Settings.Get_QuantizeFinalColor()), Get_Flag(Previous.Get_QuantizeFinalColor()));

    Set_Scalar(TEXT("EnablePattern"),
        Get_Flag(_Settings.Get_EnablePattern()), Get_Flag(Previous.Get_EnablePattern()));
    Set_Scalar(TEXT("Pattern"),
        Get_EnumIndex(_Settings.Get_Pattern()), Get_EnumIndex(Previous.Get_Pattern()));
    Set_Scalar(TEXT("PatternSpace"),
        Get_EnumIndex(_Settings.Get_PatternSpace()), Get_EnumIndex(Previous.Get_PatternSpace()));
    Set_Scalar(TEXT("PatternStrength"), _Settings.Get_PatternStrength(), Previous.Get_PatternStrength());
    Set_Scalar(TEXT("PatternContrast"), _Settings.Get_PatternContrast(), Previous.Get_PatternContrast());
    Set_Scalar(TEXT("PatternWorldSize"), _Settings.Get_PatternWorldSize(), Previous.Get_PatternWorldSize());
    Set_Scalar(TEXT("PatternPixelSize"), _Settings.Get_PatternPixelSize(), Previous.Get_PatternPixelSize());
    Set_Scalar(TEXT("TriplanarSharpness"), _Settings.Get_TriplanarSharpness(), Previous.Get_TriplanarSharpness());
    Set_Scalar(TEXT("PatternDistanceScaling"),
        _Settings.Get_PatternDistanceScaling(), Previous.Get_PatternDistanceScaling());
    Set_Scalar(TEXT("PatternOctaveMin"), _Settings.Get_PatternOctaveMin(), Previous.Get_PatternOctaveMin());
    Set_Scalar(TEXT("PatternOctaveMax"), _Settings.Get_PatternOctaveMax(), Previous.Get_PatternOctaveMax());
    Set_Scalar(TEXT("PatternScrollSpeed"), _Settings.Get_PatternScrollSpeed(), Previous.Get_PatternScrollSpeed());

    Set_Scalar(TEXT("Saturation"), _Settings.Get_Saturation(), Previous.Get_Saturation());
    Set_Scalar(TEXT("MinimumAlbedo"), _Settings.Get_MinimumAlbedo(), Previous.Get_MinimumAlbedo());
    Set_Scalar(TEXT("AffectUnlit"),
        Get_Flag(_Settings.Get_AffectUnlit()), Get_Flag(Previous.Get_AffectUnlit()));

    Set_Scalar(TEXT("EnableSky"),
        Get_Flag(_Settings.Get_EnableSky()), Get_Flag(Previous.Get_EnableSky()));
    Set_Scalar(TEXT("SkyDistance"), _Settings.Get_SkyDistance(), Previous.Get_SkyDistance());
    Set_Scalar(TEXT("SkyBands"), Get_Count(_Settings.Get_SkyBands()), Get_Count(Previous.Get_SkyBands()));
    Set_Scalar(TEXT("SkyStrength"), _Settings.Get_SkyStrength(), Previous.Get_SkyStrength());
    Set_Scalar(TEXT("SkyPattern"),
        Get_EnumIndex(_Settings.Get_SkyPattern()), Get_EnumIndex(Previous.Get_SkyPattern()));
    Set_Scalar(TEXT("SkyPatternStrength"), _Settings.Get_SkyPatternStrength(), Previous.Get_SkyPatternStrength());
    Set_Scalar(TEXT("SkyPatternScale"), _Settings.Get_SkyPatternScale(), Previous.Get_SkyPatternScale());

    Set_Scalar(TEXT("MetallicThreshold"), _Settings.Get_MetallicThreshold(), Previous.Get_MetallicThreshold());
    Set_Scalar(TEXT("MetallicBands"),
        Get_Count(_Settings.Get_MetallicBands()), Get_Count(Previous.Get_MetallicBands()));
    Set_Scalar(TEXT("MetallicStrength"), _Settings.Get_MetallicStrength(), Previous.Get_MetallicStrength());
    Set_Scalar(TEXT("MetallicPatternStrength"),
        _Settings.Get_MetallicPatternStrength(), Previous.Get_MetallicPatternStrength());

    Set_Scalar(TEXT("EnableSpecular"),
        Get_Flag(_Settings.Get_EnableSpecular()), Get_Flag(Previous.Get_EnableSpecular()));
    Set_Scalar(TEXT("SpecularSteps"),
        Get_Count(_Settings.Get_SpecularSteps()), Get_Count(Previous.Get_SpecularSteps()));
    Set_Scalar(TEXT("SpecularThreshold"), _Settings.Get_SpecularThreshold(), Previous.Get_SpecularThreshold());
    Set_Scalar(TEXT("SpecularIntensity"), _Settings.Get_SpecularIntensity(), Previous.Get_SpecularIntensity());
    Set_Scalar(TEXT("SpecularRoughnessCutoff"),
        _Settings.Get_SpecularRoughnessCutoff(), Previous.Get_SpecularRoughnessCutoff());

    Set_Scalar(TEXT("EnableRimLight"),
        Get_Flag(_Settings.Get_EnableRimLight()), Get_Flag(Previous.Get_EnableRimLight()));
    Set_Scalar(TEXT("RimPower"), _Settings.Get_RimPower(), Previous.Get_RimPower());
    Set_Scalar(TEXT("RimThreshold"), _Settings.Get_RimThreshold(), Previous.Get_RimThreshold());
    Set_Scalar(TEXT("RimSoftness"), _Settings.Get_RimSoftness(), Previous.Get_RimSoftness());
    Set_Scalar(TEXT("RimIntensity"), _Settings.Get_RimIntensity(), Previous.Get_RimIntensity());
    Set_Scalar(TEXT("RimFollowsLighting"), _Settings.Get_RimFollowsLighting(), Previous.Get_RimFollowsLighting());

    Set_Scalar(TEXT("EnableOutline"),
        Get_Flag(_Settings.Get_EnableOutline()), Get_Flag(Previous.Get_EnableOutline()));
    Set_Scalar(TEXT("OutlineThickness"), _Settings.Get_OutlineThickness(), Previous.Get_OutlineThickness());
    Set_Scalar(TEXT("OutlineQuality"),
        Get_EnumIndex(_Settings.Get_OutlineQuality()), Get_EnumIndex(Previous.Get_OutlineQuality()));
    Set_Scalar(TEXT("OutlineOpacity"), _Settings.Get_OutlineOpacity(), Previous.Get_OutlineOpacity());
    Set_Scalar(TEXT("OutlineBlendMode"),
        Get_EnumIndex(_Settings.Get_OutlineBlendMode()), Get_EnumIndex(Previous.Get_OutlineBlendMode()));
    Set_Scalar(TEXT("OutlineDepthThreshold"),
        _Settings.Get_OutlineDepthThreshold(), Previous.Get_OutlineDepthThreshold());
    Set_Scalar(TEXT("OutlineNormalThreshold"),
        _Settings.Get_OutlineNormalThreshold(), Previous.Get_OutlineNormalThreshold());
    Set_Scalar(TEXT("OutlineAlbedoThreshold"),
        _Settings.Get_OutlineAlbedoThreshold(), Previous.Get_OutlineAlbedoThreshold());
    Set_Scalar(TEXT("OutlineDistanceFade"),
        _Settings.Get_OutlineDistanceFade(), Previous.Get_OutlineDistanceFade());

    Set_Scalar(TEXT("EnableStencilPatterns"),
        Get_Flag(_Settings.Get_EnableStencilPatterns()), Get_Flag(Previous.Get_EnableStencilPatterns()));
    Set_Scalar(TEXT("StencilBase"),
        Get_Count(_Settings.Get_StencilBase()), Get_Count(Previous.Get_StencilBase()));
    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(_Settings.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Vector(TEXT("ShadowTint"), _Settings.Get_ShadowTint(), Previous.Get_ShadowTint());
    Set_Vector(TEXT("LightTint"), _Settings.Get_LightTint(), Previous.Get_LightTint());
    Set_Vector(TEXT("RimColor"), _Settings.Get_RimColor(), Previous.Get_RimColor());
    Set_Vector(TEXT("OutlineColor"), _Settings.Get_OutlineColor(), Previous.Get_OutlineColor());

    _WrittenSettings = _Settings;
}

// --------------------------------------------------------------------------------------------------------------------
