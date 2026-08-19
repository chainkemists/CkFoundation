#include "CkUsfEditor/Generator/CkUsf_Generator.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsfEditor/Generator/CkUsf_LookValidator.h"
#include "CkUsfEditor_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "MaterialEditingLibrary.h"
#include "MaterialShaderPrecompileMode.h"
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
#include "Materials/MaterialExpressionPreSkinnedPosition.h"
#include "Materials/MaterialExpressionVertexTangentWS.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
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

    static auto Resolve_TranslucencyLighting(ECk_Usf_TranslucencyLighting In) -> TOptional<ETranslucencyLightingMode>
    {
        const auto KeepEngineDefault = TOptional<ETranslucencyLightingMode>{};

        switch (In)
        {
            case ECk_Usf_TranslucencyLighting::VolumetricNonDirectional: return TLM_VolumetricNonDirectional;
            case ECk_Usf_TranslucencyLighting::VolumetricDirectional:    return TLM_VolumetricDirectional;
            case ECk_Usf_TranslucencyLighting::Surface:                  return TLM_Surface;
            case ECk_Usf_TranslucencyLighting::SurfacePerPixel:          return TLM_SurfacePerPixelLighting;
            default:                                                     return KeepEngineDefault;
        }
    }

    // Placement trade-offs (pre-TAA vs post-tonemap): CkUsf/Claude.md § Blendable location.
    static auto Resolve_BlendableLocation(ECk_Usf_BlendableLocation In) -> EBlendableLocation
    {
        switch (In)
        {
            case ECk_Usf_BlendableLocation::SceneColorAfterDOF:    return BL_SceneColorAfterDOF;
            case ECk_Usf_BlendableLocation::SceneColorBeforeDOF:   return BL_SceneColorBeforeDOF;
            case ECk_Usf_BlendableLocation::SceneColorBeforeBloom: return BL_SceneColorBeforeBloom;
            case ECk_Usf_BlendableLocation::ReplacingTonemapper:   return BL_ReplacingTonemapper;
            default:                                               return BL_SceneColorAfterTonemapping;
        }
    }

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

    static auto Is_TranslucentFamily(EBlendMode In) -> bool
    {
        return In == BLEND_Translucent || In == BLEND_Additive
            || In == BLEND_Modulate    || In == BLEND_AlphaComposite;
    }

    // The Custom node declares all of these; the pin WIRING in Generate_LookMaterial gates them by blend/shading model.
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
            // GBuffer reads. PPI_SceneColor is REJECTED in the PostProcess domain, which is why the
            // SceneColor row above stays on PPI_PostProcessInput0 — never add a PPI_SceneColor row.
            case ECk_Usf_SceneTexture::BaseColor:     return { PPI_BaseColor,         TEXT("SceneBaseColor") };
            case ECk_Usf_SceneTexture::Metallic:      return { PPI_Metallic,          TEXT("SceneMetallic") };
            case ECk_Usf_SceneTexture::Roughness:     return { PPI_Roughness,         TEXT("SceneRoughness") };
            case ECk_Usf_SceneTexture::Specular:      return { PPI_Specular,          TEXT("SceneSpecular") };
            default:                                  return { PPI_PostProcessInput0, TEXT("SceneColor") };
        }
    }

    static auto Get_EffectiveSceneTextures(const UCkUsf_LookDefinition* InDef) -> TArray<ECk_Usf_SceneTexture>
    {
        if (InDef->_SceneTextures.IsEmpty() == false)
        { return InDef->_SceneTextures; }
        return { ECk_Usf_SceneTexture::SceneColor, ECk_Usf_SceneTexture::SceneDepth, ECk_Usf_SceneTexture::SceneNormal };
    }

    // Exactly four names — UMaterialExpressionDynamicParameter indexes ParamNames[0..3] unguarded
    // (MaterialExpressions.cpp:9965-9972), so a short array from the asset would read out of bounds.
    // Blank or missing entries fall back to the engine's own Param1..4.
    static auto Get_DynamicParameterNames(const UCkUsf_LookDefinition* InDef) -> TArray<FString>
    {
        auto Names = TArray<FString>{ TEXT("Param1"), TEXT("Param2"), TEXT("Param3"), TEXT("Param4") };

        const auto NumAuthored = FMath::Min(InDef->_ParticleDynamicParameterNames.Num(), 4);
        for (auto Index = 0; Index < NumAuthored; ++Index)
        {
            if (const auto& Authored = InDef->_ParticleDynamicParameterNames[Index]; NOT Authored.IsEmpty())
            { Names[Index] = Authored; }
        }

        return Names;
    }

    // ---- Build the Custom node Code (assemble FCkUsf_SurfaceInput, call, assign outputs) ----
    static auto Build_CustomCode(const UCkUsf_LookDefinition* InDef, bool InIsPostProcess) -> FString
    {
        FString Code = TEXT("FCkUsf_SurfaceInput In = CkUsf_DefaultInput();\n");
        Code += TEXT("In.Time = Time;\n");
        Code += TEXT("In.UV = UV;\n");
        if (InIsPostProcess)
        {
            for (const auto& Tex : Get_EffectiveSceneTextures(InDef))
            {
                const auto* Name = Get_SceneTextureWiring(Tex).HlslName;
                Code += FString::Printf(TEXT("In.%s = %s;\n"), Name, Name);
            }
            if (InDef->_PostProcessWorldPosition)
            { Code += TEXT("In.WorldPosition = WorldPosition;\n"); }
        }
        else
        {
            Code += TEXT("In.WorldPosition = WorldPosition;\n");
            Code += TEXT("In.CameraVector = CameraVector;\n");
            Code += TEXT("In.VertexNormal = VertexNormal;\n");
            Code += TEXT("In.VertexTangent = VertexTangent;\n");
            Code += TEXT("In.PixelDepth = PixelDepth;\n");
            Code += TEXT("In.VertexColor = VertexColor;\n");
            if (InDef->_PixelDataChannels)
            {
                Code += TEXT("In.UV1 = UV1;\n");
                Code += TEXT("In.UV2 = UV2;\n");
            }
            if (InDef->_ParticleColor)
            {
                // UMaterialExpressionParticleColor's output 0 is "RGB" (a float3); alpha is a SEPARATE output
                // (MaterialExpressions.cpp:9869). Assembling the float4 here keeps In.ParticleColor a float4
                // for looks that need the particle's alpha.
                Code += TEXT("In.ParticleColor = float4(ParticleColor, ParticleAlpha);\n");
            }
            if (InDef->_ParticleDynamicParameter)
            {
                // Assembled from four scalar pins rather than one float4: UMaterialExpressionDynamicParameter
                // exposes four SCALAR outputs (MaterialExpressions.cpp:9965-9972), so a float4 would need an
                // AppendVector chain — which has already failed to compile under SM6 in this codebase.
                Code += TEXT("In.DynamicParameter = float4(DynParam0, DynParam1, DynParam2, DynParam3);\n");
            }
        }

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
                case ECk_Usf_ParamType::Texture2DArray:
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

    // ---- Build the WPO (vertex) Custom node Code — VS-safe FCkUsf_VertexInput in, world-space offset out ----
    static auto Build_WpoCustomCode(const UCkUsf_LookDefinition* InDef) -> FString
    {
        FString Code = TEXT("FCkUsf_VertexInput In = CkUsf_DefaultVertexInput();\n");
        Code += TEXT("In.Time = Time;\n");
        Code += TEXT("In.UV = UV;\n");
        Code += TEXT("In.UV1 = UV1;\n");
        Code += TEXT("In.UV2 = UV2;\n");
        Code += TEXT("In.WorldPosition = WorldPosition;\n");
        Code += TEXT("In.LocalPosition = LocalPosition;\n");
        Code += TEXT("In.VertexNormal = VertexNormal;\n");
        Code += TEXT("In.VertexColor = VertexColor;\n");
        // Per-INSTANCE local->world basis: TransformLocalVectorToWorld's VS overload multiplies by
        // Parameters.InstanceLocalToWorld (MaterialTemplate.ush), and Parameters is in scope inside
        // Custom-node code, so no extra pins are needed.
        Code += TEXT("In.LocalAxisX = TransformLocalVectorToWorld(Parameters, float3(1.0, 0.0, 0.0));\n");
        Code += TEXT("In.LocalAxisY = TransformLocalVectorToWorld(Parameters, float3(0.0, 1.0, 0.0));\n");
        Code += TEXT("In.LocalAxisZ = TransformLocalVectorToWorld(Parameters, float3(0.0, 0.0, 1.0));\n");

        // Same params in the same order as the pixel fn — the validator checks both against one list.
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
                case ECk_Usf_ParamType::Texture2DArray:
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

    // ConstDefaultValue is the uniform fallback a PerInstanceCustomData node returns on non-instanced meshes.
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
                S->Group = P._Group; S->SortPriority = P._SortPriority;
                return S;
            }
            case ECk_Usf_ParamType::Vector:
            {
                auto* V = Cast<UMaterialExpressionVectorParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        InMaterial, UMaterialExpressionVectorParameter::StaticClass(), -800, InRow));
                V->ParameterName = P._Name; V->DefaultValue = P._DefaultVector;
                V->Group = P._Group; V->SortPriority = P._SortPriority;
                return V;
            }
            default: // Texture2D / TextureCube
            {
                auto* T = Cast<UMaterialExpressionTextureObjectParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        InMaterial, UMaterialExpressionTextureObjectParameter::StaticClass(), -800, InRow));
                T->ParameterName = P._Name;
                // Group/SortPriority live on UMaterialExpressionTextureSampleParameter, a DIFFERENT base
                // from the scalar/vector one -- same field names, no shared parent that declares them.
                T->Group = P._Group; T->SortPriority = P._SortPriority;
                if (P._DefaultTexturePath.IsEmpty() == false)
                {
                    auto* Tex = LoadObject<UTexture>(nullptr, *P._DefaultTexturePath);
                    if (ck::IsValid(Tex))
                    { T->Texture = Tex; T->AutoSetSampleType(); }
                    else { ck::usf_editor::Warning(TEXT("Look [{}] texture not found: [{}]"), InLookName, P._DefaultTexturePath); }
                }
                return T;
            }
        }
    }

    static auto DoGenerate_LookMaterial(
        UCkUsf_LookDefinition* InDef, FGenerateResult& InOutResult, const FString& InPackageRootOverride) -> UMaterial*
    {
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
        const auto EffectiveBlend = IsSurface ? Resolve_BlendMode(InDef->_BlendMode, Config.Blend) : Config.Blend;
        const auto EffectiveShadingModel = IsSurface
            ? Resolve_ShadingModel(InDef->_ShadingModel, Config.Unlit)
            : (Config.Unlit ? MSM_Unlit : MSM_DefaultLit);
        const auto IsTranslucent = Is_TranslucentFamily(EffectiveBlend);
        const auto WantsRefraction = IsSurface && IsTranslucent && EffectiveShadingModel != MSM_Unlit;

        // ---- Create package + UMaterial (idempotent refresh) ----
        const auto PkgPath = ck::usf::Get_GeneratedMasterPackagePath(LookName, InPackageRootOverride);
        const auto AssetName = FString::Printf(TEXT("M_CkUsf_Look_%s"), *LookName.ToString());

        // An asset-registry stub leaves a previously-generated package partially loaded and SavePackage
        // asserts; the old material is renamed away so NewObject can reuse its name.
        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, *PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr)
        { Package = CreatePackage(*PkgPath); }

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
        if (IsSurface)
        { Material->TwoSided = InDef->_TwoSided; }

        Material->bUsedWithInstancedStaticMeshes = InDef->_UsedWithInstancedStaticMeshes;
        Material->bUsedWithSkeletalMesh = InDef->_UsedWithSkeletalMesh;
        Material->bUsedWithMorphTargets = InDef->_UsedWithMorphTargets;
        Material->bUsedWithNanite = InDef->_UsedWithNanite;
        Material->bUsedWithNiagaraSprites = InDef->_UsedWithNiagaraSprites;
        Material->bUsedWithNiagaraMeshParticles = InDef->_UsedWithNiagaraMeshParticles;
        Material->bUsedWithNiagaraRibbons = InDef->_UsedWithNiagaraRibbons;

        // The Refraction pin is inert unless RefractionMethod is set; a look wanting no bend outputs 1.0 (air).
        if (WantsRefraction)
        { Material->RefractionMethod = RM_IndexOfRefraction; }

        if (IsSurface && IsTranslucent && EffectiveShadingModel != MSM_Unlit)
        {
            if (const auto TranslucencyLighting = Resolve_TranslucencyLighting(InDef->_TranslucencyLighting);
                TranslucencyLighting.IsSet())
            { Material->TranslucencyLightingMode = TranslucencyLighting.GetValue(); }
        }

        if (Config.Domain == MD_PostProcess)
        {
            // A programmatically-created PP material does not reliably default to a compositing location.
            Material->BlendableLocation = Resolve_BlendableLocation(InDef->_BlendableLocation);
            Material->BlendablePriority = 0;
        }

        if (Config.Domain == MD_Surface)
        {
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

        Custom->Inputs.Reset();
        if (IsPostProcess)
        {
            for (const auto& Tex : Get_EffectiveSceneTextures(InDef))
            {
                FCustomInput In; In.InputName = FName(Get_SceneTextureWiring(Tex).HlslName); Custom->Inputs.Add(In);
            }
            if (InDef->_PostProcessWorldPosition)
            { FCustomInput In; In.InputName = TEXT("WorldPosition"); Custom->Inputs.Add(In); }
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
        if (NOT IsPostProcess && InDef->_PixelDataChannels)
        {
            { FCustomInput U; U.InputName = TEXT("UV1"); Custom->Inputs.Add(U); }
            { FCustomInput U; U.InputName = TEXT("UV2"); Custom->Inputs.Add(U); }
        }
        if (NOT IsPostProcess && InDef->_ParticleColor)
        {
            { FCustomInput P; P.InputName = TEXT("ParticleColor"); Custom->Inputs.Add(P); }
            { FCustomInput P; P.InputName = TEXT("ParticleAlpha"); Custom->Inputs.Add(P); }
        }
        if (NOT IsPostProcess && InDef->_ParticleDynamicParameter)
        {
            for (int32 Channel = 0; Channel < 4; ++Channel)
            {
                FCustomInput D;
                D.InputName = FName(*FString::Printf(TEXT("DynParam%d"), Channel));
                Custom->Inputs.Add(D);
            }
        }

        Custom->Code = Build_CustomCode(InDef, IsPostProcess);
        Apply_LookDefines(Custom, InDef);
        Custom->RebuildOutputs();
        Custom->PostEditChange();

        // ---- PostProcess scene-texture inputs — also declare the usage that legalizes raw SceneTextureLookup() in the .ush ----
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

            // The SAME engine expression the surface branch wires — in the PostProcess domain it resolves to
            // the depth-reconstructed scene surface position rather than the fullscreen quad's own.
            if (InDef->_PostProcessWorldPosition)
            {
                auto* WorldPositionExpr = UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionWorldPosition::StaticClass(), -1100, SceneTexRow * 160);
                UMaterialEditingLibrary::ConnectMaterialExpressions(
                    WorldPositionExpr, FString(), Custom, TEXT("WorldPosition"));
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

            // The pin is what forces the translator to allocate the coords; reading Parameters.TexCoords[n] would not.
            if (InDef->_PixelDataChannels)
            {
                const auto AddUvChannel = [&](const TCHAR* InInputName, int32 InCoordinateIndex, int32 InRow) -> void
                {
                    auto* Expr = Cast<UMaterialExpressionTextureCoordinate>(
                        UMaterialEditingLibrary::CreateMaterialExpression(
                            Material, UMaterialExpressionTextureCoordinate::StaticClass(), -1100, InRow * 160));
                    Expr->CoordinateIndex = InCoordinateIndex;
                    UMaterialEditingLibrary::ConnectMaterialExpressions(Expr, FString(), Custom, InInputName);
                };
                AddUvChannel(TEXT("UV1"), 1, 6);
                AddUvChannel(TEXT("UV2"), 2, 7);
            }

            if (InDef->_ParticleColor)
            {
                // One node, two taps: output "RGB" (float3) and output "A" (float). Connecting the default
                // output would hand the Custom node a float3 for a float4 field.
                auto* PColor = UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionParticleColor::StaticClass(), -1100, 8 * 160);
                UMaterialEditingLibrary::ConnectMaterialExpressions(PColor, TEXT("RGB"), Custom, TEXT("ParticleColor"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PColor, TEXT("A"),   Custom, TEXT("ParticleAlpha"));
            }

            if (InDef->_ParticleDynamicParameter)
            {
                auto* Dyn = Cast<UMaterialExpressionDynamicParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        Material, UMaterialExpressionDynamicParameter::StaticClass(), -1100, 9 * 160));

                Dyn->ParameterIndex = 0;
                Dyn->DefaultValue = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                Dyn->ParamNames = Get_DynamicParameterNames(InDef);

                // ConnectMaterialExpressions matches against the RAW Outputs array, which still carries the
                // CDO's "Param1..4" until GetOutputs() syncs it from ParamNames (MaterialExpressions.cpp:9965).
                // Without this call every by-name connect below silently no-ops.
                Dyn->GetOutputs();

                for (int32 Channel = 0; Channel < 4; ++Channel)
                {
                    UMaterialEditingLibrary::ConnectMaterialExpressions(
                        Dyn, Dyn->ParamNames[Channel],
                        Custom, FString::Printf(TEXT("DynParam%d"), Channel));
                }
            }
        }

        // ---- Parameter nodes wired to Custom inputs by name (per-instance params → PerInstanceCustomData) ----
        int32 ParamRow = 0;
        for (const auto& P : InDef->_Parameters)
        {
            auto* ParamExpr = Make_ParamExpression(Material, InDef, P, LookName, ParamRow * 120);
            if (ck::IsValid(ParamExpr))
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
            // EmissiveColor is the only valid PostProcess pin; the primary output IS the final pixel.
            UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(), MP_EmissiveColor);
        }
        else
        {
            UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(), MP_BaseColor);
            const auto IsSubsurface = EffectiveShadingModel == MSM_Subsurface;
            const auto IsClearCoat  = EffectiveShadingModel == MSM_ClearCoat;
            for (const auto& E : Get_ExtraOutputs())
            {
                if (E.Prop == MP_Opacity         && NOT IsTranslucent && NOT IsSubsurface)
                { continue; }
                if (E.Prop == MP_OpacityMask     && EffectiveBlend != BLEND_Masked)
                { continue; }
                if (E.Prop == MP_Refraction      && NOT WantsRefraction)
                { continue; }
                if (E.Prop == MP_SubsurfaceColor && NOT IsSubsurface)
                { continue; }
                if (E.Prop == MP_CustomData0     && NOT IsClearCoat)                       { continue; } // ClearCoat
                if (E.Prop == MP_CustomData1     && NOT IsClearCoat)                       { continue; } // ClearCoatRoughness
                UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(E.Name), E.Prop);
            }
        }

        // ---- WorldPositionOffset: a SEPARATE vertex node — the pixel node reads VS-illegal inputs (PixelDepth/SceneTexture) ----
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
            { FCustomInput In; In.InputName = TEXT("LocalPosition"); Wpo->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexNormal");  Wpo->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("VertexColor");   Wpo->Inputs.Add(In); }
            for (const auto& P : InDef->_Parameters)
            {
                FCustomInput In; In.InputName = P._Name; Wpo->Inputs.Add(In);
            }
            { FCustomInput T; T.InputName = TEXT("Time"); Wpo->Inputs.Add(T); }
            { FCustomInput U; U.InputName = TEXT("UV");   Wpo->Inputs.Add(U); }
            { FCustomInput U; U.InputName = TEXT("UV1");  Wpo->Inputs.Add(U); }
            { FCustomInput U; U.InputName = TEXT("UV2");  Wpo->Inputs.Add(U); }

            Wpo->Code = Build_WpoCustomCode(InDef);
            Apply_LookDefines(Wpo, InDef);
            Wpo->RebuildOutputs();
            Wpo->PostEditChange();

            const auto AddWpoInput = [&](UClass* InClass, const TCHAR* InInputName, int32 InRow) -> void
            {
                auto* Expr = UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, InClass, -1100, 800 + InRow * 160);
                UMaterialEditingLibrary::ConnectMaterialExpressions(Expr, FString(), Wpo, InInputName);
            };
            AddWpoInput(UMaterialExpressionWorldPosition::StaticClass(), TEXT("WorldPosition"), 0);
            // Pre-skinned position == the LOCAL bind-pose vertex position on static/instanced meshes.
            AddWpoInput(UMaterialExpressionPreSkinnedPosition::StaticClass(), TEXT("LocalPosition"), 3);
            AddWpoInput(UMaterialExpressionVertexNormalWS::StaticClass(), TEXT("VertexNormal"),  1);
            AddWpoInput(UMaterialExpressionVertexColor::StaticClass(),    TEXT("VertexColor"),   2);

            int32 WpoParamRow = 0;
            for (const auto& P : InDef->_Parameters)
            {
                auto* ParamExpr = Make_ParamExpression(Material, InDef, P, LookName, 800 + WpoParamRow * 120);
                if (ck::IsValid(ParamExpr))
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

            const auto AddWpoUvChannel = [&](const TCHAR* InInputName, int32 InCoordinateIndex, int32 InRow) -> void
            {
                auto* Expr = Cast<UMaterialExpressionTextureCoordinate>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        Material, UMaterialExpressionTextureCoordinate::StaticClass(), -800, 800 + InRow * 120));
                Expr->CoordinateIndex = InCoordinateIndex;
                UMaterialEditingLibrary::ConnectMaterialExpressions(Expr, FString(), Wpo, InInputName);
            };
            AddWpoUvChannel(TEXT("UV1"), 1, WpoParamRow + 3);
            AddWpoUvChannel(TEXT("UV2"), 2, WpoParamRow + 4);

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

    auto Generate_LookMaterial(UCkUsf_LookDefinition* InDef, const FString& InPackageRootOverride) -> UMaterial*
    {
        FGenerateResult DiscardedResult;   // failures are also logged inside the worker
        return DoGenerate_LookMaterial(InDef, DiscardedResult, InPackageRootOverride);
    }

    // Catches HLSL that compiles as a UMaterial object but fails its shader permutations (notably PostProcess).
    auto Validate_LookShaderCompile(
        UMaterial* InMaterial, FName InLookName, TArray<FString>& OutErrors, bool InForceSynchronousCompile) -> bool
    {
        if (ck::Is_NOT_Valid(InMaterial))
        { return true; }

        // A process that cannot render (-nullrhi CI) never builds shader maps, so the checks below would read EVERY look as failed.
        if (NOT FApp::CanEverRender())
        { return true; }

        // Opt-in, and DESTRUCTIVE — see the header for why the roster is not force-compiled. Only a caller that
        // owns a throwaway master and needs a real compile verdict should ask for this.
        if (InForceSynchronousCompile)
        {
            FMaterialUpdateContext UpdateContext;
            UpdateContext.AddMaterial(InMaterial);
            InMaterial->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Synchronous);
        }

        if (GShaderCompilingManager != nullptr)
        { GShaderCompilingManager->FinishAllCompilation(); }

        // A failed shader job does BOTH of these (ShaderCompiler.cpp:2177): it copies the job's unique errors onto
        // the resource and hands the material a null shader map. Reading the errors is what makes a failure name
        // itself instead of pointing at the log — and it is the ONLY one of the two that is trustworthy here:
        // a missing shader map is also what a not-yet-applied compile looks like, so failing on it reported
        // 49/49 healthy looks as broken. The errors decide; the missing map is a warning.
        auto CompileErrors = TArray<FString>{};
        if (const auto* Resource = InMaterial->GetMaterialResource(GMaxRHIShaderPlatform))
        { CompileErrors = Resource->GetCompileErrors(); }

        if (NOT InMaterial->IsCompilingOrHadCompileError(GMaxRHIShaderPlatform) && CompileErrors.IsEmpty())
        { return true; }

        if (CompileErrors.IsEmpty())
        {
            ck::usf_editor::Warning(
                TEXT("Look [{}] has no applied shader map yet, but reported no HLSL errors — treated as clean; "
                     "a real failure names its error"), InLookName);
            return true;
        }

        auto Msg = FString::Printf(
            TEXT("Look [%s] SHADER FAILED TO COMPILE"), *InLookName.ToString());

        constexpr auto MaxReportedErrors = 3;
        for (auto Index = 0; Index < FMath::Min(CompileErrors.Num(), MaxReportedErrors); ++Index)
        { Msg += FString::Printf(TEXT("\n    %s"), *CompileErrors[Index]); }

        if (CompileErrors.Num() > MaxReportedErrors)
        { Msg += FString::Printf(TEXT("\n    (+%d more — see LogShaderCompilers)"), CompileErrors.Num() - MaxReportedErrors); }

        ck::usf_editor::Error(TEXT("{}"), Msg);
        OutErrors.Add(Msg);
        return false;
    }

    auto Generate_AllLookMaterials(const FString& InPackageRootOverride) -> FGenerateResult
    {
        FGenerateResult Result;
        const auto& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        TArray<FAssetData> Assets;
        ARM.GetAssetsByClass(UCkUsf_LookDefinition::StaticClass()->GetClassPathName(), Assets);
        for (const auto& A : Assets)
        {
            auto* Def = Cast<UCkUsf_LookDefinition>(A.GetAsset());
            auto* Material = DoGenerate_LookMaterial(Def, Result, InPackageRootOverride);
            if (ck::Is_NOT_Valid(Material))
            { ++Result.NumSkipped; continue; }

            ++Result.NumGenerated;
            constexpr auto ForceSynchronousCompile = false;   // see the header — forcing leaves masters unrenderable
            Validate_LookShaderCompile(Material, Def->Get_EffectiveLookName(), Result.Errors, ForceSynchronousCompile);
        }
        ck::usf_editor::Log(TEXT("CkUsf generate: {} generated, {} skipped, {} error(s), {} warning(s)"),
            Result.NumGenerated, Result.NumSkipped, Result.Errors.Num(), Result.Warnings.Num());
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
