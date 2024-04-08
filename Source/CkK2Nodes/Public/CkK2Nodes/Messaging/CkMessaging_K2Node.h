#pragma once

#include <K2Node_CallFunction.h>

#include "CkMessaging_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class UCk_K2Node_Messaging_Broadcast : public UK2Node_CallFunction
{
    GENERATED_BODY()

    auto
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const -> void override;

    auto
    IsNodePure() const -> bool override;

    auto
    IsConnectionDisallowed(
        const UEdGraphPin* InMyPin,
        const UEdGraphPin* InOtherPin,
        FString& OutReason) const -> bool override;
};

// --------------------------------------------------------------------------------------------------------------------