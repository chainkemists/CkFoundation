#include "CkAnimAsset_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_AnimAsset, TEXT("AnimAsset"));

// --------------------------------------------------------------------------------------------------------------------

FCk_AnimAsset_Animation::
    FCk_AnimAsset_Animation(
        const FGameplayTag& InID,
        UAnimationAsset* InAnimation)
    : _ID(InID)
    , _Animation(InAnimation)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_AnimAsset_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_AnimAsset_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_AnimAsset_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_AnimAsset_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleAnimAsset_ParamsData_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleAnimAsset_ParamsData
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_MultipleAnimAsset_ParamsData_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InHandle) const
    -> FCk_Fragment_MultipleAnimAsset_ParamsData
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------
