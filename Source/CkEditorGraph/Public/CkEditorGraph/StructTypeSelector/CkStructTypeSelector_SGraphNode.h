#pragma once

#include <KismetNodes/SGraphNodeK2Default.h>

// --------------------------------------------------------------------------------------------------------------------

// Reusable SGraphNode that adds a compact/expanded banner below pins.
// Works with any K2 node that has an ECk_CompactExpanded _PayloadMode UPROPERTY.
class CKEDITORGRAPH_API SCk_GraphNode_WithPayloadBanner : public SGraphNodeK2Default
{
public:
    SLATE_BEGIN_ARGS(SCk_GraphNode_WithPayloadBanner) {}
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs, UEdGraphNode* InNode) -> void;

    auto CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void override;

private:
    TWeakObjectPtr<UEdGraphNode> _OwningNode;
};

// --------------------------------------------------------------------------------------------------------------------
