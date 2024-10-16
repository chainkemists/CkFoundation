#include "CkEntityCollection_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_EntityCollection, TEXT("EntityCollection"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_EntityCollection_Content::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_CollectionName() == InOther.Get_CollectionName() &&
        Get_EntitiesInCollection() == InOther.Get_EntitiesInCollection();
}

// --------------------------------------------------------------------------------------------------------------------
