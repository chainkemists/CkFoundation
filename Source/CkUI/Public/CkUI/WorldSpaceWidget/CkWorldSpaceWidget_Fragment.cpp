#include "CkWorldSpaceWidget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_WorldSpaceWidget_Current::
        FFragment_WorldSpaceWidget_Current(
            UUserWidget* InWidget)
        : _WidgetHardRef(InWidget)
        , _WidgetOwningPlayer(InWidget->GetOwningPlayer())
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
