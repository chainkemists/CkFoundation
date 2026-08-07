#include "CkUsf/Stylize/CkUsf_HandDrawnSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Stylize/CkUsf_HandDrawnPreset.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

const FName UCkUsf_HandDrawnSubsystem::kLookName = TEXT("HandDrawn");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_HandDrawnSubsystem::
    Get_HandDrawnSubsystem(
        const UObject* InWorldContextObject)
    -> UCkUsf_HandDrawnSubsystem*
{
    if (ck::Is_NOT_Valid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCkUsf_HandDrawnSubsystem>();
}

auto
    UCkUsf_HandDrawnSubsystem::
    Request_SetEnabled(
        ECk_EnableDisable InEnabled)
    -> void
{
    _Settings.Set_Enabled(InEnabled);
    DoSync_ViewEffect();
}

auto
    UCkUsf_HandDrawnSubsystem::
    Get_IsEnabled() const
    -> ECk_EnableDisable
{
    return _Settings.Get_Enabled();
}

auto
    UCkUsf_HandDrawnSubsystem::
    Apply_Preset(
        UCkUsf_HandDrawnPreset* InPreset)
    -> void
{
    const auto PresetIsValid = ck::IsValid(InPreset, ck::IsValid_Policy_NullptrOnly{});

    CK_ENSURE_IF_NOT(PresetIsValid,
        TEXT("Apply_Preset: null HandDrawn preset; settings left untouched"))
    {}

    if (NOT PresetIsValid)
    { return; }

    Request_SetSettings(InPreset->Get_AsParams());
}

auto
    UCkUsf_HandDrawnSubsystem::
    Request_SetSettings(
        const FCk_Usf_HandDrawn_Params& InSettings)
    -> void
{
    // An inverted ink fade range collapses the fade to a HARD CUTOFF at the end distance: the shader's
    // ramp divides by max(End - Start, epsilon), so a non-positive width leaves the line at full weight
    // right up to the end and gone immediately after. The setting silently stops being a fade, and
    // nothing in the frame names the cause — accepting it is worse than refusing it.
    const auto InkFadeRangeIsOrdered = InSettings.Get_InkFadeRangeIsOrdered();

    CK_ENSURE_IF_NOT(InkFadeRangeIsOrdered,
        TEXT("Request_SetSettings: HandDrawn ink fade range is INVERTED (start [{}] is not before end "
             "[{}]), which would render as a hard cutoff at the end distance rather than a fade; "
             "settings left untouched. Set the end distance to 0 to disable the fade."),
        InSettings.Get_InkFadeStartDistance(), InSettings.Get_InkFadeEndDistance())
    {}

    if (NOT InkFadeRangeIsOrdered)
    { return; }

    _Settings = InSettings;
    DoSync_ViewEffect();
}

auto
    UCkUsf_HandDrawnSubsystem::
    Get_Settings() const
    -> FCk_Usf_HandDrawn_Params
{
    return _Settings;
}

auto
    UCkUsf_HandDrawnSubsystem::
    Request_ResetToDefaults()
    -> void
{
    Request_SetSettings(FCk_Usf_HandDrawn_Params{});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_HandDrawnSubsystem::
    DoEnsure_ViewEffect()
    -> bool
{
    if (_HandDrawnMID != nullptr)
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
            ck::usf::Warning(TEXT("HandDrawn master not found at [{}] — run Generate Look Materials. "
                                  "Settings are still tracked; nothing renders until the master exists."),
                MasterPath);
        }
        return false;
    }

    _HandDrawnMID = UMaterialInstanceDynamic::Create(Master, this);

    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.Name = TEXT("CkUsf_HandDrawnView");
    _ViewActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
    if (_ViewActor == nullptr)
    {
        _HandDrawnMID = nullptr;
        return false;
    }

    _ViewPP = NewObject<UPostProcessComponent>(_ViewActor, TEXT("HandDrawnPostProcess"));
    _ViewActor->SetRootComponent(_ViewPP);
    _ViewPP->bUnbound = true;
    _ViewPP->RegisterComponent();
    _ViewPP->Settings.AddBlendable(_HandDrawnMID, 1.0f);

    return true;
}

auto
    UCkUsf_HandDrawnSubsystem::
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
    UCkUsf_HandDrawnSubsystem::
    DoWrite_ChangedParams()
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? _Settings : _WrittenSettings.GetValue();

    const auto Set_Scalar = [&](const TCHAR* InName, float InValue, float InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _HandDrawnMID->SetScalarParameterValue(InName, InValue); }
    };

    const auto Set_Vector = [&](const TCHAR* InName, const FLinearColor& InValue, const FLinearColor& InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _HandDrawnMID->SetVectorParameterValue(InName, InValue); }
    };

    const auto Get_EnumIndex = [](auto InEnumValue) -> float
    {
        return static_cast<float>(static_cast<uint8>(InEnumValue));
    };

    const auto Get_Flag = [](ECk_EnableDisable InEnableDisable) -> float
    {
        return InEnableDisable == ECk_EnableDisable::Enable ? 1.0f : 0.0f;
    };

    Set_Scalar(TEXT("StyleStrength"), _Settings.Get_StyleStrength(), Previous.Get_StyleStrength());

    Set_Scalar(TEXT("SimplifyColor"),
        Get_Flag(_Settings.Get_SimplifyColor()), Get_Flag(Previous.Get_SimplifyColor()));
    Set_Scalar(TEXT("ColorLevels"),
        static_cast<float>(_Settings.Get_ColorLevels()), static_cast<float>(Previous.Get_ColorLevels()));
    Set_Scalar(TEXT("ColorSoftness"), _Settings.Get_ColorSoftness(), Previous.Get_ColorSoftness());
    Set_Scalar(TEXT("Saturation"), _Settings.Get_Saturation(), Previous.Get_Saturation());
    Set_Scalar(TEXT("Contrast"), _Settings.Get_Contrast(), Previous.Get_Contrast());
    Set_Scalar(TEXT("TintStrength"), _Settings.Get_TintStrength(), Previous.Get_TintStrength());
    Set_Scalar(TEXT("AffectSky"),
        Get_Flag(_Settings.Get_AffectSky()), Get_Flag(Previous.Get_AffectSky()));
    Set_Scalar(TEXT("SkyDistance"), _Settings.Get_SkyDistance(), Previous.Get_SkyDistance());

    Set_Scalar(TEXT("EnableInk"),
        Get_Flag(_Settings.Get_EnableInk()), Get_Flag(Previous.Get_EnableInk()));
    Set_Scalar(TEXT("InkThickness"), _Settings.Get_InkThickness(), Previous.Get_InkThickness());
    Set_Scalar(TEXT("InkOpacity"), _Settings.Get_InkOpacity(), Previous.Get_InkOpacity());
    Set_Scalar(TEXT("DepthThreshold"), _Settings.Get_DepthThreshold(), Previous.Get_DepthThreshold());
    Set_Scalar(TEXT("NormalThreshold"), _Settings.Get_NormalThreshold(), Previous.Get_NormalThreshold());
    Set_Scalar(TEXT("ColorEdgeThreshold"),
        _Settings.Get_ColorEdgeThreshold(), Previous.Get_ColorEdgeThreshold());
    Set_Scalar(TEXT("LineVariation"), _Settings.Get_LineVariation(), Previous.Get_LineVariation());
    Set_Scalar(TEXT("LineScale"), _Settings.Get_LineScale(), Previous.Get_LineScale());
    Set_Scalar(TEXT("InkFadeStartDistance"),
        _Settings.Get_InkFadeStartDistance(), Previous.Get_InkFadeStartDistance());
    Set_Scalar(TEXT("InkFadeEndDistance"),
        _Settings.Get_InkFadeEndDistance(), Previous.Get_InkFadeEndDistance());

    Set_Scalar(TEXT("EnableShadowStrokes"),
        Get_Flag(_Settings.Get_EnableShadowStrokes()), Get_Flag(Previous.Get_EnableShadowStrokes()));
    Set_Scalar(TEXT("StrokePattern"),
        Get_EnumIndex(_Settings.Get_StrokePattern()), Get_EnumIndex(Previous.Get_StrokePattern()));
    Set_Scalar(TEXT("StrokeSpace"),
        Get_EnumIndex(_Settings.Get_StrokeSpace()), Get_EnumIndex(Previous.Get_StrokeSpace()));
    Set_Scalar(TEXT("StrokeStrength"), _Settings.Get_StrokeStrength(), Previous.Get_StrokeStrength());
    Set_Scalar(TEXT("StrokeShadowThreshold"),
        _Settings.Get_StrokeShadowThreshold(), Previous.Get_StrokeShadowThreshold());
    Set_Scalar(TEXT("StrokePixelSize"), _Settings.Get_StrokePixelSize(), Previous.Get_StrokePixelSize());
    Set_Scalar(TEXT("StrokeWorldSize"), _Settings.Get_StrokeWorldSize(), Previous.Get_StrokeWorldSize());
    Set_Scalar(TEXT("StrokeIrregularity"),
        _Settings.Get_StrokeIrregularity(), Previous.Get_StrokeIrregularity());
    Set_Scalar(TEXT("StrokeTriplanarSharpness"),
        _Settings.Get_StrokeTriplanarSharpness(), Previous.Get_StrokeTriplanarSharpness());

    Set_Scalar(TEXT("EnablePaper"),
        Get_Flag(_Settings.Get_EnablePaper()), Get_Flag(Previous.Get_EnablePaper()));
    Set_Scalar(TEXT("GrainStrength"), _Settings.Get_GrainStrength(), Previous.Get_GrainStrength());
    Set_Scalar(TEXT("GrainScale"), _Settings.Get_GrainScale(), Previous.Get_GrainScale());
    Set_Scalar(TEXT("FiberStrength"), _Settings.Get_FiberStrength(), Previous.Get_FiberStrength());
    Set_Scalar(TEXT("PaperWarmth"), _Settings.Get_PaperWarmth(), Previous.Get_PaperWarmth());

    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(_Settings.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Vector(TEXT("ShadowTint"), _Settings.Get_ShadowTint(), Previous.Get_ShadowTint());
    Set_Vector(TEXT("HighlightTint"), _Settings.Get_HighlightTint(), Previous.Get_HighlightTint());
    Set_Vector(TEXT("InkColor"), _Settings.Get_InkColor(), Previous.Get_InkColor());

    _WrittenSettings = _Settings;
}

// --------------------------------------------------------------------------------------------------------------------
