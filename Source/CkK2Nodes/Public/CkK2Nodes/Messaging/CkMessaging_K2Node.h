#pragma once

#include "CkCore/IO/CkIO_Utils.h"

#include "CkK2Nodes/CkUFunctionBase_K2Node.h"

#include "CkMessaging/CkMessaging_Fragment_Data.h"

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

UCLASS(MinimalAPI)
class UCk_K2Node_Message_Broadcast : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    // UObject interface
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // UEdGraphNode implementation
    auto PreloadRequiredAssets() -> void override;
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    // End of UEdGraphNode implementation

    auto GetNodeContextMenuActions(UToolMenu* InMenu, UGraphNodeContextMenuContext* InContext) const -> void override;

    // K2Node implementation
    auto GetMenuCategory() const -> FText override;
    auto IsNodePure() const -> bool override;
    auto AutowireNewNode(UEdGraphPin* FromPin) -> void override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto ReconstructNode() -> void override;
    // End of K2Node implementation

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

public:
    UFUNCTION()
    void RefreshTemplateMessageDefinition();

    void CreatePinsFromMessageDefinition();

public:
    UPROPERTY(EditAnywhere)
    TSubclassOf<UCk_Message_Definition_PDA> _MessageDefinition;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class UCk_K2Node_Message_Listen : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    // UObject interface
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // UEdGraphNode implementation
    auto PreloadRequiredAssets() -> void override;
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    bool IsCompatibleWithGraph(UEdGraph const* InGraph) const override;
    // End of UEdGraphNode implementation

    auto GetNodeContextMenuActions(UToolMenu* InMenu, UGraphNodeContextMenuContext* InContext) const -> void override;

    // K2Node implementation
    auto GetMenuCategory() const -> FText override;
    auto IsNodePure() const -> bool override;
    auto AutowireNewNode(UEdGraphPin* FromPin) -> void override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto GetCornerIcon() const -> FName override;
    auto ReconstructNode() -> void override;
    // End of K2Node implementation

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

public:
    UFUNCTION()
    void RefreshTemplateMessageDefinition();

    void CreatePinsFromMessageDefinition();

public:
    UPROPERTY(EditAnywhere)
    TSubclassOf<UCk_Message_Definition_PDA> _MessageDefinition;
};

// --------------------------------------------------------------------------------------------------------------------
