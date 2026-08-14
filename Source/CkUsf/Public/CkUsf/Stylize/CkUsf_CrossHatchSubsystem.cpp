#include "CkUsf/Stylize/CkUsf_CrossHatchSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Stylize/CkUsf_CrossHatchPreset.h"
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

const FName UCkUsf_CrossHatchSubsystem::kLookName = TEXT("CrossHatch");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CrossHatchSubsystem::
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
    UCkUsf_CrossHatchSubsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _CVarChangedHandle = ck::usf::stylize::Get_OnCVarChanged().AddUObject(
        this, &UCkUsf_CrossHatchSubsystem::DoOn_CVarChanged);
}

auto
    UCkUsf_CrossHatchSubsystem::
    Deinitialize()
    -> void
{
    ck::usf::stylize::Get_OnCVarChanged().Remove(_CVarChangedHandle);
    _CVarChangedHandle.Reset();

    Super::Deinitialize();
}

auto
    UCkUsf_CrossHatchSubsystem::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoApply_ProjectDefault();
}

auto
    UCkUsf_CrossHatchSubsystem::
    Get_CrossHatchSubsystem(
        const UObject* InWorldContextObject)
    -> UCkUsf_CrossHatchSubsystem*
{
    if (ck::Is_NOT_Valid(GEngine))
    { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCkUsf_CrossHatchSubsystem>();
}

auto
    UCkUsf_CrossHatchSubsystem::
    Request_SetEnabled(
        ECk_EnableDisable InEnabled)
    -> void
{
    _SettingsExplicitlySet = true;

    _Settings.Set_Enabled(InEnabled);
    DoSync_ViewEffect();
}

auto
    UCkUsf_CrossHatchSubsystem::
    Get_IsEnabled() const
    -> ECk_EnableDisable
{
    return _Settings.Get_Enabled();
}

auto
    UCkUsf_CrossHatchSubsystem::
    Apply_Preset(
        UCkUsf_CrossHatchPreset* InPreset)
    -> void
{
    const auto PresetIsValid = ck::IsValid(InPreset);

    CK_ENSURE_IF_NOT(PresetIsValid,
        TEXT("Apply_Preset: null CrossHatch preset; settings left untouched"))
    { return; }

    Request_SetSettings(InPreset->Get_AsParams());
}

auto
    UCkUsf_CrossHatchSubsystem::
    Request_SetSettings(
        const FCk_Usf_CrossHatch_Params& InSettings)
    -> void
{
    const auto& Mask = InSettings.Get_Mask();

    const auto MaskRangeIsAddressable = UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeIsAddressable(Mask);

    CK_ENSURE_IF_NOT(MaskRangeIsAddressable,
        TEXT("Request_SetSettings: CrossHatch effect-mask range [{}, {}] is inverted or reaches the "
             "engine's NO-STENCIL value 0; it would match every untagged pixel in the view or none at "
             "all. Settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    { return; }

    const auto MaskRangeAvoidsOutline =
        UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeAvoidsOutline(GetWorld(), Mask);

    CK_ENSURE_IF_NOT(MaskRangeAvoidsOutline,
        TEXT("Request_SetSettings: CrossHatch effect-mask range [{}, {}] COLLIDES with the outline "
             "subsystem's allocated range; settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    { return; }

    const auto MaskRangeAvoidsCelPatterns =
        UCk_Utils_Usf_StylizeMask_UE::Get_MaskRangeAvoidsCelPatterns(GetWorld(), Mask);

    CK_ENSURE_IF_NOT(MaskRangeAvoidsCelPatterns,
        TEXT("Request_SetSettings: CrossHatch effect-mask range [{}, {}] COLLIDES with the CelShade "
             "per-object pattern span in this world; one stencil value cannot both select a cel pattern "
             "and gate this look. Settings left untouched"),
        Mask.Get_StencilMin(), Mask.Get_StencilMax())
    { return; }

    _SettingsExplicitlySet = true;

    _Settings = InSettings;
    DoSync_ViewEffect();
}

auto
    UCkUsf_CrossHatchSubsystem::
    Get_Settings() const
    -> FCk_Usf_CrossHatch_Params
{
    return _Settings;
}

auto
    UCkUsf_CrossHatchSubsystem::
    Request_ResetToDefaults()
    -> void
{
    if (auto* DefaultPreset = DoResolve_ProjectDefaultPreset())
    {
        Apply_Preset(DefaultPreset);
        return;
    }

    Request_SetSettings(FCk_Usf_CrossHatch_Params{});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CrossHatchSubsystem::
    DoEnsure_ViewEffect()
    -> bool
{
    if (_CrossHatchMID != nullptr)
    { return true; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto MasterPath = ck::usf::Get_GeneratedMasterObjectPath(kLookName);
    auto* Master = LoadObject<UMaterialInterface>(nullptr, *MasterPath);
    if (ck::Is_NOT_Valid(Master))
    {
        if (NOT _WarnedMissingMaster)
        {
            _WarnedMissingMaster = true;
            ck::usf::Warning(TEXT("CrossHatch master not found at [{}] — run Generate Look Materials. "
                                  "Settings are still tracked; nothing renders until the master exists."),
                MasterPath);
        }
        return false;
    }

    _CrossHatchMID = UMaterialInstanceDynamic::Create(Master, this);

    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.Name = TEXT("CkUsf_CrossHatchView");
    _ViewActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
    if (_ViewActor == nullptr)
    {
        _CrossHatchMID = nullptr;
        return false;
    }

    _ViewPP = NewObject<UPostProcessComponent>(_ViewActor, TEXT("CrossHatchPostProcess"));
    _ViewActor->SetRootComponent(_ViewPP);
    _ViewPP->bUnbound = true;
    _ViewPP->RegisterComponent();
    _ViewPP->Settings.AddBlendable(_CrossHatchMID, 1.0f);

    return true;
}

auto
    UCkUsf_CrossHatchSubsystem::
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
    UCkUsf_CrossHatchSubsystem::
    DoGet_EffectiveSettings() const
    -> FCk_Usf_CrossHatch_Params
{
    using namespace ck::usf::stylize;

    auto Effective = _Settings;

    if (const auto Enabled = Get_FlagOverride(Get_EnabledOverride_CrossHatch()))
    { Effective.Set_Enabled(*Enabled); }

    if (const auto Debug = Get_EnumOverride<ECk_Usf_CrossHatch_DebugMode>(
            Get_DebugOverride_CrossHatch(), TEXT("ck.Usf.CrossHatch.Debug")))
    { Effective.Set_DebugMode(*Debug); }

    return Effective;
}

auto
    UCkUsf_CrossHatchSubsystem::
    DoOn_CVarChanged()
    -> void
{
    // A debug CVar must never be what instantiates the effect: every world has one of these subsystems,
    // and the settings default to Enabled, so an unconditional re-sync would switch CrossHatch on in
    // every world that merely exists the moment anyone touches a console value. Worlds already carrying
    // the effect re-sync, and an explicit force-on is allowed to create it.
    if (_CrossHatchMID == nullptr && ck::usf::stylize::Get_EnabledOverride_CrossHatch() != 1)
    { return; }

    DoSync_ViewEffect();
}

auto
    UCkUsf_CrossHatchSubsystem::
    DoResolve_ProjectDefaultPreset() const
    -> UCkUsf_CrossHatchPreset*
{
    const auto SoftPreset = UCk_Utils_Usf_Stylize_Settings_UE::Get_CrossHatchDefaultPreset();

    // Unset is the "no default style" answer, not a failure — the effect simply stays off.
    if (SoftPreset.IsNull())
    { return nullptr; }

    auto* Preset = SoftPreset.LoadSynchronous();

    CK_ENSURE_IF_NOT(ck::IsValid(Preset),
        TEXT("Project settings name [{}] as the default CrossHatch preset, but it could not be loaded"),
        SoftPreset.ToString())
    {}

    return Preset;
}

auto
    UCkUsf_CrossHatchSubsystem::
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
    UCkUsf_CrossHatchSubsystem::
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
    UCkUsf_CrossHatchSubsystem::
    DoWrite_ChangedParams(
        const FCk_Usf_CrossHatch_Params& InEffective)
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? InEffective : _WrittenSettings.GetValue();

    const auto Set_Scalar = [&](const TCHAR* InName, float InValue, float InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _CrossHatchMID->SetScalarParameterValue(InName, InValue); }
    };

    const auto Set_Vector = [&](const TCHAR* InName, const FLinearColor& InValue, const FLinearColor& InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _CrossHatchMID->SetVectorParameterValue(InName, InValue); }
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

    Set_Scalar(TEXT("StyleStrength"), InEffective.Get_StyleStrength(), Previous.Get_StyleStrength());
    Set_Scalar(TEXT("UseWorldSpaceNormals"),
        Get_Flag(InEffective.Get_UseWorldSpaceNormals()), Get_Flag(Previous.Get_UseWorldSpaceNormals()));
    Set_Scalar(TEXT("AngleOffset"), InEffective.Get_AngleOffset(), Previous.Get_AngleOffset());
    Set_Scalar(TEXT("NormalAlignment"), InEffective.Get_NormalAlignment(), Previous.Get_NormalAlignment());

    Set_Scalar(TEXT("Spacing"), InEffective.Get_Spacing(), Previous.Get_Spacing());
    Set_Scalar(TEXT("LayerCount"),
        Get_Count(InEffective.Get_LayerCount()), Get_Count(Previous.Get_LayerCount()));
    Set_Scalar(TEXT("LayerAngleStep"), InEffective.Get_LayerAngleStep(), Previous.Get_LayerAngleStep());
    Set_Scalar(TEXT("StrokePattern"),
        Get_EnumIndex(InEffective.Get_StrokePattern()), Get_EnumIndex(Previous.Get_StrokePattern()));
    Set_Scalar(TEXT("StrokeThickness"), InEffective.Get_StrokeThickness(), Previous.Get_StrokeThickness());
    Set_Scalar(TEXT("StrokeIrregularity"),
        InEffective.Get_StrokeIrregularity(), Previous.Get_StrokeIrregularity());

    Set_Scalar(TEXT("DarknessBias"), InEffective.Get_DarknessBias(), Previous.Get_DarknessBias());
    Set_Scalar(TEXT("DarknessContrast"), InEffective.Get_DarknessContrast(), Previous.Get_DarknessContrast());

    Set_Scalar(TEXT("BackgroundMode"),
        Get_EnumIndex(InEffective.Get_BackgroundMode()), Get_EnumIndex(Previous.Get_BackgroundMode()));
    Set_Scalar(TEXT("Saturation"), InEffective.Get_Saturation(), Previous.Get_Saturation());

    Set_Scalar(TEXT("AffectSky"),
        Get_Flag(InEffective.Get_AffectSky()), Get_Flag(Previous.Get_AffectSky()));
    Set_Scalar(TEXT("SkyDistance"), InEffective.Get_SkyDistance(), Previous.Get_SkyDistance());

    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(InEffective.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Scalar(TEXT("MaskMode"),
        Get_EnumIndex(InEffective.Get_Mask().Get_Mode()), Get_EnumIndex(Previous.Get_Mask().Get_Mode()));
    Set_Scalar(TEXT("MaskStencilMin"),
        Get_Count(InEffective.Get_Mask().Get_StencilMin()), Get_Count(Previous.Get_Mask().Get_StencilMin()));
    Set_Scalar(TEXT("MaskStencilMax"),
        Get_Count(InEffective.Get_Mask().Get_StencilMax()), Get_Count(Previous.Get_Mask().Get_StencilMax()));

    Set_Vector(TEXT("InkColor"), InEffective.Get_InkColor(), Previous.Get_InkColor());
    Set_Vector(TEXT("PaperColor"), InEffective.Get_PaperColor(), Previous.Get_PaperColor());

    _WrittenSettings = InEffective;
}

// --------------------------------------------------------------------------------------------------------------------
