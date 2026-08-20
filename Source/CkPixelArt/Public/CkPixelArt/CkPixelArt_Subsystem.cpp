#include "CkPixelArt/CkPixelArt_Subsystem.h"

#include "CkPixelArt/CkPixelArt_Preset.h"
#include "CkPixelArt/CkPixelArt_ProjectSettings.h"
#include "CkPixelArt/CkPixelArt_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkPixelArtRender/CkPixelArtRender_CVars.h"
#include "CkPixelArtRender/CkPixelArtRender_State.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"

#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_subsystem
{
    // Name of the look whose generated master backs the stylization half, and of its .ush entry point's
    // parameters. The writes below are positional in nothing but NAME, so a rename on either side is a silent
    // no-op until someone looks at a gym.
    const FName LookName = TEXT("PixelArt");

    // Anti-aliasing methods that take the primary spatial upscale slot away. TSR and TAA switch the view to
    // TEMPORAL upscaling, and the renderer's whole delivery mechanism is the spatial pass they replace — so with
    // either selected the scene renders at the internal resolution and is upscaled by something else entirely.
    constexpr int32 AntiAliasingMethod_None = 0;
    constexpr int32 AntiAliasingMethod_FXAA = 1;

    auto
        TryGet_CVarFloat(
            const TCHAR* InName)
        -> TOptional<float>
    {
        const auto* CVar = IConsoleManager::Get().FindConsoleVariable(InName);

        if (CVar == nullptr)
        { return {}; }

        return CVar->GetFloat();
    }

    // ECVF_SetByConsole, not ECVF_SetByCode: this is only ever reached from an explicit
    // Request_Apply_RecommendedCVars, and the situation it exists to rescue is precisely one where somebody has
    // already typed the offending value into the console. A SetByCode write is silently dropped under a console
    // one, which would leave the caller looking at an unchanged precondition report and no explanation.
    auto
        DoSet_CVarFloat(
            const TCHAR* InName,
            float InValue)
        -> void
    {
        auto* CVar = IConsoleManager::Get().FindConsoleVariable(InName);

        if (CVar == nullptr)
        { return; }

        CVar->Set(InValue, ECVF_SetByConsole);
    }

    auto
        Get_ScreenPercentageShowFlagIsOn(
            const UWorld* InWorld)
        -> bool
    {
        if (ck::Is_NOT_Valid(InWorld) || ck::Is_NOT_Valid(InWorld->GetGameViewport()))
        { return true; }

        return InWorld->GetGameViewport()->EngineShowFlags.ScreenPercentage != 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkPixelArt_Subsystem::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    // A dedicated server renders nothing, so a configured default preset must not make it drive a screen
    // percentage per world. Same granularity and same known gap as the Stylize subsystems: a PIE
    // dedicated-server world lives in the editor process and still gets one.
    if (IsRunningDedicatedServer())
    { return false; }

    return Super::ShouldCreateSubsystem(InOuter);
}

auto
    UCkPixelArt_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _CVarChangedHandle = ck::pixel_art::Get_OnCVarChanged().AddUObject(
        this, &UCkPixelArt_Subsystem::DoOn_CVarChanged);
}

auto
    UCkPixelArt_Subsystem::
    Deinitialize()
    -> void
{
    ck::pixel_art::Get_OnCVarChanged().Remove(_CVarChangedHandle);
    _CVarChangedHandle.Reset();

    // Leave nothing of this world behind: the registry entry would keep a dead world's configuration alive until
    // the next sweep, and a moved console variable would outlive the world that moved it.
    FCk_PixelArtRender_StateRegistry::Clear(GetWorld());
    DoRestore_RecommendedCVars();

    Super::Deinitialize();
}

auto
    UCkPixelArt_Subsystem::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoApply_ProjectDefault();
}

auto
    UCkPixelArt_Subsystem::
    Get_PixelArtSubsystem(
        const UObject* InWorldContextObject)
    -> UCkPixelArt_Subsystem*
{
    if (ck::Is_NOT_Valid(GEngine))
    { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull);

    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCkPixelArt_Subsystem>();
}

auto
    UCkPixelArt_Subsystem::
    Request_SetEnabled(
        ECk_EnableDisable InEnabled)
    -> void
{
    _SettingsExplicitlySet = true;

    if (InEnabled == ECk_EnableDisable::Disable)
    {
        _Enabled = ECk_EnableDisable::Disable;

        FCk_PixelArtRender_StateRegistry::Clear(GetWorld());
        DoSync_LookEffect();
        DoRestore_RecommendedCVars();

        return;
    }

    const auto Report = DoGet_PreconditionReport();
    const auto PreconditionsAreSatisfied = Report.Get_IsSatisfied();

    CK_ENSURE_IF_NOT(PreconditionsAreSatisfied,
        TEXT("Cannot enable the pixel-art renderer for world [{}]: {}. Call Request_Apply_RecommendedCVars to "
             "move these, or set them yourself — the renderer will not do it silently."),
        GetWorld(), Report.ToText())
    {
        // Refused, not half-enabled: the caller reads the same `Disable` it would have read before asking, and
        // Get_PreconditionReport still says why, because it recomputes.
        _Enabled = ECk_EnableDisable::Disable;

        FCk_PixelArtRender_StateRegistry::Clear(GetWorld());

        return;
    }

    _Enabled = ECk_EnableDisable::Enable;

    DoSync_RenderConfig();
    DoSync_LookEffect();
}

auto
    UCkPixelArt_Subsystem::
    Get_IsEnabled() const
    -> ECk_EnableDisable
{
    return _Enabled;
}

auto
    UCkPixelArt_Subsystem::
    Apply_Preset(
        const UCkPixelArt_Preset* InPreset)
    -> void
{
    const auto PresetIsValid = ck::IsValid(InPreset);

    CK_ENSURE_IF_NOT(PresetIsValid,
        TEXT("Apply_Preset: null PixelArt preset; settings left untouched"))
    { return; }

    Request_SetSettings(InPreset->Get_AsParams());
}

auto
    UCkPixelArt_Subsystem::
    Request_SetSettings(
        const FCk_PixelArt_Params& InParams)
    -> void
{
    _SettingsExplicitlySet = true;
    _Settings = InParams;

    DoSync_RenderConfig();
    DoSync_LookEffect();
}

auto
    UCkPixelArt_Subsystem::
    Get_Settings() const
    -> FCk_PixelArt_Params
{
    return _Settings;
}

auto
    UCkPixelArt_Subsystem::
    Request_ResetToDefaults()
    -> void
{
    if (auto* DefaultPreset = DoResolve_ProjectDefaultPreset())
    {
        Apply_Preset(DefaultPreset);
        return;
    }

    Request_SetSettings(FCk_PixelArt_Params{});
}

auto
    UCkPixelArt_Subsystem::
    Get_PreconditionReport() const
    -> FCk_PixelArt_PreconditionReport
{
    return DoGet_PreconditionReport();
}

auto
    UCkPixelArt_Subsystem::
    Request_Apply_RecommendedCVars()
    -> void
{
    const auto Remember_And_Set = [&](const TCHAR* InName, float InValue) -> void
    {
        const auto Current = ck_pixel_art_subsystem::TryGet_CVarFloat(InName);

        if (!Current.IsSet())
        {
            ck::pixelart::Warning(TEXT("Console variable [{}] does not exist; the pixel-art renderer cannot set it"),
                InName);
            return;
        }

        if (FMath::IsNearlyEqual(*Current, InValue))
        { return; }

        // Only the FIRST prior is kept. A second apply that overwrote it would make the restore put back a value
        // this subsystem itself wrote, which is how a "restore" quietly becomes a no-op.
        if (!_CVarPriors.Contains(InName))
        { _CVarPriors.Add(InName, *Current); }

        ck_pixel_art_subsystem::DoSet_CVarFloat(InName, InValue);

        ck::pixelart::Display(TEXT("Recommended CVar [{}]: {} -> {}"), InName, *Current, InValue);
    };

    Remember_And_Set(TEXT("r.AntiAliasingMethod"), ck_pixel_art_subsystem::AntiAliasingMethod_None);
    Remember_And_Set(TEXT("r.DynamicRes.OperationMode"), 0.0f);
    Remember_And_Set(TEXT("r.SecondaryScreenPercentage.GameViewport"), 100.0f);
}

auto
    UCkPixelArt_Subsystem::
    DoGet_PreconditionReport() const
    -> FCk_PixelArt_PreconditionReport
{
    auto Report = FCk_PixelArt_PreconditionReport{};
    auto Failures = TArray<FCk_PixelArt_PreconditionFailure>{};

    if (const auto AntiAliasing = ck_pixel_art_subsystem::TryGet_CVarFloat(TEXT("r.AntiAliasingMethod"));
        AntiAliasing.IsSet())
    {
        const auto Method = FMath::RoundToInt32(*AntiAliasing);
        const auto MethodIsSpatial =
            Method == ck_pixel_art_subsystem::AntiAliasingMethod_None ||
            Method == ck_pixel_art_subsystem::AntiAliasingMethod_FXAA;

        if (!MethodIsSpatial)
        {
            Failures.Emplace(
                TEXT("r.AntiAliasingMethod"),
                FString::FromInt(Method),
                TEXT("0 (None) or 1 (FXAA)"),
                TEXT("r.AntiAliasingMethod 0"));
        }
    }

    if (const auto DynamicRes = ck_pixel_art_subsystem::TryGet_CVarFloat(TEXT("r.DynamicRes.OperationMode"));
        DynamicRes.IsSet() && FMath::RoundToInt32(*DynamicRes) != 0)
    {
        Failures.Emplace(
            TEXT("r.DynamicRes.OperationMode"),
            FString::FromInt(FMath::RoundToInt32(*DynamicRes)),
            TEXT("0 (disabled)"),
            TEXT("r.DynamicRes.OperationMode 0"));
    }

    if (!ck_pixel_art_subsystem::Get_ScreenPercentageShowFlagIsOn(GetWorld()))
    {
        Failures.Emplace(
            TEXT("ShowFlag.ScreenPercentage"),
            TEXT("off"),
            TEXT("on"),
            TEXT("ShowFlag.ScreenPercentage 1"));
    }

    Report.Set_Failures(Failures);

    return Report;
}

auto
    UCkPixelArt_Subsystem::
    DoSync_RenderConfig()
    -> void
{
    auto* World = GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return; }

    if (_Enabled == ECk_EnableDisable::Disable)
    {
        FCk_PixelArtRender_StateRegistry::Clear(World);
        return;
    }

    // Only the renderer half crosses over. The console overlay is NOT folded in here: the render module applies
    // `ck.PixelArt.*` on the way out of the registry, so folding it in on the way in would make an override
    // indistinguishable from a setting and leave it behind when the CVar goes back to -1.
    auto Config = FCk_PixelArt_RenderConfig{};
    Config.Enabled = true;
    Config.ResolutionMode = _Settings.Get_ResolutionMode();
    Config.InternalHeight = _Settings.Get_InternalHeight();
    Config.TexelsPerPixel = _Settings.Get_TexelsPerPixel();
    Config.MarginTexels = _Settings.Get_MarginTexels();
    Config.SnapEnabled = _Settings.Get_SnapEnabled() == ECk_EnableDisable::Enable;
    Config.FilterMode = _Settings.Get_UpscaleFilter();

    FCk_PixelArtRender_StateRegistry::Set(World, Config);
}

auto
    UCkPixelArt_Subsystem::
    DoRestore_RecommendedCVars()
    -> void
{
    for (const auto& Prior : _CVarPriors)
    {
        ck_pixel_art_subsystem::DoSet_CVarFloat(*Prior.Key, Prior.Value);

        ck::pixelart::Display(TEXT("Restored CVar [{}] to {}"), Prior.Key, Prior.Value);
    }

    _CVarPriors.Empty();
}

auto
    UCkPixelArt_Subsystem::
    DoResolve_ProjectDefaultPreset() const
    -> UCkPixelArt_Preset*
{
    const auto SoftPreset = UCk_Utils_PixelArt_Settings_UE::Get_DefaultPreset();

    if (SoftPreset.IsNull())
    { return nullptr; }

    return SoftPreset.LoadSynchronous();
}

auto
    UCkPixelArt_Subsystem::
    DoApply_ProjectDefault()
    -> void
{
    // A packaged game runs its startup map's BeginPlay from inside FEngineLoop::Init, before
    // OnFEngineLoopInitComplete flips the blocking-load flag — resolving the soft ref there is the
    // premature-load class CkCore documents. The retry deliberately does NOT re-test the flag: it is already ON
    // that delegate, and CkCore's own flag-setting registrar may run after us in add-order.
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
    UCkPixelArt_Subsystem::
    DoApply_ProjectDefault_Now()
    -> void
{
    // A default is a DEFAULT: game code that has already spoken wins. This only bites on the deferred path — a
    // packaged game runs its startup map's BeginPlay inside FEngineLoop::Init, so gameplay can legitimately have
    // set settings before the delegate that carries us here fires, and applying the project row then would
    // silently undo it.
    if (_SettingsExplicitlySet)
    { return; }

    auto* DefaultPreset = DoResolve_ProjectDefaultPreset();

    if (DefaultPreset == nullptr)
    { return; }

    Apply_Preset(DefaultPreset);
    Request_SetEnabled(ECk_EnableDisable::Enable);

    // Applying the project row is not the game speaking, so a later explicit call must still be able to win.
    _SettingsExplicitlySet = false;
}

auto
    UCkPixelArt_Subsystem::
    DoOn_CVarChanged()
    -> void
{
    // A debug CVar must never be what turns the renderer on: every world has one of these subsystems, so an
    // unconditional re-sync would start driving the screen percentage in every world that merely exists the
    // moment anyone touches a console value. Worlds already enabled re-sync; the console force-on path lives in
    // the render module, which can bring itself up without a configuration.
    if (_Enabled == ECk_EnableDisable::Disable)
    { return; }

    DoSync_RenderConfig();
    DoSync_LookEffect();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkPixelArt_Subsystem::
    DoEnsure_LookEffect()
    -> bool
{
    if (ck::IsValid(_LookMID))
    { return true; }

    auto* World = GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto MasterPath = ck::usf::Get_GeneratedMasterObjectPath(ck_pixel_art_subsystem::LookName);
    auto* Master = LoadObject<UMaterialInterface>(nullptr, *MasterPath);

    if (ck::Is_NOT_Valid(Master))
    {
        // Warned, not ensured: a project that wants the renderer without the stylization is a supported
        // configuration, and so is a fresh checkout that has not generated look masters yet. Settings keep
        // round-tripping either way; only the look does not render.
        if (NOT _WarnedMissingMaster)
        {
            _WarnedMissingMaster = true;

            ck::pixelart::Warning(
                TEXT("PixelArt look master not found at [{}] - run `Ck_Usf_GenerateLooks PixelArt`. The renderer ")
                TEXT("half still works; the scene simply renders sharp and low-resolution without the outlines ")
                TEXT("and palette."),
                MasterPath);
        }

        return false;
    }

    _LookMID = UMaterialInstanceDynamic::Create(Master, this);

    auto SpawnParams = FActorSpawnParameters{};
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.Name = TEXT("CkPixelArt_LookView");

    _LookActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);

    if (ck::Is_NOT_Valid(_LookActor))
    {
        _LookMID = nullptr;
        return false;
    }

    _LookPP = NewObject<UPostProcessComponent>(_LookActor, TEXT("PixelArtPostProcess"));
    _LookActor->SetRootComponent(_LookPP);
    _LookPP->bUnbound = true;
    _LookPP->RegisterComponent();
    _LookPP->Settings.AddBlendable(_LookMID, 1.0f);

    return true;
}

auto
    UCkPixelArt_Subsystem::
    DoSync_LookEffect()
    -> void
{
    const auto LookShouldRender =
        _Enabled == ECk_EnableDisable::Enable &&
        _Settings.Get_ApplyLook() == ECk_EnableDisable::Enable;

    // Nothing to turn off if it was never created, and creating it here would mean a world that only ever
    // disabled the look still paid for a master load and an actor.
    if (NOT LookShouldRender && ck::Is_NOT_Valid(_LookMID))
    { return; }

    if (LookShouldRender && NOT DoEnsure_LookEffect())
    { return; }

    // The component is owned by a world-spawned actor, so it can be torn down under us while the MID - outered
    // to this subsystem - survives and keeps DoEnsure_LookEffect returning true.
    if (ck::IsValid(_LookPP))
    { _LookPP->bEnabled = LookShouldRender; }

    if (LookShouldRender)
    { DoWrite_ChangedLookParams(_Settings.Get_Look()); }
}

auto
    UCkPixelArt_Subsystem::
    DoWrite_ChangedLookParams(
        const FCk_PixelArt_LookParams& InLook)
    -> void
{
    if (ck::Is_NOT_Valid(_LookMID))
    { return; }

    const auto WriteAll = NOT _WrittenLook.IsSet();
    const auto& Previous = WriteAll ? InLook : _WrittenLook.GetValue();

    const auto Set_Scalar = [&](const TCHAR* InName, float InValue, float InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _LookMID->SetScalarParameterValue(InName, InValue); }
    };

    const auto Set_Vector = [&](const TCHAR* InName, const FLinearColor& InValue, const FLinearColor& InPrevious) -> void
    {
        if (WriteAll || InValue != InPrevious)
        { _LookMID->SetVectorParameterValue(InName, InValue); }
    };

    const auto Get_EnumIndex = [](auto InEnumValue) -> float
    {
        return static_cast<float>(static_cast<uint8>(InEnumValue));
    };

    const auto Get_Flag = [](ECk_EnableDisable InEnableDisable) -> float
    {
        return InEnableDisable == ECk_EnableDisable::Enable ? 1.0f : 0.0f;
    };

    Set_Scalar(TEXT("EnableOutline"), Get_Flag(InLook.Get_EnableOutline()), Get_Flag(Previous.Get_EnableOutline()));
    Set_Scalar(TEXT("DepthThreshold"), InLook.Get_DepthThreshold(), Previous.Get_DepthThreshold());
    Set_Scalar(TEXT("AngleZCutoff"), InLook.Get_AngleZCutoff(), Previous.Get_AngleZCutoff());
    Set_Scalar(TEXT("AngleZScale"), InLook.Get_AngleZScale(), Previous.Get_AngleZScale());
    Set_Scalar(TEXT("NormalSmoothLow"), InLook.Get_NormalSmoothLow(), Previous.Get_NormalSmoothLow());
    Set_Scalar(TEXT("NormalSmoothHigh"), InLook.Get_NormalSmoothHigh(), Previous.Get_NormalSmoothHigh());
    Set_Scalar(TEXT("LineDarken"), InLook.Get_LineDarken(), Previous.Get_LineDarken());
    Set_Scalar(TEXT("CreaseBrighten"), InLook.Get_CreaseBrighten(), Previous.Get_CreaseBrighten());
    Set_Scalar(TEXT("EdgeMode"), Get_EnumIndex(InLook.Get_EdgeMode()), Get_EnumIndex(Previous.Get_EdgeMode()));
    Set_Scalar(TEXT("Bands"), static_cast<float>(InLook.Get_Bands()), static_cast<float>(Previous.Get_Bands()));
    Set_Scalar(TEXT("ThresholdGradientSize"), InLook.Get_ThresholdGradientSize(), Previous.Get_ThresholdGradientSize());
    Set_Scalar(TEXT("DitherStrength"), InLook.Get_DitherStrength(), Previous.Get_DitherStrength());
    Set_Scalar(TEXT("PaletteMode"), Get_EnumIndex(InLook.Get_PaletteMode()), Get_EnumIndex(Previous.Get_PaletteMode()));
    Set_Scalar(TEXT("PaletteCount"), static_cast<float>(InLook.Get_PaletteCount()), static_cast<float>(Previous.Get_PaletteCount()));

    Set_Vector(TEXT("PaletteColor0"), InLook.Get_PaletteColor0(), Previous.Get_PaletteColor0());
    Set_Vector(TEXT("PaletteColor1"), InLook.Get_PaletteColor1(), Previous.Get_PaletteColor1());
    Set_Vector(TEXT("PaletteColor2"), InLook.Get_PaletteColor2(), Previous.Get_PaletteColor2());
    Set_Vector(TEXT("PaletteColor3"), InLook.Get_PaletteColor3(), Previous.Get_PaletteColor3());
    Set_Vector(TEXT("PaletteColor4"), InLook.Get_PaletteColor4(), Previous.Get_PaletteColor4());
    Set_Vector(TEXT("PaletteColor5"), InLook.Get_PaletteColor5(), Previous.Get_PaletteColor5());
    Set_Vector(TEXT("PaletteColor6"), InLook.Get_PaletteColor6(), Previous.Get_PaletteColor6());
    Set_Vector(TEXT("PaletteColor7"), InLook.Get_PaletteColor7(), Previous.Get_PaletteColor7());

    _WrittenLook = InLook;
}
