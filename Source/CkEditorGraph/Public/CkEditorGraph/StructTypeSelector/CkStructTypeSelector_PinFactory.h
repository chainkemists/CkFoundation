#pragma once

#include <EdGraphUtilities.h>

// --------------------------------------------------------------------------------------------------------------------

class CKEDITORGRAPH_API FCk_StructTypeSelector_PinFactory : public FGraphPanelPinFactory
{
public:
    auto CreatePin(UEdGraphPin* InPin) const -> TSharedPtr<SGraphPin> override;
};

// --------------------------------------------------------------------------------------------------------------------
