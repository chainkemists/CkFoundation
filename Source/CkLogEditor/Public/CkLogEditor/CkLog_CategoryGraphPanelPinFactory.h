#pragma once

#include "EdGraphUtilities.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_LogCategory_GraphPanelPinFactory : public FGraphPanelPinFactory
{
    auto CreatePin(
        class UEdGraphPin* InPin) const -> TSharedPtr<class SGraphPin> override;
};

// --------------------------------------------------------------------------------------------------------------------
