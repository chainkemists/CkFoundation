#include "CkStateTreeExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <StateTree.h>
#include <StateTreeSchema.h>
#include <StateTreeTypes.h>
#include <StateTreeEditorData.h>
#include <StateTreeState.h>
#include <StateTreeEditorNode.h>

#include <StructUtils/PropertyBag.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>

#include <UObject/UnrealType.h>
#include <UObject/FieldIterator.h>
#include <UObject/StructOnScope.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_StateTreeExporter::
    ExportStateTree(
        UStateTree* InStateTree,
        ECk_AssetExporter_SidecarFormats InFormats)
    -> FCk_StateTreeExportResult
{
    auto Result = FCk_StateTreeExportResult{};

    if (!IsValid(InStateTree))
    {
        Result.ErrorMessage = TEXT("Invalid StateTree asset");
        return Result;
    }

    Result.AssetName = InStateTree->GetName();

    const auto* EditorData = Cast<UStateTreeEditorData>(InStateTree->EditorData);
    if (!IsValid(EditorData))
    {
        Result.ErrorMessage = TEXT("StateTree has no editor data (cooked or never-opened asset)");
        return Result;
    }

    const auto JsonObject = DoSerializeToJson(InStateTree);
    if (!JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize StateTree to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    const auto WriteText = ck::asset_exporter::ShouldWrite_SummaryText(InFormats);

    const auto TextString = WriteText ? DoSerializeToText(InStateTree) : FString{};

    const auto JsonPath = DoResolveOutputPath(InStateTree, ck::asset_exporter::extension::Sidecar);
    const auto TextPath = WriteText ? DoResolveOutputPath(InStateTree, ck::asset_exporter::extension::SummaryText) : FString{};

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
    FCk_StateTreeExporter::
    ExportStateTrees(
        const TArray<UStateTree*>& InStateTrees)
    -> TArray<FCk_StateTreeExportResult>
{
    auto Results = TArray<FCk_StateTreeExportResult>{};
    Results.Reserve(InStateTrees.Num());

    for (auto* ST : InStateTrees)
    {
        Results.Add(ExportStateTree(ST));
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_StateTreeExporter::
    DoSerializeToJson(
        const UStateTree* InStateTree)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InStateTree->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InStateTree->GetPathName());
    RootObject->SetObjectField(TEXT("_meta"), FCk_AssetExportMeta::MakeMetaObject(InStateTree, ck::asset_exporter::version::StateTree));

    const auto* Schema = InStateTree->GetSchema();
    if (IsValid(Schema))
    {
        RootObject->SetStringField(TEXT("schema"), Schema->GetClass()->GetName());
    }
    else
    {
        RootObject->SetStringField(TEXT("schema"), TEXT("None"));
    }

    const auto* EditorData = Cast<UStateTreeEditorData>(InStateTree->EditorData);
    if (!IsValid(EditorData))
    { return RootObject; }

    const auto& RootParameterBag = EditorData->GetRootParametersPropertyBag();
    if (const auto* BagStruct = RootParameterBag.GetPropertyBagStruct())
    {
        RootObject->SetObjectField(TEXT("rootParameters"),
            DoSerializeProperties_Json(BagStruct, RootParameterBag.GetValue().GetMemory(), nullptr));
    }

    auto EvaluatorsArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Eval : EditorData->Evaluators)
    {
        EvaluatorsArray.Add(MakeShared<FJsonValueObject>(DoSerializeEditorNode_Json(Eval, TEXT("Evaluator"))));
    }
    RootObject->SetArrayField(TEXT("globalEvaluators"), EvaluatorsArray);

    auto GlobalTasksArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Task : EditorData->GlobalTasks)
    {
        GlobalTasksArray.Add(MakeShared<FJsonValueObject>(DoSerializeEditorNode_Json(Task, TEXT("Task"))));
    }
    RootObject->SetArrayField(TEXT("globalTasks"), GlobalTasksArray);

    auto SubTreesArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const UStateTreeState* State : EditorData->SubTrees)
    {
        if (IsValid(State))
        {
            SubTreesArray.Add(MakeShared<FJsonValueObject>(DoSerializeState_Json(State)));
        }
    }
    RootObject->SetArrayField(TEXT("subTrees"), SubTreesArray);

    return RootObject;
}

auto
    FCk_StateTreeExporter::
    DoSerializeState_Json(
        const UStateTreeState* InState)
    -> TSharedPtr<FJsonObject>
{
    auto StateObject = MakeShared<FJsonObject>();

    if (!IsValid(InState))
    { return StateObject; }

    StateObject->SetStringField(TEXT("name"), InState->Name.ToString());
    StateObject->SetStringField(TEXT("type"), DoGetEnumValueAsString(StaticEnum<EStateTreeStateType>(), static_cast<int64>(InState->Type)));
    StateObject->SetStringField(TEXT("selectionBehavior"), DoGetEnumValueAsString(StaticEnum<EStateTreeStateSelectionBehavior>(), static_cast<int64>(InState->SelectionBehavior)));
    StateObject->SetStringField(TEXT("id"), InState->ID.ToString());
    StateObject->SetBoolField(TEXT("enabled"), InState->bEnabled);

    if (!InState->Description.IsEmpty())
    {
        StateObject->SetStringField(TEXT("description"), InState->Description);
    }

    if (InState->Tag.IsValid())
    {
        StateObject->SetStringField(TEXT("tag"), InState->Tag.ToString());
    }

    if (IsValid(InState->LinkedAsset))
    {
        StateObject->SetStringField(TEXT("linkedAsset"), InState->LinkedAsset->GetPathName());
    }

    auto EnterConditionsArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Cond : InState->EnterConditions)
    {
        EnterConditionsArray.Add(MakeShared<FJsonValueObject>(DoSerializeEditorNode_Json(Cond, TEXT("Condition"))));
    }
    StateObject->SetArrayField(TEXT("enterConditions"), EnterConditionsArray);

    auto TasksArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Task : InState->Tasks)
    {
        TasksArray.Add(MakeShared<FJsonValueObject>(DoSerializeEditorNode_Json(Task, TEXT("Task"))));
    }
    if (InState->SingleTask.Node.IsValid())
    {
        TasksArray.Add(MakeShared<FJsonValueObject>(DoSerializeEditorNode_Json(InState->SingleTask, TEXT("Task"))));
    }
    StateObject->SetArrayField(TEXT("tasks"), TasksArray);

    auto TransitionsArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Transition : InState->Transitions)
    {
        TransitionsArray.Add(MakeShared<FJsonValueObject>(DoSerializeTransition_Json(Transition)));
    }
    StateObject->SetArrayField(TEXT("transitions"), TransitionsArray);

    auto ChildrenArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const UStateTreeState* Child : InState->Children)
    {
        if (IsValid(Child))
        {
            ChildrenArray.Add(MakeShared<FJsonValueObject>(DoSerializeState_Json(Child)));
        }
    }
    StateObject->SetArrayField(TEXT("children"), ChildrenArray);

    return StateObject;
}

auto
    FCk_StateTreeExporter::
    DoSerializeEditorNode_Json(
        const FStateTreeEditorNode& InNode,
        const FString& InNodeType)
    -> TSharedPtr<FJsonObject>
{
    auto NodeObject = MakeShared<FJsonObject>();

    const auto* NodeStruct = InNode.Node.GetScriptStruct();
    if (NodeStruct == nullptr)
    {
        NodeObject->SetStringField(TEXT("nodeType"), InNodeType);
        NodeObject->SetStringField(TEXT("className"), TEXT("None"));
        return NodeObject;
    }

    auto ClassName = FString{};
    auto NodeName = FString{};
    DoGetEditorNodeDisplayInfo(InNode, ClassName, NodeName);

    NodeObject->SetStringField(TEXT("nodeType"), InNodeType);
    NodeObject->SetStringField(TEXT("className"), ClassName);
    NodeObject->SetStringField(TEXT("nodeName"), NodeName);
    NodeObject->SetStringField(TEXT("id"), InNode.ID.ToString());

    const auto NodeStructDefault = FStructOnScope{NodeStruct};
    NodeObject->SetObjectField(TEXT("nodeProperties"),
        DoSerializeProperties_Json(NodeStruct, InNode.Node.GetMemory(), NodeStructDefault.GetStructMemory()));

    if (IsValid(InNode.InstanceObject))
    {
        const auto* InstanceClass = InNode.InstanceObject->GetClass();
        NodeObject->SetObjectField(TEXT("instanceProperties"),
            DoSerializeProperties_Json(InstanceClass, InNode.InstanceObject, InstanceClass->GetDefaultObject()));
    }
    else if (const auto* InstanceStruct = InNode.Instance.GetScriptStruct())
    {
        const auto InstanceDefault = FStructOnScope{InstanceStruct};
        NodeObject->SetObjectField(TEXT("instanceProperties"),
            DoSerializeProperties_Json(InstanceStruct, InNode.Instance.GetMemory(), InstanceDefault.GetStructMemory()));
    }

    return NodeObject;
}

auto
    FCk_StateTreeExporter::
    DoSerializeTransition_Json(
        const FStateTreeTransition& InTransition)
    -> TSharedPtr<FJsonObject>
{
    auto TransitionObject = MakeShared<FJsonObject>();

    TransitionObject->SetStringField(TEXT("trigger"), DoGetEnumValueAsString(StaticEnum<EStateTreeTransitionTrigger>(), static_cast<int64>(InTransition.Trigger)));
    TransitionObject->SetStringField(TEXT("priority"), DoGetEnumValueAsString(StaticEnum<EStateTreeTransitionPriority>(), static_cast<int64>(InTransition.Priority)));
    TransitionObject->SetBoolField(TEXT("enabled"), InTransition.bTransitionEnabled);

#if WITH_EDITORONLY_DATA
    TransitionObject->SetStringField(TEXT("toLinkType"), DoGetEnumValueAsString(StaticEnum<EStateTreeTransitionType>(), static_cast<int64>(InTransition.State.LinkType)));
    TransitionObject->SetStringField(TEXT("toState"), InTransition.State.Name.ToString());
#endif

    if (InTransition.RequiredEvent.Tag.IsValid())
    {
        TransitionObject->SetStringField(TEXT("requiredEventTag"), InTransition.RequiredEvent.Tag.ToString());
    }

    if (InTransition.bDelayTransition)
    {
        TransitionObject->SetNumberField(TEXT("delayDuration"), InTransition.DelayDuration);
    }

    auto ConditionsArray = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Cond : InTransition.Conditions)
    {
        ConditionsArray.Add(MakeShared<FJsonValueObject>(DoSerializeEditorNode_Json(Cond, TEXT("Condition"))));
    }
    TransitionObject->SetArrayField(TEXT("conditions"), ConditionsArray);

    return TransitionObject;
}

auto
    FCk_StateTreeExporter::
    DoSerializeProperties_Json(
        const UStruct* InStruct,
        const void* InContainer,
        const void* InDefaultContainer)
    -> TSharedPtr<FJsonObject>
{
    auto PropertiesObject = MakeShared<FJsonObject>();

    if (InStruct == nullptr || InContainer == nullptr)
    { return PropertiesObject; }

    for (TFieldIterator<FProperty> Itr(InStruct); Itr; ++Itr)
    {
        const auto* Property = *Itr;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        const auto& PropertyName = Property->GetName();
        const auto& PropertyValue = DoGetPropertyValueAsString(Property, InContainer);

        auto PropertyObject = MakeShared<FJsonObject>();
        PropertyObject->SetStringField(TEXT("value"), PropertyValue);

        if (InDefaultContainer != nullptr)
        {
            const auto* InstanceValuePtr = Property->ContainerPtrToValuePtr<void>(InContainer);
            const auto* DefaultValuePtr = Property->ContainerPtrToValuePtr<void>(InDefaultContainer);
            PropertyObject->SetBoolField(TEXT("isDefault"), Property->Identical(InstanceValuePtr, DefaultValuePtr));
        }

        PropertiesObject->SetObjectField(PropertyName, PropertyObject);
    }

    return PropertiesObject;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_StateTreeExporter::
    DoSerializeToText(
        const UStateTree* InStateTree)
    -> FString
{
    auto Text = FCk_AssetExportMeta::Get_SummaryTextBanner(InStateTree->GetName());

    Text += FString::Printf(TEXT("=== State Tree: %s ===\n"), *InStateTree->GetName());
    Text += FString::Printf(TEXT("Path: %s\n"), *InStateTree->GetPathName());

    const auto* Schema = InStateTree->GetSchema();
    const auto SchemaName = IsValid(Schema) ? Schema->GetClass()->GetName() : FString{TEXT("None")};
    Text += FString::Printf(TEXT("Schema: %s\n"), *SchemaName);
    Text += TEXT("\n");

    const auto* EditorData = Cast<UStateTreeEditorData>(InStateTree->EditorData);
    if (!IsValid(EditorData))
    {
        Text += TEXT("(No editor data)\n");
        return Text;
    }

    const auto& RootParameterBag = EditorData->GetRootParametersPropertyBag();
    if (const auto* BagStruct = RootParameterBag.GetPropertyBagStruct())
    {
        Text += TEXT("--- Root Parameters ---\n");
        DoSerializeProperties_Text(BagStruct, RootParameterBag.GetValue().GetMemory(), nullptr, Text, 1);
        Text += TEXT("\n");
    }

    if (EditorData->Evaluators.Num() > 0)
    {
        Text += TEXT("--- Global Evaluators ---\n");
        for (const auto& Eval : EditorData->Evaluators)
        {
            DoSerializeEditorNode_Text(Eval, TEXT("Evaluator"), Text, 1);
        }
        Text += TEXT("\n");
    }

    if (EditorData->GlobalTasks.Num() > 0)
    {
        Text += TEXT("--- Global Tasks ---\n");
        for (const auto& Task : EditorData->GlobalTasks)
        {
            DoSerializeEditorNode_Text(Task, TEXT("Task"), Text, 1);
        }
        Text += TEXT("\n");
    }

    Text += TEXT("--- Tree Structure ---\n");

    if (EditorData->SubTrees.Num() == 0)
    {
        Text += TEXT("(No states)\n");
    }
    else
    {
        for (const UStateTreeState* State : EditorData->SubTrees)
        {
            DoSerializeState_Text(State, Text, 0);
        }
    }

    return Text;
}

auto
    FCk_StateTreeExporter::
    DoSerializeState_Text(
        const UStateTreeState* InState,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (!IsValid(InState))
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    const auto TypeName = DoGetEnumValueAsString(StaticEnum<EStateTreeStateType>(), static_cast<int64>(InState->Type));
    OutText += FString::Printf(TEXT("%s[State] \"%s\" (%s)%s\n"),
        *Indent,
        *InState->Name.ToString(),
        *TypeName,
        InState->bEnabled ? TEXT("") : TEXT(" [disabled]"));

    if (!InState->Description.IsEmpty())
    {
        OutText += FString::Printf(TEXT("%s  Description: %s\n"), *Indent, *InState->Description);
    }

    OutText += FString::Printf(TEXT("%s  SelectionBehavior: %s\n"), *Indent,
        *DoGetEnumValueAsString(StaticEnum<EStateTreeStateSelectionBehavior>(), static_cast<int64>(InState->SelectionBehavior)));

    if (IsValid(InState->LinkedAsset))
    {
        OutText += FString::Printf(TEXT("%s  LinkedAsset: %s\n"), *Indent, *InState->LinkedAsset->GetPathName());
    }

    for (const auto& Cond : InState->EnterConditions)
    {
        DoSerializeEditorNode_Text(Cond, TEXT("EnterCondition"), OutText, InDepth + 1);
    }

    for (const auto& Task : InState->Tasks)
    {
        DoSerializeEditorNode_Text(Task, TEXT("Task"), OutText, InDepth + 1);
    }
    if (InState->SingleTask.Node.IsValid())
    {
        DoSerializeEditorNode_Text(InState->SingleTask, TEXT("Task"), OutText, InDepth + 1);
    }

    for (const auto& Transition : InState->Transitions)
    {
        DoSerializeTransition_Text(Transition, OutText, InDepth + 1);
    }

    for (const UStateTreeState* Child : InState->Children)
    {
        DoSerializeState_Text(Child, OutText, InDepth + 1);
    }
}

auto
    FCk_StateTreeExporter::
    DoSerializeEditorNode_Text(
        const FStateTreeEditorNode& InNode,
        const FString& InNodeType,
        FString& OutText,
        int32 InDepth)
    -> void
{
    const auto* NodeStruct = InNode.Node.GetScriptStruct();
    if (NodeStruct == nullptr)
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    auto ClassName = FString{};
    auto NodeName = FString{};
    DoGetEditorNodeDisplayInfo(InNode, ClassName, NodeName);

    OutText += FString::Printf(TEXT("%s{%s} %s: \"%s\"\n"), *Indent, *InNodeType, *ClassName, *NodeName);

    const auto NodeStructDefault = FStructOnScope{NodeStruct};
    DoSerializeProperties_Text(NodeStruct, InNode.Node.GetMemory(), NodeStructDefault.GetStructMemory(), OutText, InDepth + 1);

    if (IsValid(InNode.InstanceObject))
    {
        const auto* InstanceClass = InNode.InstanceObject->GetClass();
        DoSerializeProperties_Text(InstanceClass, InNode.InstanceObject, InstanceClass->GetDefaultObject(), OutText, InDepth + 1);
    }
    else if (const auto* InstanceStruct = InNode.Instance.GetScriptStruct())
    {
        const auto InstanceDefault = FStructOnScope{InstanceStruct};
        DoSerializeProperties_Text(InstanceStruct, InNode.Instance.GetMemory(), InstanceDefault.GetStructMemory(), OutText, InDepth + 1);
    }
}

auto
    FCk_StateTreeExporter::
    DoSerializeTransition_Text(
        const FStateTreeTransition& InTransition,
        FString& OutText,
        int32 InDepth)
    -> void
{
    const auto Indent = DoGetIndent(InDepth);

    auto ToState = FString{TEXT("?")};
#if WITH_EDITORONLY_DATA
    const auto LinkType = DoGetEnumValueAsString(StaticEnum<EStateTreeTransitionType>(), static_cast<int64>(InTransition.State.LinkType));
    ToState = InTransition.State.Name.IsNone()
        ? LinkType
        : FString::Printf(TEXT("%s (%s)"), *InTransition.State.Name.ToString(), *LinkType);
#endif

    OutText += FString::Printf(TEXT("%s-> [Transition] On %s -> %s (Priority: %s)%s\n"),
        *Indent,
        *DoGetEnumValueAsString(StaticEnum<EStateTreeTransitionTrigger>(), static_cast<int64>(InTransition.Trigger)),
        *ToState,
        *DoGetEnumValueAsString(StaticEnum<EStateTreeTransitionPriority>(), static_cast<int64>(InTransition.Priority)),
        InTransition.bTransitionEnabled ? TEXT("") : TEXT(" [disabled]"));

    if (InTransition.RequiredEvent.Tag.IsValid())
    {
        OutText += FString::Printf(TEXT("%s    RequiredEvent: %s\n"), *Indent, *InTransition.RequiredEvent.Tag.ToString());
    }

    for (const auto& Cond : InTransition.Conditions)
    {
        DoSerializeEditorNode_Text(Cond, TEXT("Condition"), OutText, InDepth + 1);
    }
}

auto
    FCk_StateTreeExporter::
    DoSerializeProperties_Text(
        const UStruct* InStruct,
        const void* InContainer,
        const void* InDefaultContainer,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (InStruct == nullptr || InContainer == nullptr)
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    for (TFieldIterator<FProperty> Itr(InStruct); Itr; ++Itr)
    {
        const auto* Property = *Itr;

        if (!DoShouldIncludeProperty(Property))
        { continue; }

        const auto& PropertyName = Property->GetName();
        const auto& PropertyValue = DoGetPropertyValueAsString(Property, InContainer);

        auto DefaultAnnotation = FString{};
        if (InDefaultContainer != nullptr)
        {
            const auto* InstanceValuePtr = Property->ContainerPtrToValuePtr<void>(InContainer);
            const auto* DefaultValuePtr = Property->ContainerPtrToValuePtr<void>(InDefaultContainer);
            DefaultAnnotation = Property->Identical(InstanceValuePtr, DefaultValuePtr) ? TEXT(" [default]") : TEXT("");
        }

        OutText += FString::Printf(TEXT("%s%s = %s%s\n"), *Indent, *PropertyName, *PropertyValue, *DefaultAnnotation);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_StateTreeExporter::
    DoResolveOutputPath(
        const UStateTree* InStateTree,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InStateTree->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;
    return DiskPath;
}

auto
    FCk_StateTreeExporter::
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
    FCk_StateTreeExporter::
    DoShouldIncludeProperty(
        const FProperty* InProperty)
    -> bool
{
    if (InProperty == nullptr)
    { return false; }

    if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
    { return false; }

    const auto* OwnerStruct = InProperty->GetOwnerStruct();
    if (OwnerStruct == nullptr)
    { return false; }

    static const TSet<FName> SkipStructs = {
        TEXT("Object"),
        TEXT("StateTreeNodeBase"),
    };

    if (SkipStructs.Contains(OwnerStruct->GetFName()))
    { return false; }

    if (InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
    { return true; }

    return false;
}

auto
    FCk_StateTreeExporter::
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
    FCk_StateTreeExporter::
    DoGetEditorNodeDisplayInfo(
        const FStateTreeEditorNode& InNode,
        FString& OutClassName,
        FString& OutNodeName)
    -> void
{
    const auto* NodeStruct = InNode.Node.GetScriptStruct();
    OutClassName = NodeStruct != nullptr ? NodeStruct->GetName() : FString{TEXT("Unknown")};
    OutNodeName = InNode.GetName().ToString();
}

auto
    FCk_StateTreeExporter::
    DoGetEnumValueAsString(
        const UEnum* InEnum,
        int64 InValue)
    -> FString
{
    if (InEnum == nullptr)
    { return FString::Printf(TEXT("%lld"), InValue); }

    return InEnum->GetNameStringByValue(InValue);
}

// --------------------------------------------------------------------------------------------------------------------
