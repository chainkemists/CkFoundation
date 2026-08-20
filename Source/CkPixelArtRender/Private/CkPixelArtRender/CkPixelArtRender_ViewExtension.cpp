#include "CkPixelArtRender/CkPixelArtRender_ViewExtension.h"

#include "CkPixelArtRender/CkPixelArtRender_CVars.h"
#include "CkPixelArtRender/CkPixelArtRender_Log.h"
#include "CkPixelArtRender/CkPixelArtRender_SnapMath.h"
#include "CkPixelArtRender/CkPixelArtRender_Upscaler.h"
#include "CkPixelArtRender/CkPixelArtRender_Utils.h"

#include "CoreGlobals.h"
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
    _FrameReport.Reset();
    _FrameWorld = World;

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

    // The margin fold, applied to perspective and orthographic views alike: the scene is being rasterized into
    // more texels than are displayed, so the projection has to cover proportionally more world or the extra
    // texels would come out of the framing instead of being added to it.
    //
    // Horizontal first, because width is the axis the screen percentage lands exactly. The vertical term is then
    // DERIVED from the rendered aspect rather than scaled by the same factor: the engine takes CeilToInt of the
    // rendered height, so scaling both by one number would leave texels a fraction of a percent off square, and
    // square texels are the whole premise of the look.
    const auto HorizontalScale =
        static_cast<double>(Report.InnerSizeTexels.X) / static_cast<double>(Report.RenderSize.X);

    // Read before the fold: this is the world width the CAMERA framed, spread over the DISPLAYED texels, which
    // is the mapping the player sees and therefore the one the snap has to align to. The fold widens the world
    // span and the texel count by the same ratio, so it leaves this number alone by construction.
    const auto AuthoredOrthoWidth =
        ck::pixel_art::Get_OrthoWidthFromProjection(InOutProjectionData.ProjectionMatrix);

    InOutProjectionData.ProjectionMatrix.M[0][0] *= HorizontalScale;
    InOutProjectionData.ProjectionMatrix.M[1][1] =
        InOutProjectionData.ProjectionMatrix.M[0][0] *
        (static_cast<double>(Report.RenderSize.X) / static_cast<double>(Report.RenderSize.Y));

    // No ensure: a perspective view is a legitimate state that the look simply is not stabilized for. One
    // world-space snap displaces near and far geometry by different screen amounts, so there is no snap that
    // corrects all depths — the technique is orthographic-only by construction, not by omission.
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

    FCk_PixelArtRender_StateRegistry::Set_FrameReport(_FrameWorld.Get(), Report);
}

auto
    FCk_PixelArt_ViewExtension::
    BeginRenderViewFamily(
        FSceneViewFamily& InViewFamily)
    -> void
{
    if (!_FrameConfig.IsSet() || !_FrameReport.IsSet() || InViewFamily.Views.Num() == 0)
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

    const auto& Report = *_FrameReport;

    // A report stamped with an older frame means the projection hook did not run this frame — a stereo path, or a
    // view that is not a local player's. Compensating with the previous frame's remainder would smear the image
    // by up to a texel in an unpredictable direction, so the frame renders snapped but uncompensated instead.
    const auto SnapIsFromThisFrame = Report.SnapApplied && Report.FrameNumber == GFrameCounter;

    const auto CompensationSign = ck::pixel_art::Get_DebugSnapOverrides().CompensationSign;

    auto Frame = FCk_PixelArt_UpscaleFrame{};
    Frame.RenderSize = Report.RenderSize;
    Frame.InnerOffsetTexels = Report.InnerOffsetTexels;
    Frame.InnerSizeTexels = Report.InnerSizeTexels;
    Frame.FilterMode = _FrameConfig->FilterMode;

    // Sampling further right in the source puts content that was further right at the current screen position,
    // which moves the picture LEFT — the same direction the camera moving right would have moved it. So the
    // horizontal remainder goes in as-is. The vertical one is negated because V grows downward while the
    // remainder is expressed in the world's sense of up.
    if (SnapIsFromThisFrame)
    {
        Frame.SubTexelOffsetTexels = FVector2f{
            Report.RemainderTexels.X * CompensationSign,
            -Report.RemainderTexels.Y * CompensationSign};
    }

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
    _LastFraction = 0.0f;
    _FrozenViewOrigin.Reset();
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

    const auto Aspect = static_cast<double>(ViewportSize.X) / static_cast<double>(ViewportSize.Y);
    const auto InnerWidth = ck_pixel_art_view_extension::Get_TargetWidth(InConfig.InternalHeight, ViewportSize);
    const auto InnerHeight = InConfig.InternalHeight;

    // A margin asked for in texels cannot be spent evenly on both axes — one fraction scales both, so extra width
    // buys proportionally fewer extra rows. Widening horizontally until the vertical fallout still reaches the
    // requested margin is what makes the guarantee hold at any aspect ratio rather than only at 16:9.
    const auto MarginTexels = FMath::Max(0, InConfig.MarginTexels);
    const auto HorizontalMargin = ck::pixel_art::Get_HorizontalMarginTexels(
        InnerWidth, InnerHeight, MarginTexels, Aspect);

    const auto RenderedWidth = InnerWidth + 2 * HorizontalMargin;

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

    const auto RenderSize = ck_pixel_art_view_extension::Get_PredictedInternalSize(
        ViewportSize, Fraction, SecondaryViewFraction);

    // The vertical margin is not chosen, it is what the engine's rounding leaves once the horizontal one is
    // fixed. Splitting an odd surplus towards the top keeps the displayed window's origin deterministic; the
    // extra row goes to the bottom.
    const auto SurplusRows = RenderSize.Y - InnerHeight;
    const auto TopInset = FMath::Max(0, SurplusRows / 2);

    auto Report = FCk_PixelArt_FrameReport{};
    Report.FrameNumber = GFrameCounter;
    Report.RenderSize = RenderSize;
    Report.InnerOffsetTexels = FIntPoint{HorizontalMargin, TopInset};
    Report.InnerSizeTexels = FIntPoint{InnerWidth, FMath::Min(InnerHeight, RenderSize.Y - TopInset)};

    _FrameReport = Report;

    // Published before the snap is known so a same-frame reader always finds the geometry; the projection hook
    // republishes with the snap once it has one.
    FCk_PixelArtRender_StateRegistry::Set_FrameReport(_FrameWorld.Get(), Report);

    const auto GeometryIsUnchanged =
        _LastViewportSize == ViewportSize && FMath::IsNearlyEqual(_LastFraction, Fraction);

    if (GeometryIsUnchanged)
    { return; }

    _LastViewportSize = ViewportSize;
    _LastFraction = Fraction;

    // The margin exists to absorb the half-texel snap shift and the box filter's own footprint. If the rounding
    // above left less than a texel of it on any side, the shifted sampling window reads texels that were never
    // rendered and the borders smear — loudly, because it looks like a shader bug and is not one.
    const auto BottomInset = RenderSize.Y - TopInset - Report.InnerSizeTexels.Y;
    const auto EverySideHasMargin = HorizontalMargin >= 1 && TopInset >= 1 && BottomInset >= 1;

    if (MarginTexels > 0 && !EverySideHasMargin)
    {
        UE_LOG(LogCkPixelArt, Error,
            TEXT("The render margin came out as %d horizontal, %d top, %d bottom for a %dx%d render of a %dx%d ")
            TEXT("window — less than one texel on some side. The snap compensation will sample outside the ")
            TEXT("rendered image and the borders will smear. Raise ck.PixelArt.Margin."),
            HorizontalMargin, TopInset, BottomInset,
            RenderSize.X, RenderSize.Y, InnerWidth, InnerHeight);
    }

    // r.ScreenPercentage set by console or command line outranks ECVF_SetByCode, in which case our write is
    // dropped silently and the scene renders at the wrong resolution. Read it back so that failure is visible.
    const auto AppliedPercentage = ScreenPercentageCVar->GetFloat();

    if (ck::pixel_art::Get_LogStateEnabled())
    {
        UE_LOG(LogCkPixelArt, Display,
            TEXT("Active: viewport=%dx%d rendered=%dx%d displayed=%dx%d at (%d,%d) margin=%d/%d/%d ")
            TEXT("fraction=%.6f applied=%.6f secondary=%.4f"),
            ViewportSize.X, ViewportSize.Y,
            RenderSize.X, RenderSize.Y,
            Report.InnerSizeTexels.X, Report.InnerSizeTexels.Y,
            Report.InnerOffsetTexels.X, Report.InnerOffsetTexels.Y,
            HorizontalMargin, TopInset, BottomInset,
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
