#include "CkUsfRenderer/Outline/CkUsf_Outline_Renderer.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "GlobalShader.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "ShaderParameterStruct.h"

namespace ck::usf
{
    static TAutoConsoleVariable<int32> CVarOutlineDebug(TEXT("ck.Usf.Outline.Debug"), 0,
        TEXT("Outline diagnostic view: 0=composite, 1=active stencil, 2=visible seeds, 3=occupied root, "
             "4=coverage, 5=pre-exposed preset 0, 6=preset 0, 7=pre-exposure."), ECVF_RenderThreadSafe | ECVF_Cheat);
    BEGIN_SHADER_PARAMETER_STRUCT(FOutlineSceneParameters, )
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)
        SHADER_PARAMETER_ARRAY(FVector4f, OutlineColors, [16])
        SHADER_PARAMETER_ARRAY(FVector4f, FillColors, [16])
        SHADER_PARAMETER(FIntPoint, ViewSize)
        SHADER_PARAMETER(uint32, ActiveMask)
        SHADER_PARAMETER(uint32, StencilMin)
        SHADER_PARAMETER(uint32, WorldSpace)
        SHADER_PARAMETER(uint32, SquareCorners)
        SHADER_PARAMETER(float, Thickness)
        SHADER_PARAMETER(uint32, DebugMode)
    END_SHADER_PARAMETER_STRUCT()

    class FOutlineSeedCS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FOutlineSeedCS);
        SHADER_USE_PARAMETER_STRUCT(FOutlineSeedCS, FGlobalShader);
        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
            SHADER_PARAMETER_STRUCT_INCLUDE(FOutlineSceneParameters, Scene)
            SHADER_PARAMETER(FIntPoint, HierarchySize)
            SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutDepth)
        END_SHADER_PARAMETER_STRUCT()
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& InParameters)
        { return IsFeatureLevelSupported(InParameters.Platform, ERHIFeatureLevel::SM5) && !IsMobilePlatform(InParameters.Platform); }
        static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& InParameters, FShaderCompilerEnvironment& OutEnvironment)
        {
            FGlobalShader::ModifyCompilationEnvironment(InParameters, OutEnvironment);
            OutEnvironment.SetDefine(TEXT("SHADING_PATH_DEFERRED"), 1);
            OutEnvironment.CompilerFlags.Add(CFLAG_HLSL2021);
        }
    };

    class FOutlineReduceCS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FOutlineReduceCS);
        SHADER_USE_PARAMETER_STRUCT(FOutlineReduceCS, FGlobalShader);
        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
            SHADER_PARAMETER(FIntPoint, HierarchySize)
            SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float>, InDepth)
            SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, OutDepth)
        END_SHADER_PARAMETER_STRUCT()
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& InParameters)
        { return FOutlineSeedCS::ShouldCompilePermutation(InParameters); }
        static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& InParameters, FShaderCompilerEnvironment& OutEnvironment)
        { FOutlineSeedCS::ModifyCompilationEnvironment(InParameters, OutEnvironment); }
    };

    class FOutlineCompositeCS : public FGlobalShader
    {
    public:
        DECLARE_GLOBAL_SHADER(FOutlineCompositeCS);
        SHADER_USE_PARAMETER_STRUCT(FOutlineCompositeCS, FGlobalShader);
        BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
            SHADER_PARAMETER_STRUCT_INCLUDE(FOutlineSceneParameters, Scene)
            SHADER_PARAMETER(FIntPoint, ColorMin)
            SHADER_PARAMETER(uint32, RootLevel)
            SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColor)
            SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float>, Hierarchy)
            SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputColor)
        END_SHADER_PARAMETER_STRUCT()
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& InParameters)
        { return FOutlineSeedCS::ShouldCompilePermutation(InParameters); }
        static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& InParameters, FShaderCompilerEnvironment& OutEnvironment)
        { FOutlineSeedCS::ModifyCompilationEnvironment(InParameters, OutEnvironment); }
    };

    IMPLEMENT_GLOBAL_SHADER(FOutlineSeedCS, "/CkUsfRenderer/Outline.usf", "SeedCS", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FOutlineReduceCS, "/CkUsfRenderer/Outline.usf", "ReduceCS", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FOutlineCompositeCS, "/CkUsfRenderer/Outline.usf", "CompositeCS", SF_Compute);

    class FOutlineViewExtension : public FWorldSceneViewExtension
    {
    public:
        FOutlineViewExtension(const FAutoRegister& InAutoRegister, UWorld* InWorld)
            : FWorldSceneViewExtension{InAutoRegister, InWorld}
        {}

        // Written and read on the render thread only. Every shader parameter struct copies this state.
        FOutlineRenderState State;

        virtual void SubscribeToPostProcessingPass(EPostProcessingPass InPass, const FSceneView& InView,
            FPostProcessingPassDelegateArray& InCallbacks, bool InIsPassEnabled) override
        {
            if (InPass == EPostProcessingPass::AfterDOF && State.ActiveMask != 0 &&
                InView.GetFeatureLevel() >= ERHIFeatureLevel::SM5)
            {
                InCallbacks.Add(FPostProcessingPassDelegate::CreateRaw(this, &FOutlineViewExtension::Render));
            }
        }

        auto Render(FRDGBuilder& InGraph, const FSceneView& InView, const FPostProcessMaterialInputs& InInputs)
            -> FScreenPassTexture
        {
            // No custom-depth geometry in this view is normal (including offscreen/empty worlds).
            if (State.ActiveMask == 0 || !InInputs.SceneTextures.SceneTextures || InInputs.CustomDepthTexture == nullptr)
            { return InInputs.ReturnUntouchedSceneColorForPostProcessing(InGraph); }

            const auto Color = FScreenPassTexture::CopyFromSlice(InGraph, InInputs.GetInput(EPostProcessMaterialInput::SceneColor));
            const auto Size = Color.ViewRect.Size();
            const auto Extent = FMath::RoundUpToPowerOfTwo(static_cast<uint32>(FMath::Max(Size.X, Size.Y)));
            const auto RootLevel = FMath::FloorLog2(Extent);
            auto Desc = FRDGTextureDesc::Create2D(FIntPoint(Extent, Extent), PF_R32_FLOAT, FClearValueBinding::None,
                TexCreate_ShaderResource | TexCreate_UAV, RootLevel + 1);
            const auto Depth = InGraph.CreateTexture(Desc, TEXT("CkOutline.MinDepthHierarchy"));

            FOutlineSceneParameters Scene{};
            Scene.View = InView.ViewUniformBuffer;
            Scene.SceneTextures = InInputs.SceneTextures;
            Scene.ViewSize = Size;
            Scene.ActiveMask = State.ActiveMask;
            Scene.StencilMin = State.StencilMin;
            Scene.WorldSpace = State.WorldSpace ? 1 : 0;
            Scene.SquareCorners = State.SquareCorners ? 1 : 0;
            Scene.Thickness = State.Thickness;
            Scene.DebugMode = CVarOutlineDebug.GetValueOnRenderThread();
            for (auto Index = 0; Index != 16; ++Index)
            {
                Scene.OutlineColors[Index] = State.Outline[Index];
                Scene.FillColors[Index] = State.Fill[Index];
            }

            const auto* ShaderMap = GetGlobalShaderMap(InView.GetFeatureLevel());
            auto* Seed = InGraph.AllocParameters<FOutlineSeedCS::FParameters>();
            Seed->Scene = Scene;
            Seed->HierarchySize = FIntPoint(Extent, Extent);
            Seed->OutDepth = InGraph.CreateUAV(FRDGTextureUAVDesc(Depth, 0));
            FComputeShaderUtils::AddPass(InGraph, RDG_EVENT_NAME("CkOutline.Seed"),
                TShaderMapRef<FOutlineSeedCS>(ShaderMap), Seed, FComputeShaderUtils::GetGroupCount(Seed->HierarchySize, 8));

            for (uint32 Level = 1; Level <= RootLevel; ++Level)
            {
                auto* Reduce = InGraph.AllocParameters<FOutlineReduceCS::FParameters>();
                Reduce->HierarchySize = FIntPoint(Extent >> Level, Extent >> Level);
                Reduce->InDepth = InGraph.CreateSRV(FRDGTextureSRVDesc::CreateForMipLevel(Depth, Level - 1));
                Reduce->OutDepth = InGraph.CreateUAV(FRDGTextureUAVDesc(Depth, Level));
                FComputeShaderUtils::AddPass(InGraph, RDG_EVENT_NAME("CkOutline.Reduce %u", Level),
                    TShaderMapRef<FOutlineReduceCS>(ShaderMap), Reduce, FComputeShaderUtils::GetGroupCount(Reduce->HierarchySize, 8));
            }

            const auto Output = InGraph.CreateTexture(FRDGTextureDesc::Create2D(Color.Texture->Desc.Extent,
                PF_FloatRGBA, FClearValueBinding::None, TexCreate_ShaderResource | TexCreate_UAV), TEXT("CkOutline.SceneColor"));
            auto* Composite = InGraph.AllocParameters<FOutlineCompositeCS::FParameters>();
            Composite->Scene = Scene;
            Composite->ColorMin = Color.ViewRect.Min;
            Composite->RootLevel = RootLevel;
            Composite->InputColor = Color.Texture;
            Composite->Hierarchy = Depth;
            Composite->OutputColor = InGraph.CreateUAV(Output);
            FComputeShaderUtils::AddPass(InGraph, RDG_EVENT_NAME("CkOutline.Composite"),
                TShaderMapRef<FOutlineCompositeCS>(ShaderMap), Composite, FComputeShaderUtils::GetGroupCount(Size, 8));
            return FScreenPassTexture(Output, Color.ViewRect);
        }
    };

    FOutlineRenderer::FOutlineRenderer(UWorld* InWorld)
        : _Extension{FSceneViewExtensions::NewExtension<FOutlineViewExtension>(InWorld)}
    {}

    FOutlineRenderer::~FOutlineRenderer()
    { Deactivate(); }

    auto FOutlineRenderer::Set_State(const FOutlineRenderState& InState) -> void
    {
        const auto Extension = _Extension;
        if (Extension.IsValid() == false) { return; }
        ENQUEUE_RENDER_COMMAND(CkOutlineSetState)([Extension, InState](FRHICommandListImmediate&)
        { Extension->State = InState; });
    }

    auto FOutlineRenderer::Deactivate() -> void
    {
        Set_State(FOutlineRenderState{});
        _Extension.Reset();
    }
}
