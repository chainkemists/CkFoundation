#include "CkUsf/Stylize/CkUsf_StylizeMask_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Usf_StylizeMask_Params::
    Get_ClaimsStencil() const
    -> bool
{
    return _Mode != ECk_Usf_StylizeMaskMode::Off;
}

auto
    FCk_Usf_StylizeMask_Params::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _Mode == InOther._Mode
        && _StencilMin == InOther._StencilMin
        && _StencilMax == InOther._StencilMax;
}

// --------------------------------------------------------------------------------------------------------------------
