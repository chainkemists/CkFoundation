#include "CkUsfEditor/Generator/CkUsf_Generator.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsfEditor/Generator/CkUsf_LookValidator.h"
#include "CkUsfEditor_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionVertexTangentWS.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Engine/Texture.h"
#include "SceneTypes.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::usf_editor
{
    // ---- Domain config table ----
    struct FDomainConfig { EMaterialDomain Domain; EBlendMode Blend; bool Unlit; };

    static auto Get_DomainConfig(ECk_Usf_Domain In) -> FDomainConfig
    {
        switch (In)
        {
            case ECk_Usf_Domain::SurfaceUnlit: return { MD_Surface,       BLEND_Translucent, true };
            case ECk_Usf_Domain::PostProcess:  return { MD_PostProcess,   BLEND_Opaque,      true };
            case ECk_Usf_Domain::UI:           return { MD_UI,            BLEND_Translucent, true };
            case ECk_Usf_Domain::Decal:        return { MD_DeferredDecal, BLEND_Translucent, true };
            default:                           return { MD_Surface,       BLEND_Opaque,      false };
        }
    }

    // Resolve the per-look blend override on top of the domain default (`Inherit` keeps the default).
    static auto Resolve_BlendMode(ECk_Usf_BlendMode In, EBlendMode InDomainDefault) -> EBlendMode
    {
        switch (In)
        {
            case ECk_Usf_BlendMode::Opaque:      return BLEND_Opaque;
            case ECk_Usf_BlendMode::Masked:      return BLEND_Masked;
            case ECk_Usf_BlendMode::Translucent: return BLEND_Translucent;
            case ECk_Usf_BlendMode::Additive:    return BLEND_Additive;
            case ECk_Usf_BlendMode::Modulate:    return BLEND_Modulate;
            default:                             return InDomainDefault;
        }
    }

    // Resolve the per-look shading-model override to a concrete EMaterialShadingModel.
    // `Inherit` keeps the domain default (unlit domains → MSM_Unlit, else MSM_DefaultLit).
    static auto Resolve_ShadingModel(ECk_Usf_ShadingModel In, bool InDomainUnlit) -> EMaterialShadingModel
    {
        switch (In)
        {
            case ECk_Usf_ShadingModel::Unlit:      return MSM_Unlit;
            case ECk_Usf_ShadingModel::DefaultLit: return MSM_DefaultLit;
            case ECk_Usf_ShadingModel::Subsurface: return MSM_Subsurface;
            case ECk_Usf_ShadingModel::ClearCoat:  return MSM_ClearCoat;
            default:                               return InDomainUnlit ? MSM_Unlit : MSM_DefaultLit;
        }
    }

    // Resolve the per-look translucency-lighting override. Unset optional = `Inherit` (keep the
    // engine default, TLM_VolumetricNonDirectional — cheap but flat; glass usually wants
    // SurfacePerPixel). Only consulted for LIT translucent-family surface looks.
    static auto Resolve_TranslucencyLighting(ECk_Usf_TranslucencyLighting In) -> TOptional<ETranslucencyLightingMode>
    {
        switch (In)
        {
            case ECk_Usf_TranslucencyLighting::VolumetricNonDirectional: return TLM_VolumetricNonDirectional;
            case ECk_Usf_TranslucencyLighting::VolumetricDirectional:    return TLM_VolumetricDirectional;
            case ECk_Usf_TranslucencyLighting::Surface:                  return TLM_Surface;
            case ECk_Usf_TranslucencyLighting::SurfacePerPixel:          return TLM_SurfacePerPixelLighting;
            default:                                                     return {};
        }
    }

    // Resolve the per-look post-process chain placement. Pre-TAA locations are the fix for
    // Custom Depth/Stencil-derived looks shimmering under the TAA/TSR projection jitter.
    static auto Resolve_BlendableLocation(ECk_Usf_BlendableLocation In) -> EBlendableLocation
    {
        switch (In)
        {
            case ECk_Usf_BlendableLocation::SceneColorAfterDOF:    return BL_SceneColorAfterDOF;
            case ECk_Usf_BlendableLocation::SceneColorBeforeDOF:   return BL_SceneColorBeforeDOF;
            case ECk_Usf_BlendableLocation::SceneColorBeforeBloom: return BL_SceneColorBeforeBloom;
            default:                                               return BL_SceneColorAfterTonemapping;
        }
    }

    // Inject the look's _Defines ("NAME" or "NAME=VALUE") into a Custom node — the static-switch /
    // quality-knob equivalent (e.g. retuning a #ifndef default in the .ush without editing it).
    static auto Apply_LookDefines(UMaterialExpressionCustom* InNode, const UCkUsf_LookDefinition* InDef) -> void
    {
        for (const auto& Define : InDef->_Defines)
        {
            FString Name, Value;
            if (NOT Define.Split(TEXT("="), &Name, &Value))
            { Name = Define; }
            Name.TrimStartAndEndInline();
            Value.TrimStartAndEndInline();
            if (Name.IsEmpty())
            { continue; }

            FCustomDefine NewDefine;
            NewDefine.DefineName = Name;
            NewDefine.DefineValue = Value;
            InNode->AdditionalDefines.Add(NewDefine);
        }
    }

    // Blends that read the Opacity pin (vs the Masked blend, which reads OpacityMask).
    static auto Is_TranslucentFamily(EBlendMode In) -> bool
    {
        return In == BLEND_Translucent || In == BLEND_Additive
            || In == BLEND_Modulate    || In == BLEND_AlphaComposite;
    }

    // Additional outputs every surface look exposes besides the primary (BaseColor). The Custom node
    // always outputs all of them; the pin WIRING in Generate_LookMaterial gates Opacity/OpacityMask by blend.
    struct FExtraOutput { const TCHAR* Name; ECustomMaterialOutputType Type; EMaterialProperty Prop; };

    static auto Get_ExtraOutputs() -> TArray<FExtraOutput>
    {
        return {
            { TEXT("EmissiveColor"),      CMOT_Float3, MP_EmissiveColor },
            { TEXT("Roughness"),          CMOT_Float1, MP_Roughness },
            { TEXT("Metallic"),           CMOT_Float1, MP_Metallic },
            { TEXT("Specular"),           CMOT_Float1, MP_Specular },
            { TEXT("AmbientOcclusion"),   CMOT_Float1, MP_AmbientOcclusion },
            { TEXT("Normal"),             CMOT_Float3, MP_Normal },
            { TEXT("Opacity"),            CMOT_Float1, MP_Opacity },
            { TEXT("OpacityMask"),        CMOT_Float1, MP_OpacityMask },
            { TEXT("Refraction"),         CMOT_Float1, MP_Refraction },
            { TEXT("SubsurfaceColor"),    CMOT_Float3, MP_SubsurfaceColor },
            // ClearCoat / ClearCoatRoughness have no dedicated MP_ enum — they live in the custom-data slots.
            { TEXT("ClearCoat"),          CMOT_Float1, MP_CustomData0 },
            { TEXT("ClearCoatRoughness"), CMOT_Float1, MP_CustomData1 },
        };
    }

    // ---- PostProcess scene-texture wiring: enum -> (ESceneTextureId, Custom-node input / FCkUsf_SurfaceInput field name) ----
    struct FSceneTextureWiring { ESceneTextureId Id; const TCHAR* HlslName; };

    static auto Get_SceneTextureWiring(ECk_Usf_SceneTexture In) -> FSceneTextureWiring
    {
        switch (In)
        {
            case ECk_Usf_SceneTexture::SceneColor:    return { PPI_PostProcessInput0, TEXT("SceneColor") };
            case ECk_Usf_SceneTexture::SceneDepth:    return { PPI_SceneDepth,        TEXT("SceneDepth") };
            case ECk_Usf_SceneTexture::SceneNormal:   return { PPI_WorldNormal,       TEXT("SceneNormal") };
            case ECk_Usf_SceneTexture::CustomDepth:   return { PPI_CustomDepth,       TEXT("CustomDepth") };
            case ECk_Usf_SceneTexture::CustomStencil: return { PPI_CustomStencil,     TEXT("CustomStencil") };
            default:                                  return { PPI_PostProcessInput0, TEXT("SceneColor") };
        }
    }

    // Effective PostProcess scene-texture set: an empty _SceneTextures keeps the historical default trio
    // (SceneColor/SceneDepth/SceneNormal) so existing looks regenerate byte-identically.
    static auto Get_EffectiveSceneTextures(const UCkUsf_LookDefinition* InDef) -> TArray<ECk_Usf_SceneTexture>
    {
        if (InDef->_SceneTextures.IsEmpty() == false)
        { return InDef->_SceneTextures; }
        return { ECk_Usf_SceneTexture::SceneColor, ECk_Usf_SceneTexture::SceneDepth, ECk_Usf_SceneTexture::SceneNormal };
    }

    // ---- Build the Custom node Code (assemble FCkUsf_SurfaceInput, call, assign outputs) ----
    static auto Build_CustomCode(const UCkUsf_LookDefinition* InDef, bool InIsPostProcess) -> FString
    {
        // Assemble the per-pixel input struct from the wired Custom-node inputs. The generator wires
        // the domain-appropriate inputs (see Generate_LookMaterial); unwired fields keep their defaults.
        FString Code = TEXT("FCkUsf_SurfaceInput In = CkUsf_DefaultInput();\n");
        Code += TEXT("In.Time = Time;\n");
        Code += TEXT("In.UV = UV;\n");
        if (InIsPostProcess)
        {
            // Assign only the declared scene-texture inputs (default trio when _SceneTextures is empty).
            for (const auto& Tex : Get_EffectiveSceneTextures(InDef))
            {
                const auto* Name = Get_SceneTextureWiring(Tex).HlslName;
                Code += FString::Printf(TEXT("In.%s = %s;\n"), Name, Name);
            }
        }
        else
        {
            Code += TEXT("In.WorldPosition = WorldPosition;\n");
            Code += TEXT("In.CameraVector = CameraVector;\n");
            Code += TEXT("In.VertexNormal = VertexNormal;\n");
            Code += TEXT("In.VertexTangent = VertexTangent;\n");
            Code += TEXT("In.PixelDepth = PixelDepth;\n");
            Code += TEXT("In.VertexColor = VertexColor;\n");
        }

        // The look fn takes In first, then the LookDefinition params in order.
        FString Args = TEXT("In");
        for (const auto& P : InDef->_Parameters)
        {
            const auto Name = P._Name.ToString();
            switch (P._Type)
            {
                // Vector params connect a (possibly float4) output; .rgb makes the HLSL type float3.
                case ECk_Usf_ParamType::Vector:
                    Args += FString::Printf(TEXT(", %s.rgb"), *Name);
                    break;
                // Texture object inputs expose both the texture and an auto <Name>Sampler in HLSL.
                case ECk_Usf_ParamType::Texture2D:
                case ECk_Usf_ParamType::TextureCube:
                    Args += FString::Printf(TEXT(", %s, %sSampler"), *Name, *Name);
                    break;
                default: // Scalar
                    Args += FString::Printf(TEXT(", %s"), *Name);
                    break;
            }
        }

        Code += FString::Printf(
            TEXT("FCkUsf_SurfaceOutput O = %s(%s);\n"), *InDef->_UshFunctionName.ToString(), *Args);

        if (InIsPostProcess)
        {
            // PostProcess materials expose only EmissiveColor; the primary Custom output is the final pixel.
            Code += TEXT("return O.EmissiveColor;");
            return Code;
        }

        Code += TEXT("EmissiveColor = O.EmissiveColor;\n");
        Code += TEXT("Roughness = O.Roughness;\n");
        Code += TEXT("Metallic = O.Metallic;\n");
        Code += TEXT("Specular = O.Specular;\n");
        Code += TEXT("AmbientOcclusion = O.AmbientOcclusion;\n");
        Code += TEXT("Normal = O.Normal;\n");
        Code += TEXT("Opacity = O.Opacity;\n");
        Code += TEXT("OpacityMask = O.OpacityMask;\n");
        Code += TEXT("Refraction = O.Refraction;\n");
        Code += TEXT("SubsurfaceColor = O.SubsurfaceColor;\n");
        Code += TEXT("ClearCoat = O.ClearCoat;\n");
        Code += TEXT("ClearCoatRoughness = O.ClearCoatRoughness;\n");
        Code += TEXT("return O.BaseColor;");
        return Code;
    }

    // ---- Build the WPO (vertex) Custom node Code. Mirrors Build_CustomCode but assembles the VS-safe
    //      FCkUsf_VertexInput and calls the look's _WpoFunctionName, returning a world-space offset. ----
    static auto Build_WpoCustomCode(const UCkUsf_LookDefinition* InDef) -> FString
    {
        FString Code = TEXT("FCkUsf_VertexInput In = CkUsf_DefaultVertexInput();\n");
        Code += TEXT("In.Time = Time;\n");
        Code += TEXT("In.UV = UV;\n");
        Code += TEXT("In.WorldPosition = WorldPosition;\n");
        Code += TEXT("In.VertexNormal = VertexNormal;\n");
        Code += TEXT("In.VertexColor = VertexColor;\n");

        // The WPO fn takes In first, then the same LookDefinition params in the same order as the pixel fn.
        FString Args = TEXT("In");
        for (const auto& P : InDef->_Parameters)
        {
            const auto Name = P._Name.ToString();
            switch (P._Type)
            {
                case ECk_Usf_ParamType::Vector:
                    Args += FString::Printf(TEXT(", %s.rgb"), *Name);
                    break;
                case ECk_Usf_ParamType::Texture2D:
                case ECk_Usf_ParamType::TextureCube:
                    Args += FString::Printf(TEXT(", %s, %sSampler"), *Name, *Name);
                    break;
                default: // Scalar
                    Args += FString::Printf(TEXT(", %s"), *Name);
                    break;
            }
        }

        Code += FString::Printf(
            TEXT("return %s(%s);"), *InDef->_WpoFunctionName.ToString(), *Args);
        return Code;
    }

    // Create the material expression that feeds a look param into a Custom node input.
    // A per-instance Scalar/Vector becomes a PerInstanceCustomData(3Vector) node; its DataIndex comes
    // from UCkUsf_LookDefinition::Get_PerInstanceSlotOf — the SAME layout runtime writers query, so the
    // pixel node, the WPO node, and CkIsmRenderer writers can never disagree on slots.
    // ConstDefaultValue is the uniform fallback returned on non-instanced meshes.
    // Everything else is a named parameter.
    static auto Make_ParamExpression(
        UMaterial* InMaterial, const UCkUsf_LookDefinition* InDef, const FCk_Usf_ParamDesc& P,
        FName InLookName, int32 InRow)
        -> UMaterialExpression*
    {
        if (P._Type == ECk_Usf_ParamType::Scalar && P._PerInstance)
        {
            auto* PerInstanceExpr = Cast<UMaterialExpressionPerInstanceCustomData>(
                UMaterialEditingLibrary::CreateMaterialExpression(
                    InMaterial, UMaterialExpressionPerInstanceCustomData::StaticClass(), -800, InRow));
            PerInstanceExpr->DataIndex = InDef->Get_PerInstanceSlotOf(P._Name);
            PerInstanceExpr->ConstDefaultValue = P._DefaultScalar;
            return PerInstanceExpr;
        }

        if (P._Type == ECk_Usf_ParamType::Vector && P._PerInstance)
        {
            auto* PerInstanceExpr = Cast<UMaterialExpressionPerInstanceCustomData3Vector>(
                UMaterialEditingLibrary::CreateMaterialExpression(
                    InMaterial, UMaterialExpressionPerInstanceCustomData3Vector::StaticClass(), -800, InRow));
            PerInstanceExpr->DataIndex = InDef->Get_PerInstanceSlotOf(P._Name);
            PerInstanceExpr->ConstDefaultValue = P._DefaultVector;
            return PerInstanceExpr;
        }

        switch (P._Type)
        {
            case ECk_Usf_ParamType::Scalar:
            {
                auto* S = Cast<UMaterialExpressionScalarParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        InMaterial, UMaterialExpressionScalarParameter::StaticClass(), -800, InRow));
                S->ParameterName = P._Name; S->DefaultValue = P._DefaultScalar;
                return S;
            }
            case ECk_Usf_ParamType::Vector:
            {
                auto* V = Cast<UMaterialExpressionVectorParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        InMaterial, UMaterialExpressionVectorParameter::StaticClass(), -800, InRow));
                V->ParameterName = P._Name; V->DefaultValue = P._DefaultVector;
                return V;
            }
            default: // Texture2D / TextureCube
            {
                auto* T = Cast<UMaterialExpressionTextureObjectParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        InMaterial, UMaterialExpressionTextureObjectParameter::StaticClass(), -800, InRow));
                T->ParameterName = P._Name;
                if (P._DefaultTexturePath.IsEmpty() == false)
                {
                    auto* Tex = LoadObject<UTexture>(nullptr, *P._DefaultTexturePath);
                    if (ck::IsValid(Tex, ck::IsValid_Policy_NullptrOnly{})) { T->Texture = Tex; T->AutoSetSampleType(); }
                    else { ck::usf_editor::Warning(TEXT("Look [{}] texture not found: [{}]"), InLookName, P._DefaultTexturePath); }
                }
                return T;
            }
        }
    }

    // The worker behind both public entry points; validation/compile failures land in InOutResult
    // (each entry names its look) AND the log.
    static auto DoGenerate_LookMaterial(UCkUsf_LookDefinition* InDef, FGenerateResult& InOutResult) -> UMaterial*
    {
        // Gate on the asset-vs-.ush contract BEFORE creating any material object: the Custom node
        // passes params positionally, so a mismatch either fails with an HLSL error naming nothing
        // about the asset or silently renders wrong. The validator makes it fail HERE, by name.
        const auto Validation = Validate_LookDefinition(InDef);
        for (const auto& ValidationWarning : Validation.Warnings)
        {
            ck::usf_editor::Warning(TEXT("{}"), ValidationWarning);
            InOutResult.Warnings.Add(ValidationWarning);
        }
        if (NOT Validation.Get_IsValid())
        {
            for (const auto& ValidationError : Validation.Errors)
            {
                ck::usf_editor::Error(TEXT("{}"), ValidationError);
                InOutResult.Errors.Add(ValidationError);
            }
            return nullptr;
        }

        const auto LookName = InDef->Get_EffectiveLookName();
        const auto Config = Get_DomainConfig(InDef->_Domain);
        const auto IsSurface = Config.Domain == MD_Surface;
        // Surface domains honour the per-look blend/shading/two-sided overrides; other domains keep the domain config.
        const auto EffectiveBlend = IsSurface ? Resolve_BlendMode(InDef->_BlendMode, Config.Blend) : Config.Blend;
        const auto EffectiveShadingModel = IsSurface
            ? Resolve_ShadingModel(InDef->_ShadingModel, Config.Unlit)
            : (Config.Unlit ? MSM_Unlit : MSM_DefaultLit);
        const auto IsTranslucent = Is_TranslucentFamily(EffectiveBlend);
        // Refraction applies to LIT translucent surfaces (e.g. glass). Scoping to lit keeps the existing
        // unlit emissive looks byte-unchanged (they have no use for an IOR bend) and avoids an
        // unlit-translucent + refraction permutation.
        const auto WantsRefraction = IsSurface && IsTranslucent && EffectiveShadingModel != MSM_Unlit;

        // ---- Create package + UMaterial (idempotent refresh) ----
        const auto PkgPath = ck::usf::Get_GeneratedMasterPackagePath(LookName);
        const auto AssetName = FString::Printf(TEXT("M_CkUsf_Look_%s"), *LookName.ToString());

        // Fully load any previously-generated package first; otherwise an asset-registry
        // stub leaves it partially loaded and SavePackage asserts. Then replace the old
        // material object so NewObject doesn't clash with the existing name.
        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, *PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr) { Package = CreatePackage(*PkgPath); }

        if (auto* Old = StaticFindObject(UMaterial::StaticClass(), Package, *AssetName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(),
                REN_DontCreateRedirectors | REN_NonTransactional);
        }

        auto* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone);

        Material->MaterialDomain = Config.Domain;
        Material->BlendMode = EffectiveBlend;
        Material->SetShadingModel(EffectiveShadingModel);
        if (IsSurface) { Material->TwoSided = InDef->_TwoSided; }

        // Bake usage flags from the definition — regeneration would otherwise wipe
        // any hand-set flags, and a missing flag means default-material fallback in
        // packaged builds (the CkIsm/CkIskm renderers ensure on these at setup).
        Material->bUsedWithInstancedStaticMeshes = InDef->_UsedWithInstancedStaticMeshes;
        Material->bUsedWithSkeletalMesh = InDef->_UsedWithSkeletalMesh;
        Material->bUsedWithMorphTargets = InDef->_UsedWithMorphTargets;

        // Refraction is wired only for translucent-family surface looks (see the output gating below);
        // the pin is inert unless RefractionMethod is set, so enable IOR-based refraction when it applies.
        // A look that wants no bend simply outputs Refraction == 1.0 (air) — identity, like Opacity == 1.0.
        if (WantsRefraction) { Material->RefractionMethod = RM_IndexOfRefraction; }

        // Lit translucent looks may override the translucency lighting mode (the engine default,
        // volumetric non-directional, reads flat on glass-like surfaces). Inherit = leave it alone.
        if (IsSurface && IsTranslucent && EffectiveShadingModel != MSM_Unlit)
        {
            if (const auto TranslucencyLighting = Resolve_TranslucencyLighting(InDef->_TranslucencyLighting);
                TranslucencyLighting.IsSet())
            { Material->TranslucencyLightingMode = TranslucencyLighting.GetValue(); }
        }

        if (Config.Domain == MD_PostProcess)
        {
            // A programmatically-created PP material does not reliably default to a compositing
            // location; set it explicitly so the blendable actually draws over the final image.
            Material->BlendableLocation = Resolve_BlendableLocation(InDef->_BlendableLocation);
            Material->BlendablePriority = 0;
        }

        if (Config.Domain == MD_Surface)
        {
            // Compile the instanced + skeletal permutations up-front so looks are safe on
            // CkIsmRenderer / CkIskmRenderer in cooked builds. The renderers auto-enable these at
            // runtime, but that path warns it falls back to the default material when packaged.
            Material->bUsedWithInstancedStaticMeshes = true;
            Material->bUsedWithSkeletalMesh = true;
        }

        // ---- Custom node (added to material via editing library) ----
        auto* Custom = Cast<UMaterialExpressionCustom>(
            UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionCustom::StaticClass(), -400, 0));

        const auto IsPostProcess = InDef->_Domain == ECk_Usf_Domain::PostProcess;

        Custom->OutputType = CMOT_Float3;               // primary = BaseColor (surface) / EmissiveColor (post-process)
        Custom->IncludeFilePaths.Add(InDef->_UshIncludePath);

        Custom->AdditionalOutputs.Reset();
        if (NOT IsPostProcess)
        {
            for (const auto& E : Get_ExtraOutputs())
            {
                Custom->AdditionalOutputs.Add(FCustomOutput{ FName(E.Name), E.Type });
            }
        }

        // Inputs: [scene textures (PP only) | world-space inputs (surface only)] + one per param + Time + UV.
        Custom->Inputs.Reset();
        if (IsPostProcess)
        {
            for (const auto& Tex : Get_EffectiveSceneTextures(InDef))
            {
                FCustomInput In; In.InputName = FName(Get_SceneTextureWiring(Tex).HlslName); Custom->Inputs.Add(In);
            }
        }
        else
        {
            { FCustomInput In; In.InputName = TEXT("WorldPosition"); Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("CameraVector");  Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexNormal");  Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexTangent"); Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("PixelDepth");    Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexColor");   Custom->Inputs.Add(In); }
        }
        for (const auto& P : InDef->_Parameters)
        {
            FCustomInput In; In.InputName = P._Name; Custom->Inputs.Add(In);
        }
        { FCustomInput T; T.InputName = TEXT("Time"); Custom->Inputs.Add(T); }
        { FCustomInput U; U.InputName = TEXT("UV");   Custom->Inputs.Add(U); }

        Custom->Code = Build_CustomCode(InDef, IsPostProcess);
        Apply_LookDefines(Custom, InDef);
        Custom->RebuildOutputs();
        Custom->PostEditChange();

        // ---- PostProcess: managed scene-texture inputs (also declares scene-texture usage,
        //      which legalizes raw SceneTextureLookup() in the look's .ush) ----
        if (IsPostProcess)
        {
            const auto AddSceneTexture = [&](ESceneTextureId InId, const TCHAR* InInputName, int32 InRow) -> void
            {
                auto* Expr = Cast<UMaterialExpressionSceneTexture>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        Material, UMaterialExpressionSceneTexture::StaticClass(), -1100, InRow * 160));
                Expr->SceneTextureId = InId;
                UMaterialEditingLibrary::ConnectMaterialExpressions(Expr, FString(), Custom, InInputName);
            };
            int32 SceneTexRow = 0;
            for (const auto& Tex : Get_EffectiveSceneTextures(InDef))
            {
                const auto Wiring = Get_SceneTextureWiring(Tex);
                AddSceneTexture(Wiring.Id, Wiring.HlslName, SceneTexRow++);
            }
        }
        else
        {
            // ---- Surface: world-space per-pixel inputs feeding FCkUsf_SurfaceInput ----
            const auto AddInput = [&](UClass* InClass, const TCHAR* InInputName, int32 InRow) -> void
            {
                auto* Expr = UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, InClass, -1100, InRow * 160);
                UMaterialEditingLibrary::ConnectMaterialExpressions(Expr, FString(), Custom, InInputName);
            };
            AddInput(UMaterialExpressionWorldPosition::StaticClass(),   TEXT("WorldPosition"), 0);
            AddInput(UMaterialExpressionCameraVectorWS::StaticClass(),  TEXT("CameraVector"),  1);
            AddInput(UMaterialExpressionVertexNormalWS::StaticClass(),  TEXT("VertexNormal"),  2);
            AddInput(UMaterialExpressionVertexTangentWS::StaticClass(), TEXT("VertexTangent"), 3);
            AddInput(UMaterialExpressionPixelDepth::StaticClass(),      TEXT("PixelDepth"),    4);
            AddInput(UMaterialExpressionVertexColor::StaticClass(),     TEXT("VertexColor"),   5);
        }

        // ---- Parameter nodes wired to Custom inputs by name (per-instance params → PerInstanceCustomData) ----
        int32 ParamRow = 0;
        for (const auto& P : InDef->_Parameters)
        {
            auto* ParamExpr = Make_ParamExpression(Material, InDef, P, LookName, ParamRow * 120);
            if (ck::IsValid(ParamExpr, ck::IsValid_Policy_NullptrOnly{}))
            {
                UMaterialEditingLibrary::ConnectMaterialExpressions(
                    ParamExpr, FString(), Custom, P._Name.ToString());
            }
            ++ParamRow;
        }

        // ---- Time + UV built-ins ----
        auto* TimeExpr = UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionTime::StaticClass(), -800, (ParamRow + 1) * 120);
        UMaterialEditingLibrary::ConnectMaterialExpressions(TimeExpr, FString(), Custom, TEXT("Time"));

        auto* UvExpr = IsPostProcess
            ? UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionScreenPosition::StaticClass(), -800, (ParamRow + 2) * 120)
            : UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionTextureCoordinate::StaticClass(), -800, (ParamRow + 2) * 120);
        UMaterialEditingLibrary::ConnectMaterialExpressions(UvExpr, FString(), Custom, TEXT("UV"));

        // ---- Connect Custom outputs to material pins ----
        if (IsPostProcess)
        {
            // PostProcess: the primary output IS the final emissive pixel; no other pins are valid.
            UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(), MP_EmissiveColor);
        }
        else
        {
            UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(), MP_BaseColor);
            const auto IsSubsurface = EffectiveShadingModel == MSM_Subsurface;
            const auto IsClearCoat  = EffectiveShadingModel == MSM_ClearCoat;
            for (const auto& E : Get_ExtraOutputs())
            {
                // Opacity: translucent-family blends, plus Subsurface (where it drives the scatter amount).
                if (E.Prop == MP_Opacity         && NOT IsTranslucent && NOT IsSubsurface) { continue; }
                if (E.Prop == MP_OpacityMask     && EffectiveBlend != BLEND_Masked)        { continue; }
                if (E.Prop == MP_Refraction      && NOT WantsRefraction)                   { continue; }
                if (E.Prop == MP_SubsurfaceColor && NOT IsSubsurface)                      { continue; }
                if (E.Prop == MP_CustomData0     && NOT IsClearCoat)                       { continue; } // ClearCoat
                if (E.Prop == MP_CustomData1     && NOT IsClearCoat)                       { continue; } // ClearCoatRoughness
                UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(E.Name), E.Prop);
            }
        }

        // ---- WorldPositionOffset: a SEPARATE vertex Custom node (the pixel node above reads pixel-only
        //      inputs like PixelDepth/SceneTexture, which cannot legally compile in the vertex shader). ----
        if (IsSurface && NOT InDef->_WpoFunctionName.IsNone())
        {
            auto* Wpo = Cast<UMaterialExpressionCustom>(
                UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionCustom::StaticClass(), -400, 700));
            Wpo->OutputType = CMOT_Float3;          // single float3 world-space offset, no additional outputs
            Wpo->IncludeFilePaths.Add(InDef->_UshIncludePath);
            Wpo->AdditionalOutputs.Reset();

            Wpo->Inputs.Reset();
            { FCustomInput In; In.InputName = TEXT("WorldPosition"); Wpo->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexNormal");  Wpo->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexColor");   Wpo->Inputs.Add(In); }
            for (const auto& P : InDef->_Parameters)
            {
                FCustomInput In; In.InputName = P._Name; Wpo->Inputs.Add(In);
            }
            { FCustomInput T; T.InputName = TEXT("Time"); Wpo->Inputs.Add(T); }
            { FCustomInput U; U.InputName = TEXT("UV");   Wpo->Inputs.Add(U); }

            Wpo->Code = Build_WpoCustomCode(InDef);
            Apply_LookDefines(Wpo, InDef);
            Wpo->RebuildOutputs();
            Wpo->PostEditChange();

            // VS-safe world-space inputs (WorldPosition uses absolute world position so the look reads true coords).
            const auto AddWpoInput = [&](UClass* InClass, const TCHAR* InInputName, int32 InRow) -> void
            {
                auto* Expr = UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, InClass, -1100, 800 + InRow * 160);
                UMaterialEditingLibrary::ConnectMaterialExpressions(Expr, FString(), Wpo, InInputName);
            };
            AddWpoInput(UMaterialExpressionWorldPosition::StaticClass(), TEXT("WorldPosition"), 0);
            AddWpoInput(UMaterialExpressionVertexNormalWS::StaticClass(), TEXT("VertexNormal"),  1);
            AddWpoInput(UMaterialExpressionVertexColor::StaticClass(),    TEXT("VertexColor"),   2);

            // Param nodes — same helper as the pixel node; per-instance slot indices come from the
            // LookDefinition's layout, so the two nodes can never disagree (WPO runs in the VS where
            // per-instance data is valid).
            int32 WpoParamRow = 0;
            for (const auto& P : InDef->_Parameters)
            {
                auto* ParamExpr = Make_ParamExpression(Material, InDef, P, LookName, 800 + WpoParamRow * 120);
                if (ck::IsValid(ParamExpr, ck::IsValid_Policy_NullptrOnly{}))
                {
                    UMaterialEditingLibrary::ConnectMaterialExpressions(
                        ParamExpr, FString(), Wpo, P._Name.ToString());
                }
                ++WpoParamRow;
            }

            auto* WpoTime = UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionTime::StaticClass(), -800, 800 + (WpoParamRow + 1) * 120);
            UMaterialEditingLibrary::ConnectMaterialExpressions(WpoTime, FString(), Wpo, TEXT("Time"));

            auto* WpoUv = UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionTextureCoordinate::StaticClass(), -800, 800 + (WpoParamRow + 2) * 120);
            UMaterialEditingLibrary::ConnectMaterialExpressions(WpoUv, FString(), Wpo, TEXT("UV"));

            UMaterialEditingLibrary::ConnectMaterialProperty(Wpo, FString(), MP_WorldPositionOffset);
        }

        UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
        UMaterialEditingLibrary::RecompileMaterial(Material);
        Material->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(Material);

        // ---- Save ----
        const auto FileName = FPackageName::LongPackageNameToFilename(
            PkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, Material, *FileName, Args);

        ck::usf_editor::Log(TEXT("Generated master for look [{}]"), LookName);
        return Material;
    }

    auto Generate_LookMaterial(UCkUsf_LookDefinition* InDef) -> UMaterial*
    {
        FGenerateResult DiscardedResult;   // failures are also logged inside the worker
        return DoGenerate_LookMaterial(InDef, DiscardedResult);
    }

    // Forces the just-generated material's shaders to finish compiling and reports any compile
    // errors. This catches the "compiles as a UMaterial object but the HLSL is broken" case — the
    // silent fallback-to-default-material that object + MID checks miss. Notably covers PostProcess
    // permutations (FPostProcessMaterialPS), which only surface their errors once realised.
    static auto Validate_LookShaders(UMaterial* InMaterial, FName InLookName, TArray<FString>& OutErrors) -> bool
    {
        if (ck::Is_NOT_Valid(InMaterial, ck::IsValid_Policy_NullptrOnly{}))
        { return true; }

        // In a process that cannot render (-nullrhi CI) shader maps never build, so
        // IsCompilingOrHadCompileError reads EVERY look as failed — the gate is meaningless there.
        if (NOT FApp::CanEverRender())
        { return true; }

        if (GShaderCompilingManager != nullptr)
        { GShaderCompilingManager->FinishAllCompilation(); }

        if (NOT InMaterial->IsCompilingOrHadCompileError(GMaxRHIShaderPlatform))
        { return true; }

        // The specific HLSL error (file:line + message) is emitted by LogShaderCompilers during
        // the compile above; this surfaces WHICH look failed and fails the generate/test.
        const auto Msg = FString::Printf(
            TEXT("Look [%s] SHADER FAILED TO COMPILE — see the LogShaderCompilers '*.ush ... error:' line above"),
            *InLookName.ToString());
        ck::usf_editor::Error(TEXT("{}"), Msg);
        OutErrors.Add(Msg);
        return false;
    }

    auto Generate_AllLookMaterials() -> FGenerateResult
    {
        FGenerateResult Result;
        const auto& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        TArray<FAssetData> Assets;
        ARM.GetAssetsByClass(UCkUsf_LookDefinition::StaticClass()->GetClassPathName(), Assets);
        for (const auto& A : Assets)
        {
            auto* Def = Cast<UCkUsf_LookDefinition>(A.GetAsset());
            auto* Material = DoGenerate_LookMaterial(Def, Result);
            if (ck::Is_NOT_Valid(Material, ck::IsValid_Policy_NullptrOnly{}))
            { ++Result.NumSkipped; continue; }

            ++Result.NumGenerated;
            Validate_LookShaders(Material, Def->Get_EffectiveLookName(), Result.Errors);
        }
        ck::usf_editor::Log(TEXT("CkUsf generate: {} generated, {} skipped, {} error(s), {} warning(s)"),
            Result.NumGenerated, Result.NumSkipped, Result.Errors.Num(), Result.Warnings.Num());
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
