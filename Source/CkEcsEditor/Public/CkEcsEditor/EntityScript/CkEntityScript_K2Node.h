#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include <KismetNodes/SGraphNodeK2Base.h>
#include <SGraphPin.h>

#include "CkEntityScript_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_EntityScript : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    // UObject interface
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // K2Node implementation
    auto IsNodePure() const -> bool override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto GetMenuCategory() const -> FText override;
    auto IsCompatibleWithGraph(UEdGraph const* InGraph) const -> bool override;
    auto PinConnectionListChanged(UEdGraphPin* InPin) -> void override;
    auto GetPinMetaData(FName InPinName, FName InKey) -> FString override;
    auto GetJumpTargetForDoubleClick() const -> UObject* override;
    auto CreateVisualWidget() -> TSharedPtr<SGraphNode> override;
    // End of K2Node implementation

    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    auto GetReplicationIcon() const -> FSlateIcon;
    auto GetCornerIcon() const -> FName override;
    // End of UEdGraphNode implementation

    auto OnInterfacePinButtonClicked(FName PinName) const -> void;
    auto IsPinGeneratedFromEntityScript(UEdGraphPin* Pin) const -> bool;

public:
    static auto IsInterfacePin(UEdGraphPin* Pin) -> bool;

protected:
    auto
    DoAllocate_DefaultPins()-> void override;

    auto
    DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)-> void override;

    auto
    DoGet_Menu_NodeTitle() const -> FText override;

    auto
    DoPinDefaultValueChanged(UEdGraphPin* InPin) -> void override;

public:
    auto DoGet_EntityScriptClass(TOptional<TArray<UEdGraphPin*>> InPinsToSearch = {}) const -> UClass*;
    auto DoGet_LifetimeOwnerType() const -> ECk_EntityLifetime_OwnerType;

    auto DoCreatePinsFromEntityScript(UClass* InEntityScriptClass) -> void;
    auto DoOnClassPinChanged() -> void;
    static auto
    DoGet_EntitySpawnParamsStruct(
        UClass* InEntityScriptClass,
        FKismetCompilerContext& InCompilerContext) -> UScriptStruct*;

private:
    ECk_EntityLifetime_OwnerType _LifetimeOwnerType = ECk_EntityLifetime_OwnerType::UseCustomEntity;
    EClassFlags _DisallowedFlags = CLASS_Abstract | CLASS_None | CLASS_Deprecated;
    TArray<UEdGraphPin*> _PinsGeneratedForEntityScript;
    TMap<FName, TMap<FName, FString>> _PinMetadataMap;
};

// --------------------------------------------------------------------------------------------------------------------

class SCk_GraphPin_Interface : public SGraphPin
{
public:
    SLATE_BEGIN_ARGS(SCk_GraphPin_Interface) {}
    SLATE_END_ARGS()

public:
    auto
    Construct(
        const FArguments& InArgs,
        UEdGraphPin* InPin) -> void;

protected:
    auto GetDefaultValueWidget() -> TSharedRef<SWidget> override;

private:
    auto OnImplementInterfaceClicked() const -> FReply;
    auto Get_InterfaceClass() const -> UClass*;
    auto IsInterfaceAlreadyImplemented() const -> bool;
    auto Get_EntityScriptNode() const -> UCk_K2Node_EntityScript*;
};

// --------------------------------------------------------------------------------------------------------------------

class SCk_GraphNode_EntityScript : public SGraphNodeK2Base
{
public:
    SLATE_BEGIN_ARGS(SCk_GraphNode_EntityScript) {}
    SLATE_END_ARGS()

public:
    auto
    Construct(
        const FArguments& InArgs,
        UCk_K2Node_EntityScript* InNode) -> void;

    auto
    CreateBelowPinControls(
        TSharedPtr<SVerticalBox> MainBox) -> void override;

    auto CreatePinWidgets() -> void override;

private:
    auto
    ShouldPinHaveInterfaceButton(
        UEdGraphPin* Pin) const -> bool;
    auto
    ShouldShowInstancingIcon() const -> bool;

protected:
    TWeakObjectPtr<UCk_K2Node_EntityScript> _EntityScriptNode;
};

// --------------------------------------------------------------------------------------------------------------------