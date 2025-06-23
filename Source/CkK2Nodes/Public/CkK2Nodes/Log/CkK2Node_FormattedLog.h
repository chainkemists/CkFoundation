#pragma once

#include "CkLog/CkLog_Utils.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"

#include "CkK2Node_FormattedLog.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;

UCLASS()
class UCkK2Node_FormattedLog : public UK2Node
{
    GENERATED_UCLASS_BODY()

  public:
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
        -> void override;

    auto AllocateDefaultPins() -> void override;
    auto GetNodeTitle(ENodeTitleType::Type TitleType) const -> FText override;
    auto GetNodeTitleColor() const -> FLinearColor override;
    auto ShouldShowNodeProperties() const -> bool override;
    auto PinConnectionListChanged(UEdGraphPin* Pin) -> void override;
    auto PinDefaultValueChanged(UEdGraphPin* Pin) -> void override;
    auto PinTypeChanged(UEdGraphPin* Pin) -> void override;
    auto GetTooltipText() const -> FText override;
    auto GetPinDisplayName(const UEdGraphPin* Pin) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;

    auto IsNodePure() const -> bool override;
    auto NodeCausesStructuralBlueprintChange() const -> bool override;
    auto PostReconstructNode() -> void override;
    auto ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
        -> void override;
    auto DoPinsMatchForReconstruction(const UEdGraphPin* NewPin,
                                      int32 NewPinIndex,
                                      const UEdGraphPin* OldPin,
                                      int32 OldPinIndex) const -> ERedirectType override;
    auto IsConnectionDisallowed(const UEdGraphPin* MyPin,
                                const UEdGraphPin* OtherPin,
                                FString& OutReason) const -> bool override;
    auto GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const -> void override;
    auto GetMenuCategory() const -> FText override;
    auto GetNodeRefreshPriority() const -> int32 override;
    auto CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const
        -> FNodeHandlingFunctor* override;

    auto GetLogCategoryPin() const -> UEdGraphPin*;
    auto GetVerbosityPin() const -> UEdGraphPin*;
    auto GetFormatPin() const -> UEdGraphPin*;
    auto GetExecPin() const -> UEdGraphPin*;
    auto GetThenPin() const -> UEdGraphPin*;

    auto GetArgumentCount() const -> int32;

    auto GetArgumentName(int32 InIndex) const -> FText;

    auto CanEditArguments() const -> bool;

  private:
    auto SynchronizeArgumentPinType(UEdGraphPin* Pin) -> void;
    auto GetUniquePinName() -> FName;
    auto FindArgumentPin(const FName InPinName) const -> UEdGraphPin*;

  private:
    UPROPERTY()
    TArray<FName> _PinNames;

    UEdGraphPin* _CachedLogCategoryPin = nullptr;
    UEdGraphPin* _CachedVerbosityPin = nullptr;
    UEdGraphPin* _CachedFormatPin = nullptr;

    FText _NodeTooltip;
};