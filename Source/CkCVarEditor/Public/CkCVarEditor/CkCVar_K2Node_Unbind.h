#pragma once

#include "CkCVar/CkCVar_Data.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include "CkCVar_K2Node_Unbind.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_CVar_Unbind : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetMenuCategory() const -> FText override;

protected:
    auto DoAllocate_DefaultPins() -> void override;
    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;
    auto DoGet_Menu_NodeTitle() const -> FText override;
};

// --------------------------------------------------------------------------------------------------------------------
