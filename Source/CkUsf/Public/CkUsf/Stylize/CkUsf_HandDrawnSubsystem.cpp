#include "CkUsf/Stylize/CkUsf_HandDrawnSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Stylize/CkUsf_HandDrawnPreset.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Utils.h"
#include "CkUsf/Stylize/CkUsf_Stylize_CVars.h"
#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"
#include "CkUsf_Log.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CoreDelegates.h"

// --------------------------------------------------------------------------------------------------------------------

const FName UCkUsf_HandDrawnSubsystem::kLookName = TEXT("HandDrawn");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_HandDrawnSubsystem::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    // A dedicated server renders nothing, so a configured default preset must not make it load a master,
    // build a MID and spawn a post-process actor per world. Process-level rather than per-world on
    // purpose: at this point the world has no NetDriver, so its net mode would only re-derive the same
    // process answer. Known gap, matching the UCk_LoadingScreen_Subsystem_UE precedent's granularity: a
    // PIE dedicated-server world lives in the editor process and still gets one.
    if (IsRunningDedicatedServer())
    { return false; }

    return Super::ShouldCreateSubsystem(InOuter);
}

auto
    UCkUsf_HandDrawnSubsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _CVarChangedHandle = ck::usf::stylize::Get_OnCVarChanged().AddUObject(
        this, &UCkUsf_HandDrawnSubsystem::DoOn_CVarChanged);
}

auto
    UCkUsf_HandDrawnSubsystem::
    Deinitialize()
    -> void
{
    ck::usf::stylize::Get_OnCVarChanged().Remove(_CVarChangedHandle);
    _CVarChangedHandle.Reset();

    Super::Deinitialize();
}

auto
    UCkUsf_HandDrawnSubsystem::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoApply_ProjectDefault();
}

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
    _SettingsExplicitlySet = true;

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

    const auto& Mask = InSettings.Get_Mask();

    const auto MaskRangeIsAddressable = UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeIsAddressable(Mask);

    CK_ENSURE_IF_NOT(MaskRangeIsAddressable,
        TEXT("Request_SetSettings: HandDrawn effect-mask range [{}, {}] is inverted or reaches the "
             "engine's NO-STENCIL value 0; it would match every untagged pixel in the view or none at "
             "all. Settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    {}

    if (NOT MaskRangeIsAddressable)
    { return; }

    const auto MaskRangeAvoidsOutline =
        UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeAvoidsOutline(GetWorld(), Mask);

    CK_ENSURE_IF_NOT(MaskRangeAvoidsOutline,
        TEXT("Request_SetSettings: HandDrawn effect-mask range [{}, {}] COLLIDES with the outline "
             "subsystem's allocated range; settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    {}

    if (NOT MaskRangeAvoidsOutline)
    { return; }

    const auto MaskRangeAvoidsCelPatterns =
        UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeAvoidsCelPatterns(GetWorld(), Mask);

    CK_ENSURE_IF_NOT(MaskRangeAvoidsCelPatterns,
        TEXT("Request_SetSettings: HandDrawn effect-mask range [{}, {}] COLLIDES with the CelShade "
             "per-object pattern span in this world; one stencil value cannot both select a cel pattern "
             "and gate this look. Settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    {}

    if (NOT MaskRangeAvoidsCelPatterns)
    { return; }

    _SettingsExplicitlySet = true;

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
    if (auto* DefaultPreset = DoResolve_ProjectDefaultPreset())
    {
        Apply_Preset(DefaultPreset);
        return;
    }

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

    const auto Effective = DoGet_EffectiveSettings();

    // The component is owned by a world-spawned actor, so it can be torn down under us while the MID
    // (outered to this subsystem) survives and keeps DoEnsure_ViewEffect returning true.
    if (ck::IsValid(_ViewPP))
    { _ViewPP->bEnabled = Effective.Get_Enabled() == ECk_EnableDisable::Enable; }

    DoWrite_ChangedParams(Effective);
}

auto
    UCkUsf_HandDrawnSubsystem::
    DoGet_EffectiveSettings() const
    -> FCk_Usf_HandDrawn_Params
{
    auto Effective = _Settings;

    const auto EnabledOverride = ck::usf::stylize::Get_EnabledOverride_HandDrawn();
    if (EnabledOverride >= 0)
    {
        Effective.Set_Enabled(EnabledOverride > 0 ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable);
    }

    const auto DebugOverride = ck::usf::stylize::Get_DebugOverride_HandDrawn();
    if (DebugOverride >= 0)
    {
        // UHT appends a _MAX enumerator to every UENUM and IsValidEnumValue accepts it, so
        // one-past-the-end would pass here and then fall through the shader's if-chain to the
        // final image — a debug mode that silently shows no debug view.
        const auto* DebugEnum = StaticEnum<ECk_Usf_HandDrawn_DebugMode>();
        const auto DebugOverrideIsValid =
            DebugEnum->IsValidEnumValue(DebugOverride) && DebugOverride != DebugEnum->GetMaxEnumValue();

        CK_ENSURE_IF_NOT(DebugOverrideIsValid,
            TEXT("ck.Usf.HandDrawn.Debug is [{}], which is not an ECk_Usf_HandDrawn_DebugMode value; "
                 "the setting's own debug mode is used instead"), DebugOverride)
        {}

        if (DebugOverrideIsValid)
        { Effective.Set_DebugMode(static_cast<ECk_Usf_HandDrawn_DebugMode>(DebugOverride)); }
    }

    return Effective;
}

auto
    UCkUsf_HandDrawnSubsystem::
    DoOn_CVarChanged()
    -> void
{
    // A debug CVar must never be what instantiates the effect: every world has one of these subsystems,
    // and the settings default to Enabled, so an unconditional re-sync would switch HandDrawn on in every
    // world that merely exists the moment anyone touches a console value. Worlds already carrying the
    // effect re-sync, and an explicit force-on is allowed to create it.
    if (_HandDrawnMID == nullptr && ck::usf::stylize::Get_EnabledOverride_HandDrawn() != 1)
    { return; }

    DoSync_ViewEffect();
}

auto
    UCkUsf_HandDrawnSubsystem::
    DoResolve_ProjectDefaultPreset() const
    -> UCkUsf_HandDrawnPreset*
{
    const auto SoftPreset = UCk_Utils_Usf_Stylize_Settings_UE::Get_HandDrawnDefaultPreset();

    // Unset is the "no default style" answer, not a failure — the effect simply stays off.
    if (SoftPreset.IsNull())
    { return nullptr; }

    auto* Preset = SoftPreset.LoadSynchronous();

    CK_ENSURE_IF_NOT(ck::IsValid(Preset, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Project settings name [{}] as the default HandDrawn preset, but it could not be loaded"),
        SoftPreset.ToString())
    {}

    return Preset;
}

auto
    UCkUsf_HandDrawnSubsystem::
    DoApply_ProjectDefault()
    -> void
{
    // A packaged game runs its startup map's BeginPlay from inside FEngineLoop::Init, before
    // OnFEngineLoopInitComplete flips the blocking-load flag — resolving the soft ref there is the
    // premature-load class CkCore documents. The retry deliberately does NOT re-test the flag: it is
    // already ON that delegate, and CkCore's own flag-setting registrar may run after us in add-order.
    if (UCk_Utils_IO_UE::Get_IsEngineSafeForBlockingLoads_Peek())
    {
        DoApply_ProjectDefault_Now();
        return;
    }

    FCoreDelegates::OnFEngineLoopInitComplete.AddWeakLambda(this, [this]()
    {
        DoApply_ProjectDefault_Now();
    });
}

auto
    UCkUsf_HandDrawnSubsystem::
    DoApply_ProjectDefault_Now()
    -> void
{
    // A default is a DEFAULT: game code that has already spoken wins. This only bites on the
    // deferred path — a packaged game runs its startup map's BeginPlay inside FEngineLoop::Init,
    // so gameplay can legitimately have set settings before the delegate that carries us here
    // fires, and applying the project row then would silently undo it.
    if (_SettingsExplicitlySet)
    { return; }

    auto* DefaultPreset = DoResolve_ProjectDefaultPreset();
    if (DefaultPreset == nullptr)
    { return; }

    Apply_Preset(DefaultPreset);
}

auto
    UCkUsf_HandDrawnSubsystem::
    DoWrite_ChangedParams(
        const FCk_Usf_HandDrawn_Params& InEffective)
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? InEffective : _WrittenSettings.GetValue();

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

    Set_Scalar(TEXT("StyleStrength"), InEffective.Get_StyleStrength(), Previous.Get_StyleStrength());

    Set_Scalar(TEXT("SimplifyColor"),
        Get_Flag(InEffective.Get_SimplifyColor()), Get_Flag(Previous.Get_SimplifyColor()));
    Set_Scalar(TEXT("ColorLevels"),
        static_cast<float>(InEffective.Get_ColorLevels()), static_cast<float>(Previous.Get_ColorLevels()));
    Set_Scalar(TEXT("ColorSoftness"), InEffective.Get_ColorSoftness(), Previous.Get_ColorSoftness());
    Set_Scalar(TEXT("Saturation"), InEffective.Get_Saturation(), Previous.Get_Saturation());
    Set_Scalar(TEXT("Contrast"), InEffective.Get_Contrast(), Previous.Get_Contrast());
    Set_Scalar(TEXT("TintStrength"), InEffective.Get_TintStrength(), Previous.Get_TintStrength());
    Set_Scalar(TEXT("AffectSky"),
        Get_Flag(InEffective.Get_AffectSky()), Get_Flag(Previous.Get_AffectSky()));
    Set_Scalar(TEXT("SkyDistance"), InEffective.Get_SkyDistance(), Previous.Get_SkyDistance());

    Set_Scalar(TEXT("EnableInk"),
        Get_Flag(InEffective.Get_EnableInk()), Get_Flag(Previous.Get_EnableInk()));
    Set_Scalar(TEXT("InkThickness"), InEffective.Get_InkThickness(), Previous.Get_InkThickness());
    Set_Scalar(TEXT("InkOpacity"), InEffective.Get_InkOpacity(), Previous.Get_InkOpacity());
    Set_Scalar(TEXT("DepthThreshold"), InEffective.Get_DepthThreshold(), Previous.Get_DepthThreshold());
    Set_Scalar(TEXT("NormalThreshold"), InEffective.Get_NormalThreshold(), Previous.Get_NormalThreshold());
    Set_Scalar(TEXT("ColorEdgeThreshold"),
        InEffective.Get_ColorEdgeThreshold(), Previous.Get_ColorEdgeThreshold());
    Set_Scalar(TEXT("LineVariation"), InEffective.Get_LineVariation(), Previous.Get_LineVariation());
    Set_Scalar(TEXT("LineScale"), InEffective.Get_LineScale(), Previous.Get_LineScale());
    Set_Scalar(TEXT("InkFadeStartDistance"),
        InEffective.Get_InkFadeStartDistance(), Previous.Get_InkFadeStartDistance());
    Set_Scalar(TEXT("InkFadeEndDistance"),
        InEffective.Get_InkFadeEndDistance(), Previous.Get_InkFadeEndDistance());

    Set_Scalar(TEXT("EnableShadowStrokes"),
        Get_Flag(InEffective.Get_EnableShadowStrokes()), Get_Flag(Previous.Get_EnableShadowStrokes()));
    Set_Scalar(TEXT("StrokePattern"),
        Get_EnumIndex(InEffective.Get_StrokePattern()), Get_EnumIndex(Previous.Get_StrokePattern()));
    Set_Scalar(TEXT("StrokeSpace"),
        Get_EnumIndex(InEffective.Get_StrokeSpace()), Get_EnumIndex(Previous.Get_StrokeSpace()));
    Set_Scalar(TEXT("StrokeStrength"), InEffective.Get_StrokeStrength(), Previous.Get_StrokeStrength());
    Set_Scalar(TEXT("StrokeShadowThreshold"),
        InEffective.Get_StrokeShadowThreshold(), Previous.Get_StrokeShadowThreshold());
    Set_Scalar(TEXT("StrokePixelSize"), InEffective.Get_StrokePixelSize(), Previous.Get_StrokePixelSize());
    Set_Scalar(TEXT("StrokeWorldSize"), InEffective.Get_StrokeWorldSize(), Previous.Get_StrokeWorldSize());
    Set_Scalar(TEXT("StrokeIrregularity"),
        InEffective.Get_StrokeIrregularity(), Previous.Get_StrokeIrregularity());
    Set_Scalar(TEXT("StrokeTriplanarSharpness"),
        InEffective.Get_StrokeTriplanarSharpness(), Previous.Get_StrokeTriplanarSharpness());

    Set_Scalar(TEXT("EnablePaper"),
        Get_Flag(InEffective.Get_EnablePaper()), Get_Flag(Previous.Get_EnablePaper()));
    Set_Scalar(TEXT("GrainStrength"), InEffective.Get_GrainStrength(), Previous.Get_GrainStrength());
    Set_Scalar(TEXT("GrainScale"), InEffective.Get_GrainScale(), Previous.Get_GrainScale());
    Set_Scalar(TEXT("FiberStrength"), InEffective.Get_FiberStrength(), Previous.Get_FiberStrength());
    Set_Scalar(TEXT("PaperWarmth"), InEffective.Get_PaperWarmth(), Previous.Get_PaperWarmth());

    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(InEffective.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Vector(TEXT("ShadowTint"), InEffective.Get_ShadowTint(), Previous.Get_ShadowTint());
    Set_Vector(TEXT("HighlightTint"), InEffective.Get_HighlightTint(), Previous.Get_HighlightTint());
    Set_Vector(TEXT("InkColor"), InEffective.Get_InkColor(), Previous.Get_InkColor());

    const auto Get_MaskBound = [](int32 InValue) -> float
    {
        return static_cast<float>(InValue);
    };

    Set_Scalar(TEXT("MaskMode"),
        Get_EnumIndex(InEffective.Get_Mask().Get_Mode()), Get_EnumIndex(Previous.Get_Mask().Get_Mode()));
    Set_Scalar(TEXT("MaskStencilMin"),
        Get_MaskBound(InEffective.Get_Mask().Get_StencilMin()),
        Get_MaskBound(Previous.Get_Mask().Get_StencilMin()));
    Set_Scalar(TEXT("MaskStencilMax"),
        Get_MaskBound(InEffective.Get_Mask().Get_StencilMax()),
        Get_MaskBound(Previous.Get_Mask().Get_StencilMax()));

    _WrittenSettings = InEffective;
}

// --------------------------------------------------------------------------------------------------------------------
