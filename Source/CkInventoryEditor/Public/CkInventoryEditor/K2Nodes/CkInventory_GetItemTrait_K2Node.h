#pragma once

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include "CkInventory/ItemTrait/CkItemTrait.h"

#include <K2Node_CallFunction.h>

#include "CkInventory_GetItemTrait_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCkInventory_GetItemTrait_K2Node : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkInventory_GetItemTrait_K2Node);

public:
    // UObject interface
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    // End of UEdGraphNode implementation

    // K2Node implementation
    auto AllocateDefaultPins() -> void override;
    auto GetMenuCategory() const -> FText override;
    auto IsNodePure() const -> bool override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    // End of K2Node implementation

protected:
    auto DoAllocate_DefaultPins() -> void override;

    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;

    auto DoGet_Menu_NodeTitle() const -> FText override;

    auto DoValidateNodePins(
        const TOptional<FKismetCompilerContext*>& InCompilerContext = {}) const -> ECk_ValidInvalid override;

public:
    auto PinDefaultValueChanged(
        UEdGraphPin* InPin) -> void override;

private:
    auto Get_TraitClassFromPin() const -> UClass*;

public:
    UPROPERTY(EditAnywhere, Category = "Trait Type")
    TSubclassOf<UCk_ItemTrait> _TraitClass;
};

// --------------------------------------------------------------------------------------------------------------------
