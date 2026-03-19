#include "CkStructTypeSelector_PinFactory.h"

#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_GraphPin.h"

#include "CkCore/StructTypeSelector/CkStructTypeSelector.h"

#include <EdGraphSchema_K2.h>

// --------------------------------------------------------------------------------------------------------------------

auto FCk_StructTypeSelector_PinFactory::CreatePin(
    UEdGraphPin* InPin) const -> TSharedPtr<SGraphPin>
{
    if (InPin != nullptr &&
        InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
        InPin->PinType.PinSubCategoryObject == FCk_StructTypeSelector::StaticStruct())
    {
        return SNew(ck::layout::SStructTypeSelector_GraphPin, InPin);
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
