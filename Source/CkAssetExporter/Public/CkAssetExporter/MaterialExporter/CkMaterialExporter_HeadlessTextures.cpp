#include "CkMaterialExporter_HeadlessTextures.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Engine/EngineTypes.h"
#include "Engine/Font.h"
#include "Engine/Texture.h"
#include "MaterialShared.h"
#include "Misc/ScopeExit.h"
#include "SceneTypes.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionDataDrivenShaderPlatformInfoSwitch.h"
#include "Materials/MaterialExpressionFeatureLevelSwitch.h"
#include "Materials/MaterialExpressionFontSample.h"
#include "Materials/MaterialExpressionFontSampleParameter.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionQualitySwitch.h"
#include "Materials/MaterialExpressionShadingPathSwitch.h"
#include "Materials/MaterialExpressionSparseVolumeTextureBase.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionTextureCollection.h"
#include "Materials/MaterialExpressionTextureObjectFromCollection.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA
namespace ck_material_exporter_headless_textures
{
    // The translator's fixed property compile order (HLSLMaterialTranslator::Translate) — Normal is
    // always first so its chunks lead the uniform-expression set, and everything else follows in
    // this exact sequence. Registration order within a bucket falls out of walking properties this way.
    constexpr EMaterialProperty PropertyCompileOrder[] =
    {
        MP_Normal,
        MP_EmissiveColor,
        MP_DiffuseColor,
        MP_SpecularColor,
        MP_BaseColor,
        MP_Metallic,
        MP_Specular,
        MP_Roughness,
        MP_Tangent,
        MP_ShadingModel,
        MP_Anisotropy,
        MP_Opacity,
        MP_OpacityMask,
        MP_WorldPositionOffset,
        MP_SurfaceThickness,
        MP_FrontMaterial,
        MP_SubsurfaceColor,
        MP_CustomData0,
        MP_CustomData1,
        MP_AmbientOcclusion,
        MP_Refraction,
        MP_PixelDepthOffset,
        MP_Displacement,
        MP_CustomizedUVs0,
        MP_CustomizedUVs1,
        MP_CustomizedUVs2,
        MP_CustomizedUVs3,
        MP_CustomizedUVs4,
        MP_CustomizedUVs5,
        MP_CustomizedUVs6,
        MP_CustomizedUVs7,
    };

    constexpr auto MaxRecursionDepth = 128;

    struct FFrame
    {
        const UMaterialExpressionMaterialFunctionCall* _Call = nullptr;
        int32 _ParentFrame = INDEX_NONE;
    };

    struct FWalkState
    {
        UMaterialInterface* _QueriedMaterial = nullptr;
        TArray<FFrame> _Frames;
        TArray<TArray<UTexture*>> _Buckets;
        TSet<TTuple<const UMaterialExpression*, int32, int32>> _Visited;
        int32 _Depth = 0;
        bool _Supported = true;
        FString _UnsupportedReason;

        auto MarkUnsupported(FString InReason) -> void
        {
            if (_Supported)
            {
                _Supported = false;
                _UnsupportedReason = MoveTemp(InReason);
            }
        }
    };

    auto WalkExpression(FWalkState& InState, const UMaterialExpression* InExpression, int32 InOutputIndex, int32 InFrame) -> void;

    auto WalkInput(FWalkState& InState, const FExpressionInput& InInput, int32 InFrame) -> void
    {
        if (InInput.Expression == nullptr)
        { return; }
        WalkExpression(InState, InInput.Expression, InInput.OutputIndex, InFrame);
    }

    // Mirrors FMaterialTextureParameterInfo::GetGameThreadTextureValue: a named parameter resolves
    // through the queried interface's instance chain; anything else (or a failed lookup) falls back
    // to the texture the translator registered — the node's default.
    auto RegisterTexture(FWalkState& InState, UTexture* InDefaultTexture, FName InParameterName) -> void
    {
        auto* DefaultTexture = InDefaultTexture;

        auto ResolvedTexture = DefaultTexture;
        if (NOT InParameterName.IsNone())
        {
            auto* ParamValue = static_cast<UTexture*>(nullptr);
            if (InState._QueriedMaterial->GetTextureParameterValue(
                    FHashedMaterialParameterInfo{InParameterName}, ParamValue))
            { ResolvedTexture = ParamValue; }
        }

        if (ResolvedTexture == nullptr)
        { return; }

        // The uniform expression's bucket was fixed at translate time from the node's default
        // texture; the resolved value only substitutes within that bucket.
        const auto* BucketSource = DefaultTexture != nullptr ? DefaultTexture : ResolvedTexture;
        auto Bucket = int32{INDEX_NONE};
        switch (BucketSource->GetMaterialType())
        {
            case MCT_Texture2D:        Bucket = static_cast<int32>(EMaterialTextureParameterType::Standard2D); break;
            case MCT_TextureCube:      Bucket = static_cast<int32>(EMaterialTextureParameterType::Cube); break;
            case MCT_Texture2DArray:   Bucket = static_cast<int32>(EMaterialTextureParameterType::Array2D); break;
            case MCT_TextureCubeArray: Bucket = static_cast<int32>(EMaterialTextureParameterType::ArrayCube); break;
            case MCT_VolumeTexture:    Bucket = static_cast<int32>(EMaterialTextureParameterType::Volume); break;
            case MCT_TextureVirtual:   Bucket = static_cast<int32>(EMaterialTextureParameterType::Virtual); break;
            default: break;
        }

        // External/mesh-paint/collection texture types never enter the standard uniform buckets,
        // so GetUsedTextures never reports them — neither do we.
        if (Bucket == INDEX_NONE)
        { return; }

        InState._Buckets[Bucket].AddUnique(ResolvedTexture);
    }

    auto RegisterTextureNode(FWalkState& InState, const UMaterialExpressionTextureBase* InNode) -> void
    {
        const auto* ParamNode = Cast<UMaterialExpressionTextureSampleParameter>(InNode);
        RegisterTexture(InState, InNode->Texture.Get(), ParamNode != nullptr ? ParamNode->ParameterName : FName{});
    }

    auto EvaluateStaticBool(FWalkState& InState, const FExpressionInput& InInput, int32 InFrame) -> TOptional<bool>
    {
        const auto& Traced = InInput.GetTracedInput();
        const auto* Expression = Traced.Expression;
        if (Expression == nullptr)
        { return {}; }

        if (const auto* StaticBool = Cast<UMaterialExpressionStaticBool>(Expression))
        { return StaticBool->Value != 0; }

        if (const auto* BoolParam = Cast<UMaterialExpressionStaticBoolParameter>(Expression))
        {
            auto Value = false;
            auto Guid = FGuid{};
            if (InState._QueriedMaterial->GetStaticSwitchParameterValue(
                    FHashedMaterialParameterInfo{FMaterialParameterInfo{BoolParam->ParameterName}}, Value, Guid))
            { return Value; }
            return BoolParam->DefaultValue != 0;
        }

        if (const auto* FunctionInput = Cast<UMaterialExpressionFunctionInput>(Expression))
        {
            // Copy the frame — recursion below can grow _Frames and reallocate.
            const auto Frame = InState._Frames[InFrame];
            if (Frame._Call != nullptr)
            {
                for (const auto& CallInput : Frame._Call->FunctionInputs)
                {
                    if (CallInput.ExpressionInput != FunctionInput)
                    { continue; }
                    if (CallInput.Input.GetTracedInput().Expression != nullptr)
                    { return EvaluateStaticBool(InState, CallInput.Input, Frame._ParentFrame); }
                    break;
                }
            }
            if (FunctionInput->bUsePreviewValueAsDefault)
            { return EvaluateStaticBool(InState, FunctionInput->Preview, InFrame); }
            return {};
        }

        if (const auto* NamedUsage = Cast<UMaterialExpressionNamedRerouteUsage>(Expression))
        {
            if (ck::IsValid(NamedUsage->Declaration.Get()))
            { return EvaluateStaticBool(InState, NamedUsage->Declaration->Input, InFrame); }
            return {};
        }

        return {};
    }

    auto WalkExpression(FWalkState& InState, const UMaterialExpression* InExpression, int32 InOutputIndex, int32 InFrame) -> void
    {
        if (InExpression == nullptr || NOT InState._Supported)
        { return; }

        if (InState._Depth >= MaxRecursionDepth)
        {
            InState.MarkUnsupported(TEXT("expression graph exceeds the walk's recursion cap (cycle or pathological depth)"));
            return;
        }

        const auto VisitKey = MakeTuple(InExpression, InOutputIndex, InFrame);
        if (InState._Visited.Contains(VisitKey))
        { return; }
        InState._Visited.Add(VisitKey);

        ++InState._Depth;
        ON_SCOPE_EXIT { --InState._Depth; };

        // Constructs whose translation order or content the walk does not model — refuse rather
        // than risk a silently wrong texture list (the exact failure this walk exists to prevent).
        if (InExpression->IsA<UMaterialExpressionMaterialAttributeLayers>() ||
            InExpression->IsA<UMaterialExpressionQualitySwitch>() ||
            InExpression->IsA<UMaterialExpressionFeatureLevelSwitch>() ||
            InExpression->IsA<UMaterialExpressionShadingPathSwitch>() ||
            InExpression->IsA<UMaterialExpressionDataDrivenShaderPlatformInfoSwitch>() ||
            InExpression->IsA<UMaterialExpressionCustomOutput>() ||
            InExpression->IsA<UMaterialExpressionSparseVolumeTextureBase>() ||
            InExpression->IsA<UMaterialExpressionTextureCollection>() ||
            InExpression->IsA<UMaterialExpressionTextureObjectFromCollection>())
        {
            InState.MarkUnsupported(ck::Format_UE(TEXT("expression [{}] is not modeled by the headless texture walk"),
                InExpression->GetClass()->GetName()));
            return;
        }

        if (const auto* FunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(InExpression))
        {
            auto* Function = FunctionCall->MaterialFunction.Get();
            if (Function == nullptr)
            { return; }

            if (Function->GetClass() != UMaterialFunction::StaticClass())
            {
                InState.MarkUnsupported(ck::Format_UE(TEXT("material function [{}] is a [{}] — only plain UMaterialFunction calls are modeled"),
                    Function->GetName(), Function->GetClass()->GetName()));
                return;
            }

            if (NOT FunctionCall->FunctionOutputs.IsValidIndex(InOutputIndex))
            { return; }

            const auto* FunctionOutput = FunctionCall->FunctionOutputs[InOutputIndex].ExpressionOutput.Get();
            if (FunctionOutput == nullptr)
            {
                InState.MarkUnsupported(ck::Format_UE(TEXT("material function call [{}] has an unlinked output — cannot walk its graph"),
                    Function->GetName()));
                return;
            }

            const auto NewFrame = InState._Frames.Add(FFrame{FunctionCall, InFrame});
            WalkInput(InState, FunctionOutput->A, NewFrame);
            return;
        }

        if (const auto* FunctionInput = Cast<UMaterialExpressionFunctionInput>(InExpression))
        {
            // Copy the frame — recursion below can grow _Frames and reallocate.
            const auto Frame = InState._Frames[InFrame];
            if (Frame._Call != nullptr)
            {
                for (const auto& CallInput : Frame._Call->FunctionInputs)
                {
                    if (CallInput.ExpressionInput != FunctionInput)
                    { continue; }
                    if (CallInput.Input.GetTracedInput().Expression != nullptr)
                    {
                        WalkInput(InState, CallInput.Input, Frame._ParentFrame);
                        return;
                    }
                    break;
                }
            }
            if (FunctionInput->bUsePreviewValueAsDefault)
            { WalkInput(InState, FunctionInput->Preview, InFrame); }
            return;
        }

        // StaticSwitchParameter derives from StaticBoolParameter — test it first.
        if (const auto* SwitchParam = Cast<UMaterialExpressionStaticSwitchParameter>(InExpression))
        {
            if (SwitchParam->DynamicBranch)
            {
                WalkInput(InState, SwitchParam->A, InFrame);
                WalkInput(InState, SwitchParam->B, InFrame);
                return;
            }

            auto Value = SwitchParam->DefaultValue != 0;
            {
                auto Resolved = false;
                auto Guid = FGuid{};
                if (InState._QueriedMaterial->GetStaticSwitchParameterValue(
                        FHashedMaterialParameterInfo{FMaterialParameterInfo{SwitchParam->ParameterName}}, Resolved, Guid))
                { Value = Resolved; }
            }
            WalkInput(InState, Value ? SwitchParam->A : SwitchParam->B, InFrame);
            return;
        }

        if (const auto* StaticSwitch = Cast<UMaterialExpressionStaticSwitch>(InExpression))
        {
            auto Value = TOptional<bool>{StaticSwitch->DefaultValue != 0};
            if (StaticSwitch->Value.GetTracedInput().Expression != nullptr)
            { Value = EvaluateStaticBool(InState, StaticSwitch->Value, InFrame); }

            if (Value.IsSet())
            { WalkInput(InState, *Value ? StaticSwitch->A : StaticSwitch->B, InFrame); }
            else
            {
                // A non-static bool feeds the switch — the translator emits a dynamic branch and
                // compiles both sides, A first.
                WalkInput(InState, StaticSwitch->A, InFrame);
                WalkInput(InState, StaticSwitch->B, InFrame);
            }
            return;
        }

        if (const auto* NamedUsage = Cast<UMaterialExpressionNamedRerouteUsage>(InExpression))
        {
            if (ck::IsValid(NamedUsage->Declaration.Get()))
            { WalkInput(InState, NamedUsage->Declaration->Input, InFrame); }
            return;
        }

        // TextureSample::Compile registers the texture BEFORE compiling coordinates/mips, and a
        // connected TextureObject input supersedes the node's own texture — mirror both exactly.
        if (const auto* Sample = Cast<UMaterialExpressionTextureSample>(InExpression))
        {
            const auto* ObjectInputExpression = Sample->TextureObject.GetTracedInput().Expression;
            if (Sample->Texture != nullptr || ObjectInputExpression != nullptr)
            {
                if (ObjectInputExpression != nullptr)
                { WalkInput(InState, Sample->TextureObject, InFrame); }
                else if (Sample->SamplerType != SAMPLERTYPE_External)
                { RegisterTextureNode(InState, Sample); }

                WalkInput(InState, Sample->Coordinates, InFrame);
                WalkInput(InState, Sample->MipValue, InFrame);
                WalkInput(InState, Sample->CoordinatesDX, InFrame);
                WalkInput(InState, Sample->CoordinatesDY, InFrame);
            }
            return;
        }

        // TextureObject / TextureObjectParameter (and any other TextureBase leaf) registers itself.
        if (const auto* TextureBase = Cast<UMaterialExpressionTextureBase>(InExpression))
        {
            RegisterTextureNode(InState, TextureBase);
            return;
        }

        // FontSample registers the selected font page texture; the parameter variant registers it
        // as a TEXTURE parameter named after the font parameter (FontSampleParameter::Compile),
        // and falls back to the plain-sample path when its name/font/page is unusable.
        if (const auto* FontSample = Cast<UMaterialExpressionFontSample>(InExpression))
        {
            auto* Font = FontSample->Font.Get();
            const auto FontPageIsSampleable = Font != nullptr &&
                Font->FontCacheType == EFontCacheType::Offline &&
                Font->Textures.IsValidIndex(FontSample->FontTexturePage);
            if (NOT FontPageIsSampleable)
            { return; }

            auto ParameterName = FName{};
            if (const auto* FontParam = Cast<UMaterialExpressionFontSampleParameter>(FontSample);
                FontParam != nullptr && FontParam->ParameterName.IsValid() && NOT FontParam->ParameterName.IsNone())
            { ParameterName = FontParam->ParameterName; }

            RegisterTexture(InState, Font->Textures[FontSample->FontTexturePage], ParameterName);
            return;
        }

        // Everything else: pure pass-through/math — compile order matches declared input order.
        const auto InputCount = InExpression->CountInputs();
        for (auto Index = 0; Index < InputCount; ++Index)
        {
            if (const auto* Input = InExpression->GetInput(Index))
            { WalkInput(InState, *Input, InFrame); }
        }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MaterialExporter_HeadlessTextures::
    EnumerateUsedTextures(
        UMaterialInterface* InMaterial)
    -> FCk_MaterialHeadlessTextures_Result
{
    auto Result = FCk_MaterialHeadlessTextures_Result{};

#if WITH_EDITORONLY_DATA
    namespace walk_ns = ck_material_exporter_headless_textures;

    if (ck::Is_NOT_Valid(InMaterial))
    {
        Result.UnsupportedReason = TEXT("invalid material");
        return Result;
    }

    auto* BaseMaterial = InMaterial->GetMaterial();
    if (ck::Is_NOT_Valid(BaseMaterial))
    {
        Result.UnsupportedReason = TEXT("no base material");
        return Result;
    }

    if (BaseMaterial->bUseMaterialAttributes)
    {
        Result.UnsupportedReason = TEXT("material-attributes mode is not modeled by the headless texture walk");
        return Result;
    }

    // Custom-output roots compile outside the property order the walk reproduces.
    for (const auto& Expression : BaseMaterial->GetExpressions())
    {
        if (Expression != nullptr && Expression->IsA<UMaterialExpressionCustomOutput>())
        {
            Result.UnsupportedReason = ck::Format_UE(TEXT("custom output [{}] is not modeled by the headless texture walk"),
                Expression->GetClass()->GetName());
            return Result;
        }
    }

    auto State = walk_ns::FWalkState{};
    State._QueriedMaterial = InMaterial;
    State._Frames.Add(walk_ns::FFrame{});
    State._Buckets.SetNum(NumMaterialTextureParameterTypes);

    for (const auto Property : walk_ns::PropertyCompileOrder)
    {
        if (Property >= MP_CustomizedUVs0 && Property <= MP_CustomizedUVs7 &&
            static_cast<int32>(Property) - static_cast<int32>(MP_CustomizedUVs0) >= BaseMaterial->NumCustomizedUVs)
        { continue; }

        const auto* Input = BaseMaterial->GetExpressionInputForProperty(Property);
        if (Input == nullptr || Input->GetTracedInput().Expression == nullptr)
        { continue; }

        if (NOT InMaterial->IsPropertyActive(Property))
        { continue; }

        constexpr auto TopLevelFrame = 0;
        walk_ns::WalkInput(State, *Input, TopLevelFrame);

        if (NOT State._Supported)
        {
            Result.UnsupportedReason = State._UnsupportedReason;
            return Result;
        }
    }

    Result.Supported = true;
    for (const auto& Bucket : State._Buckets)
    {
        for (auto* Texture : Bucket)
        { Result.Textures.AddUnique(Texture); }
    }
#else
    Result.UnsupportedReason = TEXT("editor-only data is unavailable in this build");
#endif

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
