#include "CkPixelArtRender/CkPixelArtRender_Upscaler.h"

#include "CkPixelArtRender/CkPixelArtRender_UpscaleShader.h"
#include "CkPixelArtRender/CkPixelArtRender_Log.h"

#include "RenderUtils.h"
#include "SceneRendering.h"
#include "ScreenPass.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_upscaler
{
    // The pass runs every frame; the breadcrumb is only interesting when the geometry changes. Render thread only.
    struct FLastLoggedGeometry
    {
        FIntPoint Input = FIntPoint::ZeroValue;
        FIntPoint Output = FIntPoint::ZeroValue;
        int32 Stage = INDEX_NONE;
    };

    FLastLoggedGeometry GLastLoggedGeometry = {};

    auto
        Get_StageName(
            EUpscaleStage InStage)
        -> const TCHAR*
    {
        switch (InStage)
        {
            case EUpscaleStage::PrimaryToSecondary: return TEXT("PrimaryToSecondary");
            case EUpscaleStage::PrimaryToOutput:    return TEXT("PrimaryToOutput");
            case EUpscaleStage::SecondaryToOutput:  return TEXT("SecondaryToOutput");
            default:                                return TEXT("Unknown");
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_PixelArt_SpatialUpscaler::
    FCk_PixelArt_SpatialUpscaler(
        const FCk_PixelArt_UpscaleFrame& InFrame)
    : _Frame(InFrame)
{
}

auto
    FCk_PixelArt_SpatialUpscaler::
    GetDebugName() const
    -> const TCHAR*
{
    return TEXT("CkPixelArt");
}

auto
    FCk_PixelArt_SpatialUpscaler::
    Fork_GameThread(
        const FSceneViewFamily& InViewFamily) const
    -> ISpatialUpscaler*
{
    // The forked family takes ownership of this and deletes it in its destructor, exactly like the instance
    // BeginRenderViewFamily allocated. Never delete either one by hand.
    return new FCk_PixelArt_SpatialUpscaler(_Frame);
}

auto
    FCk_PixelArt_SpatialUpscaler::
    AddPasses(
        FRDGBuilder& InGraphBuilder,
        const FViewInfo& InView,
        const FInputs& InPassInputs) const
    -> FScreenPassTexture
{
    // Output geometry: honour OverrideOutput when the renderer handed us one (it does when the primary upscale is
    // the last pass, and then checks that we returned exactly it), otherwise create the target the same way
    // AddDefaultUpscalePass would.
    auto Output = InPassInputs.OverrideOutput;

    if (!Output.IsValid())
    {
        auto OutputDesc = FRDGTextureDesc::Create2D(
            InPassInputs.SceneColor.Texture->Desc.Extent,
            InPassInputs.SceneColor.Texture->Desc.Format,
            FClearValueBinding::Black,
            TexCreate_ShaderResource | TexCreate_RenderTargetable);

        if (InPassInputs.Stage == EUpscaleStage::PrimaryToSecondary)
        {
            const auto SecondaryViewRectSize = InView.GetSecondaryViewRectSize();
            QuantizeSceneBufferSize(SecondaryViewRectSize, OutputDesc.Extent);
            Output.ViewRect.Min = FIntPoint::ZeroValue;
            Output.ViewRect.Max = SecondaryViewRectSize;
        }
        else
        {
            OutputDesc.Extent = InView.UnscaledViewRect.Max;
            Output.ViewRect = InView.UnscaledViewRect;
        }

        Output.Texture = InGraphBuilder.CreateTexture(OutputDesc, TEXT("CkPixelArt.Upscale"));
        Output.LoadAction = ERenderTargetLoadAction::EClear;
        Output.UpdateVisualizeTextureExtent();
    }

    // The rendered rect carries MarginTexels of extra texels per side; the displayed window is the inset. Handing
    // the inset rect to the pass as the input viewport is what makes the vertex shader's UV span exactly the
    // window, so the shader needs no window uniforms of its own.
    auto InnerRect = InPassInputs.SceneColor.ViewRect;
    InnerRect.InflateRect(-_Frame.MarginTexels);

    const auto InputExtent = InPassInputs.SceneColor.Texture->Desc.Extent;
    const auto InputViewport = FScreenPassTextureViewport{InPassInputs.SceneColor.Texture, InnerRect};
    const auto OutputViewport = FScreenPassTextureViewport{Output};

    auto* PassParameters = InGraphBuilder.AllocParameters<FCk_PixelArt_UpscalePS::FParameters>();
    PassParameters->InputTexture = InPassInputs.SceneColor.Texture;
    PassParameters->InputSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    PassParameters->InputExtent = FVector2f{static_cast<float>(InputExtent.X), static_cast<float>(InputExtent.Y)};
    PassParameters->InputExtentInv = FVector2f{1.0f / InputExtent.X, 1.0f / InputExtent.Y};
    PassParameters->SubTexelOffsetTexels = _Frame.SubTexelOffsetTexels;
    PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

    auto PermutationVector = FCk_PixelArt_UpscalePS::FPermutationDomain{};
    PermutationVector.Set<FCk_PixelArt_UpscalePS::FNearestDebugDim>(
        _Frame.FilterMode == ECk_PixelArt_UpscaleFilter::Nearest);

    const auto VertexShader = TShaderMapRef<FScreenPassVS>{InView.ShaderMap};
    const auto PixelShader = TShaderMapRef<FCk_PixelArt_UpscalePS>{InView.ShaderMap, PermutationVector};

    const auto DisplayedSize = InnerRect.Size();
    const auto OutputSize = Output.ViewRect.Size();

    auto& LastLogged = ck_pixel_art_upscaler::GLastLoggedGeometry;
    if (LastLogged.Input != DisplayedSize || LastLogged.Output != OutputSize ||
        LastLogged.Stage != static_cast<int32>(InPassInputs.Stage))
    {
        LastLogged = {DisplayedSize, OutputSize, static_cast<int32>(InPassInputs.Stage)};

        UE_LOG(LogCkPixelArt, Display,
            TEXT("Upscaler AddPasses: rendered=%dx%d displayed=%dx%d out=%dx%d margin=%d stage=%s"),
            InPassInputs.SceneColor.ViewRect.Width(), InPassInputs.SceneColor.ViewRect.Height(),
            DisplayedSize.X, DisplayedSize.Y,
            OutputSize.X, OutputSize.Y,
            _Frame.MarginTexels,
            ck_pixel_art_upscaler::Get_StageName(InPassInputs.Stage));
    }

    AddDrawScreenPass(InGraphBuilder,
        RDG_EVENT_NAME("CkPixelArt.Upscale %dx%d -> %dx%d",
            DisplayedSize.X, DisplayedSize.Y, OutputSize.X, OutputSize.Y),
        FScreenPassViewInfo{InView},
        OutputViewport,
        InputViewport,
        VertexShader,
        PixelShader,
        PassParameters);

    return MoveTemp(Output);
}
