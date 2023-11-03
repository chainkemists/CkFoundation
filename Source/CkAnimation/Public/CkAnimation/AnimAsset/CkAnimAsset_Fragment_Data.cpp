#include "CkAnimAsset_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Animation_AssetInfo::
    FCk_Animation_AssetInfo(
        const FGameplayTag& InAlias,
        UAnimationAsset* InAnimation)
    : _Alias(InAlias)
    , _Animation(InAnimation)
{
}

// --------------------------------------------------------------------------------------------------------------------
