#include "CkVfxCue_DataChannel_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// NOTE: Data Channel VFX Cues are fire-once entity scripts with no persistent state
// They don't use fragments, so these Cast functions won't be useful in practice
// They exist for API consistency
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VfxCue_DataChannel_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return false;
}

auto
    UCk_Utils_VfxCue_DataChannel_UE::
    DoCast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult)
    -> FCk_Handle_VfxCue_DataChannel
{
    OutResult = ECk_SucceededFailed::Failed;
    return {};
}

auto
    UCk_Utils_VfxCue_DataChannel_UE::
    DoCastChecked(
        FCk_Handle InHandle)
    -> FCk_Handle_VfxCue_DataChannel
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------
