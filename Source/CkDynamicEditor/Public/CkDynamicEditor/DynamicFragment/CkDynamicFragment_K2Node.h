#pragma once

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"


#include <K2Node_CallFunction.h>

#include "CkDynamicFragment_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCkDynamicFragment_K2Node : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkDynamicFragment_K2Node);

public:
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
    auto ShouldShowNodeProperties() const -> bool override;

public:
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;

public:
    auto AllocateDefaultPins() -> void override;
    auto GetMenuCategory() const -> FText override;
    auto IsNodePure() const -> bool override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto CreateVisualWidget() -> TSharedPtr<SGraphNode> override;
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
    auto CreatePinsFromFragmentStruct() -> void;

    auto DoExpandNode_Expanded(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        const UScriptStruct* InStructType) -> void;

    auto DoExpandNode_Compact(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        const UScriptStruct* InStructType) -> void;

    auto IsCompactMode() const -> bool { return _PayloadMode == ECk_CompactExpanded::Compact; }
    auto Get_SelectedStructType() const -> UScriptStruct*;

public:
    UPROPERTY(EditAnywhere, Category = "Display")
    ECk_CompactExpanded _PayloadMode = ECk_CompactExpanded::Compact;
};

// --------------------------------------------------------------------------------------------------------------------
