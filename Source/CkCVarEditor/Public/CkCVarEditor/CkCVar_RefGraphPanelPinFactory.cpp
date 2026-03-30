#include "CkCVar_RefGraphPanelPinFactory.h"

#include "CkCVarEditor/CkCVar_RefGraphPin.h"
#include "CkCVar/CkCVar_Data.h"

#include <EdGraphSchema_K2.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_CVarRef_GraphPanelPinFactory::
    CreatePin(
        UEdGraphPin* InPin) const
    -> TSharedPtr<SGraphPin>
{
    if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
        InPin->PinType.PinSubCategoryObject == FCk_CVarRef::StaticStruct())
    {
        return SNew(ck::layout::SCVarRef_GraphPin, InPin);
    }
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
