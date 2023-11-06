#include "CkAnimAsset_Fragment_Data.h"

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
