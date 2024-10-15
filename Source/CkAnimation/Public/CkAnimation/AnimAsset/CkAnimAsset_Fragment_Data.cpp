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
