#include "CkWorldSpaceWidget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_WorldSpaceWidget_Current::
        FFragment_WorldSpaceWidget_Current(
            UCk_WorldSpaceWidget_Wrapper_UE* InWrapperWidget)
        : _ContentWidgetHardRef(InWrapperWidget->Get_ContentWidget())
        , _WrapperWidget(InWrapperWidget)
        , _WidgetOwningPlayer(InWrapperWidget->GetOwningPlayer())
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
