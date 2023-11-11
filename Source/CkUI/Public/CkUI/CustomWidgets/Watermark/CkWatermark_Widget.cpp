#include "CkWatermark_Widget.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_Watermark_UserWidget_UE::
    UCk_Watermark_UserWidget_UE()
{
    _DoNotDestroyDuringTransitions = true;
}

auto
    UCk_Watermark_UserWidget_UE::
    Request_SetDisplayPolicy(
        ECk_Watermark_DisplayPolicy InNewDisplayPolicy)
    -> void
{
    if (_CurrentDisplayPolicy == InNewDisplayPolicy)
    { return; }

    const auto& PreviousDisplayPolicy = _CurrentDisplayPolicy;
    _CurrentDisplayPolicy = InNewDisplayPolicy;

    OnDisplayPolicyChanged(PreviousDisplayPolicy, _CurrentDisplayPolicy);
}

// --------------------------------------------------------------------------------------------------------------------

