#include "CkItemFragment_Dimensions.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ItemFragment_Dimensions::
    OnApplied(
        FCk_Handle_Item& InItem) const
    -> void
{
    // No additional ECS features needed.
    // The fragment itself is added as a dynamic fragment by the definition's DoConstruct,
    // making it queryable via UCk_Utils_DynamicFragment_UE.
}

// --------------------------------------------------------------------------------------------------------------------
