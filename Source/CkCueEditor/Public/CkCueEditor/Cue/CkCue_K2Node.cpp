#include "CkCue_K2Node.h"

#include "GraphEditorSettings.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEntityScript_Subsystem.h"

#include "CkCueEditor/CkCueEditor_Log.h"
#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include <K2Node_CallFunction.h>
#include <K2Node_MakeStruct.h>
#include <Kismet/BlueprintInstancedStructLibrary.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Subsystems/SubsystemBlueprintLibrary.h>

#define LOCTEXT_NAMESPACE "UCk_K2Node_Cue"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_k2node_cue
{
    static auto PinName_OwnerEntity = TEXT("InOwnerEntity");
    static auto PinName_CueName = TEXT("InCueName");
    static auto PinName_ExecutionType = TEXT("ExecutionType");
    static auto PinName_ReturnValue = TEXT("EntityUnderConstruction");
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_K2Node_Cue_Base::ShouldShowNodeProperties() const -> bool
{
    return true;
}

auto UCk_K2Node_Cue_Base::IsNodePure() const -> bool
{
    return false;
}

auto UCk_K2Node_Cue_Base::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void
{
    // Restore cached values from old pins
    for (const auto Pin : InOldPins)
    {
        if (Pin->PinName == ck_k2node_cue::PinName_ExecutionType)
        {
            const auto EnumValue = UCk_Utils_Enum_UE::Get_EnumFromString<ECk_Cue_ExecutionPolicy>(Pin->DefaultValue);
            CK_ENSURE_IF_NOT(ck::IsValid(EnumValue),
                TEXT("Failed to get Enum value from string [{}]. Some Cue nodes in graph [{}] might be faulty."),
                Pin->DefaultValue, this->GetGraph())
            { break; }

            _ExecutionType = *EnumValue;
            break;
        }
    }

    AllocateDefaultPins();

    // Get cue class from old pins
    if (auto* CueClass = DoGet_CueClass(InOldPins))
    {
        DoCreatePinsFromCue(CueClass);
    }

    RestoreSplitPins(InOldPins);
}

auto UCk_K2Node_Cue_Base::GetMenuCategory() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Cue"), TEXT("Ck|Cue"));
}

auto UCk_K2Node_Cue_Base::IsCompatibleWithGraph(UEdGraph const* InGraph) const -> bool
{
    return Super::IsCompatibleWithGraph(InGraph);
}

auto UCk_K2Node_Cue_Base::PinConnectionListChanged(UEdGraphPin* InPin) -> void
{
    Super::PinConnectionListChanged(InPin);

    if (ck::Is_NOT_Valid(InPin, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    if (InPin->PinName == ck_k2node_cue::PinName_CueName)
    {
        DoOnCueNamePinChanged();
    }
}

auto UCk_K2Node_Cue_Base::GetPinMetaData(FName InPinName, FName InKey) -> FString
{
    if (InPinName == ck_k2node_cue::PinName_CueName && InKey == TEXT("Categories"))
    {
        return Get_CueTagCategory();
    }

    if (const TMap<FName, FString>* Metadata = _PinMetadataMap.Find(InPinName))
    {
        if (const FString* Value = Metadata->Find(InKey))
        {
            return *Value;
        }
    }

    return Super::GetPinMetaData(InPinName, InKey);
}

auto UCk_K2Node_Cue_Base::GetJumpTargetForDoubleClick() const -> UObject*
{
    const auto& CueClass = DoGet_CueClass();

    if (ck::Is_NOT_Valid(CueClass))
    { return Super::GetJumpTargetForDoubleClick(); }

    return UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(CueClass);
}

auto UCk_K2Node_Cue_Base::CreateVisualWidget() -> TSharedPtr<SGraphNode>
{
    return SNew(SCk_GraphNode_Cue_Base, this);
}

auto UCk_K2Node_Cue_Base::GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText
{
    const auto& CueName = DoGet_CueName();

    // Handle empty/invalid cue name
    if (ck::Is_NOT_Valid(CueName))
    {
        return DoGet_DisplayNodeTitle();
    }

    if (const auto& CueClass = DoGet_CueClass();
        ck::Is_NOT_Valid(CueClass))
    {
        return FText::FromString(ck::Format_UE(
            TEXT("{}\n{}\n❌ NOT FOUND"),
            DoGet_DisplayNodeTitle().ToString(),
            CueName));
    }

    return FText::FromString(ck::Format_UE(
        TEXT("{}\n{}"),
        DoGet_DisplayNodeTitle().ToString(),
        CueName));
}

auto UCk_K2Node_Cue_Base::GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

auto UCk_K2Node_Cue_Base::DoAllocate_DefaultPins() -> void
{
    using namespace ck_k2node_cue;

    // Owner entity input
    CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle::StaticStruct(),
        PinName_OwnerEntity);

    // Cue name input (GameplayTag with category filter)
    auto* CueNamePin = CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Struct,
        FGameplayTag::StaticStruct(),
        PinName_CueName);
    CueNamePin->PinToolTip = TEXT("The gameplay tag identifying which cue to execute");

    // Execution type enum
    auto* ExecutionTypePin = CreatePin(
        EGPD_Input,
        UEdGraphSchema_K2::PC_Byte,
        StaticEnum<ECk_Cue_ExecutionPolicy>(),
        PinName_ExecutionType);
    ExecutionTypePin->DefaultValue = ck::Format_UE(TEXT("{}"), _ExecutionType);

    // Return value
    auto* ReturnValuePin = CreatePin(
        EGPD_Output,
        UEdGraphSchema_K2::PC_Struct,
        FCk_Handle_PendingEntityScript::StaticStruct(),
        PinName_ReturnValue);
    ReturnValuePin->PinToolTip = TEXT("The handle to the newly spawned cue entity (not yet constructed)");

    // Generate cue-specific pins
    DoCreatePinsFromCue(DoGet_CueClass());
}

auto UCk_K2Node_Cue_Base::DoExpandNode(
    FKismetCompilerContext& InCompilerContext,
    UEdGraph* InSourceGraph,
    ECk_ValidInvalid InNodeValidity)
    -> void
{
    const auto& CueName = DoGet_CueName();

    // Handle empty/invalid cue name
    if (ck::Is_NOT_Valid(CueName))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Missing Cue Name", "No cue name specified. @@").ToString(), this);
        return;
    }

    const auto& CueClass = DoGet_CueClass();

    if (ck::Is_NOT_Valid(CueClass))
    {
        // Get subsystem info for better error message
        const auto& ExecutorClass = Get_CueExecutorSubsystemClass();
        auto SubsystemInfo = FString{TEXT("unknown subsystem")};

        if (ck::IsValid(ExecutorClass))
        {
            if (const auto* ExecutorCDO = ExecutorClass->GetDefaultObject<UCk_CueExecutor_Subsystem_Base_UE>();
                ck::IsValid(ExecutorCDO))
            {
                if (const auto& CueSubsystemClass = ExecutorCDO->Get_CueSubsystemClass();
                    ck::IsValid(CueSubsystemClass))
                {
                    SubsystemInfo = CueSubsystemClass->GetName();
                }
            }
        }

        InCompilerContext.MessageLog.Error(
            *FText::Format(
                LOCTEXT("Cue Not Found", "Cue with tag '{0}' not found in {1}. @@"),
                FText::FromString(CueName.ToString()),
                FText::FromString(SubsystemInfo)
            ).ToString(),
            this);
        return;
    }

    auto* CueSpawnParamsStruct = DoGet_CueSpawnParamsStruct(CueClass, InCompilerContext);

    if (ck::Is_NOT_Valid(CueSpawnParamsStruct))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Missing Cue Spawn Params", "Invalid Cue Spawn Params struct @@").ToString(), this);
        return;
    }

    _ExecutionType = DoGet_ExecutionType();

    // Create MakeStruct node for spawn params
    auto* MakeSpawnParamsStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, InSourceGraph);
    MakeSpawnParamsStruct_Node->StructType = CueSpawnParamsStruct;
    MakeSpawnParamsStruct_Node->bMadeAfterOverridePinRemoval = true;
    MakeSpawnParamsStruct_Node->AllocateDefaultPins();

    // Create MakeInstancedStruct node
    auto* MakeInstancedStruct_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    MakeInstancedStruct_Node->SetFromFunction(
        UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStruct_Node->AllocateDefaultPins();

    // Copy pin values from this node to MakeStruct node
    const auto& TryCopyValueOrLinkPin = [&](UEdGraphPin* InPinToCopyOrLinkFrom)
    {
        auto* FoundMakeStructPin = MakeSpawnParamsStruct_Node->FindPinByPredicate([&](const UEdGraphPin* InPin)
        {
            return InPin->PinFriendlyName.ToString() == InPinToCopyOrLinkFrom->PinName.ToString();
        });

        if (ck::Is_NOT_Valid(FoundMakeStructPin, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        if (NOT InPinToCopyOrLinkFrom->DefaultValue.IsEmpty())
        {
            FoundMakeStructPin->DefaultValue = InPinToCopyOrLinkFrom->DefaultValue;
        }

        if (ck::IsValid(InPinToCopyOrLinkFrom->DefaultObject))
        {
            FoundMakeStructPin->DefaultObject = InPinToCopyOrLinkFrom->DefaultObject;
        }

        if (NOT InPinToCopyOrLinkFrom->DefaultTextValue.IsEmpty())
        {
            FoundMakeStructPin->DefaultTextValue = InPinToCopyOrLinkFrom->DefaultTextValue;
        }

        if (NOT InPinToCopyOrLinkFrom->LinkedTo.IsEmpty())
        {
            for (auto* LinkedPin : InPinToCopyOrLinkFrom->LinkedTo)
            {
                InCompilerContext.GetSchema()->TryCreateConnection(LinkedPin, FoundMakeStructPin);
            }
        }
    };

    for (auto* Pin : this->Pins)
    {
        if (Pin->Direction == EGPD_Input &&
            Pin != this->GetExecPin() &&
            Pin->PinName != ck_k2node_cue::PinName_CueName &&
            Pin->PinName != ck_k2node_cue::PinName_ExecutionType &&
            Pin->PinName != ck_k2node_cue::PinName_OwnerEntity)
        {
            TryCopyValueOrLinkPin(Pin);
        }
    }

    // Create GetSubsystem node to get the cue executor
    auto* GetSubsystem_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);
    GetSubsystem_Node->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(USubsystemBlueprintLibrary, GetWorldSubsystem),
        USubsystemBlueprintLibrary::StaticClass());
    GetSubsystem_Node->AllocateDefaultPins();

    // Set the subsystem class
    const auto& ExecutorClass = Get_CueExecutorSubsystemClass();
    auto* SubsystemClassPin = GetSubsystem_Node->FindPinChecked(TEXT("Class"));
    SubsystemClassPin->DefaultObject = ExecutorClass;

    auto* GetSubsystemResultPin = GetSubsystem_Node->GetReturnValuePin();
    GetSubsystemResultPin->PinType.PinSubCategoryObject = ExecutorClass;

    // Create the Execute Cue function node
    auto* ExecuteCue_Node = InCompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, InSourceGraph);

    const auto& ExecutionType = DoGet_ExecutionType();
    const auto& FunctionName = ExecutionType == ECk_Cue_ExecutionPolicy::Replicated
        ? GET_FUNCTION_NAME_CHECKED(UCk_CueExecutor_Subsystem_Base_UE, Request_ExecuteCue)
        : GET_FUNCTION_NAME_CHECKED(UCk_CueExecutor_Subsystem_Base_UE, Request_ExecuteCue_Local);

    ExecuteCue_Node->FunctionReference.SetExternalMember(
        FunctionName,
        UCk_CueExecutor_Subsystem_Base_UE::StaticClass());

    ExecuteCue_Node->AllocateDefaultPins();
    InCompilerContext.MessageLog.NotifyIntermediateObjectCreation(ExecuteCue_Node, this);

    // Connect everything together
    if (UCk_Utils_EditorGraph_UE::Request_TryCreateConnection(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(CueSpawnParamsStruct->GetFName(), ECk_EditorGraph_PinDirection::Output, *MakeSpawnParamsStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("Value"), ECk_EditorGraph_PinDirection::Input, *MakeInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*ExecuteCue_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*MakeInstancedStruct_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin(TEXT("InSpawnParams"), ECk_EditorGraph_PinDirection::Input, *ExecuteCue_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*GetSubsystem_Node),
                UCk_Utils_EditorGraph_UE::Get_Pin_Self(*ExecuteCue_Node),
            }
        }
    ) == ECk_SucceededFailed::Failed) { return; }

    // Set the cue name as a literal
    if (auto* CueNameParamPin = ExecuteCue_Node->FindPin(ck_k2node_cue::PinName_CueName);
        ck::IsValid(CueNameParamPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        CueNameParamPin->DefaultValue = CueName.ToString();
    }

    // Link the owner entity pin
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_cue::PinName_OwnerEntity, ECk_EditorGraph_PinDirection::Input, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_cue::PinName_OwnerEntity, ECk_EditorGraph_PinDirection::Input, *ExecuteCue_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    // Link exec and return value
    if (UCk_Utils_EditorGraph_UE::Request_LinkPins(
        InCompilerContext,
        {
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Exec(*MakeInstancedStruct_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Then(*ExecuteCue_Node)
            },
            {
                UCk_Utils_EditorGraph_UE::Get_Pin(ck_k2node_cue::PinName_ReturnValue, ECk_EditorGraph_PinDirection::Output, *this),
                UCk_Utils_EditorGraph_UE::Get_Pin_Result(*ExecuteCue_Node)
            }
        },
        ECk_EditorGraph_PinLinkType::Move
    ) == ECk_SucceededFailed::Failed) { return; }

    BreakAllNodeLinks();
}

auto UCk_K2Node_Cue_Base::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Cue"), TEXT("[Ck] Execute Cue"));
}

auto UCk_K2Node_Cue_Base::DoPinDefaultValueChanged(UEdGraphPin* InPin) -> void
{
    Super::DoPinDefaultValueChanged(InPin);

    if (ck::Is_NOT_Valid(InPin, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    if (InPin->PinName == ck_k2node_cue::PinName_ExecutionType)
    {
        if (const auto NewExecutionType = DoGet_ExecutionType();
            _ExecutionType != NewExecutionType)
        {
            _ExecutionType = NewExecutionType;
        }
        return;
    }

    if (InPin->PinName == ck_k2node_cue::PinName_CueName)
    {
        DoOnCueNamePinChanged();
        return;
    }
}

auto UCk_K2Node_Cue_Base::DoCreatePinsFromCue(UClass* InCueClass) -> void
{
    if (ck::Is_NOT_Valid(InCueClass))
    { return; }

    _PinsGeneratedFromCue.Reset();
    _PinMetadataMap.Reset();
    AdvancedPinDisplay = ENodeAdvancedPins::Type::NoPins;

    const auto& CreatePinFromProperty = [this](const FProperty* InProperty, UObject* InContainer)
    {
        auto* Pin = CreatePin(EGPD_Input, NAME_None, InProperty->GetFName());
        _PinsGeneratedFromCue.Add(Pin);

        if (ck::Is_NOT_Valid(Pin, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        Pin->PinFriendlyName = InProperty->GetDisplayNameText();

        const auto& ShowInAdvancedDisplay = InProperty->HasAllPropertyFlags(CPF_AdvancedDisplay);
        Pin->bAdvancedView = ShowInAdvancedDisplay;
        if (ShowInAdvancedDisplay && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
        {
            AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
        }

        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

        K2Schema->ConvertPropertyToPinType(InProperty, Pin->PinType);

        if (K2Schema->PinDefaultValueIsEditable(*Pin))
        {
            auto DefaultValueAsString = FString{};
            const auto& DefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(
                InProperty,
                reinterpret_cast<const uint8*>(InContainer),
                DefaultValueAsString,
                this);

            check(DefaultValueSet);

            K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
        }

        K2Schema->ConstructBasicPinTooltip(*Pin, InProperty->GetToolTipText(), Pin->PinToolTip);

        if (const auto* MetaDataMap = InProperty->GetMetaDataMap())
        {
            for (const auto& MetaDataKvp : *MetaDataMap)
            {
                _PinMetadataMap.FindOrAdd(Pin->PinName).Add(MetaDataKvp.Key, MetaDataKvp.Value);
            }
        }
    };

    auto* CueCDO = InCueClass->GetDefaultObject();

    for (const auto* ExposedProperty : UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(InCueClass))
    {
        CreatePinFromProperty(ExposedProperty, CueCDO);
    }
}

auto UCk_K2Node_Cue_Base::DoOnCueNamePinChanged() -> void
{
    TArray<UEdGraphPin*> OldPins = Pins;
    TArray<UEdGraphPin*> OldCuePins;

    for (auto* OldPin : OldPins)
    {
        if (NOT _PinsGeneratedFromCue.Contains(OldPin))
        { continue; }

        Pins.Remove(OldPin);
        OldCuePins.Add(OldPin);
    }

    if (auto* CueClass = DoGet_CueClass(Pins);
        ck::IsValid(CueClass))
    {
        DoCreatePinsFromCue(CueClass);
    }

    RestoreSplitPins(OldPins);
    RewireOldPinsToNewPins(OldCuePins, Pins, nullptr);
    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

auto UCk_K2Node_Cue_Base::DoGet_CueClass(TOptional<TArray<UEdGraphPin*>> InPinsToSearch) const -> UClass*
{
    const auto& CueName = DoGet_CueName(InPinsToSearch);

    if (ck::Is_NOT_Valid(CueName) || NOT CueName.IsValid())
    { return {}; }

    auto* CueSubsystem = DoGet_CueSubsystem();

    if (ck::Is_NOT_Valid(CueSubsystem))
    { return {}; }

    return CueSubsystem->Get_CueEntityScript(CueName);
}

auto UCk_K2Node_Cue_Base::DoGet_CueName(TOptional<TArray<UEdGraphPin*>> InPinsToSearch) const -> FGameplayTag
{
    if (ck::Is_NOT_Valid(InPinsToSearch))
    {
        InPinsToSearch = Pins;
    }

    const auto& CueNamePin = UCk_Utils_EditorGraph_UE::Get_Pin(
        ck_k2node_cue::PinName_CueName,
        ECk_EditorGraph_PinDirection::Input,
        *InPinsToSearch);

    if (ck::Is_NOT_Valid(CueNamePin))
    { return FGameplayTag{}; }

    // Try to parse the gameplay tag from the pin's default value
    auto Result = FGameplayTag{};
    if (NOT (*CueNamePin)->DefaultValue.IsEmpty())
    {
        FGameplayTag::StaticStruct()->ImportText(*(*CueNamePin)->DefaultValue, &Result, nullptr, 0, nullptr, FGameplayTag::StaticStruct()->GetName());
    }

    return Result;
}

auto UCk_K2Node_Cue_Base::DoGet_ExecutionType() const -> ECk_Cue_ExecutionPolicy
{
    return *UCk_Utils_EditorGraph_UE::Get_Pin_EnumValue<ECk_Cue_ExecutionPolicy>(
        ck_k2node_cue::PinName_ExecutionType,
        ECk_EditorGraph_PinDirection::Input,
        *this);
}

auto UCk_K2Node_Cue_Base::DoGet_CueSubsystem() const -> UCk_CueSubsystem_Base_UE*
{
    const auto& ExecutorClass = Get_CueExecutorSubsystemClass();

    if (ck::Is_NOT_Valid(ExecutorClass))
    { return {}; }

    auto* ExecutorCDO = ExecutorClass->GetDefaultObject<UCk_CueExecutor_Subsystem_Base_UE>();

    if (ck::Is_NOT_Valid(ExecutorCDO))
    { return {}; }

    const auto& CueSubsystemClass = ExecutorCDO->Get_CueSubsystemClass();

    if (ck::Is_NOT_Valid(CueSubsystemClass))
    { return {}; }

    return Cast<UCk_CueSubsystem_Base_UE>(GEngine->GetEngineSubsystemBase(CueSubsystemClass));
}

auto UCk_K2Node_Cue_Base::DoGet_CueSpawnParamsStruct(
    UClass* InCueClass,
    FKismetCompilerContext& InCompilerContext)
    -> UScriptStruct*
{
    if (ck::Is_NOT_Valid(InCueClass))
    { return {}; }

    auto* EntityScriptSubsystem = GEngine->GetEngineSubsystem<UCk_EntityScript_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EntityScriptSubsystem))
    {
        InCompilerContext.MessageLog.Error(TEXT("Failed to get Entity Script Subsystem"));
        return {};
    }

    auto* SpawnParamsStruct = EntityScriptSubsystem->GetOrCreate_SpawnParamsStructForEntity(InCueClass);
    if (ck::Is_NOT_Valid(SpawnParamsStruct))
    {
        InCompilerContext.MessageLog.Error(TEXT("Failed to find valid Spawn Params struct for Cue [{}]"), InCueClass);
        return {};
    }

    return SpawnParamsStruct;
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_K2Node_GenericCue::Get_CueExecutorSubsystemClass() const -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE>
{
    return UCk_GenericCueExecutor_Subsystem_UE::StaticClass();
}

auto UCk_K2Node_GenericCue:: Get_CueTagCategory() const -> FString
{
    return TEXT("Cue");
}

auto UCk_K2Node_GenericCue::DoGet_Menu_NodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_GenericCue"), TEXT("[Ck] Execute Generic Cue ⚡"));
}

auto UCk_K2Node_GenericCue::DoGet_DisplayNodeTitle() const -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_GenericCue"), TEXT("[Ck] Execute Generic Cue ⚡"));
}

// --------------------------------------------------------------------------------------------------------------------

auto SCk_GraphNode_Cue_Base::Construct(const FArguments& InArgs, UCk_K2Node_Cue_Base* InNode) -> void
{
    GraphNode = InNode;
    _CueNode = InNode;
    UpdateGraphNode();
}

auto SCk_GraphNode_Cue_Base::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void
{
    SGraphNode::CreateBelowPinControls(MainBox);

    if (ck::Is_NOT_Valid(_CueNode.Get()))
    { return; }

    if (const auto& CueClass = _CueNode->DoGet_CueClass();
        ck::Is_NOT_Valid(CueClass))
    { return; }

    MainBox->AddSlot()
    .AutoHeight()
    .Padding(4.0f, 2.0f)
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        .Padding(6.0f, 4.0f)
        [
            SNew(SHorizontalBox)

            // Execution Type indicator
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(2.0f, 0.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                            return FText::FromString(ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ?
                                TEXT("🌐") : TEXT("🏠"));
                        }
                        return FText::FromString(TEXT("?"));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .ColorAndOpacity_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                            return ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ?
                                FLinearColor::Green : FLinearColor(1.0f, 0.8f, 0.2f);
                        }
                        return FLinearColor::White;
                    })
                    .ToolTipText_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                            return FText::FromString(ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ?
                                TEXT("Replicated: Synchronizes across network") :
                                TEXT("Local: Runs locally only"));
                        }
                        return FText::GetEmpty();
                    })
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                            return FText::FromString(ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ?
                                TEXT("Replicated") : TEXT("Local"));
                        }
                        return FText::GetEmpty();
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                ]
            ]
        ]
    ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE