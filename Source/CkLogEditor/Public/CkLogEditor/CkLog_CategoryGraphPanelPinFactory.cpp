#include "CkLog_CategoryGraphPanelPinFactory.h"

#include <EdGraphSchema_K2.h>

#include "CkLog/CkLog_Category.h"
#include "CkLogEditor/CkLog_CategoryGraphPin.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    FCk_LogCategory_GraphPanelPinFactory::
    CreatePin(
        UEdGraphPin* InPin) const
    -> TSharedPtr<SGraphPin>
{
    if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && InPin->PinType.PinSubCategoryObject == FCk_LogCategory::StaticStruct())
    {
        return SNew(ck::layout::SLogCategory_GraphPin, InPin);
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------