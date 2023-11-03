#include "CkAnimAsset_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_AnimAsset_Animation::
    FCk_AnimAsset_Animation(
        const FGameplayTag& InAlias,
        UAnimationAsset* InAnimation)
    : _Alias(InAlias)
    , _Animation(InAnimation)
{
}

// --------------------------------------------------------------------------------------------------------------------
