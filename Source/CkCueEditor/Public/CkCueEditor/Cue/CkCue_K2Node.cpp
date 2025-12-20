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
    static auto PinName_ReturnValue = TEXT("EntityUnderConstruction");
}

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

auto UCk_K2Node_Cue_Base::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
                                ? PropertyChangedEvent.Property->GetFName()
                                : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Cue_Base, _ExecutionType) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Cue_Base, _EntityMode) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Cue_Base, _ReliabilityPolicy) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCk_K2Node_Cue_Base, _MulticastPolicy))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

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
    AllocateDefaultPins();

    auto* CueClass = _CachedCueClass.Get();
    if (ck::Is_NOT_Valid(CueClass))
    {
        CueClass = DoGet_CueClass(InOldPins);
    }

    // Get cue class from old pins
    if (ck::IsValid(CueClass))
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

    // Update owner entity pin visibility based on entity mode
    if (auto* OwnerEntityPin = FindPinChecked(PinName_OwnerEntity);
        ck::IsValid(OwnerEntityPin, ck::IsValid_Policy_NullptrOnly{}))
    {
        OwnerEntityPin->bHidden = (_EntityMode == ECk_Cue_EntityMode::Transient);
    }

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

    auto* CueSpawnParamsStruct = _CachedSpawnParamsStruct.Get();
    if (ck::Is_NOT_Valid(CueSpawnParamsStruct))
    {
        // Fallback for editor (shouldn't hit this during cook)
        CueSpawnParamsStruct = DoGet_CueSpawnParamsStruct(CueClass, InCompilerContext);
    }

    if (ck::Is_NOT_Valid(CueSpawnParamsStruct))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Missing Cue Spawn Params", "Invalid Cue Spawn Params struct @@").ToString(), this);
        return;
    }

    if (ck::Is_NOT_Valid(CueSpawnParamsStruct))
    {
        InCompilerContext.MessageLog.Error(*LOCTEXT("Missing Cue Spawn Params", "Invalid Cue Spawn Params struct @@").ToString(), this);
        return;
    }

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
    const auto& EntityMode = DoGet_EntityMode();

    // Select the appropriate function based on ExecutionType and EntityMode
    FName FunctionName;
    const auto IsReplicatedMode = (ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ||
                                    ExecutionType == ECk_Cue_ExecutionPolicy::ReplicatedAndLocal);

    if (EntityMode == ECk_Cue_EntityMode::Transient)
    {
        FunctionName = IsReplicatedMode
            ? GET_FUNCTION_NAME_CHECKED(UCk_CueExecutor_Subsystem_Base_UE, Request_ExecuteCue_Transient)
            : GET_FUNCTION_NAME_CHECKED(UCk_CueExecutor_Subsystem_Base_UE, Request_ExecuteCue_Transient_Local);
    }
    else // EntityMode::Owner
    {
        FunctionName = IsReplicatedMode
            ? GET_FUNCTION_NAME_CHECKED(UCk_CueExecutor_Subsystem_Base_UE, Request_ExecuteCue)
            : GET_FUNCTION_NAME_CHECKED(UCk_CueExecutor_Subsystem_Base_UE, Request_ExecuteCue_Local);
    }

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

    // Set the reliability policy as a literal (only for replicated execution types)
    if (IsReplicatedMode)
    {
        const auto& ReliabilityPolicy = DoGet_ReliabilityPolicy();
        if (auto* ReliabilityParamPin = ExecuteCue_Node->FindPin(TEXT("InReliability"));
            ck::IsValid(ReliabilityParamPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            const auto& EnumPath = FString::Printf(TEXT("%s::%s"),
                *StaticEnum<ECk_Cue_ReliabilityPolicy>()->GetName(),
                *StaticEnum<ECk_Cue_ReliabilityPolicy>()->GetNameStringByValue(static_cast<int64>(ReliabilityPolicy)));
            ReliabilityParamPin->DefaultValue = EnumPath;
        }

        // Set the multicast policy as a literal (for all replicated modes)
        const auto& MulticastPolicy = DoGet_MulticastPolicy();
        if (auto* MulticastParamPin = ExecuteCue_Node->FindPin(TEXT("InMulticastPolicy"));
            ck::IsValid(MulticastParamPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            const auto& EnumPath = FString::Printf(TEXT("%s::%s"),
                *StaticEnum<ECk_Cue_MulticastPolicy>()->GetName(),
                *StaticEnum<ECk_Cue_MulticastPolicy>()->GetNameStringByValue(static_cast<int64>(MulticastPolicy)));
            MulticastParamPin->DefaultValue = EnumPath;
        }

        // Set the execution policy as a literal
        if (auto* ExecutionPolicyParamPin = ExecuteCue_Node->FindPin(TEXT("InExecutionPolicy"));
            ck::IsValid(ExecutionPolicyParamPin, ck::IsValid_Policy_NullptrOnly{}))
        {
            const auto& EnumPath = FString::Printf(TEXT("%s::%s"),
                *StaticEnum<ECk_Cue_ExecutionPolicy>()->GetName(),
                *StaticEnum<ECk_Cue_ExecutionPolicy>()->GetNameStringByValue(static_cast<int64>(ExecutionType)));
            ExecutionPolicyParamPin->DefaultValue = EnumPath;
        }
    }

    // Link the owner entity pin only if EntityMode is Owner
    if (EntityMode == ECk_Cue_EntityMode::Owner)
    {
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
    }

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

    // Update the cached cue class based on the new cue name
    DoUpdateCachedCueClass();

    // Use the newly cached class
    if (auto* CueClass = _CachedCueClass.Get();
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
    // First, check if we have a valid cached class that matches the current cue name
    const auto& CueName = DoGet_CueName(InPinsToSearch);

    if (ck::Is_NOT_Valid(CueName) || NOT CueName.IsValid())
    {
        return nullptr;
    }

    // If we have a cached class, verify it matches the current cue name
    if (ck::IsValid(_CachedCueClass))
    {
        //if (const auto* CueCDO = Cast<UCk_CueBase_EntityScript>(_CachedCueClass->GetDefaultObject());
        //    ck::IsValid(CueCDO))
        //{
        //    if (CueCDO->Get_CueName() == CueName)
        //    {
        //        // Cached class is valid and matches - use it without querying subsystem
                return _CachedCueClass;
    //        }
    //    }
    }

    // No valid cache - query the subsystem (only during editor operations, not during cook)
#if WITH_EDITOR
    auto* CueSubsystem = DoGet_CueSubsystem();
    if (ck::Is_NOT_Valid(CueSubsystem))
    {
        return nullptr;
    }

    const auto ResolvedClass = CueSubsystem->Get_CueEntityScript(CueName);

    // Update cache with newly resolved class
    if (ck::IsValid(ResolvedClass))
    {
        const_cast<UCk_K2Node_Cue_Base*>(this)->_CachedCueClass = ResolvedClass;
    }

    return ResolvedClass;
#else
    // During cook/packaged builds, we should always have a cached class
    // If we don't, something went wrong during the editor save
    return _CachedCueClass;
#endif
}

auto UCk_K2Node_Cue_Base::DoUpdateCachedCueClass() -> void
{
    const auto& CueName = DoGet_CueName();

    if (ck::Is_NOT_Valid(CueName) || NOT CueName.IsValid())
    {
        _CachedCueClass = nullptr;
        _CachedSpawnParamsStruct = nullptr;
        return;
    }

#if WITH_EDITOR
    auto* CueSubsystem = DoGet_CueSubsystem();
    if (ck::Is_NOT_Valid(CueSubsystem))
    {
        _CachedCueClass = nullptr;
        _CachedSpawnParamsStruct = nullptr;
        return;
    }

    _CachedCueClass = CueSubsystem->Get_CueEntityScript(CueName);

    // Cache spawn params struct for cook-time access
    if (ck::IsValid(_CachedCueClass))
    {
        if (auto* EntityScriptSubsystem = GEngine->GetEngineSubsystem<UCk_EntityScript_Subsystem_UE>();
            ck::IsValid(EntityScriptSubsystem))
        {
            _CachedSpawnParamsStruct = EntityScriptSubsystem->GetOrCreate_SpawnParamsStructForEntity(_CachedCueClass);
        }
    }
#endif
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
    return _ExecutionType;
}

auto UCk_K2Node_Cue_Base::DoGet_EntityMode() const -> ECk_Cue_EntityMode
{
    return _EntityMode;
}

auto UCk_K2Node_Cue_Base::DoGet_ReliabilityPolicy() const -> ECk_Cue_ReliabilityPolicy
{
    return _ReliabilityPolicy;
}

auto UCk_K2Node_Cue_Base::DoGet_MulticastPolicy() const -> ECk_Cue_MulticastPolicy
{
    return _MulticastPolicy;
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
                            switch (ExecutionType)
                            {
                                case ECk_Cue_ExecutionPolicy::Replicated:
                                    return FText::FromString(TEXT("🌐"));
                                case ECk_Cue_ExecutionPolicy::ReplicatedAndLocal:
                                    return FText::FromString(TEXT("🌐🏠"));
                                case ECk_Cue_ExecutionPolicy::Local:
                                    return FText::FromString(TEXT("🏠"));
                                default:
                                    return FText::FromString(TEXT("?"));
                            }
                        }
                        return FText::FromString(TEXT("?"));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .ColorAndOpacity_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                            switch (ExecutionType)
                            {
                                case ECk_Cue_ExecutionPolicy::Replicated:
                                    return FLinearColor::Green;
                                case ECk_Cue_ExecutionPolicy::ReplicatedAndLocal:
                                    return FLinearColor(0.2f, 1.0f, 0.8f);
                                case ECk_Cue_ExecutionPolicy::Local:
                                    return FLinearColor(1.0f, 0.8f, 0.2f);
                                default:
                                    return FLinearColor::White;
                            }
                        }
                        return FLinearColor::White;
                    })
                    .ToolTipText_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                            switch (ExecutionType)
                            {
                                case ECk_Cue_ExecutionPolicy::Replicated:
                                    return FText::FromString(TEXT("Replicated: Synchronizes across network"));
                                case ECk_Cue_ExecutionPolicy::ReplicatedAndLocal:
                                    return FText::FromString(TEXT("Replicated and Local: Runs locally immediately, then replicates"));
                                case ECk_Cue_ExecutionPolicy::Local:
                                    return FText::FromString(TEXT("Local: Runs locally only"));
                                default:
                                    return FText::GetEmpty();
                            }
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
                            switch (ExecutionType)
                            {
                                case ECk_Cue_ExecutionPolicy::Replicated:
                                    return FText::FromString(TEXT("Replicated"));
                                case ECk_Cue_ExecutionPolicy::ReplicatedAndLocal:
                                    return FText::FromString(TEXT("Replicated+Local"));
                                case ECk_Cue_ExecutionPolicy::Local:
                                    return FText::FromString(TEXT("Local"));
                                default:
                                    return FText::GetEmpty();
                            }
                        }
                        return FText::GetEmpty();
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                ]
            ]

            // Entity Mode indicator
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f, 2.0f, 0.0f)
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
                            const auto& EntityMode = _CueNode->DoGet_EntityMode();
                            return FText::FromString(EntityMode == ECk_Cue_EntityMode::Owner ?
                                TEXT("👤") : TEXT("🍃"));
                        }
                        return FText::FromString(TEXT("?"));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .ColorAndOpacity_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& EntityMode = _CueNode->DoGet_EntityMode();
                            return EntityMode == ECk_Cue_EntityMode::Owner ?
                                FLinearColor(0.4f, 0.7f, 1.0f) : FLinearColor(0.6f, 1.0f, 0.6f);
                        }
                        return FLinearColor::White;
                    })
                    .ToolTipText_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& EntityMode = _CueNode->DoGet_EntityMode();
                            return FText::FromString(EntityMode == ECk_Cue_EntityMode::Owner ?
                                TEXT("Owner: Attached to owner entity") :
                                TEXT("Transient: Not attached to any entity"));
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
                            const auto& EntityMode = _CueNode->DoGet_EntityMode();
                            return FText::FromString(EntityMode == ECk_Cue_EntityMode::Owner ?
                                TEXT("Owner") : TEXT("Transient"));
                        }
                        return FText::GetEmpty();
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                ]
            ]

            // Reliability Policy indicator (only show for Replicated execution)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f, 2.0f, 0.0f)
            [
                SNew(SHorizontalBox)
                .Visibility_Lambda([this]()
                {
                    if (ck::IsValid(_CueNode.Get()))
                    {
                        const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                        return (ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ||
                                ExecutionType == ECk_Cue_ExecutionPolicy::ReplicatedAndLocal) ?
                            EVisibility::Visible : EVisibility::Collapsed;
                    }
                    return EVisibility::Collapsed;
                })
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ReliabilityPolicy = _CueNode->DoGet_ReliabilityPolicy();
                            return FText::FromString(ReliabilityPolicy == ECk_Cue_ReliabilityPolicy::Reliable ?
                                TEXT("📌") : TEXT("💨"));
                        }
                        return FText::FromString(TEXT("?"));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .ColorAndOpacity_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ReliabilityPolicy = _CueNode->DoGet_ReliabilityPolicy();
                            return ReliabilityPolicy == ECk_Cue_ReliabilityPolicy::Reliable ?
                                FLinearColor(1.0f, 0.6f, 0.2f) : FLinearColor(0.7f, 0.7f, 0.7f);
                        }
                        return FLinearColor::White;
                    })
                    .ToolTipText_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& ReliabilityPolicy = _CueNode->DoGet_ReliabilityPolicy();
                            return FText::FromString(ReliabilityPolicy == ECk_Cue_ReliabilityPolicy::Reliable ?
                                TEXT("Reliable: Guaranteed delivery") :
                                TEXT("Unreliable: Best effort delivery"));
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
                            const auto& ReliabilityPolicy = _CueNode->DoGet_ReliabilityPolicy();
                            return FText::FromString(ReliabilityPolicy == ECk_Cue_ReliabilityPolicy::Reliable ?
                                TEXT("Reliable") : TEXT("Unreliable"));
                        }
                        return FText::GetEmpty();
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                ]
            ]

            // Multicast Policy indicator (only show for Replicated execution)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f, 2.0f, 0.0f)
            [
                SNew(SHorizontalBox)
                .Visibility_Lambda([this]()
                {
                    if (ck::IsValid(_CueNode.Get()))
                    {
                        const auto& ExecutionType = _CueNode->DoGet_ExecutionType();
                        return (ExecutionType == ECk_Cue_ExecutionPolicy::Replicated ||
                                ExecutionType == ECk_Cue_ExecutionPolicy::ReplicatedAndLocal) ?
                            EVisibility::Visible : EVisibility::Collapsed;
                    }
                    return EVisibility::Collapsed;
                })
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& MulticastPolicy = _CueNode->DoGet_MulticastPolicy();
                            switch (MulticastPolicy)
                            {
                                case ECk_Cue_MulticastPolicy::MulticastToClients:
                                    return FText::FromString(TEXT("📡"));
                                case ECk_Cue_MulticastPolicy::MulticastToOtherClients:
                                    return FText::FromString(TEXT("📡➖"));
                                case ECk_Cue_MulticastPolicy::ServerOnly:
                                    return FText::FromString(TEXT("🔒"));
                                default:
                                    return FText::FromString(TEXT("?"));
                            }
                        }
                        return FText::FromString(TEXT("?"));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .ColorAndOpacity_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& MulticastPolicy = _CueNode->DoGet_MulticastPolicy();
                            switch (MulticastPolicy)
                            {
                                case ECk_Cue_MulticastPolicy::MulticastToClients:
                                    return FLinearColor(0.4f, 1.0f, 0.4f);
                                case ECk_Cue_MulticastPolicy::MulticastToOtherClients:
                                    return FLinearColor(0.6f, 0.9f, 1.0f);
                                case ECk_Cue_MulticastPolicy::ServerOnly:
                                    return FLinearColor(1.0f, 0.7f, 0.3f);
                                default:
                                    return FLinearColor::White;
                            }
                        }
                        return FLinearColor::White;
                    })
                    .ToolTipText_Lambda([this]()
                    {
                        if (ck::IsValid(_CueNode.Get()))
                        {
                            const auto& MulticastPolicy = _CueNode->DoGet_MulticastPolicy();
                            switch (MulticastPolicy)
                            {
                                case ECk_Cue_MulticastPolicy::MulticastToClients:
                                    return FText::FromString(TEXT("Multicast: Sent to all clients"));
                                case ECk_Cue_MulticastPolicy::MulticastToOtherClients:
                                    return FText::FromString(TEXT("Multicast to Other Clients: Sent to all clients except sender"));
                                case ECk_Cue_MulticastPolicy::ServerOnly:
                                    return FText::FromString(TEXT("Server Only: Executes only on server"));
                                default:
                                    return FText::GetEmpty();
                            }
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
                            const auto& MulticastPolicy = _CueNode->DoGet_MulticastPolicy();
                            switch (MulticastPolicy)
                            {
                                case ECk_Cue_MulticastPolicy::MulticastToClients:
                                    return FText::FromString(TEXT("Multicast"));
                                case ECk_Cue_MulticastPolicy::MulticastToOtherClients:
                                    return FText::FromString(TEXT("Multicast (Others)"));
                                case ECk_Cue_MulticastPolicy::ServerOnly:
                                    return FText::FromString(TEXT("Server Only"));
                                default:
                                    return FText::GetEmpty();
                            }
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