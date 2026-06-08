#include "CkUsfEditor/Generator/CkUsf_Generator.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
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
#include "Engine/Texture.h"
#include "SceneTypes.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
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

    // Additional outputs every look exposes besides the primary (BaseColor).
    struct FExtraOutput { const TCHAR* Name; ECustomMaterialOutputType Type; EMaterialProperty Prop; };

    static auto Get_ExtraOutputs() -> TArray<FExtraOutput>
    {
        return {
            { TEXT("EmissiveColor"), CMOT_Float3, MP_EmissiveColor },
            { TEXT("Roughness"),     CMOT_Float1, MP_Roughness },
            { TEXT("Metallic"),      CMOT_Float1, MP_Metallic },
            { TEXT("Normal"),        CMOT_Float3, MP_Normal },
            { TEXT("Opacity"),       CMOT_Float1, MP_Opacity },
        };
    }

    // ---- Build the Custom node Code (call + output assignments) ----
    static auto Build_CustomCode(const UCkUsf_LookDefinition* InDef, bool InIsPostProcess) -> FString
    {
        FString Args;
        if (InIsPostProcess)
        {
            // Managed scene-texture inputs come first (see Generate_LookMaterial).
            Args += TEXT("SceneColor, SceneDepth, SceneNormal, ");
        }
        for (const auto& P : InDef->_Parameters)
        {
            const auto Name = P._Name.ToString();
            switch (P._Type)
            {
                // Vector params connect a (possibly float4) output; .rgb makes the HLSL type float3.
                case ECk_Usf_ParamType::Vector:
                    Args += FString::Printf(TEXT("%s.rgb, "), *Name);
                    break;
                // Texture object inputs expose both the texture and an auto <Name>Sampler in HLSL.
                case ECk_Usf_ParamType::Texture2D:
                case ECk_Usf_ParamType::TextureCube:
                    Args += FString::Printf(TEXT("%s, %sSampler, "), *Name, *Name);
                    break;
                default: // Scalar
                    Args += FString::Printf(TEXT("%s, "), *Name);
                    break;
            }
        }
        Args += TEXT("Time, UV");

        FString Code = FString::Printf(
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
        Code += TEXT("Normal = O.Normal;\n");
        Code += TEXT("Opacity = O.Opacity;\n");
        Code += TEXT("return O.BaseColor;");
        return Code;
    }

    auto Generate_LookMaterial(UCkUsf_LookDefinition* InDef) -> UMaterial*
    {
        if (ck::Is_NOT_Valid(InDef, ck::IsValid_Policy_NullptrOnly{}))
        {
            ck::usf_editor::Warning(TEXT("Null LookDefinition"));
            return nullptr;
        }

        const auto LookName = InDef->Get_EffectiveLookName();
        if (InDef->_UshFunctionName.IsNone() || InDef->_UshIncludePath.IsEmpty())
        {
            ck::usf_editor::Warning(TEXT("Look [{}] missing ush function/include"), LookName);
            return nullptr;
        }

        const auto Config = Get_DomainConfig(InDef->_Domain);

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
        Material->BlendMode = Config.Blend;
        if (Config.Unlit) { Material->SetShadingModel(MSM_Unlit); }

        if (Config.Domain == MD_PostProcess)
        {
            // A programmatically-created PP material does not reliably default to a compositing
            // location; set it explicitly so the blendable actually draws over the final image.
            Material->BlendableLocation = BL_SceneColorAfterTonemapping;
            Material->BlendablePriority = 0;
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

        // Inputs: [scene textures (PP only)] + one per param (incl. textures) + Time + UV.
        Custom->Inputs.Reset();
        if (IsPostProcess)
        {
            { FCustomInput In; In.InputName = TEXT("SceneColor");  Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("SceneDepth");  Custom->Inputs.Add(In); }
            { FCustomInput In; In.InputName = TEXT("SceneNormal"); Custom->Inputs.Add(In); }
        }
        for (const auto& P : InDef->_Parameters)
        {
            FCustomInput In; In.InputName = P._Name; Custom->Inputs.Add(In);
        }
        { FCustomInput T; T.InputName = TEXT("Time"); Custom->Inputs.Add(T); }
        { FCustomInput U; U.InputName = TEXT("UV");   Custom->Inputs.Add(U); }

        Custom->Code = Build_CustomCode(InDef, IsPostProcess);
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
            AddSceneTexture(PPI_PostProcessInput0, TEXT("SceneColor"),  0);
            AddSceneTexture(PPI_SceneDepth,        TEXT("SceneDepth"),  1);
            AddSceneTexture(PPI_WorldNormal,       TEXT("SceneNormal"), 2);
        }

        // ---- Parameter nodes wired to Custom inputs by name ----
        int32 ParamRow = 0;
        for (const auto& P : InDef->_Parameters)
        {
            UMaterialExpression* ParamExpr = nullptr;
            switch (P._Type)
            {
                case ECk_Usf_ParamType::Scalar:
                {
                    auto* S = Cast<UMaterialExpressionScalarParameter>(
                        UMaterialEditingLibrary::CreateMaterialExpression(
                            Material, UMaterialExpressionScalarParameter::StaticClass(), -800, ParamRow * 120));
                    S->ParameterName = P._Name; S->DefaultValue = P._DefaultScalar; ParamExpr = S;
                    break;
                }
                case ECk_Usf_ParamType::Vector:
                {
                    auto* V = Cast<UMaterialExpressionVectorParameter>(
                        UMaterialEditingLibrary::CreateMaterialExpression(
                            Material, UMaterialExpressionVectorParameter::StaticClass(), -800, ParamRow * 120));
                    V->ParameterName = P._Name; V->DefaultValue = P._DefaultVector; ParamExpr = V;
                    break;
                }
                case ECk_Usf_ParamType::Texture2D:
                case ECk_Usf_ParamType::TextureCube:
                {
                    auto* T = Cast<UMaterialExpressionTextureObjectParameter>(
                        UMaterialEditingLibrary::CreateMaterialExpression(
                            Material, UMaterialExpressionTextureObjectParameter::StaticClass(), -800, ParamRow * 120));
                    T->ParameterName = P._Name;
                    if (P._DefaultTexturePath.IsEmpty() == false)
                    {
                        auto* Tex = LoadObject<UTexture>(nullptr, *P._DefaultTexturePath);
                        if (ck::IsValid(Tex, ck::IsValid_Policy_NullptrOnly{}))
                        {
                            T->Texture = Tex;
                            T->AutoSetSampleType();
                        }
                        else
                        {
                            ck::usf_editor::Warning(TEXT("Look [{}] texture not found: [{}]"),
                                LookName, P._DefaultTexturePath);
                        }
                    }
                    ParamExpr = T;
                    break;
                }
            }

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
            for (const auto& E : Get_ExtraOutputs())
            {
                UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(E.Name), E.Prop);
            }
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

    // Forces the just-generated material's shaders to finish compiling and reports any compile
    // errors. This catches the "compiles as a UMaterial object but the HLSL is broken" case — the
    // silent fallback-to-default-material that object + MID checks miss. Notably covers PostProcess
    // permutations (FPostProcessMaterialPS), which only surface their errors once realised.
    static auto Validate_LookShaders(UMaterial* InMaterial, FName InLookName, TArray<FString>& OutErrors) -> bool
    {
        if (ck::Is_NOT_Valid(InMaterial, ck::IsValid_Policy_NullptrOnly{}))
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
            auto* Material = Generate_LookMaterial(Def);
            if (ck::Is_NOT_Valid(Material, ck::IsValid_Policy_NullptrOnly{}))
            { ++Result.NumSkipped; continue; }

            ++Result.NumGenerated;
            Validate_LookShaders(Material, Def->Get_EffectiveLookName(), Result.Errors);
        }
        ck::usf_editor::Log(TEXT("CkUsf generate: {} generated, {} skipped, {} shader error(s)"),
            Result.NumGenerated, Result.NumSkipped, Result.Errors.Num());
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
