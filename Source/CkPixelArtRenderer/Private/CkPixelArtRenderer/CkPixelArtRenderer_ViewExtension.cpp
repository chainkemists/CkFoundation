#include "CkPixelArtRenderer/CkPixelArtRenderer_ViewExtension.h"

#include "CkPixelArtRenderer/CkPixelArtRenderer_CVars.h"
#include "CkPixelArtRenderer/CkPixelArtRenderer_Log.h"
#include "CkPixelArtRenderer/CkPixelArtRenderer_SnapMath.h"
#include "CkPixelArtRenderer/CkPixelArtRenderer_Upscaler.h"
#include "CkPixelArtRenderer/CkPixelArtRenderer_Utils.h"

#include "CoreGlobals.h"
#include <Engine/GameViewportClient.h>
#include <Engine/World.h>
#include <HAL/IConsoleManager.h>
#include <Misc/CoreDelegates.h>
#include <SceneView.h>
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

    // Unset means "do not run here". Only an explicit Enabled brings the renderer into existence, mirroring the
    // Stylize rule - otherwise a tuning override would start pixelating every unconfigured world in the session.
    auto
        TryGet_EffectiveConfig(
            const UWorld* InWorld)
        -> TOptional<FCk_PixelArt_RenderConfig>
    {
        const auto Stored = FCk_PixelArtRenderer_StateRegistry::TryGet(InWorld);

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

    // Render-target IDENTITY, not a list of family kinds: scene captures, reflection captures and a sky light's
    // cubemap faces each arrive with their own target, and driving the GLOBAL screen percentage off one of them
    // stamps that value over the real viewport's for the frame.
    auto
        Get_IsPrimaryViewportFamily(
            const FSceneViewFamily& InViewFamily,
            const UWorld* InWorld)
        -> bool
    {
        if (InWorld == nullptr || InWorld->GetGameViewport() == nullptr)
        { return false; }

        const auto* GameViewport = InWorld->GetGameViewport()->Viewport;

        return GameViewport != nullptr && InViewFamily.RenderTarget == GameViewport;
    }

    // One fraction scales both axes, so only one can be exact. Width is the driven axis - the fraction is derived
    // from the target width so CeilToInt lands on it exactly, and the height follows within a texel.
    auto
        Get_TargetWidth(
            int32 InInternalHeight,
            const FIntPoint& InViewportSize)
        -> int32
    {
        const auto Aspect = static_cast<double>(InViewportSize.X) / static_cast<double>(InViewportSize.Y);
        return FMath::RoundToInt(InInternalHeight * Aspect);
    }

    // In TexelsPerPixel mode this falls out of the window instead of being authored, which is the whole difference
    // between the two modes.
    auto
        Get_InnerHeight(
            const FCk_PixelArt_RenderConfig& InConfig,
            const FIntPoint& InViewportSize)
        -> int32
    {
        if (InConfig.ResolutionMode == ECk_PixelArt_ResolutionMode::FixedHeight)
        { return InConfig.InternalHeight; }

        const auto TexelsPerPixel = FMath::Max(1, InConfig.TexelsPerPixel);

        return FMath::Max(1, InViewportSize.Y / TexelsPerPixel);
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

    // A capture family must not TOUCH the per-frame state, not merely skip the drive: deferred scene captures
    // render INSIDE BeginRenderingViewFamilies, between the game viewport's SetupViewFamily and its
    // BeginRenderViewFamily — so resetting the state here would silently drop the viewport's upscaler on every
    // frame a bCaptureEveryFrame capture is alive. Leaving the lease unrenewed is safe: the game viewport's own
    // family renews it in the same frame.
    if (!ck_pixel_art_view_extension::Get_IsPrimaryViewportFamily(InViewFamily, World))
    { return; }

    auto Config = ck_pixel_art_view_extension::TryGet_EffectiveConfig(World);

    _FrameConfig.Reset();
    _FrameReport.Reset();
    _FrameWorld = World;

    if (!Config.IsSet() || !Config->Enabled)
    { return; }

    const auto FixedHeight = Config->ResolutionMode == ECk_PixelArt_ResolutionMode::FixedHeight;
    const auto ResolutionIsUsable = FixedHeight ? Config->InternalHeight > 0 : Config->TexelsPerPixel > 0;

    // ensureMsgf, not CK_ENSURE_IF_NOT - this module cannot link CkCore at PostConfigInit.
    if (!ensureMsgf(ResolutionIsUsable,
        TEXT("CkPixelArt: %s is [%d], which cannot describe a resolution. The renderer will not run for this ")
        TEXT("world until it is positive."),
        FixedHeight ? TEXT("InternalHeight") : TEXT("TexelsPerPixel"),
        FixedHeight ? Config->InternalHeight : Config->TexelsPerPixel))
    { return; }

    _FrameConfig = Config;

    DoApply_ScreenPercentage(InViewFamily, *Config);
}

auto
    FCk_PixelArt_ViewExtension::
    SetupViewProjectionMatrix(
        FSceneViewProjectionData& InOutProjectionData)
    -> void
{
    if (!_FrameConfig.IsSet() || !_FrameReport.IsSet())
    { return; }

    auto& Report = *_FrameReport;

    const auto GeometryIsUsable =
        Report.InnerSizeTexels.X > 0 && Report.InnerSizeTexels.Y > 0 &&
        Report.RenderSize.X > 0 && Report.RenderSize.Y > 0;

    if (!GeometryIsUsable)
    { return; }

    // The world width the CAMERA framed, read BEFORE the fold: the fold widens world span and texel count by the
    // same ratio, so this is the mapping the snap must align to and the fold leaves it alone by construction.
    const auto AuthoredOrthoWidth =
        ck::pixel_art::Get_OrthoWidthFromProjection(InOutProjectionData.ProjectionMatrix);

    ck::pixel_art::Apply_MarginFold(
        InOutProjectionData.ProjectionMatrix, Report.InnerSizeTexels, Report.RenderSize);

    // Orthographic-only by construction: one world-space snap displaces near and far geometry by different screen
    // amounts, so no single snap corrects all depths. A perspective view is legitimate, just not stabilized.
    if (InOutProjectionData.IsPerspectiveProjection())
    { return; }

    const auto Overrides = ck::pixel_art::Get_DebugSnapOverrides();

    if (!_FrameConfig->SnapEnabled)
    {
        _FrozenViewOrigin.Reset();
        return;
    }

    if (Overrides.FreezeSnap)
    {
        if (!_FrozenViewOrigin.IsSet())
        { _FrozenViewOrigin = InOutProjectionData.ViewOrigin; }

        InOutProjectionData.ViewOrigin = *_FrozenViewOrigin;
        return;
    }

    _FrozenViewOrigin.Reset();

    const auto TexelWorldSize = AuthoredOrthoWidth / Report.InnerSizeTexels.X;

    if (TexelWorldSize <= 0.0)
    { return; }

    auto RemainderTexels = FVector2f::ZeroVector;

    InOutProjectionData.ViewOrigin = ck::pixel_art::Get_SnappedViewOrigin(
        InOutProjectionData.ViewOrigin,
        InOutProjectionData.ViewRotationMatrix,
        TexelWorldSize,
        RemainderTexels);

    Report.TexelWorldSize = TexelWorldSize;
    Report.SnappedViewOrigin = InOutProjectionData.ViewOrigin;
    Report.RemainderTexels = Overrides.SnapOnly ? FVector2f::ZeroVector : RemainderTexels;
    Report.SnapApplied = true;

    FCk_PixelArtRenderer_StateRegistry::Set_FrameReport(_FrameWorld.Get(), Report);
}

auto
    FCk_PixelArt_ViewExtension::
    BeginRenderViewFamily(
        FSceneViewFamily& InViewFamily)
    -> void
{
    if (!_FrameConfig.IsSet() || !_FrameReport.IsSet() || InViewFamily.Views.Num() == 0)
    { return; }

    // Only the game viewport's family may consume the frame state. A capture family with persistent view state
    // (bCaptureEveryFrame allocates one) would otherwise pick up the viewport's geometry and register an
    // upscaler whose margins and remainder describe a different target entirely.
    if (!ck_pixel_art_view_extension::Get_IsPrimaryViewportFamily(InViewFamily, _FrameWorld.Get()))
    { return; }

    // Stereo and other stateless special views bypass the projection hook the snap lives in.
    const auto* PrimaryView = InViewFamily.Views[0];

    if (PrimaryView == nullptr || PrimaryView->State == nullptr)
    { return; }

    // Anything but 1 means the window gets a second, engine-owned resample after ours - the reason every visual
    // verdict for this renderer is taken standalone rather than in PIE. The drive divides the settled value out
    // from the NEXT frame on, so the internal resolution is exact again once this value is stable.
    if (!FMath::IsNearlyEqual(_LastSecondaryViewFraction, InViewFamily.SecondaryViewFraction))
    {
        _LastSecondaryViewFraction = InViewFamily.SecondaryViewFraction;

        UE_LOG(CkPixelArtRenderer, Display,
            TEXT("Secondary view fraction settled at %.4f. Below 1 the engine inserts a DPI-driven secondary ")
            TEXT("upscale after ours; the screen-percentage drive compensates for it from the next frame."),
            _LastSecondaryViewFraction);
    }

    // TSR and TAAU switch the view to temporal upscaling and the primary spatial slot is never enabled, so this
    // renderer stops running with nothing on screen or in the log to say why. Report it rather than look like our bug.
    const auto UpscaleSlotIsLive =
        PrimaryView->PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::SpatialUpscale;

    if (UpscaleSlotIsLive != _LastUpscaleSlotWasLive)
    {
        _LastUpscaleSlotWasLive = UpscaleSlotIsLive;

        if (!UpscaleSlotIsLive)
        {
            UE_LOG(CkPixelArtRenderer, Warning,
                TEXT("The primary spatial upscale slot is not available (anti-aliasing method [%d] selected ")
                TEXT("temporal upscaling), so the pixel-art upscaler will NOT run. Set r.AntiAliasingMethod to ")
                TEXT("0 (None) or 1 (FXAA)."),
                static_cast<int32>(PrimaryView->AntiAliasingMethod));
        }
        else
        {
            UE_LOG(CkPixelArtRenderer, Display, TEXT("The primary spatial upscale slot is available again."));
        }
    }

    if (!UpscaleSlotIsLive)
    { return; }

    // The renderer null-checks this slot before calling any extension, so non-null means another extension claimed
    // it first and assigning anyway would fire the engine's checkf.
    if (InViewFamily.GetPrimarySpatialUpscalerInterface() != nullptr)
    { return; }

    const auto& Report = *_FrameReport;

    // An older frame number means the projection hook did not run this frame (stereo, or a non-local-player view).
    // Reusing the previous remainder would smear by up to a texel, so render snapped but uncompensated instead.
    const auto SnapIsFromThisFrame = Report.SnapApplied && Report.FrameNumber == GFrameCounter;

    const auto CompensationSign = ck::pixel_art::Get_DebugSnapOverrides().CompensationSign;

    auto Frame = FCk_PixelArt_UpscaleFrame{};
    Frame.RenderSize = Report.RenderSize;
    Frame.InnerOffsetTexels = Report.InnerOffsetTexels;
    Frame.InnerSizeTexels = Report.InnerSizeTexels;
    Frame.FilterMode = _FrameConfig->FilterMode;

    // Sampling further right moves the picture LEFT, matching a camera moving right, so the horizontal remainder
    // goes in as-is. The vertical one is negated because V grows downward.
    if (SnapIsFromThisFrame)
    {
        Frame.SubTexelOffsetTexels = FVector2f{
            Report.RemainderTexels.X * CompensationSign,
            -Report.RemainderTexels.Y * CompensationSign};
    }

    // The view family owns and deletes this - allocate one per frame, never cache it, never delete it. It is also
    // the whole game-thread-to-render-thread transport for the frame's snap state.
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
        // Write at the CURRENT priority so the write cannot be dropped, then restore the original priority bits.
        // Writing straight at the saved priority is refused whenever the saved one is weaker, which is the normal case.
        const auto CurrentPriority =
            static_cast<EConsoleVariableFlags>(ScreenPercentageCVar->GetFlags() & ECVF_SetByMask);

        ScreenPercentageCVar->Set(*_SavedScreenPercentage, CurrentPriority);

        ScreenPercentageCVar->SetFlags(static_cast<EConsoleVariableFlags>(
            (ScreenPercentageCVar->GetFlags() & ~ECVF_SetByMask) | _SavedScreenPercentagePriority));

        if (ck::pixel_art::Get_LogStateEnabled())
        {
            UE_LOG(CkPixelArtRenderer, Display, TEXT("Released: restored r.ScreenPercentage=%.4f"),
                *_SavedScreenPercentage);
        }
    }

    _SavedScreenPercentage.Reset();
    _LastViewportSize = FIntPoint::ZeroValue;
    _LastFraction = 0.0f;
    _FrozenViewOrigin.Reset();
}

auto
    FCk_PixelArt_ViewExtension::
    DoOn_EndFrame()
    -> void
{
    if (!_LeaseRenewedThisFrame)
    { Request_ReleaseScreenPercentage(); }

    _LeaseRenewedThisFrame = false;
    _FrameConfig.Reset();
    _FrameReport.Reset();
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
        UE_LOG(CkPixelArtRenderer, Error,
            TEXT("r.ScreenPercentage does not exist — the scene cannot be driven to the internal resolution and ")
            TEXT("the upscaler would run on a full-resolution image. The renderer does nothing this frame."));
        return;
    }

    // Saved on the way IN, priority included. A console variable remembers who set it last and drops a write from a
    // weaker source, so restoring the number at ECVF_SetByCode would pin r.ScreenPercentage there for the process -
    // silently killing the resolution-quality slider, which writes at the weaker ECVF_SetByScalability.
    if (!_SavedScreenPercentage.IsSet())
    {
        _SavedScreenPercentage = ScreenPercentageCVar->GetFloat();
        _SavedScreenPercentagePriority =
            static_cast<EConsoleVariableFlags>(ScreenPercentageCVar->GetFlags() & ECVF_SetByMask);
    }

    _LeaseRenewedThisFrame = true;

    const auto Aspect = static_cast<double>(ViewportSize.X) / static_cast<double>(ViewportSize.Y);

    // Clamped to what the viewport can hold, margin included. This renderer only ever downscales, so an internal
    // height larger than the window leaves an inner rect with no margin. A small window bends rather than refuses.
    const auto RequestedHeight = ck_pixel_art_view_extension::Get_InnerHeight(InConfig, ViewportSize);
    const auto MaxHeightForViewport = FMath::Max(1, ViewportSize.Y - 2 * FMath::Max(0, InConfig.MarginTexels));

    const auto InnerHeight = FMath::Min(RequestedHeight, MaxHeightForViewport);
    const auto InnerWidth = ck_pixel_art_view_extension::Get_TargetWidth(InnerHeight, ViewportSize);

    // One fraction scales both axes, so extra width buys proportionally fewer rows. Widening horizontally until the
    // vertical fallout still reaches the requested margin is what makes the guarantee hold at any aspect ratio.
    const auto MarginTexels = FMath::Max(0, InConfig.MarginTexels);
    const auto HorizontalMargin = ck::pixel_art::Get_HorizontalMarginTexels(
        InnerWidth, InnerHeight, MarginTexels, Aspect);

    const auto RenderedWidth = InnerWidth + 2 * HorizontalMargin;

    // The family's own SecondaryViewFraction still holds the default 1 at this hook - the engine assigns the
    // DPI-derived value only afterwards. Last frame's settled value (read in BeginRenderViewFamily) is the
    // prediction instead: exact whenever the DPI and the secondary-percentage CVar are stable, and a change
    // heals itself one frame later, with the upscaler warning about the single mismatched frame.
    const auto SecondaryViewFraction = _LastSecondaryViewFraction > 0.0f ? _LastSecondaryViewFraction : 1.0f;

    const auto FractionForWidth =
        UCk_Utils_PixelArtRenderer_UE::Get_ExactFraction(RenderedWidth, ViewportSize.X);

    // Divided out because the renderer multiplies it back in when it derives the view rect.
    const auto Fraction = SecondaryViewFraction > 0.0f ? FractionForWidth / SecondaryViewFraction : FractionForWidth;

    ScreenPercentageCVar->Set(Fraction * 100.0f, ECVF_SetByCode);

    const auto RenderSize = ck_pixel_art_view_extension::Get_PredictedInternalSize(
        ViewportSize, Fraction, SecondaryViewFraction);

    // Not chosen - it is what the engine's rounding leaves once the horizontal margin is fixed. An odd surplus
    // splits towards the top so the displayed origin stays deterministic.
    const auto SurplusRows = RenderSize.Y - InnerHeight;
    const auto TopInset = FMath::Max(0, SurplusRows / 2);

    auto Report = FCk_PixelArt_FrameReport{};
    Report.FrameNumber = GFrameCounter;
    Report.RenderSize = RenderSize;
    Report.InnerOffsetTexels = FIntPoint{HorizontalMargin, TopInset};
    Report.InnerSizeTexels = FIntPoint{InnerWidth, FMath::Min(InnerHeight, RenderSize.Y - TopInset)};

    _FrameReport = Report;

    // Published before the snap is known so a same-frame reader finds the geometry; the projection hook republishes.
    FCk_PixelArtRenderer_StateRegistry::Set_FrameReport(_FrameWorld.Get(), Report);

    // BEFORE the geometry early-out: a console or command-line r.ScreenPercentage outranks ECVF_SetByCode and drops
    // our write silently. That can start at any moment, including while the geometry is perfectly stable.
    const auto AppliedPercentage = ScreenPercentageCVar->GetFloat();

    if (!FMath::IsNearlyEqual(AppliedPercentage, Fraction * 100.0f, 1e-3f))
    {
        UE_LOG(CkPixelArtRenderer, Error,
            TEXT("r.ScreenPercentage rejected the renderer's value (asked %.6f, kept %.6f) — something set it at ")
            TEXT("a higher priority than ECVF_SetByCode (console or command line). The scene will NOT render at ")
            TEXT("the internal resolution until that is cleared."),
            Fraction * 100.0f, AppliedPercentage);
    }

    const auto GeometryIsUnchanged =
        _LastViewportSize == ViewportSize && FMath::IsNearlyEqual(_LastFraction, Fraction);

    if (GeometryIsUnchanged)
    { return; }

    _LastViewportSize = ViewportSize;
    _LastFraction = Fraction;

    // The margin absorbs the half-texel snap shift and the box filter's footprint. Less than a texel on any side and
    // the shifted window reads texels that were never rendered, which looks like a shader bug and is not one.
    const auto BottomInset = RenderSize.Y - TopInset - Report.InnerSizeTexels.Y;
    const auto EverySideHasMargin = HorizontalMargin >= 1 && TopInset >= 1 && BottomInset >= 1;

    if (InnerHeight != RequestedHeight)
    {
        UE_LOG(CkPixelArtRenderer, Warning,
            TEXT("Internal height %d does not fit a %dx%d viewport with a %d-texel margin; rendering at %d ")
            TEXT("instead. The texel grid is coarser than configured until the window grows."),
            RequestedHeight, ViewportSize.X, ViewportSize.Y, MarginTexels, InnerHeight);
    }

    if (MarginTexels > 0 && !EverySideHasMargin)
    {
        UE_LOG(CkPixelArtRenderer, Error,
            TEXT("The render margin came out as %d horizontal, %d top, %d bottom for a %dx%d render of a %dx%d ")
            TEXT("window — less than one texel on some side. The snap compensation will sample outside the ")
            TEXT("rendered image and the borders will smear. If the render is SMALLER than the window the ")
            TEXT("cause is an internal resolution at or above the viewport, not the margin; otherwise raise ")
            TEXT("ck.PixelArt.Margin."),
            HorizontalMargin, TopInset, BottomInset,
            RenderSize.X, RenderSize.Y, InnerWidth, InnerHeight);
    }

    if (ck::pixel_art::Get_LogStateEnabled())
    {
        UE_LOG(CkPixelArtRenderer, Display,
            TEXT("Active: viewport=%dx%d rendered=%dx%d displayed=%dx%d at (%d,%d) margin=%d/%d/%d ")
            TEXT("fraction=%.6f applied=%.6f secondary=%.4f"),
            ViewportSize.X, ViewportSize.Y,
            RenderSize.X, RenderSize.Y,
            Report.InnerSizeTexels.X, Report.InnerSizeTexels.Y,
            Report.InnerOffsetTexels.X, Report.InnerOffsetTexels.Y,
            HorizontalMargin, TopInset, BottomInset,
            Fraction, AppliedPercentage / 100.0f, SecondaryViewFraction);
    }

}
