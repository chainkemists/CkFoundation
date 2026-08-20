#include "CkPixelArtRender/CkPixelArtRender_ViewExtension.h"

#include "CkPixelArtRender/CkPixelArtRender_Log.h"
#include "CkPixelArtRender/CkPixelArtRender_Upscaler.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "SceneView.h"
#include "UnrealClient.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_view_extension
{
    constexpr auto DisplayedWidth = 640;
    constexpr auto DisplayedHeight = 360;
    constexpr auto MarginTexels = 2;

    // The scene is rendered wider than the window that gets displayed, so shifting the sampling window by the
    // camera's sub-texel snap remainder never reads texels that were not rendered.
    constexpr auto RenderedWidth = DisplayedWidth + 2 * MarginTexels;
    constexpr auto RenderedHeight = DisplayedHeight + 2 * MarginTexels;

    static_assert(RenderedWidth > 2 * MarginTexels && RenderedHeight > 2 * MarginTexels,
        "The render margin must leave a non-empty displayed window on both axes");

    TAutoConsoleVariable<int32> CVar_Spike(
        TEXT("ck.PixelArt.Spike"),
        0,
        TEXT("Drives the screen percentage down to a fixed internal resolution and replaces the engine's primary\n")
        TEXT("spatial upscale with the pixel-art box filter.\n")
        TEXT("Requires an anti-aliasing method of None or FXAA: TSR/TAAU switch the view to temporal upscaling,\n")
        TEXT("which structurally disables the spatial upscale slot and this renderer silently stops running.\n")
        TEXT(" 0: off (default)\n")
        TEXT(" 1: on"),
        ECVF_Cheat);

    auto
        Get_ScreenPercentageCVar()
        -> IConsoleVariable*
    {
        static auto* ScreenPercentageCVar =
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));

        return ScreenPercentageCVar;
    }

    // Fraction the engine must apply so CeilToInt lands EXACTLY on InTargetWidth. The renderer computes the view
    // size as CeilToInt(UnscaledViewSize * Primary * Secondary), so subtracting half a pixel before dividing keeps
    // the product just under the target and immune to float error, and the secondary fraction has to be divided
    // out here because it multiplies back in there.
    auto
        Get_ExactResolutionFraction(
            int32 InTargetWidth,
            int32 InViewportWidth,
            float InSecondaryViewFraction)
        -> float
    {
        return (InTargetWidth - 0.5f) / (InViewportWidth * InSecondaryViewFraction);
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
    ck_pixel_art_view_extension::CVar_Spike.AsVariable()->SetOnChangedCallback(
        FConsoleVariableDelegate::CreateLambda([this](IConsoleVariable* InVariable)
        {
            if (InVariable->GetInt() == 0)
            { Request_RestoreScreenPercentage(); }
        }));
}

FCk_PixelArt_ViewExtension::
    ~FCk_PixelArt_ViewExtension()
{
    ck_pixel_art_view_extension::CVar_Spike.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate{});
    Request_RestoreScreenPercentage();
}

auto
    FCk_PixelArt_ViewExtension::
    SetupViewFamily(
        FSceneViewFamily& InViewFamily)
    -> void
{
    DoApply_ScreenPercentage(InViewFamily);
}

auto
    FCk_PixelArt_ViewExtension::
    BeginRenderViewFamily(
        FSceneViewFamily& InViewFamily)
    -> void
{
    if (InViewFamily.Views.Num() == 0)
    { return; }

    // Scene captures and reflection captures have no view state and are explicitly out of scope: they bypass the
    // projection hook the camera snap will live in, so pixelating them would be a different, unsnapped image.
    const auto* PrimaryView = InViewFamily.Views[0];
    if (PrimaryView == nullptr || PrimaryView->State == nullptr)
    { return; }

    // The renderer checks this slot is null before it calls any extension, so a non-null value here means another
    // extension claimed it first. Assigning anyway would fire the engine's checkf.
    if (InViewFamily.GetPrimarySpatialUpscalerInterface() != nullptr)
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

    auto Frame = FCk_PixelArt_UpscaleFrame{};
    Frame.InternalSize = _LastInternalSize;
    Frame.MarginTexels = ck_pixel_art_view_extension::MarginTexels;

    // The view family owns this instance and deletes it in its destructor — allocate one per frame, never cache it
    // and never delete it. It is also the whole game-thread-to-render-thread transport for the frame's snap state.
    InViewFamily.SetPrimarySpatialUpscalerInterface(new FCk_PixelArt_SpatialUpscaler(Frame));
}

auto
    FCk_PixelArt_ViewExtension::
    IsActiveThisFrame_Internal(
        const FSceneViewExtensionContext& InContext) const
    -> bool
{
    if (IsRunningDedicatedServer())
    { return false; }

    if (ck_pixel_art_view_extension::CVar_Spike.GetValueOnGameThread() == 0)
    { return false; }

    const auto* World = InContext.GetWorld();

    return World != nullptr && World->IsGameWorld();
}

auto
    FCk_PixelArt_ViewExtension::
    Request_RestoreScreenPercentage()
    -> void
{
    if (!_SavedScreenPercentage.IsSet())
    { return; }

    auto* ScreenPercentageCVar = ck_pixel_art_view_extension::Get_ScreenPercentageCVar();
    if (ScreenPercentageCVar != nullptr)
    {
        ScreenPercentageCVar->Set(*_SavedScreenPercentage, ECVF_SetByCode);

        UE_LOG(LogCkPixelArt, Display, TEXT("Spike inactive: restored r.ScreenPercentage=%.4f"),
            *_SavedScreenPercentage);
    }

    _SavedScreenPercentage.Reset();
    _LastViewportSize = FIntPoint::ZeroValue;
    _LastInternalSize = FIntPoint::ZeroValue;
    _LastFraction = 0.0f;
}

auto
    FCk_PixelArt_ViewExtension::
    DoApply_ScreenPercentage(
        const FSceneViewFamily& InViewFamily)
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
            TEXT("the upscaler would run on a full-resolution image. Spike does nothing this frame."));
        return;
    }

    // Read on the way IN, not at construction: whatever the value is the first time the spike drives it is what
    // gets put back on disable.
    if (!_SavedScreenPercentage.IsSet())
    { _SavedScreenPercentage = ScreenPercentageCVar->GetFloat(); }

    // At this point in UGameViewportClient::Draw the secondary fraction has not been computed yet, so this reads
    // the family default of 1. It is correct in standalone at 100% DPI; BeginRenderViewFamily logs the value the
    // engine actually settled on so a PIE/DPI divergence names itself instead of showing up as a blurry image.
    const auto SecondaryViewFraction = InViewFamily.SecondaryViewFraction;

    const auto Fraction = ck_pixel_art_view_extension::Get_ExactResolutionFraction(
        ck_pixel_art_view_extension::RenderedWidth, ViewportSize.X, SecondaryViewFraction);

    ScreenPercentageCVar->Set(Fraction * 100.0f, ECVF_SetByCode);

    const auto PredictedInternalSize = ck_pixel_art_view_extension::Get_PredictedInternalSize(
        ViewportSize, Fraction, SecondaryViewFraction);

    if (_LastViewportSize == ViewportSize && FMath::IsNearlyEqual(_LastFraction, Fraction))
    {
        _LastInternalSize = PredictedInternalSize;
        return;
    }

    _LastViewportSize = ViewportSize;
    _LastInternalSize = PredictedInternalSize;
    _LastFraction = Fraction;

    // r.ScreenPercentage set by console or command line outranks ECVF_SetByCode, in which case our write is
    // dropped silently and the scene renders at the wrong resolution. Read it back so that failure is visible.
    const auto AppliedPercentage = ScreenPercentageCVar->GetFloat();

    UE_LOG(LogCkPixelArt, Display,
        TEXT("Spike active: viewport=%dx%d rendered=%dx%d displayed=%dx%d fraction=%.6f applied=%.6f secondary=%.4f"),
        ViewportSize.X, ViewportSize.Y,
        PredictedInternalSize.X, PredictedInternalSize.Y,
        PredictedInternalSize.X - 2 * ck_pixel_art_view_extension::MarginTexels,
        PredictedInternalSize.Y - 2 * ck_pixel_art_view_extension::MarginTexels,
        Fraction, AppliedPercentage / 100.0f, SecondaryViewFraction);

    if (!FMath::IsNearlyEqual(AppliedPercentage, Fraction * 100.0f, 1e-3f))
    {
        UE_LOG(LogCkPixelArt, Error,
            TEXT("r.ScreenPercentage rejected the spike's value (asked %.6f, kept %.6f) — something set it at a ")
            TEXT("higher priority than ECVF_SetByCode (console or command line). The scene will NOT render at the ")
            TEXT("internal resolution until that is cleared."),
            Fraction * 100.0f, AppliedPercentage);
    }
}
