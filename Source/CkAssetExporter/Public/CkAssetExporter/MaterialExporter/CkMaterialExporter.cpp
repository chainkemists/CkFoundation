#include "CkMaterialExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Engine/Texture.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Misc/Paths.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MaterialExporter::
    ExportMaterial(
        UMaterialInterface* InMaterial,
        const FString& InOutputDir)
    -> FCk_MaterialExportResult
{
    auto Result = FCk_MaterialExportResult{};
    if (ck::Is_NOT_Valid(InMaterial))
    {
        Result.ErrorMessage = TEXT("Invalid Material asset");
        return Result;
    }

    Result.AssetName = InMaterial->GetName();

    auto Textures = TSet<FString>{};
    const auto JsonObject = DoSerializeToJson(InMaterial, Textures);
    Result.ReferencedTextures = Textures.Array();

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    constexpr auto CreateTree = true;
    IFileManager::Get().MakeDirectory(*InOutputDir, CreateTree);

    const auto JsonPath = FPaths::Combine(InOutputDir, InMaterial->GetName() + TEXT(".json"));
    if (NOT FFileHelper::SaveStringToFile(JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write [{}]"), JsonPath);
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MaterialExporter::
    DoSerializeToJson(
        UMaterialInterface* InMaterial,
        TSet<FString>& OutTextures)
    -> TSharedPtr<FJsonObject>
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("material"), InMaterial->GetName());
    Root->SetStringField(TEXT("packagePath"), InMaterial->GetOutermost()->GetName());

    // ---- Instance chain down to the base material ----
    auto ParentChain = TArray<TSharedPtr<FJsonValue>>{};
    auto* Current = InMaterial;
    while (const auto* Instance = Cast<UMaterialInstance>(Current))
    {
        if (Instance->Parent == nullptr)
        { break; }
        ParentChain.Add(MakeShared<FJsonValueString>(Instance->Parent->GetPathName()));
        Current = Instance->Parent;
    }
    if (ParentChain.Num() > 0)
    { Root->SetArrayField(TEXT("parentChain"), ParentChain); }

    // ---- Base properties (resolved through the interface, so instance overrides apply) ----
    Root->SetStringField(TEXT("blendMode"), StaticEnum<EBlendMode>()->GetNameStringByValue(static_cast<int64>(InMaterial->GetBlendMode())));
    Root->SetBoolField(TEXT("twoSided"), InMaterial->IsTwoSided());

    const auto ShadingModels = InMaterial->GetShadingModels();
    Root->SetBoolField(TEXT("unlit"), ShadingModels.HasShadingModel(MSM_Unlit));
    Root->SetStringField(TEXT("shadingModel"),
        StaticEnum<EMaterialShadingModel>()->GetNameStringByValue(static_cast<int64>(ShadingModels.GetFirstShadingModel())));

    const auto* BaseMaterial = InMaterial->GetMaterial();
    if (BaseMaterial != nullptr)
    {
        Root->SetStringField(TEXT("domain"), StaticEnum<EMaterialDomain>()->GetNameStringByValue(static_cast<int64>(BaseMaterial->MaterialDomain)));

#if WITH_EDITORONLY_DATA
        // Which outputs are actually wired — additive glow vs alpha-blended smoke is decided here.
        if (const auto* EditorOnly = BaseMaterial->GetEditorOnlyData())
        {
            auto Connected = TArray<TSharedPtr<FJsonValue>>{};
            if (EditorOnly->EmissiveColor.IsConnected()) { Connected.Add(MakeShared<FJsonValueString>(TEXT("EmissiveColor"))); }
            if (EditorOnly->BaseColor.IsConnected())     { Connected.Add(MakeShared<FJsonValueString>(TEXT("BaseColor"))); }
            if (EditorOnly->Opacity.IsConnected())       { Connected.Add(MakeShared<FJsonValueString>(TEXT("Opacity"))); }
            if (EditorOnly->OpacityMask.IsConnected())   { Connected.Add(MakeShared<FJsonValueString>(TEXT("OpacityMask"))); }
            if (EditorOnly->Normal.IsConnected())        { Connected.Add(MakeShared<FJsonValueString>(TEXT("Normal"))); }
            Root->SetArrayField(TEXT("connectedOutputs"), Connected);
        }

        // Expression histogram — the "tricks list". Full counts by class plus dynamic-parameter names.
        auto Histogram = TMap<FString, int32>{};
        auto DynamicParamNames = TSet<FString>{};
        for (const auto& Expression : BaseMaterial->GetExpressions())
        {
            if (Expression == nullptr)
            { continue; }
            auto ClassName = Expression->GetClass()->GetName();
            ClassName.RemoveFromStart(TEXT("MaterialExpression"));
            ++Histogram.FindOrAdd(ClassName);

            if (const auto* DynParam = Cast<UMaterialExpressionDynamicParameter>(Expression))
            {
                for (const auto& ParamName : DynParam->ParamNames)
                { DynamicParamNames.Add(ParamName); }
            }
        }
        Histogram.KeySort(TLess<FString>{});

        auto HistogramObj = MakeShared<FJsonObject>();
        for (const auto& Kvp : Histogram)
        { HistogramObj->SetNumberField(Kvp.Key, Kvp.Value); }
        Root->SetObjectField(TEXT("expressions"), HistogramObj);

        if (DynamicParamNames.Num() > 0)
        {
            auto DynArr = TArray<TSharedPtr<FJsonValue>>{};
            for (const auto& Name : DynamicParamNames)
            { DynArr.Add(MakeShared<FJsonValueString>(Name)); }
            Root->SetArrayField(TEXT("dynamicParameters"), DynArr);
        }
#endif
    }

    // ---- Effective parameter values (instance chain resolved) ----
    auto ScalarArr = TArray<TSharedPtr<FJsonValue>>{};
    {
        auto Infos = TArray<FMaterialParameterInfo>{};
        auto Ids = TArray<FGuid>{};
        InMaterial->GetAllScalarParameterInfo(Infos, Ids);
        for (const auto& Info : Infos)
        {
            auto Value = 0.0f;
            if (NOT InMaterial->GetScalarParameterValue(Info, Value))
            { continue; }
            auto Obj = MakeShared<FJsonObject>();
            Obj->SetStringField(TEXT("name"), Info.Name.ToString());
            Obj->SetNumberField(TEXT("value"), Value);
            ScalarArr.Add(MakeShared<FJsonValueObject>(Obj));
        }
    }
    Root->SetArrayField(TEXT("scalarParams"), ScalarArr);

    auto VectorArr = TArray<TSharedPtr<FJsonValue>>{};
    {
        auto Infos = TArray<FMaterialParameterInfo>{};
        auto Ids = TArray<FGuid>{};
        InMaterial->GetAllVectorParameterInfo(Infos, Ids);
        for (const auto& Info : Infos)
        {
            auto Value = FLinearColor{};
            if (NOT InMaterial->GetVectorParameterValue(Info, Value))
            { continue; }
            auto Obj = MakeShared<FJsonObject>();
            Obj->SetStringField(TEXT("name"), Info.Name.ToString());
            Obj->SetStringField(TEXT("value"), FString::Printf(TEXT("RGBA(%g, %g, %g, %g)"), Value.R, Value.G, Value.B, Value.A));
            VectorArr.Add(MakeShared<FJsonValueObject>(Obj));
        }
    }
    Root->SetArrayField(TEXT("vectorParams"), VectorArr);

    auto TextureArr = TArray<TSharedPtr<FJsonValue>>{};
    {
        auto Infos = TArray<FMaterialParameterInfo>{};
        auto Ids = TArray<FGuid>{};
        InMaterial->GetAllTextureParameterInfo(Infos, Ids);
        for (const auto& Info : Infos)
        {
            auto* Value = static_cast<UTexture*>(nullptr);
            if (NOT InMaterial->GetTextureParameterValue(Info, Value) || Value == nullptr)
            { continue; }
            auto Obj = MakeShared<FJsonObject>();
            Obj->SetStringField(TEXT("name"), Info.Name.ToString());
            Obj->SetStringField(TEXT("value"), Value->GetPathName());
            TextureArr.Add(MakeShared<FJsonValueObject>(Obj));
            OutTextures.Add(Value->GetPathName());
        }
    }
    Root->SetArrayField(TEXT("textureParams"), TextureArr);

    // ---- Every texture the material uses (parameters + hard-wired samples) ----
    // NOTE: the old 5-arg GetUsedTextures overload is a UE_DEPRECATED(5.7) empty-body no-op on this
    // engine — the TOptional overload with defaults (= all quality levels / platforms) is the live one.
    auto UsedTextures = TArray<UTexture*>{};
    InMaterial->GetUsedTextures(UsedTextures);

    auto UsedArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Texture : UsedTextures)
    {
        if (Texture == nullptr)
        { continue; }
        UsedArr.Add(MakeShared<FJsonValueString>(Texture->GetPathName()));
        OutTextures.Add(Texture->GetPathName());
    }
    Root->SetArrayField(TEXT("usedTextures"), UsedArr);

    return Root;
}
