#include "CkBehaviorTreeExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <BehaviorTree/BehaviorTree.h>
#include <BehaviorTree/BTCompositeNode.h>
#include <BehaviorTree/BTTaskNode.h>
#include <BehaviorTree/BTDecorator.h>
#include <BehaviorTree/BTService.h>
#include <BehaviorTree/BlackboardData.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>

#include <UObject/UnrealType.h>
#include <UObject/FieldIterator.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BehaviorTreeExporter::
    ExportBehaviorTree(
        UBehaviorTree* InBehaviorTree,
        ECk_AssetExporter_SidecarFormats InFormats)
    -> FCk_BehaviorTreeExportResult
{
    auto Result = FCk_BehaviorTreeExportResult{};

    if (!IsValid(InBehaviorTree))
    {
        Result.ErrorMessage = TEXT("Invalid BehaviorTree asset");
        return Result;
    }

    Result.AssetName = InBehaviorTree->GetName();

    const auto JsonObject = DoSerializeToJson(InBehaviorTree);
    if (!JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize BehaviorTree to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    const auto WriteText = InFormats == ECk_AssetExporter_SidecarFormats::JsonAndText;

    const auto TextString = WriteText ? DoSerializeToText(InBehaviorTree) : FString{};

    const auto JsonPath = DoResolveOutputPath(InBehaviorTree, ck::asset_exporter::extension::Sidecar);
    const auto TextPath = WriteText ? DoResolveOutputPath(InBehaviorTree, ck::asset_exporter::extension::SummaryText) : FString{};

    if (JsonPath.IsEmpty() || (WriteText && TextPath.IsEmpty()))
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    const auto bJsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto bTextWritten = NOT WriteText || FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (!bJsonWritten || !bTextWritten)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to write files (JSON: %s, Text: %s)"),
            bJsonWritten ? TEXT("OK") : TEXT("FAILED"),
            bTextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_BehaviorTreeExporter::
    ExportBehaviorTrees(
        const TArray<UBehaviorTree*>& InBehaviorTrees)
    -> TArray<FCk_BehaviorTreeExportResult>
{
    auto Results = TArray<FCk_BehaviorTreeExportResult>{};
    Results.Reserve(InBehaviorTrees.Num());

    for (auto* BT : InBehaviorTrees)
    {
        Results.Add(ExportBehaviorTree(BT));
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BehaviorTreeExporter::
    DoSerializeToJson(
        const UBehaviorTree* InBehaviorTree)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InBehaviorTree->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InBehaviorTree->GetPathName());
    RootObject->SetObjectField(TEXT("_meta"), FCk_AssetExportMeta::MakeMetaObject(InBehaviorTree, ck::asset_exporter::version::BehaviorTree));

    RootObject->SetObjectField(TEXT("blackboard"),
        DoSerializeBlackboard_Json(InBehaviorTree->BlackboardAsset));

    if (IsValid(InBehaviorTree->RootNode))
    {
        RootObject->SetObjectField(TEXT("rootNode"),
            DoSerializeCompositeNode_Json(InBehaviorTree->RootNode));
    }

    return RootObject;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeCompositeNode_Json(
        const UBTCompositeNode* InNode)
    -> TSharedPtr<FJsonObject>
{
    auto NodeObject = MakeShared<FJsonObject>();

    if (!IsValid(InNode))
    { return NodeObject; }

    auto ClassName = FString{};
    auto NodeName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InNode, ClassName, NodeName, Description);

    NodeObject->SetStringField(TEXT("nodeType"), TEXT("Composite"));
    NodeObject->SetStringField(TEXT("className"), ClassName);
    NodeObject->SetStringField(TEXT("nodeName"), NodeName);

    if (!Description.IsEmpty())
    {
        NodeObject->SetStringField(TEXT("description"), Description);
    }

    NodeObject->SetObjectField(TEXT("properties"), DoSerializeNodeProperties_Json(InNode));

    auto ServicesArray = TArray<UBTService*>{};
    for (int32 i = 0; i < InNode->Services.Num(); ++i)
    {
        if (IsValid(InNode->Services[i]))
        {
            ServicesArray.Add(InNode->Services[i]);
        }
    }
    NodeObject->SetArrayField(TEXT("services"), DoSerializeServices_Json(ServicesArray));

    auto ChildrenArray = TArray<TSharedPtr<FJsonValue>>{};
    for (int32 ChildIdx = 0; ChildIdx < InNode->Children.Num(); ++ChildIdx)
    {
        const auto& Child = InNode->Children[ChildIdx];

        auto ChildWrapper = MakeShared<FJsonObject>();

        auto ChildDecorators = TArray<UBTDecorator*>{};
        for (int32 i = 0; i < Child.Decorators.Num(); ++i)
        {
            if (IsValid(Child.Decorators[i]))
            {
                ChildDecorators.Add(Child.Decorators[i]);
            }
        }
        ChildWrapper->SetArrayField(TEXT("decorators"), DoSerializeDecorators_Json(ChildDecorators));

        if (IsValid(Child.ChildComposite))
        {
            ChildWrapper->SetObjectField(TEXT("node"),
                DoSerializeCompositeNode_Json(Child.ChildComposite));
        }
        else if (IsValid(Child.ChildTask))
        {
            ChildWrapper->SetObjectField(TEXT("node"),
                DoSerializeTaskNode_Json(Child.ChildTask));
        }

        ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildWrapper));
    }

    NodeObject->SetArrayField(TEXT("children"), ChildrenArray);

    return NodeObject;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeTaskNode_Json(
        const UBTTaskNode* InNode)
    -> TSharedPtr<FJsonObject>
{
    auto NodeObject = MakeShared<FJsonObject>();

    if (!IsValid(InNode))
    { return NodeObject; }

    auto ClassName = FString{};
    auto NodeName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InNode, ClassName, NodeName, Description);

    NodeObject->SetStringField(TEXT("nodeType"), TEXT("Task"));
    NodeObject->SetStringField(TEXT("className"), ClassName);
    NodeObject->SetStringField(TEXT("nodeName"), NodeName);

    if (!Description.IsEmpty())
    {
        NodeObject->SetStringField(TEXT("description"), Description);
    }

    NodeObject->SetObjectField(TEXT("properties"), DoSerializeNodeProperties_Json(InNode));

    auto ServicesArray = TArray<UBTService*>{};
    for (int32 i = 0; i < InNode->Services.Num(); ++i)
    {
        if (IsValid(InNode->Services[i]))
        {
            ServicesArray.Add(InNode->Services[i]);
        }
    }
    NodeObject->SetArrayField(TEXT("services"), DoSerializeServices_Json(ServicesArray));

    return NodeObject;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeDecorators_Json(
        const TArray<UBTDecorator*>& InDecorators)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto* Decorator : InDecorators)
    {
        if (!IsValid(Decorator))
        { continue; }

        auto DecObj = MakeShared<FJsonObject>();

        auto ClassName = FString{};
        auto NodeName = FString{};
        auto Description = FString{};
        DoGetNodeDisplayInfo(Decorator, ClassName, NodeName, Description);

        DecObj->SetStringField(TEXT("nodeType"), TEXT("Decorator"));
        DecObj->SetStringField(TEXT("className"), ClassName);
        DecObj->SetStringField(TEXT("nodeName"), NodeName);

        if (!Description.IsEmpty())
        {
            DecObj->SetStringField(TEXT("description"), Description);
        }

        DecObj->SetObjectField(TEXT("properties"), DoSerializeNodeProperties_Json(Decorator));

        Result.Add(MakeShared<FJsonValueObject>(DecObj));
    }

    return Result;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeServices_Json(
        const TArray<UBTService*>& InServices)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto* Service : InServices)
    {
        if (!IsValid(Service))
        { continue; }

        auto SvcObj = MakeShared<FJsonObject>();

        auto ClassName = FString{};
        auto NodeName = FString{};
        auto Description = FString{};
        DoGetNodeDisplayInfo(Service, ClassName, NodeName, Description);

        SvcObj->SetStringField(TEXT("nodeType"), TEXT("Service"));
        SvcObj->SetStringField(TEXT("className"), ClassName);
        SvcObj->SetStringField(TEXT("nodeName"), NodeName);

        if (!Description.IsEmpty())
        {
            SvcObj->SetStringField(TEXT("description"), Description);
        }

        SvcObj->SetObjectField(TEXT("properties"), DoSerializeNodeProperties_Json(Service));

        Result.Add(MakeShared<FJsonValueObject>(SvcObj));
    }

    return Result;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeNodeProperties_Json(
        const UBTNode* InNode)
    -> TSharedPtr<FJsonObject>
{
    auto PropertiesObject = MakeShared<FJsonObject>();

    if (!IsValid(InNode))
    { return PropertiesObject; }

    const auto* CDO = InNode->GetClass()->GetDefaultObject();

    for (TFieldIterator<FProperty> Itr(InNode->GetClass()); Itr; ++Itr)
    {
        const auto* Property = *Itr;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        const auto& PropertyName = Property->GetName();
        const auto& PropertyValue = DoGetPropertyValueAsString(Property, InNode);

        const auto* InstanceValuePtr = Property->ContainerPtrToValuePtr<void>(InNode);
        const auto* DefaultValuePtr = Property->ContainerPtrToValuePtr<void>(CDO);
        const auto bIsDefault = Property->Identical(InstanceValuePtr, DefaultValuePtr);

        auto PropertyObject = MakeShared<FJsonObject>();
        PropertyObject->SetStringField(TEXT("value"), PropertyValue);
        PropertyObject->SetBoolField(TEXT("isDefault"), bIsDefault);

        PropertiesObject->SetObjectField(PropertyName, PropertyObject);
    }

    return PropertiesObject;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeBlackboard_Json(
        const UBlackboardData* InBlackboardData)
    -> TSharedPtr<FJsonObject>
{
    auto BlackboardObj = MakeShared<FJsonObject>();

    if (!IsValid(InBlackboardData))
    {
        BlackboardObj->SetStringField(TEXT("assetName"), TEXT("None"));
        return BlackboardObj;
    }

    BlackboardObj->SetStringField(TEXT("assetName"), InBlackboardData->GetName());
    BlackboardObj->SetStringField(TEXT("assetPath"), InBlackboardData->GetPathName());

    if (IsValid(InBlackboardData->Parent))
    {
        BlackboardObj->SetStringField(TEXT("parentBlackboard"), InBlackboardData->Parent->GetPathName());
    }

    auto KeysArray = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto& Key : InBlackboardData->Keys)
    {
        auto KeyObj = MakeShared<FJsonObject>();
        KeyObj->SetStringField(TEXT("entryName"), Key.EntryName.ToString());

        if (IsValid(Key.KeyType))
        {
            KeyObj->SetStringField(TEXT("keyType"), Key.KeyType->GetClass()->GetName());
        }
        else
        {
            KeyObj->SetStringField(TEXT("keyType"), TEXT("None"));
        }

        KeyObj->SetBoolField(TEXT("isInstanceSynced"), Key.bInstanceSynced);

        if (!Key.EntryDescription.IsEmpty())
        {
            KeyObj->SetStringField(TEXT("entryDescription"), Key.EntryDescription);
        }

        KeysArray.Add(MakeShared<FJsonValueObject>(KeyObj));
    }

    if (IsValid(InBlackboardData->Parent))
    {
        auto ParentKeysArray = TArray<TSharedPtr<FJsonValue>>{};

        for (const auto& Key : InBlackboardData->Parent->Keys)
        {
            auto KeyObj = MakeShared<FJsonObject>();
            KeyObj->SetStringField(TEXT("entryName"), Key.EntryName.ToString());

            if (IsValid(Key.KeyType))
            {
                KeyObj->SetStringField(TEXT("keyType"), Key.KeyType->GetClass()->GetName());
            }
            else
            {
                KeyObj->SetStringField(TEXT("keyType"), TEXT("None"));
            }

            KeyObj->SetBoolField(TEXT("isInstanceSynced"), Key.bInstanceSynced);

            ParentKeysArray.Add(MakeShared<FJsonValueObject>(KeyObj));
        }

        BlackboardObj->SetArrayField(TEXT("parentKeys"), ParentKeysArray);
    }

    BlackboardObj->SetArrayField(TEXT("keys"), KeysArray);

    return BlackboardObj;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BehaviorTreeExporter::
    DoSerializeToText(
        const UBehaviorTree* InBehaviorTree)
    -> FString
{
    auto Text = FCk_AssetExportMeta::Get_SummaryTextBanner(InBehaviorTree->GetName());

    Text += FString::Printf(TEXT("=== Behavior Tree: %s ===\n"), *InBehaviorTree->GetName());
    Text += FString::Printf(TEXT("Path: %s\n"), *InBehaviorTree->GetPathName());
    Text += TEXT("\n");

    DoSerializeBlackboard_Text(InBehaviorTree->BlackboardAsset, Text);

    Text += TEXT("--- Tree Structure ---\n");

    if (IsValid(InBehaviorTree->RootNode))
    {
        DoSerializeCompositeNode_Text(InBehaviorTree->RootNode, Text, 0);
    }
    else
    {
        Text += TEXT("(No root node)\n");
    }

    return Text;
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeCompositeNode_Text(
        const UBTCompositeNode* InNode,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InNode))
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    auto ClassName = FString{};
    auto NodeName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InNode, ClassName, NodeName, Description);

    OutText += FString::Printf(TEXT("%s[Composite] %s: \"%s\"\n"), *Indent, *ClassName, *NodeName);

    if (!Description.IsEmpty())
    {
        OutText += FString::Printf(TEXT("%s  Description: %s\n"), *Indent, *Description);
    }

    DoSerializeNodeProperties_Text(InNode, OutText, InDepth + 1);

    auto ServicesArray = TArray<UBTService*>{};
    for (int32 i = 0; i < InNode->Services.Num(); ++i)
    {
        if (IsValid(InNode->Services[i]))
        {
            ServicesArray.Add(InNode->Services[i]);
        }
    }
    DoSerializeServices_Text(ServicesArray, OutText, InDepth + 1);

    for (int32 ChildIdx = 0; ChildIdx < InNode->Children.Num(); ++ChildIdx)
    {
        const auto& Child = InNode->Children[ChildIdx];

        auto ChildDecorators = TArray<UBTDecorator*>{};
        for (int32 i = 0; i < Child.Decorators.Num(); ++i)
        {
            if (IsValid(Child.Decorators[i]))
            {
                ChildDecorators.Add(Child.Decorators[i]);
            }
        }

        if (ChildDecorators.Num() > 0)
        {
            DoSerializeDecorators_Text(ChildDecorators, OutText, InDepth + 1);
        }

        if (IsValid(Child.ChildComposite))
        {
            DoSerializeCompositeNode_Text(Child.ChildComposite, OutText, InDepth + 1);
        }
        else if (IsValid(Child.ChildTask))
        {
            DoSerializeTaskNode_Text(Child.ChildTask, OutText, InDepth + 1);
        }
    }
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeTaskNode_Text(
        const UBTTaskNode* InNode,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InNode))
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    auto ClassName = FString{};
    auto NodeName = FString{};
    auto Description = FString{};
    DoGetNodeDisplayInfo(InNode, ClassName, NodeName, Description);

    OutText += FString::Printf(TEXT("%s[Task] %s: \"%s\"\n"), *Indent, *ClassName, *NodeName);

    if (!Description.IsEmpty())
    {
        OutText += FString::Printf(TEXT("%s    Description: %s\n"), *Indent, *Description);
    }

    DoSerializeNodeProperties_Text(InNode, OutText, InDepth + 1);

    auto ServicesArray = TArray<UBTService*>{};
    for (int32 i = 0; i < InNode->Services.Num(); ++i)
    {
        if (IsValid(InNode->Services[i]))
        {
            ServicesArray.Add(InNode->Services[i]);
        }
    }
    DoSerializeServices_Text(ServicesArray, OutText, InDepth + 1);
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeDecorators_Text(
        const TArray<UBTDecorator*>& InDecorators,
        FString& OutText,
        int32 InDepth)
    -> void
{
    for (const auto* Decorator : InDecorators)
    {
        if (!IsValid(Decorator))
        { continue; }

        const auto Indent = DoGetIndent(InDepth);

        auto ClassName = FString{};
        auto NodeName = FString{};
        auto Description = FString{};
        DoGetNodeDisplayInfo(Decorator, ClassName, NodeName, Description);

        OutText += FString::Printf(TEXT("%s{Decorator} %s: \"%s\"\n"), *Indent, *ClassName, *NodeName);

        if (!Description.IsEmpty())
        {
            OutText += FString::Printf(TEXT("%s  Description: %s\n"), *Indent, *Description);
        }

        DoSerializeNodeProperties_Text(Decorator, OutText, InDepth);
    }
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeServices_Text(
        const TArray<UBTService*>& InServices,
        FString& OutText,
        int32 InDepth)
    -> void
{
    for (const auto* Service : InServices)
    {
        if (!IsValid(Service))
        { continue; }

        const auto Indent = DoGetIndent(InDepth);

        auto ClassName = FString{};
        auto NodeName = FString{};
        auto Description = FString{};
        DoGetNodeDisplayInfo(Service, ClassName, NodeName, Description);

        OutText += FString::Printf(TEXT("%s{Service} %s: \"%s\"\n"), *Indent, *ClassName, *NodeName);

        if (!Description.IsEmpty())
        {
            OutText += FString::Printf(TEXT("%s  Description: %s\n"), *Indent, *Description);
        }

        DoSerializeNodeProperties_Text(Service, OutText, InDepth);
    }
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeNodeProperties_Text(
        const UBTNode* InNode,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InNode))
    { return; }

    const auto Indent = DoGetIndent(InDepth);
    const auto* CDO = InNode->GetClass()->GetDefaultObject();

    auto bHasProperties = false;

    for (TFieldIterator<FProperty> Itr(InNode->GetClass()); Itr; ++Itr)
    {
        const auto* Property = *Itr;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        if (!bHasProperties)
        {
            OutText += FString::Printf(TEXT("%sProperties:\n"), *Indent);
            bHasProperties = true;
        }

        const auto& PropertyName = Property->GetName();
        const auto& PropertyValue = DoGetPropertyValueAsString(Property, InNode);
        const auto& DefaultAnnotation = DoGetPropertyDefaultAnnotation(Property, InNode, CDO);

        OutText += FString::Printf(TEXT("%s  %s = %s%s\n"), *Indent, *PropertyName, *PropertyValue, *DefaultAnnotation);
    }
}

auto
    FCk_BehaviorTreeExporter::
    DoSerializeBlackboard_Text(
        const UBlackboardData* InBlackboardData,
        FString& OutText)
    -> void
{
    if (!IsValid(InBlackboardData))
    {
        OutText += TEXT("--- Blackboard: None ---\n\n");
        return;
    }

    OutText += FString::Printf(TEXT("--- Blackboard: %s ---\n"), *InBlackboardData->GetName());
    OutText += FString::Printf(TEXT("  Path: %s\n"), *InBlackboardData->GetPathName());

    if (IsValid(InBlackboardData->Parent))
    {
        OutText += FString::Printf(TEXT("  Parent: %s\n"), *InBlackboardData->Parent->GetName());
        OutText += TEXT("  Parent Keys:\n");

        for (const auto& Key : InBlackboardData->Parent->Keys)
        {
            const auto KeyTypeName = IsValid(Key.KeyType) ? Key.KeyType->GetClass()->GetName() : TEXT("None");
            OutText += FString::Printf(TEXT("    [%s] %s (InstanceSynced: %s)\n"),
                *KeyTypeName,
                *Key.EntryName.ToString(),
                Key.bInstanceSynced ? TEXT("Yes") : TEXT("No"));
        }
    }

    OutText += TEXT("  Keys:\n");

    for (const auto& Key : InBlackboardData->Keys)
    {
        const auto KeyTypeName = IsValid(Key.KeyType) ? Key.KeyType->GetClass()->GetName() : TEXT("None");
        OutText += FString::Printf(TEXT("    [%s] %s (InstanceSynced: %s)\n"),
            *KeyTypeName,
            *Key.EntryName.ToString(),
            Key.bInstanceSynced ? TEXT("Yes") : TEXT("No"));
    }

    OutText += TEXT("\n");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BehaviorTreeExporter::
    DoResolveOutputPath(
        const UBehaviorTree* InBehaviorTree,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InBehaviorTree->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;

    return DiskPath;
}

auto
    FCk_BehaviorTreeExporter::
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer)
    -> FString
{
    auto Value = FString{};
    const auto* ValuePtr = InProperty->ContainerPtrToValuePtr<void>(InContainer);
    InProperty->ExportTextItem_Direct(Value, ValuePtr, nullptr, nullptr, PPF_None);

    const auto ExportedAsEmptyStruct = Value == TEXT("()");
    if (ExportedAsEmptyStruct)
    {
        const auto* StructProperty = CastField<FStructProperty>(InProperty);
        if (StructProperty != nullptr)
        {
            auto ExpandedParts = TArray<FString>{};

            for (TFieldIterator<FProperty> SubItr(StructProperty->Struct); SubItr; ++SubItr)
            {
                const auto* SubProperty = *SubItr;
                auto SubValue = FString{};
                const auto* SubValuePtr = SubProperty->ContainerPtrToValuePtr<void>(ValuePtr);
                SubProperty->ExportTextItem_Direct(SubValue, SubValuePtr, nullptr, nullptr, PPF_None);
                ExpandedParts.Add(FString::Printf(TEXT("%s=%s"), *SubProperty->GetName(), *SubValue));
            }

            Value = FString::Printf(TEXT("(%s)"), *FString::Join(ExpandedParts, TEXT(", ")));
        }
    }

    return Value;
}

auto
    FCk_BehaviorTreeExporter::
    DoGetPropertyDefaultAnnotation(
        const FProperty* InProperty,
        const UBTNode* InNode,
        const UObject* InCDO)
    -> FString
{
    const auto* InstanceValuePtr = InProperty->ContainerPtrToValuePtr<void>(InNode);
    const auto* DefaultValuePtr = InProperty->ContainerPtrToValuePtr<void>(InCDO);
    const auto bIdentical = InProperty->Identical(InstanceValuePtr, DefaultValuePtr);
    return bIdentical ? TEXT(" [default]") : TEXT("");
}

auto
    FCk_BehaviorTreeExporter::
    DoShouldIncludeProperty(
        const FProperty* InProperty)
    -> bool
{
    if (InProperty == nullptr)
    { return false; }

    if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
    { return false; }

    const auto* OwnerClass = InProperty->GetOwnerClass();
    if (OwnerClass == nullptr)
    { return false; }

    static const TSet<FName> SkipClasses = {
        TEXT("Object"),
        TEXT("BTNode"),
        TEXT("BTAuxiliaryNode"),
    };

    if (SkipClasses.Contains(OwnerClass->GetFName()))
    { return false; }

    if (InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
    { return true; }

    return false;
}

auto
    FCk_BehaviorTreeExporter::
    DoGetIndent(
        int32 InDepth)
    -> FString
{
    auto Indent = FString{};
    for (int32 i = 0; i < InDepth; ++i)
    {
        Indent += TEXT("    ");
    }
    return Indent;
}

auto
    FCk_BehaviorTreeExporter::
    DoGetNodeDisplayInfo(
        const UBTNode* InNode,
        FString& OutClassName,
        FString& OutNodeName,
        FString& OutDescription)
    -> void
{
    if (!IsValid(InNode))
    {
        OutClassName = TEXT("Unknown");
        OutNodeName = TEXT("Unknown");
        OutDescription = FString{};
        return;
    }

    OutClassName = InNode->GetClass()->GetName();
    OutNodeName = InNode->GetNodeName();
    OutDescription = InNode->GetStaticDescription();

    OutDescription.ReplaceInline(LINE_TERMINATOR, TEXT(" | "));
}

// --------------------------------------------------------------------------------------------------------------------
