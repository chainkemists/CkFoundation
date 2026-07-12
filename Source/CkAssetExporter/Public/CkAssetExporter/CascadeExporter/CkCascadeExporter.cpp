#include "CkCascadeExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/TypeData/ParticleModuleTypeDataBase.h"
#include "Particles/TypeData/ParticleModuleTypeDataMesh.h"

#include "Distributions/DistributionFloat.h"
#include "Distributions/DistributionFloatConstant.h"
#include "Distributions/DistributionFloatConstantCurve.h"
#include "Distributions/DistributionFloatUniform.h"
#include "Distributions/DistributionFloatUniformCurve.h"
#include "Distributions/DistributionFloatParameterBase.h"
#include "Distributions/DistributionVector.h"
#include "Distributions/DistributionVectorConstant.h"
#include "Distributions/DistributionVectorConstantCurve.h"
#include "Distributions/DistributionVectorUniform.h"
#include "Distributions/DistributionVectorUniformCurve.h"
#include "Distributions/DistributionVectorParameterBase.h"

#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::cascade
{
    static auto InterpModeToLetter(EInterpCurveMode InMode) -> FString
    {
        switch (InMode)
        {
            case CIM_Linear:   return TEXT("L");
            case CIM_Constant: return TEXT("S");
            default:           return TEXT("C");
        }
    }

    // Properties whose owner class contributes nothing to the recipe (module plumbing, editor cosmetics).
    static auto ShouldSkipProperty(const FProperty* InProperty) -> bool
    {
        if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated))
        { return true; }

        const auto* OwnerClass = InProperty->GetOwnerClass();
        if (OwnerClass == UObject::StaticClass() || OwnerClass == UParticleModule::StaticClass())
        { return true; }

        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Public API

auto
    FCk_CascadeExporter::
    ExportParticleSystem(
        UParticleSystem* InSystem,
        const FString& InOutputDir)
    -> FCk_CascadeExportResult
{
    auto Result = FCk_CascadeExportResult{};
    if (ck::Is_NOT_Valid(InSystem))
    {
        Result.ErrorMessage = TEXT("Invalid Particle System asset");
        return Result;
    }

    Result.AssetName = InSystem->GetName();

    auto Materials = TSet<FString>{};
    const auto JsonObject = DoSerializeToJson(InSystem, Materials);
    Result.ReferencedMaterials = Materials.Array();

    auto JsonString = FString{};
    if (JsonObject.IsValid())
    {
        const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
    }

    const auto TextString = DoSerializeToText(JsonObject);

    const auto JsonPath = DoResolveOutputPath(InSystem, TEXT(".json"), InOutputDir);
    const auto TextPath = DoResolveOutputPath(InSystem, TEXT(".txt"), InOutputDir);
    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    if (NOT InOutputDir.IsEmpty())
    {
        constexpr auto CreateTree = true;
        IFileManager::Get().MakeDirectory(*InOutputDir, CreateTree);
    }

    const auto JsonWritten = FFileHelper::SaveStringToFile(JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = FFileHelper::SaveStringToFile(TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    if (NOT JsonWritten || NOT TextWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, Text: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"), TextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_CascadeExporter::
    ExportParticleSystems(
        const TArray<UParticleSystem*>& InSystems)
    -> TArray<FCk_CascadeExportResult>
{
    auto Results = TArray<FCk_CascadeExportResult>{};
    Results.Reserve(InSystems.Num());
    ck::algo::ForEach(InSystems, [&](UParticleSystem* Sys){ Results.Add(ExportParticleSystem(Sys)); });
    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// Distributions — where Cascade keeps the authored numbers.

auto
    FCk_CascadeExporter::
    DoFormatDistributionFloat(
        const UDistributionFloat* InDistribution)
    -> FString
{
    if (InDistribution == nullptr)
    { return TEXT("<none>"); }

    if (const auto* Constant = Cast<UDistributionFloatConstant>(InDistribution))
    { return FString::Printf(TEXT("const %g"), Constant->Constant); }

    if (const auto* Uniform = Cast<UDistributionFloatUniform>(InDistribution))
    { return FString::Printf(TEXT("uniform[%g, %g]"), Uniform->Min, Uniform->Max); }

    if (const auto* Curve = Cast<UDistributionFloatConstantCurve>(InDistribution))
    {
        auto Keys = TArray<FString>{};
        for (const auto& Point : Curve->ConstantCurve.Points)
        {
            Keys.Add(FString::Printf(TEXT("(%g, %g)%s"), Point.InVal, Point.OutVal,
                *ck::asset_exporter::cascade::InterpModeToLetter(Point.InterpMode)));
        }
        return ck::Format_UE(TEXT("curve[{}]"), FString::Join(Keys, TEXT(" ")));
    }

    if (const auto* UniformCurve = Cast<UDistributionFloatUniformCurve>(InDistribution))
    {
        auto Keys = TArray<FString>{};
        for (const auto& Point : UniformCurve->ConstantCurve.Points)
        {
            Keys.Add(FString::Printf(TEXT("(%g: %g..%g)%s"), Point.InVal, Point.OutVal.X, Point.OutVal.Y,
                *ck::asset_exporter::cascade::InterpModeToLetter(Point.InterpMode)));
        }
        return ck::Format_UE(TEXT("uniformCurve[{}]"), FString::Join(Keys, TEXT(" ")));
    }

    if (const auto* Parameter = Cast<UDistributionFloatParameterBase>(InDistribution))
    { return ck::Format_UE(TEXT("param:{}"), Parameter->ParameterName.ToString()); }

    return InDistribution->GetClass()->GetName();
}

auto
    FCk_CascadeExporter::
    DoFormatDistributionVector(
        const UDistributionVector* InDistribution)
    -> FString
{
    if (InDistribution == nullptr)
    { return TEXT("<none>"); }

    if (const auto* Constant = Cast<UDistributionVectorConstant>(InDistribution))
    { return FString::Printf(TEXT("const (%g, %g, %g)"), Constant->Constant.X, Constant->Constant.Y, Constant->Constant.Z); }

    if (const auto* Uniform = Cast<UDistributionVectorUniform>(InDistribution))
    {
        return FString::Printf(TEXT("uniform[(%g, %g, %g), (%g, %g, %g)]"),
            Uniform->Min.X, Uniform->Min.Y, Uniform->Min.Z, Uniform->Max.X, Uniform->Max.Y, Uniform->Max.Z);
    }

    if (const auto* Curve = Cast<UDistributionVectorConstantCurve>(InDistribution))
    {
        auto Keys = TArray<FString>{};
        for (const auto& Point : Curve->ConstantCurve.Points)
        {
            Keys.Add(FString::Printf(TEXT("(%g: (%g, %g, %g))%s"), Point.InVal, Point.OutVal.X, Point.OutVal.Y, Point.OutVal.Z,
                *ck::asset_exporter::cascade::InterpModeToLetter(Point.InterpMode)));
        }
        return ck::Format_UE(TEXT("curve[{}]"), FString::Join(Keys, TEXT(" ")));
    }

    if (const auto* UniformCurve = Cast<UDistributionVectorUniformCurve>(InDistribution))
    {
        auto Keys = TArray<FString>{};
        for (const auto& Point : UniformCurve->ConstantCurve.Points)
        {
            Keys.Add(FString::Printf(TEXT("(%g: (%g, %g, %g)..(%g, %g, %g))%s"), Point.InVal,
                Point.OutVal.v1.X, Point.OutVal.v1.Y, Point.OutVal.v1.Z,
                Point.OutVal.v2.X, Point.OutVal.v2.Y, Point.OutVal.v2.Z,
                *ck::asset_exporter::cascade::InterpModeToLetter(Point.InterpMode)));
        }
        return ck::Format_UE(TEXT("uniformCurve[{}]"), FString::Join(Keys, TEXT(" ")));
    }

    if (const auto* Parameter = Cast<UDistributionVectorParameterBase>(InDistribution))
    { return ck::Format_UE(TEXT("param:{}"), Parameter->ParameterName.ToString()); }

    return InDistribution->GetClass()->GetName();
}

// --------------------------------------------------------------------------------------------------------------------
// Module serialization — generic reflection dump; distributions rasterized, object refs recorded as paths.

auto
    FCk_CascadeExporter::
    DoSerializeModule_Json(
        UParticleModule* InModule,
        TSet<FString>& OutMaterials)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    if (InModule == nullptr)
    { return Obj; }

    auto ClassName = InModule->GetClass()->GetName();
    ClassName.RemoveFromStart(TEXT("ParticleModule"));
    Obj->SetStringField(TEXT("class"), ClassName);
    if (NOT InModule->bEnabled)
    { Obj->SetBoolField(TEXT("enabled"), false); }

    auto PropsArr = TArray<TSharedPtr<FJsonValue>>{};
    for (TFieldIterator<FProperty> It{InModule->GetClass()}; It; ++It)
    {
        const auto* Prop = *It;
        if (ck::asset_exporter::cascade::ShouldSkipProperty(Prop))
        { continue; }

        auto Value = FString{};

        if (const auto* StructProp = CastField<FStructProperty>(Prop))
        {
            if (StructProp->Struct->GetFName() == TEXT("RawDistributionFloat"))
            {
                const auto* Raw = StructProp->ContainerPtrToValuePtr<FRawDistributionFloat>(InModule);
                Value = DoFormatDistributionFloat(Raw->Distribution);
            }
            else if (StructProp->Struct->GetFName() == TEXT("RawDistributionVector"))
            {
                const auto* Raw = StructProp->ContainerPtrToValuePtr<FRawDistributionVector>(InModule);
                Value = DoFormatDistributionVector(Raw->Distribution);
            }
        }
        else if (const auto* ObjProp = CastField<FObjectPropertyBase>(Prop))
        {
            const auto* Referenced = ObjProp->GetObjectPropertyValue_InContainer(InModule);
            if (Referenced == nullptr)
            { continue; }
            Value = Referenced->GetPathName();

            if (const auto* Material = Cast<UMaterialInterface>(Referenced))
            { OutMaterials.Add(Material->GetPathName()); }
            else if (const auto* Mesh = Cast<UStaticMesh>(Referenced))
            {
                for (const auto& StaticMaterial : Mesh->GetStaticMaterials())
                {
                    if (StaticMaterial.MaterialInterface != nullptr)
                    { OutMaterials.Add(StaticMaterial.MaterialInterface->GetPathName()); }
                }
            }
        }

        if (Value.IsEmpty())
        {
            Prop->ExportText_InContainer(0, Value, InModule, InModule, nullptr, PPF_None);
            if (Value.IsEmpty())
            { continue; }
        }

        auto PropObj = MakeShared<FJsonObject>();
        PropObj->SetStringField(TEXT("name"), Prop->GetName());
        PropObj->SetStringField(TEXT("value"), Value);
        PropsArr.Add(MakeShared<FJsonValueObject>(PropObj));
    }
    Obj->SetArrayField(TEXT("props"), PropsArr);
    return Obj;
}

auto
    FCk_CascadeExporter::
    DoSerializeEmitter_Json(
        const UParticleEmitter* InEmitter,
        TSet<FString>& OutMaterials)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), InEmitter->EmitterName.ToString());

    if (InEmitter->LODLevels.Num() == 0 || InEmitter->LODLevels[0] == nullptr)
    {
        Obj->SetStringField(TEXT("error"), TEXT("no LOD 0"));
        return Obj;
    }

    const auto* Lod = InEmitter->LODLevels[0].Get();
    Obj->SetBoolField(TEXT("enabled"), Lod->bEnabled != 0);
    Obj->SetObjectField(TEXT("required"), DoSerializeModule_Json(Lod->RequiredModule, OutMaterials));
    Obj->SetObjectField(TEXT("spawn"), DoSerializeModule_Json(Lod->SpawnModule, OutMaterials));
    if (Lod->TypeDataModule != nullptr)
    { Obj->SetObjectField(TEXT("typeData"), DoSerializeModule_Json(Lod->TypeDataModule, OutMaterials)); }

    auto ModulesArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Module : Lod->Modules)
    {
        if (Module == nullptr)
        { continue; }
        ModulesArr.Add(MakeShared<FJsonValueObject>(DoSerializeModule_Json(Module, OutMaterials)));
    }
    Obj->SetArrayField(TEXT("modules"), ModulesArr);
    return Obj;
}

auto
    FCk_CascadeExporter::
    DoSerializeToJson(
        UParticleSystem* InSystem,
        TSet<FString>& OutMaterials)
    -> TSharedPtr<FJsonObject>
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("system"), InSystem->GetName());
    Root->SetStringField(TEXT("packagePath"), InSystem->GetOutermost()->GetName());
    Root->SetStringField(TEXT("type"), TEXT("cascade"));

    auto EmittersArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Emitter : InSystem->Emitters)
    {
        if (Emitter == nullptr)
        { continue; }
        EmittersArr.Add(MakeShared<FJsonValueObject>(DoSerializeEmitter_Json(Emitter, OutMaterials)));
    }
    Root->SetArrayField(TEXT("emitters"), EmittersArr);
    return Root;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain text — rendered from the JSON so the two never diverge.

namespace ck::asset_exporter::cascade
{
    static auto AppendModule_Text(FString& OutText, const TSharedPtr<FJsonObject>& InModule, const FString& InLabel, const FString& InIndent) -> void
    {
        if (NOT InModule.IsValid() || NOT InModule->HasTypedField<EJson::String>(TEXT("class")))
        { return; }

        const auto Disabled = InModule->HasField(TEXT("enabled")) && NOT InModule->GetBoolField(TEXT("enabled"));
        OutText += ck::Format_UE(TEXT("{}{}{}{}\n"), InIndent, InLabel,
            InModule->GetStringField(TEXT("class")), Disabled ? TEXT("  (DISABLED)") : TEXT(""));

        const auto* Props = static_cast<const TArray<TSharedPtr<FJsonValue>>*>(nullptr);
        if (NOT InModule->TryGetArrayField(TEXT("props"), Props))
        { return; }

        for (const auto& PropValue : *Props)
        {
            const auto& Prop = PropValue->AsObject();
            OutText += ck::Format_UE(TEXT("{}   {} = {}\n"), InIndent,
                Prop->GetStringField(TEXT("name")), Prop->GetStringField(TEXT("value")));
        }
    }
}

auto
    FCk_CascadeExporter::
    DoSerializeToText(
        const TSharedPtr<FJsonObject>& InJson)
    -> FString
{
    auto Out = FString{};
    if (NOT InJson.IsValid())
    { return Out; }

    Out += ck::Format_UE(TEXT("CASCADE SYSTEM: {}\n"), InJson->GetStringField(TEXT("system")));
    Out += TEXT("====================================================================\n");

    const auto* Emitters = static_cast<const TArray<TSharedPtr<FJsonValue>>*>(nullptr);
    if (NOT InJson->TryGetArrayField(TEXT("emitters"), Emitters))
    { return Out; }

    for (const auto& EmitterValue : *Emitters)
    {
        const auto& Emitter = EmitterValue->AsObject();
        const auto Disabled = Emitter->HasField(TEXT("enabled")) && NOT Emitter->GetBoolField(TEXT("enabled"));
        Out += ck::Format_UE(TEXT("\n[EMITTER] {}{}\n"),
            Emitter->GetStringField(TEXT("name")), Disabled ? TEXT("  (DISABLED)") : TEXT(""));

        if (Emitter->HasTypedField<EJson::String>(TEXT("error")))
        { Out += ck::Format_UE(TEXT("  <{}>\n"), Emitter->GetStringField(TEXT("error"))); }
        if (Emitter->HasTypedField<EJson::Object>(TEXT("required")))
        { ck::asset_exporter::cascade::AppendModule_Text(Out, Emitter->GetObjectField(TEXT("required")), TEXT("Required: "), TEXT("  ")); }
        if (Emitter->HasTypedField<EJson::Object>(TEXT("spawn")))
        { ck::asset_exporter::cascade::AppendModule_Text(Out, Emitter->GetObjectField(TEXT("spawn")), TEXT("Spawn: "), TEXT("  ")); }
        if (Emitter->HasTypedField<EJson::Object>(TEXT("typeData")))
        { ck::asset_exporter::cascade::AppendModule_Text(Out, Emitter->GetObjectField(TEXT("typeData")), TEXT("TypeData: "), TEXT("  ")); }

        const auto* Modules = static_cast<const TArray<TSharedPtr<FJsonValue>>*>(nullptr);
        if (NOT Emitter->TryGetArrayField(TEXT("modules"), Modules))
        { continue; }

        Out += ck::Format_UE(TEXT("  Modules ({}):\n"), FString::FromInt(Modules->Num()));
        auto Index = int32{0};
        for (const auto& ModuleValue : *Modules)
        {
            ck::asset_exporter::cascade::AppendModule_Text(Out, ModuleValue->AsObject(),
                ck::Format_UE(TEXT("{}. "), FString::FromInt(++Index)), TEXT("    "));
        }
    }
    return Out;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_CascadeExporter::
    DoResolveOutputPath(
        const UParticleSystem* InSystem,
        const FString& InExtension,
        const FString& InOutputDir)
    -> FString
{
    if (NOT InOutputDir.IsEmpty())
    { return FPaths::Combine(InOutputDir, InSystem->GetName() + InExtension); }

    const auto PackageName = InSystem->GetOutermost()->GetName();
    auto FilePath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, FilePath, InExtension))
    { return FString{}; }
    return FilePath;
}
