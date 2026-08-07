#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"
#include "CkUsf/Stylize/CkUsf_CelShadePreset.h"
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

const FName UCkUsf_CelShadeSubsystem::kLookName = TEXT("CelShade");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CelShadeSubsystem::
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
    UCkUsf_CelShadeSubsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _CVarChangedHandle = ck::usf::stylize::Get_OnCVarChanged().AddUObject(
        this, &UCkUsf_CelShadeSubsystem::DoOn_CVarChanged);
}

auto
    UCkUsf_CelShadeSubsystem::
    Deinitialize()
    -> void
{
    ck::usf::stylize::Get_OnCVarChanged().Remove(_CVarChangedHandle);
    _CVarChangedHandle.Reset();

    Super::Deinitialize();
}

auto
    UCkUsf_CelShadeSubsystem::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoApply_ProjectDefault();
}

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
    _SettingsExplicitlySet = true;

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

    const auto BandEdgesAreValid = Get_BandEdgesAreValid(InSettings);

    CK_ENSURE_IF_NOT(BandEdgesAreValid,
        TEXT("Request_SetSettings: CelShade DistributionMode is CustomEdges with an EMPTY, NON-ASCENDING "
             "or out-of-range band-edge list [{}]; the bands are the intervals between the edges, so a "
             "list like that collapses one to zero width or hides it entirely. Settings left untouched"),
        FString::JoinBy(InSettings.Get_EffectiveBandEdges(), TEXT(", "),
            [](float InEdge) -> FString { return FString::SanitizeFloat(InEdge); }))
    {}

    if (NOT BandEdgesAreValid)
    { return; }

    const auto& Mask = InSettings.Get_Mask();

    const auto MaskRangeIsAddressable = UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeIsAddressable(Mask);

    CK_ENSURE_IF_NOT(MaskRangeIsAddressable,
        TEXT("Request_SetSettings: CelShade effect-mask range [{}, {}] is inverted or reaches the engine's "
             "NO-STENCIL value 0; it would match every untagged pixel in the view or none at all. "
             "Settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    {}

    if (NOT MaskRangeIsAddressable)
    { return; }

    const auto MaskRangeAvoidsOutline =
        UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeAvoidsOutline(GetWorld(), Mask);

    CK_ENSURE_IF_NOT(MaskRangeAvoidsOutline,
        TEXT("Request_SetSettings: CelShade effect-mask range [{}, {}] COLLIDES with the outline "
             "subsystem's allocated range; settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    {}

    if (NOT MaskRangeAvoidsOutline)
    { return; }

    // Against the INCOMING pattern span, not the stored one: both live in this same settings value, so a
    // caller can move the base and the mask together and only their final relationship matters.
    const auto MaskRangeAvoidsCelSpan =
        UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeAvoidsCelSpan(Mask, InSettings);

    CK_ENSURE_IF_NOT(MaskRangeAvoidsCelSpan,
        TEXT("Request_SetSettings: CelShade effect-mask range [{}, {}] COLLIDES with its own per-object "
             "pattern span [{}, {}]; one stencil value cannot both select a pattern and gate the look. "
             "Settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax(),
        InSettings.Get_StencilRangeMin(), InSettings.Get_StencilRangeMax())
    {}

    if (NOT MaskRangeAvoidsCelSpan)
    { return; }

    // The same rule the other way round. A sibling's mask was validated against the cel span that existed
    // when the mask was set; moving StencilBase afterwards would walk this span onto that mask with
    // nothing having rejected anything.
    const auto CelSpanAvoidsSiblingMasks =
        UCk_Utils_Usf_StylizeMask_UE::Get_CelSpanAvoidsSiblingMasks(GetWorld(), InSettings);

    CK_ENSURE_IF_NOT(CelSpanAvoidsSiblingMasks,
        TEXT("Request_SetSettings: CelShade per-object pattern span [{}, {}] COLLIDES with an effect-mask "
             "range another stylize effect in this world already holds; one stencil value cannot both "
             "select a cel pattern and gate that look. Settings left untouched"),
        InSettings.Get_StencilRangeMin(), InSettings.Get_StencilRangeMax())
    {}

    if (NOT CelSpanAvoidsSiblingMasks)
    { return; }

    _SettingsExplicitlySet = true;

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
    if (auto* DefaultPreset = DoResolve_ProjectDefaultPreset())
    {
        Apply_Preset(DefaultPreset);
        return;
    }

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
    Get_BandEdgesAreValid(
        const FCk_Usf_CelShade_Params& InSettings)
    -> bool
{
    if (InSettings.Get_DistributionMode() != ECk_Usf_CelDistribution::CustomEdges)
    { return true; }

    const auto Edges = InSettings.Get_EffectiveBandEdges();

    // No edges means no boundaries, which is one flat band — the artist asked for custom placement and
    // would get the look switched off with nothing saying so.
    if (Edges.IsEmpty())
    { return false; }

    // Strictly ascending AND strictly inside 0..1: the shader counts edges at or below the ramp to get a
    // band ordinal, which is only monotone on an ascending list; and an edge sitting exactly on 0 or 1
    // fences off a zero-width band that no pixel can ever land in.
    auto Previous = 0.0f;
    for (const auto Edge : Edges)
    {
        if (Edge <= Previous || Edge >= 1.0f)
        { return false; }

        Previous = Edge;
    }

    return true;
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

    const auto Effective = DoGet_EffectiveSettings();

    // The component is owned by a world-spawned actor, so it can be torn down under us while the MID
    // (outered to this subsystem) survives and keeps DoEnsure_ViewEffect returning true.
    if (ck::IsValid(_ViewPP))
    { _ViewPP->bEnabled = Effective.Get_Enabled() == ECk_EnableDisable::Enable; }

    DoWrite_ChangedParams(Effective);
}

auto
    UCkUsf_CelShadeSubsystem::
    DoGet_EffectiveSettings() const
    -> FCk_Usf_CelShade_Params
{
    auto Effective = _Settings;

    const auto EnabledOverride = ck::usf::stylize::Get_EnabledOverride_CelShade();
    if (EnabledOverride >= 0)
    {
        Effective.Set_Enabled(EnabledOverride > 0 ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable);
    }

    const auto DebugOverride = ck::usf::stylize::Get_DebugOverride_CelShade();
    if (DebugOverride >= 0)
    {
        // UHT appends a _MAX enumerator to every UENUM and IsValidEnumValue accepts it, so
        // one-past-the-end would pass here and then fall through the shader's if-chain to the
        // final image — a debug mode that silently shows no debug view.
        const auto* DebugEnum = StaticEnum<ECk_Usf_CelShade_DebugMode>();
        const auto DebugOverrideIsValid =
            DebugEnum->IsValidEnumValue(DebugOverride) && DebugOverride != DebugEnum->GetMaxEnumValue();

        CK_ENSURE_IF_NOT(DebugOverrideIsValid,
            TEXT("ck.Usf.CelShade.Debug is [{}], which is not an ECk_Usf_CelShade_DebugMode value; "
                 "the setting's own debug mode is used instead"), DebugOverride)
        {}

        if (DebugOverrideIsValid)
        { Effective.Set_DebugMode(static_cast<ECk_Usf_CelShade_DebugMode>(DebugOverride)); }
    }

    return Effective;
}

auto
    UCkUsf_CelShadeSubsystem::
    DoOn_CVarChanged()
    -> void
{
    // A debug CVar must never be what instantiates the effect: every world has one of these subsystems,
    // and the settings default to Enabled, so an unconditional re-sync would switch CelShade on in every
    // world that merely exists the moment anyone touches a console value. Worlds already carrying the
    // effect re-sync, and an explicit force-on is allowed to create it.
    if (_CelMID == nullptr && ck::usf::stylize::Get_EnabledOverride_CelShade() != 1)
    { return; }

    DoSync_ViewEffect();
}

auto
    UCkUsf_CelShadeSubsystem::
    DoResolve_ProjectDefaultPreset() const
    -> UCkUsf_CelShadePreset*
{
    const auto SoftPreset = UCk_Utils_Usf_Stylize_Settings_UE::Get_CelShadeDefaultPreset();

    // Unset is the "no default style" answer, not a failure — the effect simply stays off.
    if (SoftPreset.IsNull())
    { return nullptr; }

    auto* Preset = SoftPreset.LoadSynchronous();

    CK_ENSURE_IF_NOT(ck::IsValid(Preset, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Project settings name [{}] as the default CelShade preset, but it could not be loaded"),
        SoftPreset.ToString())
    {}

    return Preset;
}

auto
    UCkUsf_CelShadeSubsystem::
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
    UCkUsf_CelShadeSubsystem::
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
    UCkUsf_CelShadeSubsystem::
    DoWrite_ChangedParams(
        const FCk_Usf_CelShade_Params& InEffective)
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? InEffective : _WrittenSettings.GetValue();

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

    Set_Scalar(TEXT("Bands"), Get_Count(InEffective.Get_Bands()), Get_Count(Previous.Get_Bands()));
    Set_Scalar(TEXT("Midpoint"), InEffective.Get_Midpoint(), Previous.Get_Midpoint());
    Set_Scalar(TEXT("BandOffset"), InEffective.Get_BandOffset(), Previous.Get_BandOffset());
    Set_Scalar(TEXT("Distribution"), InEffective.Get_Distribution(), Previous.Get_Distribution());
    Set_Scalar(TEXT("BandSoftness"), InEffective.Get_BandSoftness(), Previous.Get_BandSoftness());
    Set_Scalar(TEXT("ShadowLift"), InEffective.Get_ShadowLift(), Previous.Get_ShadowLift());
    Set_Scalar(TEXT("Strength"), InEffective.Get_Strength(), Previous.Get_Strength());
    Set_Scalar(TEXT("QuantizeFinalColor"),
        Get_Flag(InEffective.Get_QuantizeFinalColor()), Get_Flag(Previous.Get_QuantizeFinalColor()));

    Set_Scalar(TEXT("EnablePattern"),
        Get_Flag(InEffective.Get_EnablePattern()), Get_Flag(Previous.Get_EnablePattern()));
    Set_Scalar(TEXT("Pattern"),
        Get_EnumIndex(InEffective.Get_Pattern()), Get_EnumIndex(Previous.Get_Pattern()));
    Set_Scalar(TEXT("PatternSpace"),
        Get_EnumIndex(InEffective.Get_PatternSpace()), Get_EnumIndex(Previous.Get_PatternSpace()));
    Set_Scalar(TEXT("PatternStrength"), InEffective.Get_PatternStrength(), Previous.Get_PatternStrength());
    Set_Scalar(TEXT("PatternContrast"), InEffective.Get_PatternContrast(), Previous.Get_PatternContrast());
    Set_Scalar(TEXT("PatternWorldSize"), InEffective.Get_PatternWorldSize(), Previous.Get_PatternWorldSize());
    Set_Scalar(TEXT("PatternPixelSize"), InEffective.Get_PatternPixelSize(), Previous.Get_PatternPixelSize());
    Set_Scalar(TEXT("TriplanarSharpness"), InEffective.Get_TriplanarSharpness(), Previous.Get_TriplanarSharpness());
    Set_Scalar(TEXT("PatternDistanceScaling"),
        InEffective.Get_PatternDistanceScaling(), Previous.Get_PatternDistanceScaling());
    Set_Scalar(TEXT("PatternOctaveMin"), InEffective.Get_PatternOctaveMin(), Previous.Get_PatternOctaveMin());
    Set_Scalar(TEXT("PatternOctaveMax"), InEffective.Get_PatternOctaveMax(), Previous.Get_PatternOctaveMax());
    Set_Scalar(TEXT("PatternScrollSpeed"), InEffective.Get_PatternScrollSpeed(), Previous.Get_PatternScrollSpeed());

    Set_Scalar(TEXT("Saturation"), InEffective.Get_Saturation(), Previous.Get_Saturation());
    Set_Scalar(TEXT("MinimumAlbedo"), InEffective.Get_MinimumAlbedo(), Previous.Get_MinimumAlbedo());
    Set_Scalar(TEXT("AffectUnlit"),
        Get_Flag(InEffective.Get_AffectUnlit()), Get_Flag(Previous.Get_AffectUnlit()));

    Set_Scalar(TEXT("EnableSky"),
        Get_Flag(InEffective.Get_EnableSky()), Get_Flag(Previous.Get_EnableSky()));
    Set_Scalar(TEXT("SkyDistance"), InEffective.Get_SkyDistance(), Previous.Get_SkyDistance());
    Set_Scalar(TEXT("SkyBands"), Get_Count(InEffective.Get_SkyBands()), Get_Count(Previous.Get_SkyBands()));
    Set_Scalar(TEXT("SkyStrength"), InEffective.Get_SkyStrength(), Previous.Get_SkyStrength());
    Set_Scalar(TEXT("SkyPattern"),
        Get_EnumIndex(InEffective.Get_SkyPattern()), Get_EnumIndex(Previous.Get_SkyPattern()));
    Set_Scalar(TEXT("SkyPatternStrength"), InEffective.Get_SkyPatternStrength(), Previous.Get_SkyPatternStrength());
    Set_Scalar(TEXT("SkyPatternScale"), InEffective.Get_SkyPatternScale(), Previous.Get_SkyPatternScale());

    Set_Scalar(TEXT("MetallicThreshold"), InEffective.Get_MetallicThreshold(), Previous.Get_MetallicThreshold());
    Set_Scalar(TEXT("MetallicBands"),
        Get_Count(InEffective.Get_MetallicBands()), Get_Count(Previous.Get_MetallicBands()));
    Set_Scalar(TEXT("MetallicStrength"), InEffective.Get_MetallicStrength(), Previous.Get_MetallicStrength());
    Set_Scalar(TEXT("MetallicPatternStrength"),
        InEffective.Get_MetallicPatternStrength(), Previous.Get_MetallicPatternStrength());

    Set_Scalar(TEXT("EnableSpecular"),
        Get_Flag(InEffective.Get_EnableSpecular()), Get_Flag(Previous.Get_EnableSpecular()));
    Set_Scalar(TEXT("SpecularSteps"),
        Get_Count(InEffective.Get_SpecularSteps()), Get_Count(Previous.Get_SpecularSteps()));
    Set_Scalar(TEXT("SpecularThreshold"), InEffective.Get_SpecularThreshold(), Previous.Get_SpecularThreshold());
    Set_Scalar(TEXT("SpecularIntensity"), InEffective.Get_SpecularIntensity(), Previous.Get_SpecularIntensity());
    Set_Scalar(TEXT("SpecularRoughnessCutoff"),
        InEffective.Get_SpecularRoughnessCutoff(), Previous.Get_SpecularRoughnessCutoff());

    Set_Scalar(TEXT("EnableRimLight"),
        Get_Flag(InEffective.Get_EnableRimLight()), Get_Flag(Previous.Get_EnableRimLight()));
    Set_Scalar(TEXT("RimPower"), InEffective.Get_RimPower(), Previous.Get_RimPower());
    Set_Scalar(TEXT("RimThreshold"), InEffective.Get_RimThreshold(), Previous.Get_RimThreshold());
    Set_Scalar(TEXT("RimSoftness"), InEffective.Get_RimSoftness(), Previous.Get_RimSoftness());
    Set_Scalar(TEXT("RimIntensity"), InEffective.Get_RimIntensity(), Previous.Get_RimIntensity());
    Set_Scalar(TEXT("RimFollowsLighting"), InEffective.Get_RimFollowsLighting(), Previous.Get_RimFollowsLighting());

    Set_Scalar(TEXT("EnableOutline"),
        Get_Flag(InEffective.Get_EnableOutline()), Get_Flag(Previous.Get_EnableOutline()));
    Set_Scalar(TEXT("OutlineThickness"), InEffective.Get_OutlineThickness(), Previous.Get_OutlineThickness());
    Set_Scalar(TEXT("OutlineQuality"),
        Get_EnumIndex(InEffective.Get_OutlineQuality()), Get_EnumIndex(Previous.Get_OutlineQuality()));
    Set_Scalar(TEXT("OutlineOpacity"), InEffective.Get_OutlineOpacity(), Previous.Get_OutlineOpacity());
    Set_Scalar(TEXT("OutlineBlendMode"),
        Get_EnumIndex(InEffective.Get_OutlineBlendMode()), Get_EnumIndex(Previous.Get_OutlineBlendMode()));
    Set_Scalar(TEXT("OutlineDepthThreshold"),
        InEffective.Get_OutlineDepthThreshold(), Previous.Get_OutlineDepthThreshold());
    Set_Scalar(TEXT("OutlineNormalThreshold"),
        InEffective.Get_OutlineNormalThreshold(), Previous.Get_OutlineNormalThreshold());
    Set_Scalar(TEXT("OutlineAlbedoThreshold"),
        InEffective.Get_OutlineAlbedoThreshold(), Previous.Get_OutlineAlbedoThreshold());
    Set_Scalar(TEXT("OutlineDistanceFade"),
        InEffective.Get_OutlineDistanceFade(), Previous.Get_OutlineDistanceFade());

    Set_Scalar(TEXT("EnableStencilPatterns"),
        Get_Flag(InEffective.Get_EnableStencilPatterns()), Get_Flag(Previous.Get_EnableStencilPatterns()));
    Set_Scalar(TEXT("StencilBase"),
        Get_Count(InEffective.Get_StencilBase()), Get_Count(Previous.Get_StencilBase()));
    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(InEffective.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Vector(TEXT("ShadowTint"), InEffective.Get_ShadowTint(), Previous.Get_ShadowTint());
    Set_Vector(TEXT("LightTint"), InEffective.Get_LightTint(), Previous.Get_LightTint());
    Set_Vector(TEXT("RimColor"), InEffective.Get_RimColor(), Previous.Get_RimColor());
    Set_Vector(TEXT("OutlineColor"), InEffective.Get_OutlineColor(), Previous.Get_OutlineColor());

    Set_Scalar(TEXT("DistributionMode"),
        Get_EnumIndex(InEffective.Get_DistributionMode()), Get_EnumIndex(Previous.Get_DistributionMode()));

    // Entries past the shader's fixed 8-wide list are dropped, and unfilled slots are written 1.0 rather
    // than 0: the shader counts edges at or below the ramp, so a stale 0 in a slot the count no longer
    // covers would still read as "below" if the count itself were ever wrong, and would silently add a
    // band. 1.0 is the only filler that cannot.
    const auto Get_BandEdge = [](const FCk_Usf_CelShade_Params& InParams, int32 InIndex) -> float
    {
        const auto Edges = InParams.Get_EffectiveBandEdges();
        return Edges.IsValidIndex(InIndex) ? Edges[InIndex] : 1.0f;
    };

    const auto Get_BandEdgeCount = [](const FCk_Usf_CelShade_Params& InParams) -> float
    {
        return static_cast<float>(FMath::Clamp(
            InParams.Get_EffectiveBandEdges().Num(), 1, FCk_Usf_CelShade_Params::MaxBandEdges));
    };

    Set_Scalar(TEXT("BandEdgeCount"), Get_BandEdgeCount(InEffective), Get_BandEdgeCount(Previous));

    for (auto Index = 0; Index < FCk_Usf_CelShade_Params::MaxBandEdges; ++Index)
    {
        Set_Scalar(*FString::Printf(TEXT("BandEdge%d"), Index),
            Get_BandEdge(InEffective, Index), Get_BandEdge(Previous, Index));
    }

    Set_Scalar(TEXT("MaskMode"),
        Get_EnumIndex(InEffective.Get_Mask().Get_Mode()), Get_EnumIndex(Previous.Get_Mask().Get_Mode()));
    Set_Scalar(TEXT("MaskStencilMin"),
        Get_Count(InEffective.Get_Mask().Get_StencilMin()), Get_Count(Previous.Get_Mask().Get_StencilMin()));
    Set_Scalar(TEXT("MaskStencilMax"),
        Get_Count(InEffective.Get_Mask().Get_StencilMax()), Get_Count(Previous.Get_Mask().Get_StencilMax()));

    _WrittenSettings = InEffective;
}

// --------------------------------------------------------------------------------------------------------------------
