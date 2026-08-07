#include "CkUsf/Stylize/CkUsf_ScreenDitherSubsystem.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf/Stylize/CkUsf_ScreenDitherPreset.h"
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

const FName UCkUsf_ScreenDitherSubsystem::kLookName = TEXT("ScreenDither");

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_ScreenDitherSubsystem::
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
    UCkUsf_ScreenDitherSubsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _CVarChangedHandle = ck::usf::stylize::Get_OnCVarChanged().AddUObject(
        this, &UCkUsf_ScreenDitherSubsystem::DoOn_CVarChanged);
}

auto
    UCkUsf_ScreenDitherSubsystem::
    Deinitialize()
    -> void
{
    ck::usf::stylize::Get_OnCVarChanged().Remove(_CVarChangedHandle);
    _CVarChangedHandle.Reset();

    Super::Deinitialize();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoApply_ProjectDefault();
}

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
    _SettingsExplicitlySet = true;

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

    _SettingsExplicitlySet = true;

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
    if (auto* DefaultPreset = DoResolve_ProjectDefaultPreset())
    {
        Apply_Preset(DefaultPreset);
        return;
    }

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

    const auto Effective = DoGet_EffectiveSettings();

    // The component is owned by a world-spawned actor, so it can be torn down under us while the MID
    // (outered to this subsystem) survives and keeps DoEnsure_ViewEffect returning true.
    if (ck::IsValid(_ViewPP))
    { _ViewPP->bEnabled = Effective.Get_Enabled() == ECk_EnableDisable::Enable; }

    DoWrite_ChangedParams(Effective);
}

auto
    UCkUsf_ScreenDitherSubsystem::
    DoGet_EffectiveSettings() const
    -> FCk_Usf_ScreenDither_Params
{
    auto Effective = _Settings;

    const auto EnabledOverride = ck::usf::stylize::Get_EnabledOverride_ScreenDither();
    if (EnabledOverride >= 0)
    {
        Effective.Set_Enabled(EnabledOverride > 0 ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable);
    }

    const auto DebugOverride = ck::usf::stylize::Get_DebugOverride_ScreenDither();
    if (DebugOverride >= 0)
    {
        // UHT appends a _MAX enumerator to every UENUM and IsValidEnumValue accepts it, so
        // one-past-the-end would pass here and then fall through the shader's if-chain to the
        // final image — a debug mode that silently shows no debug view.
        const auto* DebugEnum = StaticEnum<ECk_Usf_ScreenDither_DebugMode>();
        const auto DebugOverrideIsValid =
            DebugEnum->IsValidEnumValue(DebugOverride) && DebugOverride != DebugEnum->GetMaxEnumValue();

        CK_ENSURE_IF_NOT(DebugOverrideIsValid,
            TEXT("ck.Usf.ScreenDither.Debug is [{}], which is not an ECk_Usf_ScreenDither_DebugMode value; "
                 "the setting's own debug mode is used instead"), DebugOverride)
        {}

        if (DebugOverrideIsValid)
        { Effective.Set_DebugMode(static_cast<ECk_Usf_ScreenDither_DebugMode>(DebugOverride)); }
    }

    return Effective;
}

auto
    UCkUsf_ScreenDitherSubsystem::
    DoOn_CVarChanged()
    -> void
{
    // A debug CVar must never be what instantiates the effect: every world has one of these subsystems,
    // and the settings default to Enabled, so an unconditional re-sync would switch dithering on in every
    // world that merely exists the moment anyone touches a console value. Worlds already carrying the
    // effect re-sync, and an explicit force-on is allowed to create it.
    if (_DitherMID == nullptr && ck::usf::stylize::Get_EnabledOverride_ScreenDither() != 1)
    { return; }

    DoSync_ViewEffect();
}

auto
    UCkUsf_ScreenDitherSubsystem::
    DoResolve_ProjectDefaultPreset() const
    -> UCkUsf_ScreenDitherPreset*
{
    const auto SoftPreset = UCk_Utils_Usf_Stylize_Settings_UE::Get_ScreenDitherDefaultPreset();

    // Unset is the "no default style" answer, not a failure — the effect simply stays off.
    if (SoftPreset.IsNull())
    { return nullptr; }

    auto* Preset = SoftPreset.LoadSynchronous();

    CK_ENSURE_IF_NOT(ck::IsValid(Preset, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Project settings name [{}] as the default ScreenDither preset, but it could not be loaded"),
        SoftPreset.ToString())
    {}

    return Preset;
}

auto
    UCkUsf_ScreenDitherSubsystem::
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
    UCkUsf_ScreenDitherSubsystem::
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
    UCkUsf_ScreenDitherSubsystem::
    DoWrite_ChangedParams(
        const FCk_Usf_ScreenDither_Params& InEffective)
    -> void
{
    const auto WriteAll = _WrittenSettings.IsSet() == false;
    const auto& Previous = WriteAll ? InEffective : _WrittenSettings.GetValue();

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
        Get_EnumIndex(InEffective.Get_Pattern()), Get_EnumIndex(Previous.Get_Pattern()));
    Set_Scalar(TEXT("PixelScale"), InEffective.Get_PixelScale(), Previous.Get_PixelScale());
    Set_Scalar(TEXT("DitherStrength"), InEffective.Get_DitherStrength(), Previous.Get_DitherStrength());
    Set_Scalar(TEXT("Animate"),
        Get_Flag(InEffective.Get_Animate()), Get_Flag(Previous.Get_Animate()));
    Set_Scalar(TEXT("AnimationPeriod"), InEffective.Get_AnimationPeriod(), Previous.Get_AnimationPeriod());
    Set_Scalar(TEXT("BoxFilterDownsample"),
        Get_Flag(InEffective.Get_BoxFilterDownsample()), Get_Flag(Previous.Get_BoxFilterDownsample()));
    Set_Scalar(TEXT("StabilizeGrid"),
        Get_Flag(InEffective.Get_StabilizeGrid()), Get_Flag(Previous.Get_StabilizeGrid()));
    Set_Scalar(TEXT("PaletteMode"),
        Get_EnumIndex(InEffective.Get_PaletteMode()), Get_EnumIndex(Previous.Get_PaletteMode()));
    Set_Scalar(TEXT("ColorSteps"),
        static_cast<float>(InEffective.Get_ColorSteps()), static_cast<float>(Previous.Get_ColorSteps()));
    Set_Scalar(TEXT("ColorSpace"),
        Get_EnumIndex(InEffective.Get_ColorSpace()), Get_EnumIndex(Previous.Get_ColorSpace()));
    Set_Scalar(TEXT("PreGamma"), InEffective.Get_PreGamma(), Previous.Get_PreGamma());
    Set_Scalar(TEXT("Monochrome"),
        Get_Flag(InEffective.Get_Monochrome()), Get_Flag(Previous.Get_Monochrome()));
    Set_Scalar(TEXT("Saturation"), InEffective.Get_Saturation(), Previous.Get_Saturation());
    Set_Scalar(TEXT("Contrast"), InEffective.Get_Contrast(), Previous.Get_Contrast());
    Set_Scalar(TEXT("Weight"), InEffective.Get_Weight(), Previous.Get_Weight());
    Set_Scalar(TEXT("DebugMode"),
        Get_EnumIndex(InEffective.Get_DebugMode()), Get_EnumIndex(Previous.Get_DebugMode()));

    Set_Vector(TEXT("MonochromeShadowTint"),
        InEffective.Get_MonochromeShadowTint(), Previous.Get_MonochromeShadowTint());
    Set_Vector(TEXT("MonochromeHighlightTint"),
        InEffective.Get_MonochromeHighlightTint(), Previous.Get_MonochromeHighlightTint());

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

    Set_Scalar(TEXT("PaletteCount"), Get_PaletteCount(InEffective), Get_PaletteCount(Previous));

    for (auto Index = 0; Index < FCk_Usf_ScreenDither_Params::MaxPaletteEntries; ++Index)
    {
        Set_Vector(*FString::Printf(TEXT("PaletteColor%d"), Index),
            Get_PaletteEntry(InEffective, Index), Get_PaletteEntry(Previous, Index));
    }

    _WrittenSettings = InEffective;
}

// --------------------------------------------------------------------------------------------------------------------
