#pragma once

#include "CkCVar/CkCVar_Data.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include "CkCVar_K2Node_Get.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_CVar_Get : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetNodeTitleColor() const -> FLinearColor override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    auto GetMenuCategory() const -> FText override;
    auto IsNodePure() const -> bool override { return true; }
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;

protected:
    auto DoAllocate_DefaultPins() -> void override;
    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;
    auto DoGet_Menu_NodeTitle() const -> FText override;
    auto DoPinDefaultValueChanged(UEdGraphPin* InPin) -> void override;

private:
    auto GetInternalFunctionNameForType() const -> FName;
    auto UpdateDetectedType() -> void;
    auto CreateValuePinForType() -> void;

private:
    TOptional<ECk_CVarType> _DetectedType;
};

// --------------------------------------------------------------------------------------------------------------------
