#include "CkUsfEditor/Generator/CkUsf_Generator.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsfEditor_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "MaterialEditingLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "SceneTypes.h"
#include "UObject/SavePackage.h"
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
    static auto Build_CustomCode(const UCkUsf_LookDefinition* InDef) -> FString
    {
        FString Args;
        for (const auto& P : InDef->_Parameters)
        {
            if (P._Type == ECk_Usf_ParamType::Texture) { continue; }  // v1: skip textures
            // Vector params connect a (possibly float4) output; .rgb makes the HLSL type float3.
            const auto Suffix = (P._Type == ECk_Usf_ParamType::Vector) ? TEXT(".rgb") : TEXT("");
            Args += FString::Printf(TEXT("%s%s, "), *P._Name.ToString(), Suffix);
        }
        Args += TEXT("Time, UV");

        FString Code = FString::Printf(
            TEXT("FCkUsf_SurfaceOutput O = %s(%s);\n"), *InDef->_UshFunctionName.ToString(), *Args);
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

        // ---- Create package + UMaterial (overwrite existing -> idempotent refresh) ----
        const auto PkgPath = ck::usf::Get_GeneratedMasterPackagePath(LookName);
        auto* Package = CreatePackage(*PkgPath);
        const auto AssetName = FString::Printf(TEXT("M_CkUsf_Look_%s"), *LookName.ToString());
        auto* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone);

        Material->MaterialDomain = Config.Domain;
        Material->BlendMode = Config.Blend;
        if (Config.Unlit) { Material->SetShadingModel(MSM_Unlit); }

        // ---- Custom node (added to material via editing library) ----
        auto* Custom = Cast<UMaterialExpressionCustom>(
            UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionCustom::StaticClass(), -400, 0));

        Custom->OutputType = CMOT_Float3;               // primary = BaseColor
        Custom->IncludeFilePaths.Add(InDef->_UshIncludePath);

        Custom->AdditionalOutputs.Reset();
        for (const auto& E : Get_ExtraOutputs())
        {
            Custom->AdditionalOutputs.Add(FCustomOutput{ FName(E.Name), E.Type });
        }

        // Inputs: one per (non-texture) param, then Time + UV.
        Custom->Inputs.Reset();
        for (const auto& P : InDef->_Parameters)
        {
            if (P._Type == ECk_Usf_ParamType::Texture) { continue; }
            FCustomInput In; In.InputName = P._Name; Custom->Inputs.Add(In);
        }
        { FCustomInput T; T.InputName = TEXT("Time"); Custom->Inputs.Add(T); }
        { FCustomInput U; U.InputName = TEXT("UV");   Custom->Inputs.Add(U); }

        Custom->Code = Build_CustomCode(InDef);
        Custom->RebuildOutputs();
        Custom->PostEditChange();

        // ---- Parameter nodes wired to Custom inputs by name ----
        int32 ParamRow = 0;
        for (const auto& P : InDef->_Parameters)
        {
            if (P._Type == ECk_Usf_ParamType::Texture) { continue; }  // v1
            UMaterialExpression* ParamExpr = nullptr;
            if (P._Type == ECk_Usf_ParamType::Scalar)
            {
                auto* S = Cast<UMaterialExpressionScalarParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        Material, UMaterialExpressionScalarParameter::StaticClass(), -800, ParamRow * 120));
                S->ParameterName = P._Name; S->DefaultValue = P._DefaultScalar; ParamExpr = S;
            }
            else // Vector
            {
                auto* V = Cast<UMaterialExpressionVectorParameter>(
                    UMaterialEditingLibrary::CreateMaterialExpression(
                        Material, UMaterialExpressionVectorParameter::StaticClass(), -800, ParamRow * 120));
                V->ParameterName = P._Name; V->DefaultValue = P._DefaultVector; ParamExpr = V;
            }
            UMaterialEditingLibrary::ConnectMaterialExpressions(
                ParamExpr, FString(), Custom, P._Name.ToString());
            ++ParamRow;
        }

        // ---- Time + UV built-ins ----
        auto* TimeExpr = UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionTime::StaticClass(), -800, (ParamRow + 1) * 120);
        UMaterialEditingLibrary::ConnectMaterialExpressions(TimeExpr, FString(), Custom, TEXT("Time"));

        auto* UvExpr = UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionTextureCoordinate::StaticClass(), -800, (ParamRow + 2) * 120);
        UMaterialEditingLibrary::ConnectMaterialExpressions(UvExpr, FString(), Custom, TEXT("UV"));

        // ---- Connect Custom outputs to material pins ----
        UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(), MP_BaseColor);
        for (const auto& E : Get_ExtraOutputs())
        {
            UMaterialEditingLibrary::ConnectMaterialProperty(Custom, FString(E.Name), E.Prop);
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

    auto Generate_AllLookMaterials() -> FGenerateResult
    {
        FGenerateResult Result;
        const auto& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        TArray<FAssetData> Assets;
        ARM.GetAssetsByClass(UCkUsf_LookDefinition::StaticClass()->GetClassPathName(), Assets);
        for (const auto& A : Assets)
        {
            auto* Def = Cast<UCkUsf_LookDefinition>(A.GetAsset());
            if (Generate_LookMaterial(Def) != nullptr) { ++Result.NumGenerated; }
            else { ++Result.NumSkipped; }
        }
        ck::usf_editor::Log(TEXT("CkUsf generate: {} generated, {} skipped"),
            Result.NumGenerated, Result.NumSkipped);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
