#pragma once

#include "CkCore/IO/CkIO_Utils.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_K2NodeHelpers.h"

#include "CkMessaging/CkMessaging_Fragment_Data.h"

#include <K2Node_CallFunction.h>
#include <KismetNodes/SGraphNodeK2Base.h>

#include "CkMessaging_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, MinimalAPI)
class UCk_K2Node_Message_Base : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    // UObject interface
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // UEdGraphNode implementation
    auto PreloadRequiredAssets() -> void override;
    // End of UEdGraphNode implementation

    // K2Node implementation
    auto IsNodePure() const -> bool override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto GetJumpTargetForDoubleClick() const -> UObject* override;
    auto JumpToDefinition() const -> void;
    // End of K2Node implementation

protected:
    virtual auto DoGet_MessageDefinitionPinsDirection() const -> ECk_EditorGraph_PinDirection
        PURE_VIRTUAL(UCk_K2Node_Message_Base::DoGet_MessageDefinitionPinsDirection, return ECk_EditorGraph_PinDirection::Input; );

public:
    auto CreatePinsFromMessageDefinition() -> void;
    auto GetMessageNameFromStruct() const -> FGameplayTag;
    auto IsCompactPayloadMode() const -> bool;
    auto Get_SelectedMessageStruct() const -> UScriptStruct*;

    auto PinDefaultValueChanged(
        UEdGraphPin* InPin) -> void override;

public:
    // UEdGraphNode interface
    auto PostLoad() -> void override;
    // End of UEdGraphNode interface

public:
    UPROPERTY(EditAnywhere, Category = "Display")
    ECk_CompactExpanded _PayloadMode = ECk_CompactExpanded::Expanded;

protected:
    TArray<UEdGraphPin*> _PinsGeneratedFromPayload;

private:
    // DEPRECATED — kept for deserialization migration only. Use the selector pin instead.
    UPROPERTY()
    FInstancedStruct _MessagePayload_DEPRECATED;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_Message_Broadcast : public UCk_K2Node_Message_Base
{
    GENERATED_BODY()

public:
    friend class SCk_GraphNode_Message_Broadcast;

public:
    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetNodeTitleColor() const -> FLinearColor override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    auto CreateVisualWidget() -> TSharedPtr<SGraphNode> override;
    // End of UEdGraphNode implementation

    // K2Node implementation
    auto GetMenuCategory() const -> FText override;
    // End of K2Node implementation

protected:
    auto DoAllocate_DefaultPins() -> void override;

    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;

    auto DoGet_Menu_NodeTitle() const -> FText override;

    auto DoGet_MessageDefinitionPinsDirection() const -> ECk_EditorGraph_PinDirection override;

private:
    auto DoExpandNode_Expanded(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph) -> void;

    auto DoExpandNode_Compact(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_Message_Listen : public UCk_K2Node_Message_Base
{
    GENERATED_BODY()

public:
    friend class SCk_GraphNode_Message_Listen;

public:
    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetNodeTitleColor() const -> FLinearColor override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    auto CreateVisualWidget() -> TSharedPtr<SGraphNode> override;
    auto IsCompatibleWithGraph(UEdGraph const* InGraph) const -> bool override;
    // End of UEdGraphNode implementation

    // K2Node implementation
    auto GetMenuCategory() const -> FText override;
    auto GetCornerIcon() const -> FName override;
    // End of K2Node implementation

protected:
    auto DoAllocate_DefaultPins() -> void override;

    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;

    auto DoGet_Menu_NodeTitle() const -> FText override;

    auto DoGet_MessageDefinitionPinsDirection() const -> ECk_EditorGraph_PinDirection override;

private:
    auto DoExpandNode_Expanded(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph) -> void;

    auto DoExpandNode_Compact(
        FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

class SCk_GraphNode_Message_Broadcast : public SGraphNodeK2Base
{
public:
    SLATE_BEGIN_ARGS(SCk_GraphNode_Message_Broadcast) {}
    SLATE_END_ARGS()

public:
    auto Construct(const FArguments& InArgs, UCk_K2Node_Message_Broadcast* InNode) -> void;
    auto CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void override;

protected:
    TWeakObjectPtr<UCk_K2Node_Message_Broadcast> _MessageNode;
};

// --------------------------------------------------------------------------------------------------------------------

class SCk_GraphNode_Message_Listen : public SGraphNodeK2Base
{
public:
    SLATE_BEGIN_ARGS(SCk_GraphNode_Message_Listen) {}
    SLATE_END_ARGS()

public:
    auto Construct(const FArguments& InArgs, UCk_K2Node_Message_Listen* InNode) -> void;
    auto CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void override;

protected:
    TWeakObjectPtr<UCk_K2Node_Message_Listen> _MessageNode;
};

// --------------------------------------------------------------------------------------------------------------------