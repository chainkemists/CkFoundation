#include "CkMaterialExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"
#include "CkAssetExporter/MaterialExporter/CkMaterialExporter_HeadlessTextures.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Engine/Texture.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "SceneTypes.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck_material_exporter_internal
{
    // Node-graph cosmetics and editor bookkeeping. Every one of these differs per node, so the CDO diff that
    // filters `props` would emit all of them, and none says anything about what the material computes.
    // Desc is not dropped, only relocated: it is the node's authored comment and rides as "desc".
    auto
        Get_IsSkippedExpressionProperty(
            const FName& InName)
        -> bool
    {
        static const TSet<FName> Skipped
        {
            FName{TEXT("MaterialExpressionEditorX")}, FName{TEXT("MaterialExpressionEditorY")},
            FName{TEXT("MaterialExpressionGuid")},    FName{TEXT("ExpressionGUID")},
            FName{TEXT("GraphNode")},                 FName{TEXT("SubgraphExpression")},
            FName{TEXT("Material")},                  FName{TEXT("Function")},
            FName{TEXT("MenuCategories")},            FName{TEXT("Desc")},
            FName{TEXT("bCollapsed")},                FName{TEXT("bRealtimePreview")},
            FName{TEXT("bNeedToUpdatePreview")},      FName{TEXT("bShowOutputNameOnPin")},
            FName{TEXT("bHidePreviewWindow")},        FName{TEXT("bShowMaskColorsOnPin")},
            FName{TEXT("bIsParameterExpression")},
        };
        return Skipped.Contains(InName);
    }

    // Structs exported as their canonical ExportText form. An ALLOW-list, not a deny-list, because the general
    // case is unbounded: FExpressionInput is a struct too, and walking one re-descends the entire graph through
    // its Expression pointer. Connectivity has its own representation below; this is for plain value structs.
    auto
        Get_IsExportableValueStruct(
            const UScriptStruct* InStruct)
        -> bool
    {
        if (InStruct == nullptr)
        { return false; }

        static const TSet<FName> Exportable
        {
            FName{TEXT("LinearColor")}, FName{TEXT("Color")},    FName{TEXT("Vector")},
            FName{TEXT("Vector2D")},    FName{TEXT("Vector4")},  FName{TEXT("Vector3f")},
            FName{TEXT("Vector2f")},    FName{TEXT("Vector4f")}, FName{TEXT("Rotator")},
            FName{TEXT("IntPoint")},    FName{TEXT("Guid")},
        };
        return Exportable.Contains(InStruct->GetFName());
    }

    // Channel string for a pin's component mask; empty when the pin passes its output unmasked.
    auto
        Get_MaskString(
            int32 InMask, int32 InR, int32 InG, int32 InB, int32 InA)
        -> FString
    {
        if (InMask == 0)
        { return FString{}; }

        auto Out = FString{};
        if (InR != 0) { Out += TEXT("r"); }
        if (InG != 0) { Out += TEXT("g"); }
        if (InB != 0) { Out += TEXT("b"); }
        if (InA != 0) { Out += TEXT("a"); }
        return Out;
    }

    // An INVALID return means "not a value this export models" and the caller drops the property entirely —
    // never a null, which would claim the property exists and is empty.
    auto
        Get_PropertyValueJson(
            const FProperty* InProperty,
            const void* InValuePtr,
            const TMap<const UMaterialExpression*, int32>& InNodeIds)
        -> TSharedPtr<FJsonValue>
    {
        if (const auto* BoolProperty = CastField<FBoolProperty>(InProperty))
        { return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(InValuePtr)); }

        if (const auto* EnumProperty = CastField<FEnumProperty>(InProperty))
        {
            const auto Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(InValuePtr);
            const auto* Enum = EnumProperty->GetEnum();
            if (Enum != nullptr)
            { return MakeShared<FJsonValueString>(Enum->GetNameStringByValue(Value)); }
            return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
        }

        if (const auto* ByteProperty = CastField<FByteProperty>(InProperty))
        {
            const auto Value = static_cast<int64>(ByteProperty->GetPropertyValue(InValuePtr));
            if (ByteProperty->Enum != nullptr)
            { return MakeShared<FJsonValueString>(ByteProperty->Enum->GetNameStringByValue(Value)); }
            return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
        }

        if (const auto* NumericProperty = CastField<FNumericProperty>(InProperty))
        {
            if (NumericProperty->IsFloatingPoint())
            { return MakeShared<FJsonValueNumber>(NumericProperty->GetFloatingPointPropertyValue(InValuePtr)); }
            return MakeShared<FJsonValueNumber>(static_cast<double>(NumericProperty->GetSignedIntPropertyValue(InValuePtr)));
        }

        if (const auto* StringProperty = CastField<FStrProperty>(InProperty))
        { return MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(InValuePtr)); }

        if (const auto* NameProperty = CastField<FNameProperty>(InProperty))
        { return MakeShared<FJsonValueString>(NameProperty->GetPropertyValue(InValuePtr).ToString()); }

        if (const auto* TextProperty = CastField<FTextProperty>(InProperty))
        { return MakeShared<FJsonValueString>(TextProperty->GetPropertyValue(InValuePtr).ToString()); }

        // Soft refs first: FSoftObjectProperty derives from FObjectPropertyBase and would otherwise be
        // resolved (and loaded) by the hard-ref branch below.
        if (const auto* SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty))
        { return MakeShared<FJsonValueString>(SoftObjectProperty->GetPropertyValue(InValuePtr).ToString()); }

        if (const auto* ObjectProperty = CastField<FObjectPropertyBase>(InProperty))
        {
            const auto* Value = ObjectProperty->GetObjectPropertyValue(InValuePtr);
            if (Value == nullptr)
            { return MakeShared<FJsonValueString>(FString{}); }

            // A property pointing AT another expression is a graph edge that never appears in the input
            // iterator — NamedRerouteUsage->Declaration is the one that matters — so it resolves to that
            // node's id rather than to an object path nothing else in this file is keyed by.
            if (const auto* AsExpression = Cast<UMaterialExpression>(Value))
            {
                if (const auto* NodeId = InNodeIds.Find(AsExpression))
                {
                    auto Ref = MakeShared<FJsonObject>();
                    Ref->SetNumberField(TEXT("node"), *NodeId);
                    return MakeShared<FJsonValueObject>(Ref);
                }
            }
            return MakeShared<FJsonValueString>(Value->GetPathName());
        }

        if (const auto* StructProperty = CastField<FStructProperty>(InProperty))
        {
            if (NOT Get_IsExportableValueStruct(StructProperty->Struct))
            { return nullptr; }

            auto Exported = FString{};
            StructProperty->ExportTextItem_Direct(Exported, InValuePtr, nullptr, nullptr, PPF_None);
            return MakeShared<FJsonValueString>(Exported);
        }

        if (const auto* ArrayProperty = CastField<FArrayProperty>(InProperty))
        {
            auto Helper = FScriptArrayHelper{ArrayProperty, InValuePtr};
            auto Elements = TArray<TSharedPtr<FJsonValue>>{};
            for (auto Index = 0; Index < Helper.Num(); ++Index)
            {
                const auto Element = Get_PropertyValueJson(ArrayProperty->Inner, Helper.GetRawPtr(Index), InNodeIds);
                // One unmodellable element makes the whole array a lie about its own length; drop it instead.
                if (NOT Element.IsValid())
                { return nullptr; }
                Elements.Add(Element);
            }
            return MakeShared<FJsonValueArray>(Elements);
        }

        return nullptr;
    }

    // Non-default, non-transient properties of one expression: the constants the graph's arithmetic actually
    // uses (Constant.R, ScalarParameter.DefaultValue, Custom.Code, ComponentMask.R/G/B/A, ...). Diffing against
    // the CDO is what keeps this to the handful of fields the author touched.
    auto
        Build_ExpressionProps(
            const UMaterialExpression* InExpression,
            const TMap<const UMaterialExpression*, int32>& InNodeIds)
        -> TSharedPtr<FJsonObject>
    {
        const auto* Defaults = InExpression->GetClass()->GetDefaultObject();
        if (Defaults == nullptr)
        { return nullptr; }

        auto Props = MakeShared<FJsonObject>();
        for (TFieldIterator<FProperty> It(InExpression->GetClass()); It; ++It)
        {
            const auto* Property = *It;

            if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated))
            { continue; }

            if (Get_IsSkippedExpressionProperty(Property->GetFName()))
            { continue; }

            if (Property->Identical_InContainer(InExpression, Defaults))
            { continue; }

            const auto Value = Get_PropertyValueJson(
                Property, Property->ContainerPtrToValuePtr<void>(InExpression), InNodeIds);
            if (NOT Value.IsValid())
            { continue; }

            Props->SetField(Property->GetName(), Value);
        }

        if (Props->Values.IsEmpty())
        { return nullptr; }

        return Props;
    }

    // Class histogram of an expression list — the pre-v3 summary, kept because it answers "how big / what
    // shape" without walking nodes. Shared by the material and material-function paths.
    auto
        Build_Histogram(
            TConstArrayView<TObjectPtr<UMaterialExpression>> InExpressions)
        -> TSharedPtr<FJsonObject>
    {
        auto Histogram = TMap<FString, int32>{};
        for (const auto& Expression : InExpressions)
        {
            if (Expression == nullptr)
            { continue; }
            auto ClassName = Expression->GetClass()->GetName();
            ClassName.RemoveFromStart(TEXT("MaterialExpression"));
            ++Histogram.FindOrAdd(ClassName);
        }
        Histogram.KeySort(TLess<FString>{});

        auto Obj = MakeShared<FJsonObject>();
        for (const auto& Kvp : Histogram)
        { Obj->SetNumberField(Kvp.Key, Kvp.Value); }
        return Obj;
    }

    // { nodes: [...], outputs: [...] } — the whole authored graph. Invalid when there are no expressions
    // (a pure parameter-driven instance parent, or an asset whose editor data was stripped).
    //
    // InOutputsOwner supplies the "outputs" array (which node feeds MP_EmissiveColor and friends) and is null
    // for a material FUNCTION, whose outputs are FunctionOutput NODES in the graph rather than fixed pins.
    auto
        Build_Graph(
            TConstArrayView<TObjectPtr<UMaterialExpression>> InExpressions,
            UMaterial* InOutputsOwner)
        -> TSharedPtr<FJsonObject>
    {
        const auto Expressions = InExpressions;
        if (Expressions.IsEmpty())
        { return nullptr; }

        // Ids are indices into the package's own expression array, so two exports of one .uasset agree — the
        // byte-identical requirement the _meta contract makes of every exporter.
        auto NodeIds = TMap<const UMaterialExpression*, int32>{};
        for (auto Index = 0; Index < Expressions.Num(); ++Index)
        {
            if (Expressions[Index] != nullptr)
            { NodeIds.Add(Expressions[Index], Index); }
        }

        auto Nodes = TArray<TSharedPtr<FJsonValue>>{};
        for (auto Index = 0; Index < Expressions.Num(); ++Index)
        {
            // Spelled out, NOT `auto*`: the view yields TObjectPtr, and auto* deduction does not apply a
            // user-defined conversion — it deduces nothing, and every use of the variable below fails instead.
            UMaterialExpression* Expression = Expressions[Index];
            if (Expression == nullptr)
            { continue; }

            auto Node = MakeShared<FJsonObject>();
            Node->SetNumberField(TEXT("id"), Index);

            auto ClassName = Expression->GetClass()->GetName();
            ClassName.RemoveFromStart(TEXT("MaterialExpression"));
            Node->SetStringField(TEXT("class"), ClassName);
            Node->SetStringField(TEXT("name"), Expression->GetName());

            if (NOT Expression->Desc.IsEmpty())
            { Node->SetStringField(TEXT("desc"), Expression->Desc); }

            // Only CONNECTED inputs are emitted: an absent input is an unconnected pin, whose value comes from
            // the node's own constant (Multiply's ConstB and friends), which "props" already carries.
            auto Inputs = TArray<TSharedPtr<FJsonValue>>{};
            for (FExpressionInputIterator It{Expression}; It; ++It)
            {
                if (It->Expression == nullptr)
                { continue; }

                const auto* SourceId = NodeIds.Find(It->Expression);
                if (SourceId == nullptr)
                { continue; }

                auto InputObj = MakeShared<FJsonObject>();
                InputObj->SetNumberField(TEXT("index"), It.Index);

                const auto InputName = Expression->GetInputName(It.Index);
                if (NOT InputName.IsNone())
                { InputObj->SetStringField(TEXT("name"), InputName.ToString()); }

                InputObj->SetNumberField(TEXT("node"), *SourceId);
                InputObj->SetNumberField(TEXT("output"), It->OutputIndex);

                const auto Mask = Get_MaskString(It->Mask, It->MaskR, It->MaskG, It->MaskB, It->MaskA);
                if (NOT Mask.IsEmpty())
                { InputObj->SetStringField(TEXT("mask"), Mask); }

                Inputs.Add(MakeShared<FJsonValueObject>(InputObj));
            }
            if (NOT Inputs.IsEmpty())
            { Node->SetArrayField(TEXT("inputs"), Inputs); }

            // Output NAMES, so the "output" index on an input above is readable rather than ordinal-only —
            // it is the difference between reading ParticleColor's RGB pin and its A pin.
            const auto& Outputs = Expression->GetOutputs();
            if (Outputs.Num() > 1)
            {
                auto OutputNames = TArray<TSharedPtr<FJsonValue>>{};
                for (const auto& Output : Outputs)
                { OutputNames.Add(MakeShared<FJsonValueString>(Output.OutputName.ToString())); }
                Node->SetArrayField(TEXT("outputs"), OutputNames);
            }

            const auto Props = Build_ExpressionProps(Expression, NodeIds);
            if (Props.IsValid())
            { Node->SetObjectField(TEXT("props"), Props); }

            Nodes.Add(MakeShared<FJsonValueObject>(Node));
        }

        auto Graph = MakeShared<FJsonObject>();
        Graph->SetArrayField(TEXT("nodes"), Nodes);

        // Which node feeds each material output pin. connectedOutputs (kept above for readers that only ask
        // "is this additive?") says THAT Emissive is wired; this says what it is wired to. A material function
        // has no such pins — its outputs are FunctionOutput nodes already in "nodes" — so this is skipped.
        if (InOutputsOwner == nullptr)
        { return Graph; }

        auto Outputs = TArray<TSharedPtr<FJsonValue>>{};
        const auto* PropertyEnum = StaticEnum<EMaterialProperty>();
        for (auto Index = 0; Index < static_cast<int32>(MP_MAX); ++Index)
        {
            const auto* Input = InOutputsOwner->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Index));
            if (Input == nullptr || Input->Expression == nullptr)
            { continue; }

            const auto* SourceId = NodeIds.Find(Input->Expression);
            if (SourceId == nullptr)
            { continue; }

            auto OutputObj = MakeShared<FJsonObject>();
            OutputObj->SetStringField(TEXT("property"),
                PropertyEnum != nullptr ? PropertyEnum->GetNameStringByValue(Index) : FString::FromInt(Index));
            OutputObj->SetNumberField(TEXT("node"), *SourceId);
            OutputObj->SetNumberField(TEXT("output"), Input->OutputIndex);

            const auto Mask = Get_MaskString(Input->Mask, Input->MaskR, Input->MaskG, Input->MaskB, Input->MaskA);
            if (NOT Mask.IsEmpty())
            { OutputObj->SetStringField(TEXT("mask"), Mask); }

            Outputs.Add(MakeShared<FJsonValueObject>(OutputObj));
        }
        Graph->SetArrayField(TEXT("outputs"), Outputs);

        return Graph;
    }
}
#endif

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

    // GetUsedTextures walks compiled FMaterialResources, which never exist in a render-incapable
    // process (commandlet/nullrhi) — the export would silently write "usedTextures": [] while the
    // MD5 freshness oracle keeps calling the sidecar fresh. There, the editor-data walk emulates
    // the translator's traversal instead (verified byte-identical against a live-editor corpus);
    // materials it cannot model keep the old refusal so a wrong texture list never ships.
    auto UsedTextures = TArray<UTexture*>{};
    if (FApp::CanEverRender())
    {
        InMaterial->GetUsedTextures(UsedTextures);
    }
    else
    {
        const auto Walk = FCk_MaterialExporter_HeadlessTextures::EnumerateUsedTextures(InMaterial);
        const auto WalkServesThisMaterial = Walk.Supported;
        CK_ENSURE_IF_NOT(WalkServesThisMaterial,
            TEXT("Refusing to export Material [{}] from a render-incapable process — the headless texture walk "
                 "cannot model it ({}), so the texture list could silently export wrong. Export it through an "
                 "open editor."),
            InMaterial->GetName(), Walk.UnsupportedReason)
        {
            Result.ErrorMessage = ck::Format_UE(
                TEXT("Material export requires a render-capable context for [{}] ({}) — it was NOT exported; "
                     "use an open editor (bridge) instead"), InMaterial->GetName(), Walk.UnsupportedReason);
            return Result;
        }
        UsedTextures = Walk.Textures;
    }

    auto Textures = TSet<FString>{};
    const auto JsonObject = DoSerializeToJson(InMaterial, UsedTextures, Textures);
    Result.ReferencedTextures = Textures.Array();

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Corpus mode writes into InOutputDir; empty means sibling-json mode (dispatch / right-click / tab).
    auto JsonPath = FString{};
    if (NOT InOutputDir.IsEmpty())
    {
        constexpr auto CreateTree = true;
        IFileManager::Get().MakeDirectory(*InOutputDir, CreateTree);
        JsonPath = FPaths::Combine(InOutputDir, InMaterial->GetName() + TEXT(".json"));
    }
    else if (NOT FPackageName::TryConvertLongPackageNameToFilename(
        InMaterial->GetOutermost()->GetName(), JsonPath, ck::asset_exporter::extension::Sidecar))
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to resolve sibling json path for [{}]"), InMaterial->GetName());
        return Result;
    }

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
    ExportMaterialFunction(
        UMaterialFunctionInterface* InFunction,
        const FString& InOutputDir)
    -> FCk_MaterialExportResult
{
    auto Result = FCk_MaterialExportResult{};
    if (ck::Is_NOT_Valid(InFunction))
    {
        Result.ErrorMessage = TEXT("Invalid MaterialFunction asset");
        return Result;
    }

    Result.AssetName = InFunction->GetName();

    // No GetUsedTextures equivalent and no render-capability gate: a function has no compiled
    // FMaterialResource to walk, so the headless refusal that guards ExportMaterial does not apply here.
    // Textures a function samples surface as object paths in its nodes' props instead.
    const auto JsonObject = DoSerializeFunctionToJson(InFunction);

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    auto JsonPath = FString{};
    if (NOT InOutputDir.IsEmpty())
    {
        constexpr auto CreateTree = true;
        IFileManager::Get().MakeDirectory(*InOutputDir, CreateTree);
        JsonPath = FPaths::Combine(InOutputDir, InFunction->GetName() + TEXT(".json"));
    }
    else if (NOT FPackageName::TryConvertLongPackageNameToFilename(
        InFunction->GetOutermost()->GetName(), JsonPath, ck::asset_exporter::extension::Sidecar))
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to resolve sibling json path for [{}]"), InFunction->GetName());
        return Result;
    }

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
    ExportMaterialFunctions(
        const TArray<UMaterialFunctionInterface*>& InFunctions)
    -> TArray<FCk_MaterialExportResult>
{
    auto Results = TArray<FCk_MaterialExportResult>{};
    Results.Reserve(InFunctions.Num());
    for (auto* Function : InFunctions)
    { Results.Add(ExportMaterialFunction(Function)); }
    return Results;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MaterialExporter::
    DoSerializeFunctionToJson(
        UMaterialFunctionInterface* InFunction)
    -> TSharedPtr<FJsonObject>
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("materialFunction"), InFunction->GetName());
    Root->SetStringField(TEXT("packagePath"), InFunction->GetOutermost()->GetName());
    Root->SetObjectField(TEXT("_meta"),
        FCk_AssetExportMeta::MakeMetaObject(InFunction, ck::asset_exporter::version::MaterialFunction));

#if WITH_EDITOR
    // GetDescription is WITH_EDITOR; GetExpressions is WITH_EDITORONLY_DATA, which WITH_EDITOR implies.
    if (NOT InFunction->GetDescription().IsEmpty())
    { Root->SetStringField(TEXT("description"), InFunction->GetDescription()); }

    const auto Expressions = InFunction->GetExpressions();
    Root->SetObjectField(TEXT("expressions"), ck_material_exporter_internal::Build_Histogram(Expressions));

    // Null owner: a function's outputs are FunctionOutput nodes inside the graph, not material pins.
    if (const auto Graph = ck_material_exporter_internal::Build_Graph(Expressions, nullptr))
    { Root->SetObjectField(TEXT("graph"), Graph); }
#endif

    return Root;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MaterialExporter::
    ExportMaterials(
        const TArray<UMaterialInterface*>& InMaterials)
    -> TArray<FCk_MaterialExportResult>
{
    auto Results = TArray<FCk_MaterialExportResult>{};
    Results.Reserve(InMaterials.Num());

    for (auto* Material : InMaterials)
    { Results.Add(ExportMaterial(Material)); }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MaterialExporter::
    DoSerializeToJson(
        UMaterialInterface* InMaterial,
        const TArray<UTexture*>& InUsedTextures,
        TSet<FString>& OutTextures)
    -> TSharedPtr<FJsonObject>
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("material"), InMaterial->GetName());
    Root->SetStringField(TEXT("packagePath"), InMaterial->GetOutermost()->GetName());
    Root->SetObjectField(TEXT("_meta"), FCk_AssetExportMeta::MakeMetaObject(InMaterial, ck::asset_exporter::version::Material));

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
            if (EditorOnly->EmissiveColor.IsConnected())
            { Connected.Add(MakeShared<FJsonValueString>(TEXT("EmissiveColor"))); }
            if (EditorOnly->BaseColor.IsConnected())
            { Connected.Add(MakeShared<FJsonValueString>(TEXT("BaseColor"))); }
            if (EditorOnly->Opacity.IsConnected())
            { Connected.Add(MakeShared<FJsonValueString>(TEXT("Opacity"))); }
            if (EditorOnly->OpacityMask.IsConnected())
            { Connected.Add(MakeShared<FJsonValueString>(TEXT("OpacityMask"))); }
            if (EditorOnly->Normal.IsConnected())
            { Connected.Add(MakeShared<FJsonValueString>(TEXT("Normal"))); }
            Root->SetArrayField(TEXT("connectedOutputs"), Connected);
        }

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

#if WITH_EDITOR
        // The graph itself (exporter v3). GetExpressionInputForProperty and the input iterator are WITH_EDITOR,
        // not merely WITH_EDITORONLY_DATA, so this is a second gate inside the one above.
        if (auto* GraphOwner = InMaterial->GetMaterial())
        {
            if (const auto Graph = ck_material_exporter_internal::Build_Graph(GraphOwner->GetExpressions(), GraphOwner))
            { Root->SetObjectField(TEXT("graph"), Graph); }
        }
#endif
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
    auto UsedArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Texture : InUsedTextures)
    {
        if (Texture == nullptr)
        { continue; }
        UsedArr.Add(MakeShared<FJsonValueString>(Texture->GetPathName()));
        OutTextures.Add(Texture->GetPathName());
    }
    Root->SetArrayField(TEXT("usedTextures"), UsedArr);

    return Root;
}
