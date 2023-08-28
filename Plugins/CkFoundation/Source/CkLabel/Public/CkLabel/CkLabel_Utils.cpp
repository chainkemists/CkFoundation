#include "CkLabel_Utils.h"

#include "CkLabel/CkLabel_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameplayLabel_UE::
    Add(
        FCk_Handle InHandle,
        const FGameplayTag& InLabel)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_GameplayLabel>(InLabel);
}

auto
    UCk_Utils_GameplayLabel_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FCk_Fragment_GameplayLabel>();
}

auto
    UCk_Utils_GameplayLabel_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Gameplay Label"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_GameplayLabel_UE::
    Get_Label(
        FCk_Handle InHandle)
    -> FGameplayTag
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FCk_Fragment_GameplayLabel>().Get_Label();
}

// --------------------------------------------------------------------------------------------------------------------
