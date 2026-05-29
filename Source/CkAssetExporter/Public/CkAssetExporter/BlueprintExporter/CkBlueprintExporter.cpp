#include "CkBlueprintExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/DataAssetExporter/CkDataAssetExporter.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Blueprint.h>
#include <Engine/BlueprintGeneratedClass.h>
#include <Engine/InheritableComponentHandler.h>
#include <Engine/SimpleConstructionScript.h>
#include <Engine/SCS_Node.h>
#include <Components/ActorComponent.h>
#include <Components/SceneComponent.h>
#include <GameFramework/Actor.h>
#include <UObject/Interface.h>
#include <EdGraph/EdGraph.h>
#include <EdGraph/EdGraphNode.h>
#include <EdGraph/EdGraphPin.h>
#include <EdGraphSchema_K2.h>
#include <K2Node.h>
#include <K2Node_Event.h>
#include <K2Node_CallFunction.h>
#include <K2Node_FunctionEntry.h>
#include <K2Node_FunctionResult.h>

#include <WidgetBlueprint.h>
#include <Blueprint/WidgetTree.h>
#include <Components/Widget.h>
#include <Components/PanelWidget.h>
#include <Components/PanelSlot.h>
#include <Components/NamedSlotInterface.h>
#include <Components/SlateWrapperTypes.h>
#include <Animation/WidgetAnimation.h>
#include <Animation/WidgetAnimationBinding.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    ExportBlueprint(
        UBlueprint* InBlueprint)
    -> FCk_BlueprintExportResult
{
    auto Result = FCk_BlueprintExportResult{};

    if (ck::Is_NOT_Valid(InBlueprint))
    {
        Result.ErrorMessage = TEXT("Invalid Blueprint asset");
        return Result;
    }

    Result.AssetName = InBlueprint->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InBlueprint);
    if (NOT JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize Blueprint to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InBlueprint);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InBlueprint, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InBlueprint, TEXT(".txt"));

    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    // Write files
    const auto JsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (NOT JsonWritten || NOT TextWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, Text: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"),
            TextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_BlueprintExporter::
    ExportBlueprints(
        const TArray<UBlueprint*>& InBlueprints)
    -> TArray<FCk_BlueprintExportResult>
{
    auto Results = TArray<FCk_BlueprintExportResult>{};
    Results.Reserve(InBlueprints.Num());

    ck::algo::ForEach(InBlueprints, [&](UBlueprint* BP)
    {
        Results.Add(ExportBlueprint(BP));
    });

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    DoSerializeToJson(
        const UBlueprint* InBlueprint)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InBlueprint->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InBlueprint->GetPathName());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());
    RootObject->SetStringField(TEXT("blueprintType"), InBlueprint->GetClass()->GetName());

    // Parent class
    if (InBlueprint->ParentClass != nullptr)
    {
        RootObject->SetStringField(TEXT("parentClass"), InBlueprint->ParentClass->GetName());
        RootObject->SetStringField(TEXT("parentClassPath"), InBlueprint->ParentClass->GetPathName());
    }
    else
    {
        RootObject->SetStringField(TEXT("parentClass"), TEXT("None"));
    }

    RootObject->SetArrayField(TEXT("implementedInterfaces"),
        DoSerializeImplementedInterfaces_Json(InBlueprint));

    // Variables
    RootObject->SetArrayField(TEXT("variables"),
        DoSerializeVariables_Json(InBlueprint->NewVariables));

    // Class Default Object property values — captures EditDefaultsOnly /
    // BlueprintVisible inherited properties (essential for data-only Blueprints
    // where NewVariables is empty but the CDO carries all the configured data,
    // including instanced UObject subobjects).
    if (const auto* GeneratedClass = InBlueprint->GeneratedClass.Get())
    {
        if (const auto* CDO = GeneratedClass->GetDefaultObject())
        {
            RootObject->SetArrayField(TEXT("classDefaults"),
                FCk_DataAssetExporter::DoSerializeProperties_Json(CDO, UObject::StaticClass()));
        }
    }

    RootObject->SetArrayField(TEXT("components"),
        DoSerializeComponents_Json(InBlueprint));

    const auto SerializeGraphSet = [](const TArray<UEdGraph*>& InGraphs, const TCHAR* InCategory)
        -> TArray<TSharedPtr<FJsonValue>>
    {
        auto Out = TArray<TSharedPtr<FJsonValue>>{};
        ck::algo::ForEachIsValid(InGraphs, [&](const UEdGraph* Graph)
        {
            Out.Add(MakeShared<FJsonValueObject>(DoSerializeGraph_Json(Graph, InCategory)));
        });
        return Out;
    };

    RootObject->SetArrayField(TEXT("eventGraphs"),    SerializeGraphSet(InBlueprint->UbergraphPages, TEXT("EventGraph")));
    RootObject->SetArrayField(TEXT("functionGraphs"), SerializeGraphSet(InBlueprint->FunctionGraphs, TEXT("Function")));
    RootObject->SetArrayField(TEXT("macroGraphs"),    SerializeGraphSet(InBlueprint->MacroGraphs,    TEXT("Macro")));

    // Widget Blueprint (UMG) — widget hierarchy and animations live as editor-only
    // data on the UWidgetBlueprint and are not captured by the generic Blueprint sections.
    if (const auto* WidgetBlueprint = Cast<UWidgetBlueprint>(InBlueprint))
    {
        RootObject->SetObjectField(TEXT("widgetHierarchy"),
            DoSerializeWidgetHierarchy_Json(WidgetBlueprint));
        RootObject->SetArrayField(TEXT("animations"),
            DoSerializeAnimations_Json(WidgetBlueprint));
    }

    return RootObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializeVariables_Json(
        const TArray<FBPVariableDescription>& InVariables)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto& Var : InVariables)
    {
        auto VarObject = MakeShared<FJsonObject>();

        VarObject->SetStringField(TEXT("varName"), Var.VarName.ToString());
        VarObject->SetStringField(TEXT("friendlyName"), Var.FriendlyName);
        VarObject->SetStringField(TEXT("varType"), DoGetPinTypeAsString(Var.VarType));
        VarObject->SetStringField(TEXT("category"), Var.Category.ToString());
        VarObject->SetStringField(TEXT("defaultValue"), Var.DefaultValue);
        VarObject->SetBoolField(TEXT("isReplicated"), Var.PropertyFlags & CPF_Net);

        if (NOT Var.RepNotifyFunc.IsNone())
        {
            VarObject->SetStringField(TEXT("repNotifyFunc"), Var.RepNotifyFunc.ToString());
        }

        Result.Add(MakeShared<FJsonValueObject>(VarObject));
    }

    return Result;
}

auto
    FCk_BlueprintExporter::
    DoSerializeGraph_Json(
        const UEdGraph* InGraph,
        const FString& InGraphCategory)
    -> TSharedPtr<FJsonObject>
{
    auto GraphObject = MakeShared<FJsonObject>();

    GraphObject->SetStringField(TEXT("graphName"), InGraph->GetName());
    GraphObject->SetStringField(TEXT("graphCategory"), InGraphCategory);

    // Execution flows
    GraphObject->SetArrayField(TEXT("executionFlows"), DoExtractExecutionFlow_Json(InGraph));

    // All nodes
    auto NodesArray = TArray<TSharedPtr<FJsonValue>>{};
    ck::algo::ForEachIsValid(InGraph->Nodes, [&](const UEdGraphNode* Node)
    {
        NodesArray.Add(MakeShared<FJsonValueObject>(DoSerializeNode_Json(Node)));
    });
    GraphObject->SetArrayField(TEXT("nodes"), NodesArray);

    return GraphObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializeNode_Json(
        const UEdGraphNode* InNode)
    -> TSharedPtr<FJsonObject>
{
    auto NodeObject = MakeShared<FJsonObject>();

    NodeObject->SetStringField(TEXT("nodeId"), DoGetNodeId(InNode));
    NodeObject->SetStringField(TEXT("nodeClass"), InNode->GetClass()->GetName());
    NodeObject->SetStringField(TEXT("nodeTitle"), DoGetNodeDisplayName(InNode));

    if (NOT InNode->NodeComment.IsEmpty())
    {
        NodeObject->SetStringField(TEXT("nodeComment"), InNode->NodeComment);
    }

    // Separate input and output pins
    auto InputPins = TArray<TSharedPtr<FJsonValue>>{};
    auto OutputPins = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto* Pin : InNode->Pins)
    {
        if (Pin == nullptr || Pin->bHidden || Pin->bOrphanedPin)
        { continue; }

        auto PinJson = DoSerializePin_Json(Pin);

        if (Pin->Direction == EGPD_Input)
        {
            InputPins.Add(MakeShared<FJsonValueObject>(PinJson));
        }
        else
        {
            OutputPins.Add(MakeShared<FJsonValueObject>(PinJson));
        }
    }

    NodeObject->SetArrayField(TEXT("inputPins"), InputPins);
    NodeObject->SetArrayField(TEXT("outputPins"), OutputPins);

    return NodeObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializePin_Json(
        const UEdGraphPin* InPin)
    -> TSharedPtr<FJsonObject>
{
    auto PinObject = MakeShared<FJsonObject>();

    PinObject->SetStringField(TEXT("pinName"), InPin->PinName.ToString());
    PinObject->SetStringField(TEXT("pinType"), DoGetPinTypeAsString(InPin->PinType));
    PinObject->SetStringField(TEXT("direction"),
        InPin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));

    if (NOT InPin->DefaultValue.IsEmpty())
    {
        PinObject->SetStringField(TEXT("defaultValue"), InPin->DefaultValue);
    }
    else if (NOT InPin->DefaultTextValue.IsEmpty())
    {
        PinObject->SetStringField(TEXT("defaultValue"), InPin->DefaultTextValue.ToString());
    }
    else if (ck::IsValid(InPin->DefaultObject))
    {
        PinObject->SetStringField(TEXT("defaultValue"), InPin->DefaultObject->GetPathName());
    }

    // Connections
    auto Connections = TArray<TSharedPtr<FJsonValue>>{};
    ck::algo::ForEach(InPin->LinkedTo, [&](const UEdGraphPin* LinkedPin)
    {
        if (LinkedPin == nullptr || LinkedPin->GetOwningNode() == nullptr)
        { return; }

        Connections.Add(MakeShared<FJsonValueString>(ck::Format_UE(TEXT("{}.{}"),
            DoGetNodeId(LinkedPin->GetOwningNode()),
            LinkedPin->PinName.ToString())));
    });

    if (Connections.Num() > 0)
    {
        PinObject->SetArrayField(TEXT("connections"), Connections);
    }

    return PinObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializePinType_Json(
        const FEdGraphPinType& InPinType)
    -> TSharedPtr<FJsonObject>
{
    auto TypeObject = MakeShared<FJsonObject>();

    TypeObject->SetStringField(TEXT("category"), InPinType.PinCategory.ToString());

    if (NOT InPinType.PinSubCategory.IsNone())
    {
        TypeObject->SetStringField(TEXT("subCategory"), InPinType.PinSubCategory.ToString());
    }

    if (InPinType.PinSubCategoryObject.IsValid())
    {
        TypeObject->SetStringField(TEXT("subCategoryObject"),
            InPinType.PinSubCategoryObject->GetName());
    }

    if (InPinType.ContainerType == EPinContainerType::Array)
    {
        TypeObject->SetBoolField(TEXT("isArray"), true);
    }
    else if (InPinType.ContainerType == EPinContainerType::Set)
    {
        TypeObject->SetBoolField(TEXT("isSet"), true);
    }
    else if (InPinType.ContainerType == EPinContainerType::Map)
    {
        TypeObject->SetBoolField(TEXT("isMap"), true);
    }

    return TypeObject;
}

auto
    FCk_BlueprintExporter::
    DoExtractExecutionFlow_Json(
        const UEdGraph* InGraph)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Flows = TArray<TSharedPtr<FJsonValue>>{};

    // Find entry point nodes (events, function entries)
    for (const UEdGraphNode* Node : InGraph->Nodes)
    {
        if (ck::Is_NOT_Valid(Node))
        { continue; }

        const auto IsEntryPoint =
            Node->IsA<UK2Node_Event>() ||
            Node->IsA<UK2Node_FunctionEntry>();

        if (NOT IsEntryPoint)
        { continue; }

        auto FlowObject = MakeShared<FJsonObject>();
        FlowObject->SetStringField(TEXT("entryPoint"), DoGetNodeDisplayName(Node));
        FlowObject->SetStringField(TEXT("entryNodeId"), DoGetNodeId(Node));

        // Walk exec chain
        auto Steps = TArray<TSharedPtr<FJsonValue>>{};
        auto Visited = TSet<const UEdGraphNode*>{};

        struct FWalkItem
        {
            const UEdGraphNode* Node;
            FString BranchLabel;
        };

        auto Stack = TArray<FWalkItem>{};
        Stack.Push({Node, FString{}});

        constexpr auto MaxSteps = int32{200};
        auto StepCount = int32{0};

        while (Stack.Num() > 0 && StepCount < MaxSteps)
        {
            const auto Current = Stack.Pop(EAllowShrinking::No);

            if (Current.Node == nullptr || Visited.Contains(Current.Node))
            { continue; }

            Visited.Add(Current.Node);
            ++StepCount;

            auto StepObject = MakeShared<FJsonObject>();
            StepObject->SetStringField(TEXT("nodeId"), DoGetNodeId(Current.Node));
            StepObject->SetStringField(TEXT("title"), DoGetNodeDisplayName(Current.Node));

            if (NOT Current.BranchLabel.IsEmpty())
            {
                StepObject->SetStringField(TEXT("branchLabel"), Current.BranchLabel);
            }

            Steps.Add(MakeShared<FJsonValueObject>(StepObject));

            // Find output exec pins and follow them
            auto ExecOutputPins = TArray<const UEdGraphPin*>{};
            for (const auto* Pin : Current.Node->Pins)
            {
                if (Pin != nullptr &&
                    Pin->Direction == EGPD_Output &&
                    DoIsExecPin(Pin) &&
                    Pin->LinkedTo.Num() > 0)
                {
                    ExecOutputPins.Add(Pin);
                }
            }

            // If single exec output, continue chain. If multiple, label branches.
            if (ExecOutputPins.Num() == 1)
            {
                for (const auto* LinkedPin : ExecOutputPins[0]->LinkedTo)
                {
                    if (LinkedPin != nullptr && LinkedPin->GetOwningNode() != nullptr)
                    {
                        Stack.Push({LinkedPin->GetOwningNode(), FString{}});
                    }
                }
            }
            else if (ExecOutputPins.Num() > 1)
            {
                // Reverse iterate to maintain order when popping from stack
                for (auto i = ExecOutputPins.Num() - 1; i >= 0; --i)
                {
                    const auto* ExecPin = ExecOutputPins[i];
                    for (const auto* LinkedPin : ExecPin->LinkedTo)
                    {
                        if (LinkedPin != nullptr && LinkedPin->GetOwningNode() != nullptr)
                        {
                            Stack.Push({LinkedPin->GetOwningNode(),
                                ck::Format_UE(TEXT("[{}]"), ExecPin->PinName.ToString())});
                        }
                    }
                }
            }
        }

        FlowObject->SetArrayField(TEXT("steps"), Steps);
        Flows.Add(MakeShared<FJsonValueObject>(FlowObject));
    }

    return Flows;
}

// --------------------------------------------------------------------------------------------------------------------
// Components & Interfaces — JSON
// --------------------------------------------------------------------------------------------------------------------

namespace ck_blueprint_exporter_internal
{
    struct FCollectedComponent
    {
        const UActorComponent* Template = nullptr;
        FName VariableName = NAME_None;
        FName AttachParent = NAME_None;
        FName AttachSocket = NAME_None;
        FString Origin;
    };

    // Resolves the effective component template for a parent-chain SCS node
    // by consulting the leaf BP's InheritableComponentHandler (ICH). UE stores
    // child-BP property overrides on inherited SCS components in the ICH, not
    // on the parent SCS node itself, so reading InNode->ComponentTemplate
    // directly always returns the un-overridden parent value. Walks the chain
    // from leaf upward so the closest override wins.
    //
    // Returns the parent's template when no override is present.
    static auto
        DoResolveEffectiveTemplate(
            const USCS_Node* InNode,
            const UBlueprintGeneratedClass* InLeafBPGC)
        -> const UActorComponent*
    {
        const auto* ParentTemplate = ck::IsValid(InNode)
            ? InNode->ComponentTemplate.Get()
            : nullptr;

        if (ck::Is_NOT_Valid(InLeafBPGC) || ck::Is_NOT_Valid(InNode))
        { return ParentTemplate; }

        const auto Key = FComponentKey{InNode};

        // UBlueprintGeneratedClass::GetInheritableComponentHandler is not
        // const-callable in UE 5.5/5.6/5.7, even with bCreateIfNecessary=false.
        // const_cast once at loop entry — we only read from the ICH here.
        auto* Cursor = const_cast<UBlueprintGeneratedClass*>(InLeafBPGC);
        while (ck::IsValid(Cursor))
        {
            if (const auto* ICH = Cursor->GetInheritableComponentHandler(false))
            {
                if (auto* Override = ICH->GetOverridenComponentTemplate(Key))
                { return Override; }
            }

            auto* SuperCls = Cursor->GetSuperClass();
            Cursor = Cast<UBlueprintGeneratedClass>(SuperCls);
        }

        return ParentTemplate;
    }

    static auto
        DoVisitScsNode(
            const USCS_Node* InNode,
            const FName& InAttachParent,
            const FString& InOrigin,
            const UBlueprintGeneratedClass* InLeafBPGC,
            TArray<FCollectedComponent>& OutCollected,
            TSet<const UActorComponent*>& InOutSeen)
        -> void
    {
        if (ck::Is_NOT_Valid(InNode))
        { return; }

        const auto* Template = DoResolveEffectiveTemplate(InNode, InLeafBPGC);
        if (ck::IsValid(Template) && NOT InOutSeen.Contains(Template))
        {
            InOutSeen.Add(Template);

            OutCollected.Add(FCollectedComponent
            {
                Template,
                InNode->GetVariableName(),
                InAttachParent,
                InNode->AttachToName,
                InOrigin
            });
        }

        const auto SelfName = InNode->GetVariableName();
        for (const auto* Child : InNode->GetChildNodes())
        {
            DoVisitScsNode(Child, SelfName, InOrigin, InLeafBPGC, OutCollected, InOutSeen);
        }
    }

    static auto
        DoCollectScsNodes(
            const USimpleConstructionScript* InScs,
            const FString& InOrigin,
            const UBlueprintGeneratedClass* InLeafBPGC,
            TArray<FCollectedComponent>& OutCollected,
            TSet<const UActorComponent*>& InOutSeen)
        -> void
    {
        if (ck::Is_NOT_Valid(InScs))
        { return; }

        for (const auto* Root : InScs->GetRootNodes())
        {
            if (ck::Is_NOT_Valid(Root))
            { continue; }

            DoVisitScsNode(Root, Root->ParentComponentOrVariableName, InOrigin, InLeafBPGC, OutCollected, InOutSeen);
        }
    }

    static auto
        DoCollectComponents(
            const UBlueprint* InBlueprint)
        -> TArray<FCollectedComponent>
    {
        auto Collected = TArray<FCollectedComponent>{};
        auto Seen = TSet<const UActorComponent*>{};

        const auto* GeneratedClass = InBlueprint->GeneratedClass.Get();
        if (ck::Is_NOT_Valid(GeneratedClass))
        { return Collected; }

        const auto* ActorCDO = Cast<AActor>(GeneratedClass->GetDefaultObject());
        if (ck::Is_NOT_Valid(ActorCDO))
        { return Collected; }

        // UBlueprint::GeneratedClass is typed UClass* (not UBlueprintGeneratedClass*).
        // Cast once here so the InheritedSCS walk below can pass the leaf BPGC into
        // DoResolveEffectiveTemplate for ICH lookup. Any UBlueprint we hit in practice
        // has a UBlueprintGeneratedClass at runtime, but guard for the editor edge case.
        const auto* LeafBPGC = Cast<UBlueprintGeneratedClass>(GeneratedClass);

        // 1. Native components on the CDO
        auto NativeComponents = TArray<UActorComponent*>{};
        ActorCDO->GetComponents(NativeComponents);
        for (const auto* Component : NativeComponents)
        {
            if (ck::Is_NOT_Valid(Component) || Seen.Contains(Component))
            { continue; }

            Seen.Add(Component);

            const auto* SceneComponent = Cast<USceneComponent>(Component);
            const auto AttachParentName = ck::IsValid(SceneComponent) && ck::IsValid(SceneComponent->GetAttachParent())
                ? SceneComponent->GetAttachParent()->GetFName()
                : NAME_None;

            Collected.Add(FCollectedComponent
            {
                Component,
                Component->GetFName(),
                AttachParentName,
                NAME_None,
                FString{TEXT("Native")}
            });
        }

        // 2. This-BP SCS nodes
        DoCollectScsNodes(InBlueprint->SimpleConstructionScript, FString{TEXT("SCS")}, LeafBPGC, Collected, Seen);

        // 3. Inherited SCS from parent BP chain — leaf BPGC threaded through
        // so DoResolveEffectiveTemplate can consult the child's ICH for
        // property overrides on inherited components.
        const auto* ParentClass = GeneratedClass->GetSuperClass();
        while (ck::IsValid(ParentClass))
        {
            const auto* ParentBPGC = Cast<UBlueprintGeneratedClass>(ParentClass);
            if (ck::IsValid(ParentBPGC))
            {
                DoCollectScsNodes(ParentBPGC->SimpleConstructionScript,
                    FString{TEXT("InheritedSCS")}, LeafBPGC, Collected, Seen);
            }
            ParentClass = ParentClass->GetSuperClass();
        }

        return Collected;
    }
}

auto
    FCk_BlueprintExporter::
    DoSerializeComponents_Json(
        const UBlueprint* InBlueprint)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    const auto Components = ck_blueprint_exporter_internal::DoCollectComponents(InBlueprint);
    for (const auto& Entry : Components)
    {
        const auto Object = DoSerializeComponent_Json(
            Entry.Template, Entry.VariableName, Entry.AttachParent, Entry.AttachSocket, Entry.Origin);
        if (Object.IsValid())
        {
            Result.Add(MakeShared<FJsonValueObject>(Object));
        }
    }

    return Result;
}

auto
    FCk_BlueprintExporter::
    DoSerializeComponent_Json(
        const UActorComponent* InComponent,
        const FName& InVariableName,
        const FName& InAttachParent,
        const FName& InAttachSocket,
        const FString& InOrigin)
    -> TSharedPtr<FJsonObject>
{
    if (ck::Is_NOT_Valid(InComponent))
    { return {}; }

    auto Object = MakeShared<FJsonObject>();

    const auto NameStr = InVariableName.IsNone() ? InComponent->GetName() : InVariableName.ToString();

    Object->SetStringField(TEXT("componentName"), NameStr);
    Object->SetStringField(TEXT("componentClass"), InComponent->GetClass()->GetName());
    Object->SetStringField(TEXT("componentClassPath"), InComponent->GetClass()->GetPathName());
    Object->SetStringField(TEXT("origin"), InOrigin);

    if (NOT InAttachParent.IsNone())
    {
        Object->SetStringField(TEXT("attachParent"), InAttachParent.ToString());
    }

    if (NOT InAttachSocket.IsNone())
    {
        Object->SetStringField(TEXT("attachSocket"), InAttachSocket.ToString());
    }

    if (const auto* SceneComponent = Cast<USceneComponent>(InComponent))
    {
        const auto Loc = SceneComponent->GetRelativeLocation();
        const auto Rot = SceneComponent->GetRelativeRotation();
        const auto Scale = SceneComponent->GetRelativeScale3D();

        auto LocObj = MakeShared<FJsonObject>();
        LocObj->SetNumberField(TEXT("x"), Loc.X);
        LocObj->SetNumberField(TEXT("y"), Loc.Y);
        LocObj->SetNumberField(TEXT("z"), Loc.Z);

        auto RotObj = MakeShared<FJsonObject>();
        RotObj->SetNumberField(TEXT("pitch"), Rot.Pitch);
        RotObj->SetNumberField(TEXT("yaw"), Rot.Yaw);
        RotObj->SetNumberField(TEXT("roll"), Rot.Roll);

        auto ScaleObj = MakeShared<FJsonObject>();
        ScaleObj->SetNumberField(TEXT("x"), Scale.X);
        ScaleObj->SetNumberField(TEXT("y"), Scale.Y);
        ScaleObj->SetNumberField(TEXT("z"), Scale.Z);

        auto TransformObj = MakeShared<FJsonObject>();
        TransformObj->SetObjectField(TEXT("location"), LocObj);
        TransformObj->SetObjectField(TEXT("rotation"), RotObj);
        TransformObj->SetObjectField(TEXT("scale"), ScaleObj);

        Object->SetObjectField(TEXT("relativeTransform"), TransformObj);
    }

    Object->SetArrayField(TEXT("properties"),
        FCk_DataAssetExporter::DoSerializeProperties_Json(InComponent, UActorComponent::StaticClass()));

    return Object;
}

auto
    FCk_BlueprintExporter::
    DoSerializeImplementedInterfaces_Json(
        const UBlueprint* InBlueprint)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    const auto* GeneratedClass = InBlueprint->GeneratedClass.Get();
    if (ck::Is_NOT_Valid(GeneratedClass))
    { return Result; }

    for (const auto& Implemented : GeneratedClass->Interfaces)
    {
        const auto* InterfaceClass = Implemented.Class.Get();
        if (ck::Is_NOT_Valid(InterfaceClass))
        { continue; }

        auto Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("interfaceName"), InterfaceClass->GetName());
        Object->SetStringField(TEXT("interfacePath"), InterfaceClass->GetPathName());

        auto Functions = TArray<TSharedPtr<FJsonValue>>{};
        for (TFieldIterator<UFunction> It(InterfaceClass); It; ++It)
        {
            const auto* Function = *It;
            if (ck::Is_NOT_Valid(Function))
            { continue; }

            if (Function->GetOwnerClass() == UInterface::StaticClass())
            { continue; }

            const auto FunctionObject = DoSerializeInterfaceFunction_Json(Function);
            if (FunctionObject.IsValid())
            {
                Functions.Add(MakeShared<FJsonValueObject>(FunctionObject));
            }
        }
        Object->SetArrayField(TEXT("functions"), Functions);

        Result.Add(MakeShared<FJsonValueObject>(Object));
    }

    return Result;
}

auto
    FCk_BlueprintExporter::
    DoSerializeInterfaceFunction_Json(
        const UFunction* InFunction)
    -> TSharedPtr<FJsonObject>
{
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("functionName"), InFunction->GetName());

    auto ReturnType = FString{TEXT("void")};
    auto Parameters = TArray<TSharedPtr<FJsonValue>>{};

    for (TFieldIterator<FProperty> It(InFunction); It; ++It)
    {
        const auto* Property = *It;
        if (ck::Is_NOT_Valid(Property) || NOT Property->HasAnyPropertyFlags(CPF_Parm))
        { continue; }

        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            ReturnType = Property->GetCPPType();
            continue;
        }

        auto ParamObject = MakeShared<FJsonObject>();
        ParamObject->SetStringField(TEXT("name"), Property->GetName());
        ParamObject->SetStringField(TEXT("type"), Property->GetCPPType());
        ParamObject->SetBoolField(TEXT("isOut"), Property->HasAnyPropertyFlags(CPF_OutParm));
        ParamObject->SetBoolField(TEXT("isReference"), Property->HasAnyPropertyFlags(CPF_ReferenceParm));

        Parameters.Add(MakeShared<FJsonValueObject>(ParamObject));
    }

    Object->SetStringField(TEXT("returnType"), ReturnType);
    Object->SetArrayField(TEXT("parameters"), Parameters);

    return Object;
}

// --------------------------------------------------------------------------------------------------------------------
// Widget Blueprint (UMG) Serialization
// --------------------------------------------------------------------------------------------------------------------

namespace ck_blueprint_exporter
{
    // Guards against pathological / cyclic trees (mirrors the execution-flow step guard).
    constexpr int32 MaxWidgetTreeDepth = 100;
}

auto
    FCk_BlueprintExporter::
    DoSerializeWidgetHierarchy_Json(
        const UWidgetBlueprint* InWidgetBlueprint)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    const auto* WidgetTree = InWidgetBlueprint->WidgetTree.Get();
    if (ck::Is_NOT_Valid(WidgetTree) || ck::Is_NOT_Valid(WidgetTree->RootWidget))
    { return RootObject; }

    RootObject->SetObjectField(TEXT("root"),
        DoSerializeWidget_Json(WidgetTree->RootWidget, FString{}, 0));

    return RootObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializeWidget_Json(
        const UWidget* InWidget,
        const FString& InSlotName,
        int32 InDepth)
    -> TSharedPtr<FJsonObject>
{
    auto WidgetObject = MakeShared<FJsonObject>();

    if (ck::Is_NOT_Valid(InWidget))
    { return WidgetObject; }

    WidgetObject->SetStringField(TEXT("name"), InWidget->GetName());
    WidgetObject->SetStringField(TEXT("class"), InWidget->GetClass()->GetName());
    WidgetObject->SetBoolField(TEXT("isVariable"), InWidget->bIsVariable);
    WidgetObject->SetStringField(TEXT("visibility"),
        StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(InWidget->GetVisibility())));

    if (NOT InSlotName.IsEmpty())
    {
        WidgetObject->SetStringField(TEXT("namedSlot"), InSlotName);
    }

    if (const auto ToolTip = InWidget->GetToolTipText().ToString(); NOT ToolTip.IsEmpty())
    {
        WidgetObject->SetStringField(TEXT("toolTip"), ToolTip);
    }

    // Layout slot (Canvas anchors, VerticalBox padding/alignment, etc.) — properties
    // declared by the concrete UPanelSlot subclass.
    if (const auto* PanelSlot = InWidget->Slot.Get())
    {
        auto SlotObject = MakeShared<FJsonObject>();
        SlotObject->SetStringField(TEXT("type"), PanelSlot->GetClass()->GetName());
        SlotObject->SetArrayField(TEXT("properties"),
            FCk_DataAssetExporter::DoSerializeProperties_Json(PanelSlot, UPanelSlot::StaticClass()));
        WidgetObject->SetObjectField(TEXT("slot"), SlotObject);
    }

    // Type-specific editable properties (Text, Brush, fonts, colors, …) — declared by
    // classes derived from UWidget (UWidget-level fields are captured explicitly above).
    if (auto Properties = FCk_DataAssetExporter::DoSerializeProperties_Json(InWidget, UWidget::StaticClass());
        Properties.Num() > 0)
    {
        WidgetObject->SetArrayField(TEXT("properties"), Properties);
    }

    if (InDepth >= ck_blueprint_exporter::MaxWidgetTreeDepth)
    { return WidgetObject; }

    auto Children = TArray<TSharedPtr<FJsonValue>>{};

    // Panel children
    if (const auto* PanelWidget = Cast<UPanelWidget>(InWidget))
    {
        for (auto ChildIndex = int32{0}; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
        {
            if (const auto* Child = PanelWidget->GetChildAt(ChildIndex))
            {
                Children.Add(MakeShared<FJsonValueObject>(
                    DoSerializeWidget_Json(Child, FString{}, InDepth + 1)));
            }
        }
    }

    // Named slots (UserWidgets, etc.)
    if (const auto* NamedSlotHost = Cast<INamedSlotInterface>(InWidget))
    {
        auto SlotNames = TArray<FName>{};
        NamedSlotHost->GetSlotNames(SlotNames);

        for (const auto& SlotName : SlotNames)
        {
            if (const auto* SlotContent = NamedSlotHost->GetContentForSlot(SlotName))
            {
                Children.Add(MakeShared<FJsonValueObject>(
                    DoSerializeWidget_Json(SlotContent, SlotName.ToString(), InDepth + 1)));
            }
        }
    }

    if (Children.Num() > 0)
    {
        WidgetObject->SetArrayField(TEXT("children"), Children);
    }

    return WidgetObject;
}

auto
    FCk_BlueprintExporter::
    DoSerializeAnimations_Json(
        const UWidgetBlueprint* InWidgetBlueprint)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Result = TArray<TSharedPtr<FJsonValue>>{};

    for (const auto& Animation : InWidgetBlueprint->Animations)
    {
        if (ck::Is_NOT_Valid(Animation))
        { continue; }

        auto AnimObject = MakeShared<FJsonObject>();

        AnimObject->SetStringField(TEXT("name"), Animation->GetName());
        AnimObject->SetStringField(TEXT("displayLabel"), Animation->GetDisplayLabel());
        AnimObject->SetNumberField(TEXT("startTime"), Animation->GetStartTime());
        AnimObject->SetNumberField(TEXT("endTime"), Animation->GetEndTime());
        AnimObject->SetNumberField(TEXT("length"), Animation->GetEndTime() - Animation->GetStartTime());

        auto BoundWidgets = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& Binding : Animation->GetBindings())
        {
            BoundWidgets.Add(MakeShared<FJsonValueString>(Binding.WidgetName.ToString()));
        }
        AnimObject->SetArrayField(TEXT("boundWidgets"), BoundWidgets);

        Result.Add(MakeShared<FJsonValueObject>(AnimObject));
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    DoSerializeToText(
        const UBlueprint* InBlueprint)
    -> FString
{
    auto Text = FString{};

    // Header
    Text += ck::Format_UE(TEXT("=== Blueprint: {} ===\n"), InBlueprint->GetName());
    Text += ck::Format_UE(TEXT("Path: {}\n"), InBlueprint->GetPathName());
    Text += ck::Format_UE(TEXT("Type: {}\n"), InBlueprint->GetClass()->GetName());

    if (ck::IsValid(InBlueprint->ParentClass))
    {
        Text += ck::Format_UE(TEXT("Parent Class: {} ({})\n"),
            InBlueprint->ParentClass->GetName(),
            InBlueprint->ParentClass->GetPathName());
    }

    Text += ck::Format_UE(TEXT("Exported: {}\n\n"), FDateTime::UtcNow().ToString());

    DoSerializeImplementedInterfaces_Text(InBlueprint, Text);

    // Variables
    DoSerializeVariables_Text(InBlueprint->NewVariables, Text);

    // Class Default Object property values
    if (const auto* GeneratedClass = InBlueprint->GeneratedClass.Get())
    {
        if (const auto* CDO = GeneratedClass->GetDefaultObject())
        {
            Text += TEXT("--- Class Defaults ---\n");
            FCk_DataAssetExporter::DoSerializeProperties_Text(CDO, UObject::StaticClass(), Text, 0);
        }
    }

    DoSerializeComponents_Text(InBlueprint, Text);

    const auto DumpGraphs = [&Text](const TArray<UEdGraph*>& InGraphs, const TCHAR* InCategory)
    {
        ck::algo::ForEachIsValid(InGraphs, [&](const UEdGraph* Graph)
        {
            DoSerializeGraph_Text(Graph, InCategory, Text);
        });
    };

    DumpGraphs(InBlueprint->UbergraphPages, TEXT("Event Graph"));
    DumpGraphs(InBlueprint->FunctionGraphs, TEXT("Function"));
    DumpGraphs(InBlueprint->MacroGraphs,    TEXT("Macro"));

    if (const auto* WidgetBlueprint = Cast<UWidgetBlueprint>(InBlueprint))
    {
        DoSerializeWidgetHierarchy_Text(WidgetBlueprint, Text);
        DoSerializeAnimations_Text(WidgetBlueprint, Text);
    }

    return Text;
}

auto
    FCk_BlueprintExporter::
    DoSerializeVariables_Text(
        const TArray<FBPVariableDescription>& InVariables,
        FString& OutText)
    -> void
{
    if (InVariables.Num() == 0)
    { return; }

    OutText += ck::Format_UE(TEXT("--- Variables ({}) ---\n"), InVariables.Num());

    for (const auto& Var : InVariables)
    {
        OutText += ck::Format_UE(TEXT("  [{}] {}"),
            DoGetPinTypeAsString(Var.VarType), Var.VarName.ToString());

        if (NOT Var.DefaultValue.IsEmpty())
        {
            OutText += ck::Format_UE(TEXT(" = {}"), Var.DefaultValue);
        }

        // Metadata in parentheses
        auto Meta = TArray<FString>{};

        if (NOT Var.Category.IsEmpty())
        {
            Meta.Add(ck::Format_UE(TEXT("Category: {}"), Var.Category.ToString()));
        }

        const auto Replicated = (Var.PropertyFlags & CPF_Net) != 0;
        if (Replicated)
        {
            Meta.Add(TEXT("Replicated: Yes"));
            if (NOT Var.RepNotifyFunc.IsNone())
            {
                Meta.Add(ck::Format_UE(TEXT("RepNotify: {}"), Var.RepNotifyFunc.ToString()));
            }
        }

        if (Meta.Num() > 0)
        {
            OutText += ck::Format_UE(TEXT(" ({})"), FString::Join(Meta, TEXT(", ")));
        }

        OutText += TEXT("\n");
    }

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeGraph_Text(
        const UEdGraph* InGraph,
        const FString& InGraphCategory,
        FString& OutText)
    -> void
{
    OutText += ck::Format_UE(TEXT("--- {}: {} ---\n"), InGraphCategory, InGraph->GetName());

    // Execution flows first (most useful for understanding logic)
    DoExtractExecutionFlow_Text(InGraph, OutText);

    // All nodes
    const auto ValidNodeCount = ck::algo::CountIf(InGraph->Nodes,
        [](const UEdGraphNode* Node) { return ck::IsValid(Node); });

    OutText += ck::Format_UE(TEXT("  Nodes ({}):\n"), ValidNodeCount);

    ck::algo::ForEachIsValid(InGraph->Nodes, [&](const UEdGraphNode* Node)
    {
        DoSerializeNode_Text(Node, OutText, 2);
    });

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeNode_Text(
        const UEdGraphNode* InNode,
        FString& OutText,
        int32 InDepth)
    -> void
{
    const auto Indent = DoGetIndent(InDepth);

    OutText += ck::Format_UE(TEXT("{}[{}] \"{}\" ({})\n"),
        Indent, InNode->GetClass()->GetName(), DoGetNodeDisplayName(InNode), DoGetNodeId(InNode));

    if (NOT InNode->NodeComment.IsEmpty())
    {
        OutText += ck::Format_UE(TEXT("{}  Comment: {}\n"), Indent, InNode->NodeComment);
    }

    // Shared per-direction pin serializer to de-dup input / output blocks.
    const auto SerializePinBlock = [&](EEdGraphPinDirection InDirection,
                                       const TCHAR* InHeader,
                                       const TCHAR* InArrow)
    {
        auto HeaderEmitted = false;
        for (const auto* Pin : InNode->Pins)
        {
            if (Pin == nullptr || Pin->bHidden || Pin->bOrphanedPin || Pin->Direction != InDirection)
            { continue; }

            if (NOT HeaderEmitted)
            {
                OutText += ck::Format_UE(TEXT("{}  {}:\n"), Indent, InHeader);
                HeaderEmitted = true;
            }

            const auto TypeStr = DoIsExecPin(Pin) ? FString{TEXT("exec")} : DoGetPinTypeAsString(Pin->PinType);
            OutText += ck::Format_UE(TEXT("{}    ({}) {}"), Indent, TypeStr, Pin->PinName.ToString());

            if (Pin->LinkedTo.Num() > 0)
            {
                auto Connections = TArray<FString>{};
                ck::algo::ForEach(Pin->LinkedTo, [&](const UEdGraphPin* Linked)
                {
                    if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                    {
                        Connections.Add(ck::Format_UE(TEXT("{}.{}"),
                            DoGetNodeId(Linked->GetOwningNode()),
                            Linked->PinName.ToString()));
                    }
                });
                OutText += ck::Format_UE(TEXT(" {} {}"), InArrow, FString::Join(Connections, TEXT(", ")));
            }
            else if (InDirection == EGPD_Input && NOT DoIsExecPin(Pin))
            {
                if (NOT Pin->DefaultValue.IsEmpty())
                {
                    OutText += ck::Format_UE(TEXT(" = {}"), Pin->DefaultValue);
                }
                else if (NOT Pin->DefaultTextValue.IsEmpty())
                {
                    OutText += ck::Format_UE(TEXT(" = {}"), Pin->DefaultTextValue.ToString());
                }
                else if (ck::IsValid(Pin->DefaultObject))
                {
                    OutText += ck::Format_UE(TEXT(" = {}"), Pin->DefaultObject->GetName());
                }
            }

            OutText += TEXT("\n");
        }
    };

    SerializePinBlock(EGPD_Input,  TEXT("In Pins"),  TEXT("<-"));
    SerializePinBlock(EGPD_Output, TEXT("Out Pins"), TEXT("->"));
}

auto
    FCk_BlueprintExporter::
    DoExtractExecutionFlow_Text(
        const UEdGraph* InGraph,
        FString& OutText)
    -> void
{
    // Find entry points
    auto EntryNodes = TArray<const UEdGraphNode*>{};
    ck::algo::ForEachIsValid(InGraph->Nodes, [&](const UEdGraphNode* Node)
    {
        if (Node->IsA<UK2Node_Event>() || Node->IsA<UK2Node_FunctionEntry>())
        {
            EntryNodes.Add(Node);
        }
    });

    if (EntryNodes.Num() == 0)
    { return; }

    OutText += TEXT("  Execution Flows:\n");

    auto FlowIndex = int32{1};
    for (const auto* EntryNode : EntryNodes)
    {
        OutText += ck::Format_UE(TEXT("    [{}] {}"),
            FlowIndex++, DoGetNodeDisplayName(EntryNode));

        // Walk the exec chain
        auto Visited = TSet<const UEdGraphNode*>{};
        Visited.Add(EntryNode);

        struct FWalkState
        {
            const UEdGraphNode* Node;
            int32 IndentLevel;
            FString Prefix;
        };

        auto Queue = TArray<FWalkState>{};

        // Seed with first exec output
        for (const auto* Pin : EntryNode->Pins)
        {
            if (Pin == nullptr || Pin->Direction != EGPD_Output || NOT DoIsExecPin(Pin))
            { continue; }

            ck::algo::ForEach(Pin->LinkedTo, [&](const UEdGraphPin* Linked)
            {
                if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                {
                    Queue.Add({Linked->GetOwningNode(), 0, FString{}});
                }
            });
        }

        constexpr auto MaxSteps = int32{200};
        auto StepCount = int32{0};

        while (Queue.Num() > 0 && StepCount < MaxSteps)
        {
            const auto Current = Queue[0];
            Queue.RemoveAt(0);
            ++StepCount;

            if (Current.Node == nullptr || Visited.Contains(Current.Node))
            {
                if (Current.IndentLevel > 0 && NOT Current.Prefix.IsEmpty())
                {
                    OutText += ck::Format_UE(TEXT("\n{}{} -> (cycle/end)"),
                        DoGetIndent(4 + Current.IndentLevel), Current.Prefix);
                }
                continue;
            }

            Visited.Add(Current.Node);

            if (Current.IndentLevel == 0 && Current.Prefix.IsEmpty())
            {
                OutText += ck::Format_UE(TEXT(" -> {}"), DoGetNodeDisplayName(Current.Node));
            }
            else
            {
                OutText += ck::Format_UE(TEXT("\n{}{} -> {}"),
                    DoGetIndent(4 + Current.IndentLevel),
                    Current.Prefix,
                    DoGetNodeDisplayName(Current.Node));
            }

            // Find exec outputs
            auto ExecOutputPins = TArray<const UEdGraphPin*>{};
            for (const auto* Pin : Current.Node->Pins)
            {
                if (Pin != nullptr &&
                    Pin->Direction == EGPD_Output &&
                    DoIsExecPin(Pin) &&
                    Pin->LinkedTo.Num() > 0)
                {
                    ExecOutputPins.Add(Pin);
                }
            }

            if (ExecOutputPins.Num() == 1)
            {
                for (const auto* Linked : ExecOutputPins[0]->LinkedTo)
                {
                    if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                    {
                        Queue.Insert({Linked->GetOwningNode(), Current.IndentLevel, FString{}}, 0);
                    }
                }
            }
            else if (ExecOutputPins.Num() > 1)
            {
                // Multiple branches - add each with a label
                for (auto i = ExecOutputPins.Num() - 1; i >= 0; --i)
                {
                    const auto* ExecPin = ExecOutputPins[i];
                    for (const auto* Linked : ExecPin->LinkedTo)
                    {
                        if (Linked != nullptr && Linked->GetOwningNode() != nullptr)
                        {
                            Queue.Insert({
                                Linked->GetOwningNode(),
                                Current.IndentLevel + 1,
                                ck::Format_UE(TEXT("[{}]"), ExecPin->PinName.ToString())
                            }, i == 0 ? 0 : Queue.Num());
                        }
                    }
                }
            }
        }

        OutText += TEXT("\n");
    }

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeComponents_Text(
        const UBlueprint* InBlueprint,
        FString& OutText)
    -> void
{
    const auto Components = ck_blueprint_exporter_internal::DoCollectComponents(InBlueprint);
    if (Components.Num() == 0)
    { return; }

    OutText += ck::Format_UE(TEXT("--- Components ({}) ---\n"), Components.Num());

    for (const auto& Entry : Components)
    {
        if (ck::Is_NOT_Valid(Entry.Template))
        { continue; }

        const auto NameStr = Entry.VariableName.IsNone() ? Entry.Template->GetName() : Entry.VariableName.ToString();

        OutText += ck::Format_UE(TEXT("  [{}] {} ({})\n"),
            Entry.Template->GetClass()->GetName(), NameStr, Entry.Origin);

        if (NOT Entry.AttachParent.IsNone())
        {
            OutText += ck::Format_UE(TEXT("    AttachParent: {}\n"), Entry.AttachParent.ToString());
        }
        if (NOT Entry.AttachSocket.IsNone())
        {
            OutText += ck::Format_UE(TEXT("    AttachSocket: {}\n"), Entry.AttachSocket.ToString());
        }

        if (const auto* SceneComponent = Cast<USceneComponent>(Entry.Template))
        {
            const auto Loc = SceneComponent->GetRelativeLocation();
            const auto Rot = SceneComponent->GetRelativeRotation();
            const auto Scale = SceneComponent->GetRelativeScale3D();
            OutText += ck::Format_UE(TEXT("    Location: ({}, {}, {})\n"), Loc.X, Loc.Y, Loc.Z);
            OutText += ck::Format_UE(TEXT("    Rotation: (P={}, Y={}, R={})\n"), Rot.Pitch, Rot.Yaw, Rot.Roll);
            OutText += ck::Format_UE(TEXT("    Scale:    ({}, {}, {})\n"), Scale.X, Scale.Y, Scale.Z);
        }

        FCk_DataAssetExporter::DoSerializeProperties_Text(
            Entry.Template, UActorComponent::StaticClass(), OutText, 2);
    }

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeImplementedInterfaces_Text(
        const UBlueprint* InBlueprint,
        FString& OutText)
    -> void
{
    const auto* GeneratedClass = InBlueprint->GeneratedClass.Get();
    if (ck::Is_NOT_Valid(GeneratedClass) || GeneratedClass->Interfaces.Num() == 0)
    { return; }

    OutText += ck::Format_UE(TEXT("--- Implemented Interfaces ({}) ---\n"),
        GeneratedClass->Interfaces.Num());

    for (const auto& Implemented : GeneratedClass->Interfaces)
    {
        const auto* InterfaceClass = Implemented.Class.Get();
        if (ck::Is_NOT_Valid(InterfaceClass))
        { continue; }

        OutText += ck::Format_UE(TEXT("  [{}] {}\n"),
            InterfaceClass->GetName(), InterfaceClass->GetPathName());

        for (TFieldIterator<UFunction> It(InterfaceClass); It; ++It)
        {
            const auto* Function = *It;
            if (ck::Is_NOT_Valid(Function) || Function->GetOwnerClass() == UInterface::StaticClass())
            { continue; }

            auto ReturnType = FString{TEXT("void")};
            auto Params = TArray<FString>{};

            for (TFieldIterator<FProperty> ParamIt(Function); ParamIt; ++ParamIt)
            {
                const auto* Property = *ParamIt;
                if (ck::Is_NOT_Valid(Property) || NOT Property->HasAnyPropertyFlags(CPF_Parm))
                { continue; }

                if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
                {
                    ReturnType = Property->GetCPPType();
                    continue;
                }

                Params.Add(ck::Format_UE(TEXT("{} {}"),
                    Property->GetCPPType(), Property->GetName()));
            }

            OutText += ck::Format_UE(TEXT("    {} {}({})\n"),
                ReturnType, Function->GetName(), FString::Join(Params, TEXT(", ")));
        }
    }

    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeWidgetHierarchy_Text(
        const UWidgetBlueprint* InWidgetBlueprint,
        FString& OutText)
    -> void
{
    OutText += TEXT("--- Widget Hierarchy ---\n");

    const auto* WidgetTree = InWidgetBlueprint->WidgetTree.Get();
    if (ck::Is_NOT_Valid(WidgetTree) || ck::Is_NOT_Valid(WidgetTree->RootWidget))
    {
        OutText += TEXT("  (empty)\n\n");
        return;
    }

    DoSerializeWidget_Text(WidgetTree->RootWidget, FString{}, 0, OutText);
    OutText += TEXT("\n");
}

auto
    FCk_BlueprintExporter::
    DoSerializeWidget_Text(
        const UWidget* InWidget,
        const FString& InSlotName,
        int32 InDepth,
        FString& OutText)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    const auto Indent = DoGetIndent(InDepth + 1);
    const auto NamedSlotSuffix = InSlotName.IsEmpty()
        ? FString{}
        : ck::Format_UE(TEXT(" (named slot: {})"), InSlotName);

    OutText += ck::Format_UE(TEXT("{}[{}] {}{}\n"),
        Indent, InWidget->GetClass()->GetName(), InWidget->GetName(), NamedSlotSuffix);

    const auto AttrIndent = DoGetIndent(InDepth + 2);
    OutText += ck::Format_UE(TEXT("{}isVariable={} | visibility={}\n"),
        AttrIndent,
        InWidget->bIsVariable ? TEXT("true") : TEXT("false"),
        StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(InWidget->GetVisibility())));

    if (const auto ToolTip = InWidget->GetToolTipText().ToString(); NOT ToolTip.IsEmpty())
    {
        OutText += ck::Format_UE(TEXT("{}toolTip=\"{}\"\n"), AttrIndent, ToolTip);
    }

    if (const auto* PanelSlot = InWidget->Slot.Get())
    {
        OutText += ck::Format_UE(TEXT("{}slot: {}\n"), AttrIndent, PanelSlot->GetClass()->GetName());
        FCk_DataAssetExporter::DoSerializeProperties_Text(PanelSlot, UPanelSlot::StaticClass(), OutText, InDepth + 3);
    }

    FCk_DataAssetExporter::DoSerializeProperties_Text(InWidget, UWidget::StaticClass(), OutText, InDepth + 2);

    if (InDepth >= ck_blueprint_exporter::MaxWidgetTreeDepth)
    { return; }

    if (const auto* PanelWidget = Cast<UPanelWidget>(InWidget))
    {
        for (auto ChildIndex = int32{0}; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
        {
            if (const auto* Child = PanelWidget->GetChildAt(ChildIndex))
            {
                DoSerializeWidget_Text(Child, FString{}, InDepth + 1, OutText);
            }
        }
    }

    if (const auto* NamedSlotHost = Cast<INamedSlotInterface>(InWidget))
    {
        auto SlotNames = TArray<FName>{};
        NamedSlotHost->GetSlotNames(SlotNames);

        for (const auto& SlotName : SlotNames)
        {
            if (const auto* SlotContent = NamedSlotHost->GetContentForSlot(SlotName))
            {
                DoSerializeWidget_Text(SlotContent, SlotName.ToString(), InDepth + 1, OutText);
            }
        }
    }
}

auto
    FCk_BlueprintExporter::
    DoSerializeAnimations_Text(
        const UWidgetBlueprint* InWidgetBlueprint,
        FString& OutText)
    -> void
{
    OutText += ck::Format_UE(TEXT("--- Animations ({}) ---\n"), InWidgetBlueprint->Animations.Num());

    for (const auto& Animation : InWidgetBlueprint->Animations)
    {
        if (ck::Is_NOT_Valid(Animation))
        { continue; }

        OutText += ck::Format_UE(TEXT("  {} [{:.3f}s -> {:.3f}s]\n"),
            Animation->GetName(),
            Animation->GetStartTime(),
            Animation->GetEndTime());

        for (const auto& Binding : Animation->GetBindings())
        {
            OutText += ck::Format_UE(TEXT("    -> {}\n"), Binding.WidgetName.ToString());
        }
    }

    OutText += TEXT("\n");
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_BlueprintExporter::
    DoResolveOutputPath(
        const UBlueprint* InBlueprint,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InBlueprint->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;

    return DiskPath;
}

auto
    FCk_BlueprintExporter::
    DoGetNodeId(
        const UEdGraphNode* InNode)
    -> FString
{
    if (InNode == nullptr)
    { return TEXT("None"); }

    return ck::Format_UE(TEXT("Node_{}"), InNode->NodeGuid.ToString(EGuidFormats::Short));
}

// Returns true if the string only contains printable ASCII characters
static auto
    DoIsCleanAscii(
        const FString& InStr)
    -> bool
{
    if (InStr.IsEmpty())
    { return false; }

    return ck::algo::AllOf(InStr, [](TCHAR Ch) { return Ch >= 32 && Ch <= 126; });
}

// Attempts to get a clean name from a UObject, returns empty string if garbled
static auto
    DoGetCleanObjectName(
        const TWeakObjectPtr<UObject>& InObj)
    -> FString
{
    if (ck::Is_NOT_Valid(InObj))
    { return FString{}; }

    const auto Name = InObj->GetName();
    if (DoIsCleanAscii(Name))
    { return Name; }

    // Try the class name of the object instead
    if (ck::IsValid(InObj->GetClass()))
    {
        const auto ClassName = InObj->GetClass()->GetName();
        if (DoIsCleanAscii(ClassName))
        { return ClassName; }
    }

    return FString{};
}

auto
    FCk_BlueprintExporter::
    DoGetPinTypeAsString(
        const FEdGraphPinType& InPinType)
    -> FString
{
    const auto Cat = InPinType.PinCategory;
    auto BaseName = FString{};

    // Map known K2 categories to readable strings
    if (Cat == UEdGraphSchema_K2::PC_Exec)
    {
        BaseName = TEXT("exec");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Boolean)
    {
        BaseName = TEXT("Boolean");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Byte)
    {
        BaseName = TEXT("Byte");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Int)
    {
        BaseName = TEXT("Integer");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Int64)
    {
        BaseName = TEXT("Integer64");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Real)
    {
        const auto Sub = InPinType.PinSubCategory.ToString();
        BaseName = (DoIsCleanAscii(Sub)) ? Sub : TEXT("Float");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Name)
    {
        BaseName = TEXT("Name");
    }
    else if (Cat == UEdGraphSchema_K2::PC_String)
    {
        BaseName = TEXT("String");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Text)
    {
        BaseName = TEXT("Text");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Delegate)
    {
        BaseName = TEXT("Delegate");
    }
    else if (Cat == UEdGraphSchema_K2::PC_MCDelegate)
    {
        BaseName = TEXT("MC Delegate");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Wildcard)
    {
        BaseName = TEXT("Wildcard");
    }
    else if (Cat == UEdGraphSchema_K2::PC_Object || Cat == UEdGraphSchema_K2::PC_SoftObject)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Object") : ObjName;
    }
    else if (Cat == UEdGraphSchema_K2::PC_Class || Cat == UEdGraphSchema_K2::PC_SoftClass)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Class") : ck::Format_UE(TEXT("Class<{}>"), ObjName);
    }
    else if (Cat == UEdGraphSchema_K2::PC_Struct)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Struct") : ObjName;
    }
    else if (Cat == UEdGraphSchema_K2::PC_Enum)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Enum") : ObjName;
    }
    else if (Cat == UEdGraphSchema_K2::PC_Interface)
    {
        const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
        BaseName = ObjName.IsEmpty() ? TEXT("Interface") : ObjName;
    }
    else
    {
        // Unknown category — try to extract something readable
        const auto CatStr = Cat.ToString();
        if (DoIsCleanAscii(CatStr))
        {
            BaseName = CatStr;
        }
        else
        {
            // Last resort: try SubCategoryObject name
            const auto ObjName = DoGetCleanObjectName(InPinType.PinSubCategoryObject);
            BaseName = ObjName.IsEmpty() ? TEXT("Pin") : ObjName;
        }
    }

    // Container types
    if (InPinType.ContainerType == EPinContainerType::Array)
    {
        return ck::Format_UE(TEXT("TArray<{}>"), BaseName);
    }
    if (InPinType.ContainerType == EPinContainerType::Set)
    {
        return ck::Format_UE(TEXT("TSet<{}>"), BaseName);
    }
    if (InPinType.ContainerType == EPinContainerType::Map)
    {
        const auto ValName = DoGetCleanObjectName(InPinType.PinValueType.TerminalSubCategoryObject);
        const auto ValCat = InPinType.PinValueType.TerminalCategory.ToString();
        const auto ValueType = NOT ValName.IsEmpty() ? ValName
            : (DoIsCleanAscii(ValCat) ? ValCat : FString{TEXT("?")});
        return ck::Format_UE(TEXT("TMap<{}, {}>"), BaseName, ValueType);
    }

    return BaseName;
}

auto
    FCk_BlueprintExporter::
    DoGetNodeDisplayName(
        const UEdGraphNode* InNode)
    -> FString
{
    if (InNode == nullptr)
    { return TEXT("None"); }

    const auto Title = InNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    if (NOT Title.IsEmpty())
    {
        return Title;
    }

    return InNode->GetClass()->GetName();
}

auto
    FCk_BlueprintExporter::
    DoGetIndent(
        int32 InDepth)
    -> FString
{
    auto Indent = FString{};
    for (auto i = int32{0}; i < InDepth; ++i)
    {
        Indent += TEXT("  ");
    }
    return Indent;
}

auto
    FCk_BlueprintExporter::
    DoIsExecPin(
        const UEdGraphPin* InPin)
    -> bool
{
    if (InPin == nullptr)
    { return false; }

    return InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
}

// --------------------------------------------------------------------------------------------------------------------
