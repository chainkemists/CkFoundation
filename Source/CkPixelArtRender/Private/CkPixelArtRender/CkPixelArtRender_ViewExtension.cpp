#include "CkPixelArtRender/CkPixelArtRender_ViewExtension.h"

#include "CkPixelArtRender/CkPixelArtRender_CVars.h"
#include "CkPixelArtRender/CkPixelArtRender_Log.h"
#include "CkPixelArtRender/CkPixelArtRender_Upscaler.h"
#include "CkPixelArtRender/CkPixelArtRender_Utils.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"
#include "SceneView.h"
#include "UnrealClient.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_view_extension
{
    auto
        Get_ScreenPercentageCVar()
        -> IConsoleVariable*
    {
        static auto* ScreenPercentageCVar =
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));

        return ScreenPercentageCVar;
    }

    // The configuration this world will actually render with: what was registered for it, with the console
    // overlay folded on top.
    //
    // Unset means "do not run here". A world with no registered configuration stays unset unless the console
    // explicitly forces the renderer on, which mirrors the Stylize rule that only `Enabled` may bring an effect
    // into existence — otherwise every unconfigured world in a PIE session would start rendering pixelated the
    // moment someone typed a tuning override.
    auto
        TryGet_EffectiveConfig(
            const UWorld* InWorld)
        -> TOptional<FCk_PixelArt_RenderConfig>
    {
        const auto Stored = FCk_PixelArtRender_StateRegistry::TryGet(InWorld);

        auto Effective = Stored.IsSet() ? *Stored : FCk_PixelArt_RenderConfig{};
        ck::pixel_art::Fold_Overrides(Effective);

        if (!Stored.IsSet() && !Effective.Enabled)
        { return {}; }

        return Effective;
    }

    auto
        TryGet_World(
            const FSceneViewFamily& InViewFamily)
        -> const UWorld*
    {
        return InViewFamily.Scene != nullptr ? InViewFamily.Scene->GetWorld() : nullptr;
    }

    // The scene renderer applies ONE resolution fraction to both axes, so only one of them can be made exact.
    // The authored knob is the vertical texel count, but the driven axis is width: the fraction is derived from
    // the target width so CeilToInt lands on it exactly, and the height follows within a texel.
    auto
        Get_TargetWidth(
            int32 InInternalHeight,
            const FIntPoint& InViewportSize)
        -> int32
    {
        const auto Aspect = static_cast<double>(InViewportSize.X) / static_cast<double>(InViewportSize.Y);
        return FMath::RoundToInt(InInternalHeight * Aspect);
    }

    auto
        Get_PredictedInternalSize(
            const FIntPoint& InViewportSize,
            float InFraction,
            float InSecondaryViewFraction)
        -> FIntPoint
    {
        return FIntPoint{
            FMath::CeilToInt(InViewportSize.X * InFraction * InSecondaryViewFraction),
            FMath::CeilToInt(InViewportSize.Y * InFraction * InSecondaryViewFraction)};
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_PixelArt_ViewExtension::
    FCk_PixelArt_ViewExtension(
        const FAutoRegister& InAutoRegister)
    : FSceneViewExtensionBase(InAutoRegister)
{
    _EndFrameHandle = FCoreDelegates::OnEndFrame.AddRaw(this, &FCk_PixelArt_ViewExtension::DoOn_EndFrame);
}

FCk_PixelArt_ViewExtension::
    ~FCk_PixelArt_ViewExtension()
{
    FCoreDelegates::OnEndFrame.Remove(_EndFrameHandle);
    _EndFrameHandle.Reset();

    Request_ReleaseScreenPercentage();
}

auto
    FCk_PixelArt_ViewExtension::
    IsActiveThisFrame_Internal(
        const FSceneViewExtensionContext& InContext) const
    -> bool
{
    if (IsRunningDedicatedServer())
    { return false; }

    const auto* World = InContext.GetWorld();

    if (World == nullptr || !World->IsGameWorld())
    { return false; }

    const auto Config = ck_pixel_art_view_extension::TryGet_EffectiveConfig(World);

    return Config.IsSet() && Config->Enabled;
}

auto
    FCk_PixelArt_ViewExtension::
    SetupViewFamily(
        FSceneViewFamily& InViewFamily)
    -> void
{
    const auto* World = ck_pixel_art_view_extension::TryGet_World(InViewFamily);
    auto Config = ck_pixel_art_view_extension::TryGet_EffectiveConfig(World);

    _FrameConfig.Reset();

    if (!Config.IsSet() || !Config->Enabled)
    { return; }

    const auto InternalHeightIsUsable = Config->InternalHeight > 0;

    // ensureMsgf rather than the house CK_ENSURE_IF_NOT: this module cannot link CkCore at PostConfigInit. It is
    // still an ensure and not a log-and-continue — a non-positive internal height would otherwise resolve to a
    // handful of texels and look like a broken renderer rather than a bad setting.
    if (!ensureMsgf(InternalHeightIsUsable,
        TEXT("CkPixelArt: InternalHeight is [%d], which cannot describe a resolution. The renderer will not run ")
        TEXT("for this world until it is positive."),
        Config->InternalHeight))
    { return; }

    _FrameConfig = Config;

    DoApply_ScreenPercentage(InViewFamily, *Config);
}

auto
    FCk_PixelArt_ViewExtension::
    BeginRenderViewFamily(
        FSceneViewFamily& InViewFamily)
    -> void
{
    if (!_FrameConfig.IsSet() || InViewFamily.Views.Num() == 0)
    { return; }

    // Scene and reflection captures have no view state and are explicitly out of scope: they bypass the
    // projection hook the camera snap lives in, so pixelating them would produce a different, unsnapped image.
    const auto* PrimaryView = InViewFamily.Views[0];

    if (PrimaryView == nullptr || PrimaryView->State == nullptr)
    { return; }

    // By now the engine HAS computed the secondary (DPI-driven) fraction that SetupViewFamily could only assume.
    // Anything but 1 means the window gets a second, engine-owned resample after ours — the documented reason
    // every visual verdict for this renderer is taken standalone rather than in PIE.
    if (!FMath::IsNearlyEqual(_LastSecondaryViewFraction, InViewFamily.SecondaryViewFraction))
    {
        _LastSecondaryViewFraction = InViewFamily.SecondaryViewFraction;

        UE_LOG(LogCkPixelArt, Display,
            TEXT("Secondary view fraction is %.4f (SetupViewFamily assumed 1.0000). Below 1 the engine inserts a ")
            TEXT("DPI-driven secondary upscale after ours and the internal resolution is smaller than requested."),
            _LastSecondaryViewFraction);
    }

    // The upscale slot is silently conditional upstream: TSR and TAAU switch the view to temporal upscaling, and
    // the primary spatial upscale pass is then never enabled, so this renderer stops running with nothing on
    // screen or in the log to say why. Report the transition rather than letting it look like our bug.
    const auto UpscaleSlotIsLive =
        PrimaryView->PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::SpatialUpscale;

    if (UpscaleSlotIsLive != _LastUpscaleSlotWasLive)
    {
        _LastUpscaleSlotWasLive = UpscaleSlotIsLive;

        if (!UpscaleSlotIsLive)
        {
            UE_LOG(LogCkPixelArt, Warning,
                TEXT("The primary spatial upscale slot is not available (anti-aliasing method [%d] selected ")
                TEXT("temporal upscaling), so the pixel-art upscaler will NOT run. Set r.AntiAliasingMethod to ")
                TEXT("0 (None) or 1 (FXAA)."),
                static_cast<int32>(PrimaryView->AntiAliasingMethod));
        }
        else
        {
            UE_LOG(LogCkPixelArt, Display, TEXT("The primary spatial upscale slot is available again."));
        }
    }

    if (!UpscaleSlotIsLive)
    { return; }

    // The renderer checks this slot is null before it calls any extension, so a non-null value here means another
    // extension claimed it first. Assigning anyway would fire the engine's checkf.
    if (InViewFamily.GetPrimarySpatialUpscalerInterface() != nullptr)
    { return; }

    auto Frame = FCk_PixelArt_UpscaleFrame{};
    Frame.InternalSize = _LastInternalSize;
    Frame.MarginTexels = _LastMarginTexels;
    Frame.FilterMode = _FrameConfig->FilterMode;

    // The view family owns this instance and deletes it in its destructor — allocate one per frame, never cache it
    // and never delete it. It is also the whole game-thread-to-render-thread transport for the frame's snap state.
    InViewFamily.SetPrimarySpatialUpscalerInterface(new FCk_PixelArt_SpatialUpscaler(Frame));
}

auto
    FCk_PixelArt_ViewExtension::
    Request_ReleaseScreenPercentage()
    -> void
{
    if (!_SavedScreenPercentage.IsSet())
    { return; }

    auto* ScreenPercentageCVar = ck_pixel_art_view_extension::Get_ScreenPercentageCVar();

    if (ScreenPercentageCVar != nullptr)
    {
        ScreenPercentageCVar->Set(*_SavedScreenPercentage, ECVF_SetByCode);

        if (ck::pixel_art::Get_LogStateEnabled())
        {
            UE_LOG(LogCkPixelArt, Display, TEXT("Released: restored r.ScreenPercentage=%.4f"),
                *_SavedScreenPercentage);
        }
    }

    _SavedScreenPercentage.Reset();
    _LastViewportSize = FIntPoint::ZeroValue;
    _LastInternalSize = FIntPoint::ZeroValue;
    _LastMarginTexels = 0;
    _LastFraction = 0.0f;
}

auto
    FCk_PixelArt_ViewExtension::
    DoOn_EndFrame()
    -> void
{
    // The lease was not renewed this frame, so nothing wants the renderer any more — whatever the reason.
    if (!_LeaseRenewedThisFrame)
    { Request_ReleaseScreenPercentage(); }

    _LeaseRenewedThisFrame = false;
    _FrameConfig.Reset();
}

auto
    FCk_PixelArt_ViewExtension::
    DoApply_ScreenPercentage(
        const FSceneViewFamily& InViewFamily,
        const FCk_PixelArt_RenderConfig& InConfig)
    -> void
{
    const auto* RenderTarget = InViewFamily.RenderTarget;

    if (RenderTarget == nullptr)
    { return; }

    const auto ViewportSize = RenderTarget->GetSizeXY();

    if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
    { return; }

    auto* ScreenPercentageCVar = ck_pixel_art_view_extension::Get_ScreenPercentageCVar();

    if (ScreenPercentageCVar == nullptr)
    {
        UE_LOG(LogCkPixelArt, Error,
            TEXT("r.ScreenPercentage does not exist — the scene cannot be driven to the internal resolution and ")
            TEXT("the upscaler would run on a full-resolution image. The renderer does nothing this frame."));
        return;
    }

    // Read on the way IN, not at construction: whatever the value is the first time the renderer drives it is
    // what gets put back when the lease is released.
    if (!_SavedScreenPercentage.IsSet())
    { _SavedScreenPercentage = ScreenPercentageCVar->GetFloat(); }

    _LeaseRenewedThisFrame = true;

    const auto MarginTexels = FMath::Max(0, InConfig.MarginTexels);
    const auto TargetWidth = ck_pixel_art_view_extension::Get_TargetWidth(InConfig.InternalHeight, ViewportSize);
    const auto RenderedWidth = TargetWidth + 2 * MarginTexels;

    // At this point in UGameViewportClient::Draw the secondary fraction has not been computed yet, so this reads
    // the family default of 1. It is correct in standalone at 100% DPI; BeginRenderViewFamily logs the value the
    // engine actually settled on so a PIE/DPI divergence names itself instead of showing up as a blurry image.
    const auto SecondaryViewFraction = InViewFamily.SecondaryViewFraction;

    const auto FractionForWidth =
        UCk_Utils_PixelArtRender_UE::Get_ExactFraction(RenderedWidth, ViewportSize.X);

    // The renderer multiplies the secondary fraction back in when it derives the view rect, so divide it out here
    // or the internal resolution comes out scaled by it twice.
    const auto Fraction = SecondaryViewFraction > 0.0f ? FractionForWidth / SecondaryViewFraction : FractionForWidth;

    ScreenPercentageCVar->Set(Fraction * 100.0f, ECVF_SetByCode);

    const auto PredictedInternalSize = ck_pixel_art_view_extension::Get_PredictedInternalSize(
        ViewportSize, Fraction, SecondaryViewFraction);

    _LastInternalSize = PredictedInternalSize;
    _LastMarginTexels = MarginTexels;

    const auto GeometryIsUnchanged =
        _LastViewportSize == ViewportSize && FMath::IsNearlyEqual(_LastFraction, Fraction);

    if (GeometryIsUnchanged)
    { return; }

    _LastViewportSize = ViewportSize;
    _LastFraction = Fraction;

    // r.ScreenPercentage set by console or command line outranks ECVF_SetByCode, in which case our write is
    // dropped silently and the scene renders at the wrong resolution. Read it back so that failure is visible.
    const auto AppliedPercentage = ScreenPercentageCVar->GetFloat();

    if (ck::pixel_art::Get_LogStateEnabled())
    {
        UE_LOG(LogCkPixelArt, Display,
            TEXT("Active: viewport=%dx%d rendered=%dx%d displayed=%dx%d fraction=%.6f applied=%.6f secondary=%.4f"),
            ViewportSize.X, ViewportSize.Y,
            PredictedInternalSize.X, PredictedInternalSize.Y,
            PredictedInternalSize.X - 2 * MarginTexels,
            PredictedInternalSize.Y - 2 * MarginTexels,
            Fraction, AppliedPercentage / 100.0f, SecondaryViewFraction);
    }

    if (!FMath::IsNearlyEqual(AppliedPercentage, Fraction * 100.0f, 1e-3f))
    {
        UE_LOG(LogCkPixelArt, Error,
            TEXT("r.ScreenPercentage rejected the renderer's value (asked %.6f, kept %.6f) — something set it at ")
            TEXT("a higher priority than ECVF_SetByCode (console or command line). The scene will NOT render at ")
            TEXT("the internal resolution until that is cleared."),
            Fraction * 100.0f, AppliedPercentage);
    }
}
