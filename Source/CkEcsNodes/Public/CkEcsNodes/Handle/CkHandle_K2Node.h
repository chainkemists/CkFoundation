#pragma once

#include <K2Node_CallFunction.h>

#include "CkHandle_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class UCkHandle_K2Node : public UK2Node_CallFunction
{
    GENERATED_BODY()

    //~ Begin UEdGraphNode Interface
    auto
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const -> void override;
    //~ End UEdGraphNode Interface

    //~ Begin K2Node Interface
    auto
    IsNodePure() const -> bool override;

    auto
    IsConnectionDisallowed(
        const UEdGraphPin* InMyPin,
        const UEdGraphPin* InOtherPin,
        FString& OutReason) const -> bool override;
    //~ End K2Node Interface
};

// --------------------------------------------------------------------------------------------------------------------