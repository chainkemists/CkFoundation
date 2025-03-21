#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include <K2Node_CallFunction.h>

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
    auto PostReconstructNode() -> void override;
    // End of K2Node implementation

    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    // End of UEdGraphNode implementation

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

private:
    auto DoGet_EntityScriptClass(TOptional<TArray<UEdGraphPin*>> InPinsToSearch = {}) const -> UClass*;
    auto DoGet_LifetimeOwnerType() const -> ECk_EntityLifetime_OwnerType;

    auto DoCreatePinsFromEntityScript(UClass* InEntityScriptClass) -> void;
    auto DoOnClassPinChanged() -> void;

private:
    ECk_EntityLifetime_OwnerType _LifetimeOwnerType = ECk_EntityLifetime_OwnerType::UseTransientEntity;
    EClassFlags _DisallowedFlags = CLASS_Abstract | CLASS_None | CLASS_Deprecated;
    TArray<UEdGraphPin*> _PinsGeneratedForEntityScript;
};

// --------------------------------------------------------------------------------------------------------------------
